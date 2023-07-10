#pragma once

namespace weasel::ipc
{

class client
{
public:
  client();
  client(bool suppress_error);
  ~client();
  client(const client&) = delete;
  client& operator=(const client&) = delete;
  client(client &&) = delete;
  client& operator=(client &&) = delete;
  
  void shutdown_server();
  void start_session();
  void end_session();
  void start_maintenance();
  void end_maintenance();
  bool echo();
  // 请求服务处理按键消息
  bool process_key_event(const key_event& e);
  // 上屏正在編輯的文字
  bool commit_composition();
  // 清除正在編輯的文字
  bool clear_composition();
  // 更新输入位置
  void update_input_position(const RECT& rc);
  // 输入窗口获得焦点
  void focus_in();
  // 输入窗口失去焦点
  void focus_out();
  // 托盤菜單
  void tray_command(UINT menu_id);
  // 读取server返回的数据
  // TODO:
  using response_handler = std::function<bool (LPWSTR buffer, UINT length)>;
  bool get_response_data(const response_handler& handler) const;

private:
  [[nodiscard]] bool active() const noexcept { return session_id_ != 0; }
  
  uint32_t session_id_;
  std::wstring app_name_{};
  bool is_ime_;
  pipe_client pipe_;
};


}
