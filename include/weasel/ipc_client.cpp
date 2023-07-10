#include "pch.h"
#include <weasel/ipc.h>

#include "log.h"

namespace weasel::ipc
{
client::client() : client(false)
{
}

client::client(bool suppress_error)
  : session_id_(0),
    is_ime_(false),
    pipe_(suppress_error)
{
  // initialize client info
  // host name
  WCHAR path[MAX_PATH] = {0};
  DWORD len = GetModuleFileNameW(NULL, path, ARRAYSIZE(path));
  if (len)
  {
    PCWSTR start = path + len;
    while ((start > path) && (*(start -1) != '\\')) start--;
    app_name_ = std::wstring(start);
  } else
  {
    app_name_ = L"unknown";
  }
  // LOG(debug, "app_name_: {}", app_name_.c_str());
  
  // get client type
  std::string module_name(wil::details::GetCurrentModuleName());
  DLOG(debug, "module_name: {}", module_name);
  const std::string ime_suffix(".ime");
  is_ime_ = module_name.ends_with(ime_suffix);
}

client::~client()
{
  if (active()) end_session();
}

void client::shutdown_server()
{
  pipe_.write_ipc_header( {WEASEL_IPC_SHUTDOWN_SERVER, 0, 0});
  pipe_.transact();
}

void client::start_session()
{
  if (active() && echo()) return;

  pipe_.write_ipc_header({WEASEL_IPC_START_SESSION, 0, 0});
  pipe_.write_wstring(L"action=session\n");
  pipe_.write_wstring(L"session.client_app=" + app_name_);
  pipe_.write_wstring(L"session.client_type=");
  if (is_ime_) pipe_.write_wstring(L"ime\n");
  else pipe_.write_wstring(L"tsf\n");
  pipe_.write_wstring(L".\n");
  pipe_.transact();
  session_id_ = pipe_.read_ret_code();
}

void client::end_session()
{
  pipe_.write_ipc_header({WEASEL_IPC_END_SESSION, 0, session_id_});
  pipe_.transact();
  session_id_ = 0;
}

void client::start_maintenance()
{
  pipe_.write_ipc_header({WEASEL_IPC_START_MAINTENANCE, 0, session_id_});
  pipe_.transact();
  session_id_ = 0;
}

void client::end_maintenance()
{
  pipe_.write_ipc_header({WEASEL_IPC_END_MAINTENANCE, 0, session_id_});
  pipe_.transact();
  session_id_ = 0;
}

bool client::echo()
{
  if (!active()) return false;

  pipe_.write_ipc_header({WEASEL_IPC_ECHO, 0, session_id_});
  pipe_.transact();
  return pipe_.read_ret_code() == session_id_;
}

bool client::process_key_event(const key_event& e)
{
  if (!active()) return false;

  pipe_.write_ipc_header({WEASEL_IPC_PROCESS_KEY_EVENT, static_cast<uint32_t>(e), session_id_});
  pipe_.transact();
  return pipe_.read_ret_code() != 0;
}

bool client::commit_composition()
{
  if (!active()) return false;

  pipe_.write_ipc_header({WEASEL_IPC_COMMIT_COMPOSITION, 0, session_id_});
  pipe_.transact();
  return pipe_.read_ret_code() != 0;
}

bool client::clear_composition()
{
  if (!active()) return false;

  pipe_.write_ipc_header({WEASEL_IPC_CLEAR_COMPOSITION, 0, session_id_});
  pipe_.transact();
  return pipe_.read_ret_code() != 0;
}

void client::update_input_position(const RECT& rc)
{
  if (!active()) return;
	/*
	移位标志 = 1bit == 0
	height:0~127 = 7bit
	top:-2048~2047 = 12bit（有符号）
	left:-2048~2047 = 12bit（有符号）

	高解析度下：
	移位标志 = 1bit == 1
	height:0~254 = 7bit（舍弃低1位）
	top:-4096~4094 = 12bit（有符号，舍弃低1位）
	left:-4096~4094 = 12bit（有符号，舍弃低1位）
	*/
	int hi_res = (rc.bottom - rc.top >= 128 || rc.left < -2048 || rc.left >= 2048 || rc.top < -2048 || rc.top >= 2048);
	int left = std::max(-2048, std::min(2047, static_cast<int>(rc.left >> hi_res)));
	int top = std::max(-2048, std::min(2047, static_cast<int>(rc.top >> hi_res)));
	int height = std::max(0, std::min(127, static_cast<int>((rc.bottom - rc.top) >> hi_res)));
	DWORD compressed_rect = ((hi_res & 0x01) << 31) | ((height & 0x7f) << 24) | 
		                    ((top & 0xfff) << 12) | (left & 0xfff);

  pipe_.write_ipc_header({WEASEL_IPC_UPDATE_INPUT_POS, compressed_rect, session_id_});
  pipe_.transact();
}

void client::focus_in()
{
  pipe_.write_ipc_header({WEASEL_IPC_FOCUS_IN, 0, session_id_});
  pipe_.transact();
}

void client::focus_out()
{
  pipe_.write_ipc_header({WEASEL_IPC_FOCUS_OUT, 0, session_id_});
  pipe_.transact();
}

void client::tray_command(UINT menu_id)
{
  pipe_.write_ipc_header({WEASEL_IPC_TRAY_COMMAND, menu_id, session_id_});
  pipe_.transact();
}

bool client::get_response_data(const response_handler& handler) const
{
  if (!handler) false;

  // TODO
  return handler((LPWSTR)(pipe_.buf_res.data() + sizeof(DWORD)), (pipe_.buf_res.size() - sizeof(DWORD)) / sizeof(WCHAR));
}
}
