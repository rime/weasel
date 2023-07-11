#pragma once
#include <weasel/ipc.h>

struct message
{
  virtual ~message() = default;
};

struct menu_message : message
{
  UINT op{};
  UINT session_id{};

  explicit menu_message(WPARAM wparam, LPARAM lparam) : op(LOWORD(wparam)), session_id(lparam) {}
};

struct pipe_message : message
{
  weasel::ipc::buffer* in;
  weasel::ipc::ipc_header* in_msg;
  weasel::ipc::buffer::byte* in_data;
  weasel::ipc::buffer* out;
  DWORD* out_res_code;
  weasel::ipc::buffer::byte* out_data;
  
  explicit pipe_message(weasel::ipc::buffer& _in, weasel::ipc::buffer& _out) : in(&_in), out(&_out)
  {
    in_msg = reinterpret_cast<weasel::ipc::ipc_header*>(in->data());
    in_data = in->data() + sizeof(weasel::ipc::ipc_header);
    out_res_code = reinterpret_cast<DWORD*>(out->data());
    out_data = out->data() + sizeof(DWORD);
  }
};

struct rime_ui_message: message {};
