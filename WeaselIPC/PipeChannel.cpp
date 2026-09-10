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
      *phandle = _Connect(pname.c_str());
      return !_Invalid(*phandle);
    }
  } catch (...) {
    return false;
  }

  return true;
}

HANDLE PipeChannelBase::_Connect(const wchar_t* name) {
  HANDLE pipe = INVALID_HANDLE_VALUE;
  const ULONGLONG deadline = ::GetTickCount64() + io_timeout;
  while (_Invalid(pipe = _TryConnect())) {
    if (io_timeout != INFINITE && ::GetTickCount64() >= deadline)
      // WAIT_TIMEOUT 是 int 字面量，须按 DWORD 抛出才能被 catch(DWORD) 捕获
      _ThrowCode(static_cast<DWORD>(WAIT_TIMEOUT));
    // 管道实例全部忙时 WaitNamedPipe 会等待实例可用；而在算法服务重启等
    // 场景下没有任何实例处于监听态，它会立即失败，稍作休眠避免忙转
    if (!::WaitNamedPipe(name, 200))
      ::Sleep(20);
  }
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

HANDLE PipeChannelBase::_GetIoEvent() {
  auto ctx = _GetContext();
  if (ctx->io_event == NULL) {
    ctx->io_event = ::CreateEventW(NULL, TRUE, FALSE, NULL);
    if (ctx->io_event == NULL)
      _ThrowLastError;
  }
  return ctx->io_event;
}

// 在重叠句柄上执行一次带超时的读写，返回传输字节数；
// 失败时抛出 DWORD 错误码，超时以 WAIT_TIMEOUT 上报
DWORD PipeChannelBase::_OverlappedIo(HANDLE pipe,
                                     bool is_write,
                                     LPVOID buffer,
                                     DWORD len) {
  HANDLE event = _GetIoEvent();
  OVERLAPPED ov = {};
  ov.hEvent = event;
  ::ResetEvent(event);
  BOOL pending = is_write ? ::WriteFile(pipe, buffer, len, NULL, &ov)
                          : ::ReadFile(pipe, buffer, len, NULL, &ov);
  if (!pending && ::GetLastError() != ERROR_IO_PENDING) {
    // 同步完成（含 ERROR_MORE_DATA、ERROR_BROKEN_PIPE 等），直接上报错误码
    throw ::GetLastError();
  }
  DWORD transferred = 0;
  DWORD wait = ::WaitForSingleObject(event, io_timeout);
  if (wait == WAIT_OBJECT_0) {
    if (::GetOverlappedResult(pipe, &ov, &transferred, FALSE))
      return transferred;
    // 操作以错误状态完成，取消收尾后上报原错误码
    DWORD err = ::GetLastError();
    ::CancelIoEx(pipe, &ov);
    ::GetOverlappedResult(pipe, &ov, &transferred, TRUE);
    throw err;
  }
  if (wait == WAIT_FAILED)
    _ThrowLastError;
  // 超时：先取消未决操作，再以 WAIT_TIMEOUT 上报（转 DWORD 以匹配
  // catch(DWORD)）
  ::CancelIoEx(pipe, &ov);
  ::GetOverlappedResult(pipe, &ov, &transferred, TRUE);
  _ThrowCode(static_cast<DWORD>(WAIT_TIMEOUT));
}

size_t PipeChannelBase::_WritePipe(HANDLE pipe, size_t s, char* b) {
  DWORD lwritten = _OverlappedIo(pipe, true, b, static_cast<DWORD>(s));
  if (lwritten <= 0) {
    _ThrowLastError;
  }
  // 注：不调用 FlushFileBuffers。命名管道上它会等到对端读走数据才返回，
  // 而请求/响应模式下对端本就必须先读完请求才会产生响应，
  // 该调用只会把对端的迟滞变成调用方（宿主 UI 线程）的额外阻塞点
  return lwritten;
}

void PipeChannelBase::_FinalizePipe(HANDLE& p) {
  if (!_Invalid(p)) {
    DisconnectNamedPipe(p);
    CloseHandle(p);
  }
  p = INVALID_HANDLE_VALUE;
}

void PipeChannelBase::_Receive(HANDLE pipe, LPVOID msg, size_t rec_len) {
  try {
    _OverlappedIo(pipe, false, msg, static_cast<DWORD>(rec_len));
  } catch (DWORD err) {
    if (err != ERROR_MORE_DATA)
      throw;

    auto ctx = _GetContext();
    memset(ctx->buffer.get(), 0, buff_size);
    _OverlappedIo(pipe, false, ctx->buffer.get(),
                  static_cast<DWORD>(buff_size));
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
  OVERLAPPED ov = {};
  ov.hEvent = _GetIoEvent();
  if (!::ConnectNamedPipe(pipe, &ov)) {
    DWORD err = ::GetLastError();
    // 客户端在 CreateNamedPipe 与 ConnectNamedPipe 之间连入是合法竞态
    if (err != ERROR_PIPE_CONNECTED) {
      if (err != ERROR_IO_PENDING) {
        ::CloseHandle(pipe);
        throw err;
      }
      DWORD connected = 0;
      if (!::GetOverlappedResult(pipe, &ov, &connected, TRUE)) {
        err = ::GetLastError();
        if (err != ERROR_PIPE_CONNECTED) {
          ::CloseHandle(pipe);
          throw err;
        }
      }
    }
  }
  return pipe;
}
