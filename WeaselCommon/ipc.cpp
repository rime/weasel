#include "pch.h"
#include <weasel/ipc.h>
#include <weasel/util.h>
#include <weasel/log.h>

namespace weasel
{
namespace
{

HANDLE create_pipe(std::wstring pipe_name)
{
  // std::wstring wpipe_name = LR"(\\.\pipe\weasel\)" + get_wusername();
  std::wstring& wpipe_name = pipe_name;
  SECURITY_ATTRIBUTES sa;

  sa = utils::make_security_attributes();

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
    LOG_LASTERROR(critical, "CreateNamedPipe failed");
    THROW_LAST_ERROR_MSG("CreateNamedPipe failed");
  }

  return handle;
}

}

void pipe::fetch_result()
{
  if (!has_pending_io_) return;

  DWORD bytes_transferred;
  BOOL success = GetOverlappedResult(
    h_pipe_,
    &ov_,
    &bytes_transferred,
    FALSE
  );

  switch (state_)
  {
  case reading:
    if (!success || bytes_transferred == 0)
    {
      valid_ = false;
      return;
    }
    buf_req_.set_length(bytes_transferred);
    state_ = writing;
    break;
  case writing:
    if (!success || bytes_transferred != buf_res_.length())
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

void pipe::dispatch_data()
{
  BOOL success;
  DWORD dw_error;
  DWORD bytes_transferred;
  bool should_run = true;

  switch (state_)
  {
  case reading:
    DWORD bytes_read;
    success = ReadFile(
      h_pipe_,
      buf_req_.data(),
      buf_req_.size(),
      &bytes_read,
      &ov_
    );
    buf_req_.set_length(bytes_read);

    if (success && buf_req_.length() != 0)
    {
      has_pending_io_ = false;
      state_ = writing;
      return;
    }

    dw_error = GetLastError();
    if (!success && (dw_error == ERROR_IO_PENDING))
    {
      has_pending_io_ = true;
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
      buf_res_.data(),
      buf_res_.length(),
      &bytes_transferred,
      &ov_
    );

    if (success && bytes_transferred == buf_res_.length())
    {
      has_pending_io_ = false;
      state_ = reading;
      return;
    }

    dw_error = GetLastError();
    if (!success && (dw_error == ERROR_IO_PENDING))
    {
      has_pending_io_ = true;
      return;
    }

    valid_ = false;
    return;

  default:
    // sanity check
    throw std::runtime_error("unknown pipe state");
  }
}



pipe::~pipe()
{
  CloseHandle(h_pipe_);
  CloseHandle(ov_.hEvent);
}

void pipe::run_internal()
{
  const HANDLE h[ 2 ] = { ov_.hEvent, h_terminate_ };
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
      LOG(err, "WaitForMultipleObjects failed (code {})", i);
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

void pipe::join()
{
  if (th_ == nullptr) return;
  th_->join();
  th_.reset(nullptr);
}

orch::orch(std::wstring pipe_name) :
  pipe_name_(std::move(pipe_name)),
  running_(false)
{
  h_terminate_ = CreateEventW(
    NULL,
    TRUE, // manual-reset
    FALSE, // initial state
    NULL
  );
  if (h_terminate_ == NULL)
  {
    LOG_LASTERROR(err, "CreateEvent TerminateEvent failed");
  }
}

void orch::start(const callback& cb)
{
  if (running_) return;
  run(cb);
}

void orch::run(const callback& cb)
{
  // h[0]: overlapped event handle
  // h[1]: terminate event handle
  HANDLE h[ 2 ] = { INVALID_HANDLE_VALUE, h_terminate_ };

  while (true)
  {
    // create named pipe handle
    HANDLE h_pipe;
    try
    {
      h_pipe = create_pipe(pipe_name_);
    } catch (...)
    {
      continue;
    }
    OVERLAPPED ov{};
    ov.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (ov.hEvent == NULL)
    {
      LOG_LASTERROR(err, "CreateEvent failed");
      CloseHandle(h_pipe);
      continue;
    }

    // wait for connection
    if (ConnectNamedPipe(h_pipe, &ov))
    {
      // TRUE for error
      LOG_LASTERROR(err, "ConnectNamedPipe failed");
      CloseHandle(ov.hEvent);
      CloseHandle(h_pipe);
      continue;
    }
    switch (GetLastError())
    {
    case ERROR_PIPE_CONNECTED:
      // client already connected
      // dispatch to pipe
      break;
    case ERROR_IO_PENDING:
    {
      // wait for it
      h[ 0 ] = ov.hEvent;
      DWORD i = WaitForMultipleObjects(
        2,
        h,
        FALSE,
        INFINITE
      );
      if (i >= WAIT_ABANDONED_0 && i <= WAIT_ABANDONED_0 + 1)
      {
        LOG(err, "handle " + std::to_string(i - WAIT_ABANDONED_0) + " may be corrupted");
        CloseHandle(ov.hEvent);
        CloseHandle(h_pipe);
        continue;
      } else if (i == WAIT_FAILED)
      {
        LOG(err, "WaitForMultipleObjects: wait failed");
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
      LOG(err, "ConnectNamedPipe unknown error");
      CloseHandle(ov.hEvent);
      CloseHandle(h_pipe);
      continue;
    }

    // pipe should ready to be dispatched here
    auto& i = this->pipes_.emplace_back(std::make_unique<pipe>(h_pipe, ov, this->h_terminate_, cb));
    i->run();
  }
}

void orch::stop()
{
  SetEvent(h_terminate_);
}

void orch::join()
{
  for (auto& pipe : this->pipes_)
  {
    pipe->join();
  }
  this->running_ = false;
}

orch::~orch()
{
  stop();
  join();
}
}
