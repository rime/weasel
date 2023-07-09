#pragma once
#include <memory>
#include <mutex>
#include <weasel/ipc.h>
#include "RimeWithWeasel.h"
#include "Message.h"

class message_dispatcher
  : public CWindowImpl<message_dispatcher, CWindow,
                       CWinTraits<WS_DISABLED, WS_EX_TRANSPARENT>>
{
public:
  message_dispatcher();
  ~message_dispatcher();
  int init();
  int run();

  // win32 message
  DECLARE_WND_CLASS(WEASEL_IPC_WINDOW)
  BEGIN_MSG_MAP(WEASEL_IPC_WINDOW)
    MESSAGE_HANDLER(WM_CREATE, on_win32_create)
    MESSAGE_HANDLER(WM_DESTROY, on_win32_destroy)
    MESSAGE_HANDLER(WM_CLOSE, on_win32_close)
    MESSAGE_HANDLER(WM_QUERYENDSESSION, on_win32_queryendsession)
    MESSAGE_HANDLER(WM_ENDSESSION, on_win32_endsession)
    MESSAGE_HANDLER(WM_COMMAND, on_win32_command)
  END_MSG_MAP()
  LRESULT on_win32_create(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT on_win32_destroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT on_win32_close(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT on_win32_queryendsession(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT on_win32_endsession(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT on_win32_command(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
private:
  void stop();
  bool process_message(const message* message);
  bool process_message(const message* message, bool lock);
  // pipe message
  void pipe_msg_converter(weasel::buffer& in, weasel::buffer& out, bool& should_run);
  weasel::orch orch_;
  
  std::unique_ptr<RimeWithWeaselHandler> handler_;

  std::atomic<bool> running_;
  std::mutex mu_;
  wil::unique_mutex_nothrow mu_singleton_;
  // TODO:
  // weasel::UI
  // trayicon
};
