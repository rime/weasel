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
  weasel::buffer* in;
  weasel::PipeMessage* in_msg;
  weasel::buffer::byte* in_data;
  weasel::buffer* out;
  DWORD* out_res_code;
  weasel::buffer::byte* out_data;
  
  explicit pipe_message(weasel::buffer& _in, weasel::buffer& _out) : in(&_in), out(&_out)
  {
    in_msg = reinterpret_cast<weasel::PipeMessage*>(in->data());
    in_data = in->data() + sizeof(weasel::PipeMessage);
    out_res_code = reinterpret_cast<DWORD*>(out->data());
    out_data = out->data() + sizeof(DWORD);
  }
};

struct rime_ui_message: message {};
