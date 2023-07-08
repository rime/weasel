#pragma once
#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <wil/resource.h>
#include <boost/core/span.hpp>

namespace weasel
{

class buffer
{
public:
  buffer(size_t size) : length_(0), span_(new char[ size ], size)
  {
  }

  ~buffer() { delete[ ] span_.data(); }

  char* data() const noexcept { return span_.data(); }
  size_t size() const noexcept { return span_.size(); }
  size_t size_bytes() const noexcept { return span_.size_bytes(); }
  size_t length() const noexcept { return length_; }
  void clear() noexcept { length_ = 0; std::memset(span_.data(), 0, span_.size()); }

  bool set_length(const size_t length)
  {
    if (length > span_.size()) return false;
    length_ = length;
    return true;
  }

  bool write_bytes(const char* buf, const size_t length)
  {
    if (length_ + length > size()) return false;
    std::copy_n(buf, length, span_.data() + length_);
    length_ += length;
    return true;
  }

  bool write_string(const std::string& s)
  {
    return write_bytes(s.c_str(), s.size() + 1);
  }

  bool read_bytes(std::string& out)
  {
    if (length_ < 1) return false;
    span_[ length_ - 1 ] = '\0';
    out.assign(span_.data());
    return true;
  }

  bool write_wstring(const std::wstring& s)
  {
    const wchar_t* cstr = s.c_str();
    constexpr auto f = sizeof(wchar_t) / sizeof(char);
    const size_t len = (s.size() + 1) * sizeof(wchar_t) / sizeof(char);
    return write_bytes(reinterpret_cast<const char*>(cstr), len);
  }

  bool read_wstring(std::wstring& out)
  {
    // Windows platform specific
    if (length_ < 2) return false;
    span_[ length_ - 1 ] = '\0';
    span_[ length_ - 2 ] = '\0';
    out.assign(reinterpret_cast<const wchar_t*>(span_.data()));
    return true;
  }

private:
  size_t length_;
  boost::span<char, boost::dynamic_extent> span_;
};

using pbuffer = std::shared_ptr<buffer>;
using callback = std::function<void(pbuffer in, pbuffer out, bool& should_run)>;

enum pipe_state
{
  reading,
  writing
};

class pipe
{
public:
  pipe(
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
    buf_req_(std::make_shared<buffer>(8192)),
    buf_res_(std::make_shared<buffer>(8192)),
    valid_(true) { }
  pipe(const pipe&) = delete;
  pipe& operator=(const pipe&) = delete;
  pipe(pipe&&) = delete;
  pipe& operator=(pipe&&) = delete;
  ~pipe();

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
  pbuffer buf_req_;
  pbuffer buf_res_;
  bool valid_;
  std::unique_ptr<std::thread> th_;
};



class orch
{
public:
  orch(std::wstring pipe_name);
  orch(const orch&) = delete;
  orch(orch&&) = delete;
  orch& operator=(const orch&) = delete;
  orch& operator=(orch&&) = delete;
  ~orch();

  void start(const callback& cb);
  void stop();

private:
  void run(const callback& cb);

  std::wstring pipe_name_;
  std::vector<std::unique_ptr<pipe>> pipes_;
  std::unique_ptr<std::thread> lobby_th_;
  std::atomic<bool> running_;
  HANDLE h_terminate_;
};

}

