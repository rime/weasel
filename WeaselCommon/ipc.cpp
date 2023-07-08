#include "pch.h"
#include <weasel/ipc.h>
#include <weasel/util.h>

#include "weasel/log.h"

namespace weasel
{
  namespace
  {

    HANDLE create_pipe(std::wstring pipe_name)
    {
      // std::wstring wpipe_name = LR"(\\.\pipe\weasel\)" + get_wusername();
      std::wstring& wpipe_name = pipe_name;
      SECURITY_ATTRIBUTES sa;

      try
      {
        sa = make_security_attributes();
      }
      catch (...)
      {
        throw;
      }

      HANDLE handle = CreateNamedPipe(
        wpipe_name.c_str(),
        PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS,
        PIPE_UNLIMITED_INSTANCES,
        8192,
        8192,
        0,
        &sa);

      LocalFree(sa.lpSecurityDescriptor);

      if (handle == INVALID_HANDLE_VALUE)
      {
        LOG_LASTERROR("CreateNamedPipe failed");
        THROW_LAST_ERROR_MSG("CreateNamedPipe failed");
      }

      return handle;
    }

  }

  void pipe_impl::fetch_result()
  {
    if (!has_pending_io) return;

    DWORD bytes_transferred;
    BOOL success = GetOverlappedResult(
      h_pipe_,
      &ov_,
      &bytes_transferred,
      FALSE
    );

    switch (state)
    {
    case reading:
      if (!success || bytes_transferred == 0)
      {
        valid_ = false;
        return;
      }
      buf_req_->set_length(bytes_transferred);
      state = writing;
      break;
    case writing:
      if (!success || bytes_transferred != buf_res_->length())
      {
        valid_ = false;
        return;
      }
      break;
    default:
      // sanity check
      throw std::runtime_error("unknown pipe state");
    }
  }

  void pipe_impl::dispatch_data()
  {
    BOOL success;
    DWORD dw_error;
    DWORD bytes_transferred;
    bool should_run = true;

    switch (state)
    {
    case reading:
      DWORD bytes_read;
      success = ReadFile(
        h_pipe_,
        buf_req_->data(),
        static_cast<DWORD>(buf_req_->size_bytes()),
        &bytes_read,
        &ov_
      );
      buf_req_->set_length(bytes_read);

      if (success && buf_req_->length() != 0)
      {
        has_pending_io = false;
        state = writing;
        return;
      }

      dw_error = GetLastError();
      if (!success && (dw_error == ERROR_IO_PENDING))
      {
        has_pending_io = true;
        return;
      }

      valid_ = false;
      return;

    case writing:
      cb_(buf_req_, buf_res_, should_run);
      if (!should_run) {
        SetEvent(h_terminate_);
        valid_ = false;
        return;
      }

      success = WriteFile(
        h_pipe_,
        buf_res_->data(),
        static_cast<DWORD>(buf_res_->length()),
        &bytes_transferred,
        &ov_
      );

      if (success && bytes_transferred == buf_res_->length())
      {
        has_pending_io = false;
        state = reading;
        return;
      }

      dw_error = GetLastError();
      if (!success && (dw_error == ERROR_IO_PENDING))
      {
        has_pending_io = true;
        return;
      }

      valid_ = false;
      return;

    default:
      // sanity check
      throw std::runtime_error("unknown pipe state");
    }
  }



  pipe_impl::~pipe_impl()
  {
    CloseHandle(h_pipe_);
    CloseHandle(ov_.hEvent);
    LOG(INFO, "pipe destructed");
  }

  void pipe_impl::run()
  {
    LOG(INFO, "pipe constructed");
    const HANDLE h[2] = { ov_.hEvent, h_terminate_ };
    while (true)
    {
      DWORD i = WaitForMultipleObjects(
        2,
        h,
        FALSE,
        INFINITE
      );
      if (i >= WAIT_ABANDONED_0 && i <= WAIT_ABANDONED_0 + 1 || i == WAIT_FAILED)
      {
        LOG(ERROR, "WaitForMultipleObjects failed (code %d)", i);
        return;
      }

      i -= WAIT_OBJECT_0;
      if (i == 1)
      {
        return;
      }

      fetch_result();
      if (!valid_) return;
      dispatch_data();
      if (!valid_) return;
    }
  }

