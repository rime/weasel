#include "stdafx.h"

#include <PipeChannel.h>

using namespace weasel;
using namespace std;
using namespace boost;

#define _ThrowLastError throw ::GetLastError()
#define _ThrowCode(__c) throw __c
#define _ThrowIfNot(__c)                 \
  {                                      \
    DWORD err;                           \
    if ((err = ::GetLastError()) != __c) \
      throw err;                         \
  }

namespace {
struct OverlappedOp {
  OVERLAPPED ov;
  OverlappedOp() : ov() { ov.hEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL); }
  ~OverlappedOp() {
    if (ov.hEvent)
      ::CloseHandle(ov.hEvent);
  }
};
}  // namespace

PipeChannelBase::PipeChannelBase(std::wstring&& pn_cmd,
                                 size_t bs = 4 * 1024,
                                 SECURITY_ATTRIBUTES* s = NULL)
    : pname(pn_cmd), buff_size(bs), sa(s) {};

PipeChannelBase::~PipeChannelBase() {
  // Thread-specific pointers are cleaned up automatically
}

bool PipeChannelBase::_Ensure() {
  try {
    HANDLE* phandle = _GetPipeHandle();
    if (_Invalid(*phandle)) {
      *phandle = _Connect();
      return !_Invalid(*phandle);
    }
  } catch (...) {
    return false;
  }

  return true;
}

HANDLE PipeChannelBase::_Connect() {
  // fail fast when all instances are busy: this runs on the host app's UI
  // thread, which must never wait for the server
  HANDLE pipe = _TryConnect();
  if (_Invalid(pipe))
    _ThrowCode(static_cast<DWORD>(ERROR_PIPE_BUSY));
  DWORD mode = PIPE_READMODE_MESSAGE;
  if (!SetNamedPipeHandleState(pipe, &mode, NULL, NULL)) {
    _ThrowLastError;
  }
  return pipe;
}

void PipeChannelBase::_Reconnect() {
  HANDLE* phandle = _GetPipeHandle();
  _FinalizePipe(*phandle);
  _Ensure();
}

HANDLE PipeChannelBase::_TryConnect() {
  auto pipe = ::CreateFile(pname.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL,
                           OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
  if (!_Invalid(pipe)) {
    // connected to the pipe
    return pipe;
  }
  // being busy is not really an error since we just need to wait.
  _ThrowIfNot(ERROR_PIPE_BUSY);
  // All pipe instances are busy
  return INVALID_HANDLE_VALUE;
}

DWORD PipeChannelBase::_WaitIo(HANDLE pipe,
                               OVERLAPPED& ov,
                               BOOL op_result,
                               DWORD timeout_ms,
                               DWORD* transferred) {
  if (!op_result) {
    DWORD err = ::GetLastError();
    if (err == ERROR_MORE_DATA)
      return ERROR_MORE_DATA;
    if (err != ERROR_IO_PENDING)
      _ThrowCode(err);
  }
  DWORD wait = ::WaitForSingleObject(ov.hEvent, timeout_ms);
  if (wait != WAIT_OBJECT_0) {
    ::CancelIoEx(pipe, &ov);
    ::WaitForSingleObject(ov.hEvent, INFINITE);
    _ThrowCode(wait == WAIT_TIMEOUT ? static_cast<DWORD>(ERROR_TIMEOUT)
                                    : ::GetLastError());
  }
  DWORD n = 0;
  if (!::GetOverlappedResult(pipe, &ov, &n, FALSE)) {
    DWORD err = ::GetLastError();
    if (transferred)
      *transferred = n;
    if (err == ERROR_MORE_DATA)
      return ERROR_MORE_DATA;
    _ThrowCode(err);
  }
  if (transferred)
    *transferred = n;
  return ERROR_SUCCESS;
}

size_t PipeChannelBase::_WritePipe(HANDLE pipe,
                                   size_t s,
                                   char* b,
                                   DWORD timeout_ms) {
  OverlappedOp op;
  BOOL success = ::WriteFile(pipe, b, static_cast<DWORD>(s), NULL, &op.ov);
  DWORD lwritten = 0;
  DWORD err = _WaitIo(pipe, op.ov, success, timeout_ms, &lwritten);
  if (err != ERROR_SUCCESS || lwritten == 0) {
    _ThrowCode(err != ERROR_SUCCESS ? err
                                    : static_cast<DWORD>(ERROR_WRITE_FAULT));
  }
  return lwritten;
}

void PipeChannelBase::_FinalizePipe(HANDLE& p) {
  if (!_Invalid(p)) {
    DisconnectNamedPipe(p);
    CloseHandle(p);
  }
  p = INVALID_HANDLE_VALUE;
}

void PipeChannelBase::_Receive(HANDLE pipe,
                               LPVOID msg,
                               size_t rec_len,
                               DWORD timeout_ms) {
  DWORD err;
  {
    OverlappedOp op;
    BOOL success =
        ::ReadFile(pipe, msg, static_cast<DWORD>(rec_len), NULL, &op.ov);
    err = _WaitIo(pipe, op.ov, success, timeout_ms, NULL);
  }
  if (err == ERROR_MORE_DATA) {
    auto ctx = _GetContext();
    memset(ctx->buffer.get(), 0, buff_size);
    OverlappedOp op;
    BOOL success = ::ReadFile(pipe, ctx->buffer.get(),
                              static_cast<DWORD>(buff_size), NULL, &op.ov);
    err = _WaitIo(pipe, op.ov, success, timeout_ms, NULL);
    if (err != ERROR_SUCCESS)
      _ThrowCode(err);
  }
  _GetContext()->has_body = false;
}

HANDLE PipeChannelBase::_ConnectServerPipe(std::wstring& pn) {
  HANDLE pipe =
      CreateNamedPipe(pn.c_str(), PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                      PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                      PIPE_UNLIMITED_INSTANCES, buff_size, buff_size, 0, sa);
  if (pipe == INVALID_HANDLE_VALUE) {
    _ThrowLastError;
  }
  OverlappedOp op;
  BOOL ok = ::ConnectNamedPipe(pipe, &op.ov);
  DWORD err = ok ? ERROR_SUCCESS : ::GetLastError();
  if (!ok && err == ERROR_IO_PENDING) {
    ::WaitForSingleObject(op.ov.hEvent, INFINITE);
    DWORD n = 0;
    if (!::GetOverlappedResult(pipe, &op.ov, &n, FALSE)) {
      err = ::GetLastError();
      ::CloseHandle(pipe);
      _ThrowCode(err);
    }
  } else if (!ok && err != ERROR_PIPE_CONNECTED) {
    ::CloseHandle(pipe);
    _ThrowCode(err);
  }
  return pipe;
}
