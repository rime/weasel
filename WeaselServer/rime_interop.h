#pragma once
#include <unordered_map>
#include <string>
#include <weasel/ipc.h>

namespace weasel::server
{

class rime
{
public:
  using app_options_t = std::unordered_map<std::string, bool>;
  
  rime();
  ~rime();
  rime(const rime&) = delete;
  rime& operator=(const rime&) = delete;
  rime(rime&&) = delete;
  rime& operator=(rime&&) = delete;

  void init();
  void finalize();
  session_id_t find_session(session_id_t session_id) const;
  session_id_t add_session(const ipc::buffer& in, ipc::buffer& out);
  session_id_t remove_session(session_id_t session_id);
  bool process_key_event(ipc::key_event key_event, session_id_t session_id, ipc::buffer& out);
  void commit_composition(session_id_t session_id);
  void clear_composition(session_id_t session_id);
  void focus_in(uint32_t param, session_id_t session_id);
  void focus_out(uint32_t param, session_id_t session_id);
  void update_input_position(const RECT &rc, session_id_t session_id);
  void start_maintenance();
  void end_maintenance();
  void set_option(session_id_t session_id, const std::string& opt, bool val);
  void set_notify_msg(const char* message_type, const char* message_value);
  
private:
  
  bool disabled_;
  uint32_t active_session_;
  std::unordered_map<std::string, app_options_t> app_options_{};
  std::string notify_msg_type_;
  std::string notify_msg_value_;
  std::string last_schema_id_;
};

}