  void pipe::run()
  {
    th_.reset(new std::thread(&pipe::run_internal, this));
  }

  void pipe::run_internal()
  {
    if (pipe_impl_ == nullptr) return;
    pipe_impl_->run();
    pipe_impl_.reset(nullptr);
  }

  void pipe::join()
  {
    if (th_ == nullptr) return;
    th_->join();
    th_.reset(nullptr);
    pipe_impl_.reset(nullptr);
  }

  pipe::~pipe()
  {
    join();
  }

  orch::orch(std::wstring pipe_name) :
    pipe_name_(std::move(pipe_name)),
    running_(false)
  {
    h_terminate_ = CreateEventW(
      NULL,
      TRUE, // manual-reset
      FALSE, // initial state
      L"TerminateEvent"
    );
    if (h_terminate_ == NULL)
    {
      LOG_LASTERROR("CreateEvent TerminateEvent failed");
    }
  }

  void orch::start(const callback& cb)
  {
    if (running_) return;
    run(cb);

    //lobby_th_.reset(new std::thread(&orch::run, this, cb));
  }

  void orch::run(const callback& cb)
  {
    // h[0]: overlapped event handle
    // h[1]: terminate event handle
    HANDLE h[2] = { INVALID_HANDLE_VALUE, h_terminate_ };

    while (true)
    {
      // create named pipe handle
      HANDLE h_pipe;
      try
      {
        h_pipe = create_pipe(pipe_name_);
      }
      catch (...)
      {
        continue;
      }
      OVERLAPPED ov{};
      ov.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
      if (ov.hEvent == NULL)
      {
        LOG_LASTERROR("CreateEvent failed");
        CloseHandle(h_pipe);
        continue;
      }

      // wait for connection
     if (ConnectNamedPipe(h_pipe, &ov))
      {
        // TRUE for error
        LOG_LASTERROR("ConnectNamedPipe failed");
        CloseHandle(ov.hEvent);
        CloseHandle(h_pipe);
        continue;
      }
      switch (GetLastError())
      {
      case ERROR_PIPE_CONNECTED:
        // client already connected
        // dispatch to pipe
        LOG(TRACE, "ConnectNamedPipe: ERROR_PIPE_CONNECTED");
        break;
      case ERROR_IO_PENDING:
      {
        LOG(TRACE, "ConnectNamedPipe: ERROR_IO_PENDING");
        // wait for it
        h[0] = ov.hEvent;
        DWORD i = WaitForMultipleObjects(
          2,
          h,
          FALSE,
          INFINITE
        );
        LOG(INFO, "Got signal");
        if (i >= WAIT_ABANDONED_0 && i <= WAIT_ABANDONED_0 + 1)
        {
          LOG(ERROR, "handle " + std::to_string(i - WAIT_ABANDONED_0) + " may be corrupted");
          CloseHandle(ov.hEvent);
          CloseHandle(h_pipe);
          continue;
        }
        else if (i == WAIT_FAILED)
        {
          LOG(ERROR, "WaitForMultipleObjects: wait failed");
          CloseHandle(ov.hEvent);
          CloseHandle(h_pipe);
          continue;
        }
        i -= WAIT_OBJECT_0;
        if (i == 1)
        {
          // terminate
          CloseHandle(ov.hEvent);
          CloseHandle(h_pipe);
          return;
        }
      }
      break;
      default:
        LOG(ERROR, "ConnectNamedPipe unknown error");
        CloseHandle(ov.hEvent);
        CloseHandle(h_pipe);
        continue;
      }

      // pipe should ready to be dispatched here
      auto& i = this->pipes_.emplace_back(std::make_unique<pipe>(h_pipe, ov, this->h_terminate_, cb));
      LOG(INFO, "dispatch pipe");
      i->run();
      LOG(INFO, "new loop");
    }
  }

  void orch::stop()
  {
    SetEvent(h_terminate_);

    std::thread th([&]
      {
        this->lobby_th_->join();
        for (auto& pipe : this->pipes_)
        {
          pipe->join();
        }
        this->running_ = false;
      });
    th.detach();
  }

  orch::~orch()
  {
    stop();
  }
}
