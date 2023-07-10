#pragma once
#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <wil/resource.h>

#define WEASEL_IPC_WINDOW L"WeaselIPCWindow_1.0"
#define WEASEL_IPC_PIPE_NAME L"WeaselNamedPipe"
#define WEASEL_IPC_BUFFER_LENGTH 8192 // bytes

namespace weasel::ipc
{

enum ipc_command
{	
	WEASEL_IPC_ECHO = (WM_APP + 1),
	WEASEL_IPC_START_SESSION,
	WEASEL_IPC_END_SESSION,
	WEASEL_IPC_PROCESS_KEY_EVENT,
	WEASEL_IPC_SHUTDOWN_SERVER,
	WEASEL_IPC_FOCUS_IN,
	WEASEL_IPC_FOCUS_OUT,
	WEASEL_IPC_UPDATE_INPUT_POS,
	WEASEL_IPC_START_MAINTENANCE,
	WEASEL_IPC_END_MAINTENANCE,
	WEASEL_IPC_COMMIT_COMPOSITION,
	WEASEL_IPC_CLEAR_COMPOSITION,
	WEASEL_IPC_TRAY_COMMAND,
	WEASEL_IPC_LAST_COMMAND
};

class buffer
{
public:
  using byte = unsigned char;

  buffer() : size_(), length_(), data_(nullptr)
  {
  }
  buffer(const size_t size)
  {
    create(size);
  }
  ~buffer() = default;
  buffer (const buffer &) = delete;
  buffer& operator=(const buffer&) = delete;
  buffer (buffer &&rhs) = delete;
  buffer& operator=(buffer &&) = delete;

  void create(const size_t size) noexcept
  {
    size_ = size;
    length_ = 0;
    data_.reset(new byte[size+2]);
    // for safe string/wstring
    std::memset(data_.get(), 0, size+2);
  }
  [[nodiscard]] byte* data() const noexcept { return data_.get(); }
  [[nodiscard]] size_t size() const noexcept { return size_; }
  [[nodiscard]] size_t length() const noexcept { return length_; }
  bool set_length(const size_t length) noexcept { if (length > size_) return false; length_ = length; return true; }
  void clear() noexcept { length_ = 0; std::memset(data_.get(), 0, size_); }
  
private:
  size_t size_{};
  size_t length_{};
  std::shared_ptr<byte[]> data_;
};

template <typename T>
bool write_buffer(buffer& buf, const T* data, const size_t length)
{
    const size_t len_in_byte = sizeof(T) * length;
    if (len_in_byte + buf.length() > buf.size()) return false;
    std::memcpy(buf.data() + buf.length(), data, len_in_byte);
    buf.set_length(buf.length() + len_in_byte);
    return true;
}

template <std::size_t Size>
bool write_buffer(buffer& buf, const char (&data)[Size])
{
  return write_buffer(buf, data, Size-1);
}

template <std::size_t Size>
bool write_buffer(buffer& buf, const wchar_t (&data)[Size])
{
  return write_buffer(buf, data, Size-1);
}

using callback = std::function<void(buffer& in, buffer& out, bool& should_run)>;

class pipe_client
{
public:
  pipe_client();
  pipe_client(bool expect_error);
  bool transact();
  bool disabled() const noexcept;
  void reset() noexcept;
  
  buffer buf_req;
  buffer buf_res;
  
private:
  bool transact_internal();
  bool reconnect();

  bool expect_error_;
  int fail_count_;
  wil::unique_handle h_pipe_;
  wil::unique_event_nothrow ov_event_;
  OVERLAPPED ov_;
};

class pipe_server
{
public:
  enum pipe_state
  {
    reading,
    writing
  };
  
  pipe_server(
    HANDLE h_pipe,
    OVERLAPPED ov,
    HANDLE h_terminate,
    callback cb
  ) :
    state_(reading),
    has_pending_io_(false),
    h_pipe_(h_pipe),
    ov_(ov),
    h_terminate_(h_terminate),
    cb_(std::move(cb)),
    buf_req_(8192),
    buf_res_(8192),
    valid_(true) { }
  pipe_server(const pipe_server&) = delete;
  pipe_server& operator=(const pipe_server&) = delete;
  pipe_server(pipe_server&&) = delete;
  pipe_server& operator=(pipe_server&&) = delete;
  ~pipe_server();

  void run();
  void join();

private:
  void run_internal();
  void fetch_result();
  void dispatch_data();

  pipe_state state_;
  bool has_pending_io_;
  HANDLE h_pipe_;
  OVERLAPPED ov_;
  HANDLE h_terminate_;
  callback cb_;
  buffer buf_req_;
  buffer buf_res_;
  bool valid_;
  std::unique_ptr<std::thread> th_;
};

class orch
{
public:
  orch();
  orch(std::wstring pipe_name);
  orch(const orch&) = delete;
  orch(orch&&) = delete;
  orch& operator=(const orch&) = delete;
  orch& operator=(orch&&) = delete;
  ~orch();

  void start(const callback& cb);
  void stop();
  void join();

private:
  void run(const callback& cb);

  std::wstring pipe_name_;
  std::vector<std::unique_ptr<pipe_server>> pipes_;
  std::atomic<bool> running_;
  HANDLE h_terminate_;
};

}

