#include "stdafx.h"
#include "MessageDispatcher.h"
#include <winsparkle.h>
#include <weasel/util.h>
#include <weasel/log.h>
#include "Util.h"
#include "resource.h"

#define MSG_IS(type) auto msg = dynamic_cast<const type *>(message); msg != nullptr

message_dispatcher::message_dispatcher()
  : handler_(std::make_unique<RimeWithWeaselHandler>())
{
}

message_dispatcher::~message_dispatcher()
{
  stop();
}

int message_dispatcher::init()
{
  // check for singleton
  std::wstring instanceName = L"(WEASEL)Furandōru-Sukāretto-";
  instanceName += getUsername();
  HANDLE mu = ::CreateMutex(NULL, FALSE, instanceName.c_str());
  auto last_error = GetLastError();
  if (mu == NULL || last_error == ERROR_ALREADY_EXISTS || last_error == ERROR_ACCESS_DENIED)
  {
    LOG(info, "Another instance is running, exiting");
    return 1;
  }
  mu_singleton_.reset(mu);
  
  // create window
  HWND hwnd = Create(NULL);
  if (hwnd == NULL)
  {
    LOG_LASTERROR(critical, "Create hWnd failed");
    return -1;
  }

  // init win sparkle
	// win_sparkle_set_appcast_url("http://localhost:8000/weasel/update/appcast.xml");
  win_sparkle_set_registry_path(R"(Software\Rime\Weasel\Updates)");
  win_sparkle_init();

  // m_ui.Create(m_server.GetHWnd())
  // tray_icon.Create(...)
  // tray_icon.Refresh()

  handler_->Initialize();
  handler_->OnUpdateUI([this]() {process_message(new rime_ui_message{});});

  return 0;
}

int message_dispatcher::run()
{
  if (running_) return -1;
  running_ = true;

  // start pipe orchestrator in background
  std::thread orch_th([this]()
  {
    this->orch_.start([this](weasel::ipc::buffer& in, weasel::ipc::buffer& out, bool &should_run)
    {
      this->pipe_msg_converter(in, out, should_run);
    });
  });

  // start win32 msg loop
  CMessageLoop loop;
  _Module.AddMessageLoop(&loop);
  int ret = loop.Run();

  // program should exit
  this->orch_.join();
  orch_th.join();
  _Module.RemoveMessageLoop();

  return ret;
}

void message_dispatcher::stop()
{
  if (running_)
  {
    std::lock_guard lock(this->mu_);
    running_ = false;
    // tray_icon.RemoveIcon()
    // m_ui.destroy
    this->orch_.stop();
    handler_->Finalize();
  }

  win_sparkle_cleanup();
  PostMessage(WM_QUIT);
}

bool message_dispatcher::process_message(const message* message)
{
  return process_message(message, true);
}

bool message_dispatcher::process_message(const message* message, bool lock)
{
  using weasel::utils::install_dir;
  using namespace weasel;

  if (lock) std::lock_guard lg(this->mu_);

  // use msg instead of message
  if (MSG_IS(menu_message))
  {
    switch (msg->op)
    {
    case ID_WEASELTRAY_ENABLE_ASCII:
      handler_->SetOption(msg->session_id, "ascii_mode", true);
      break;
    case ID_WEASELTRAY_DISABLE_ASCII:
      handler_->SetOption(msg->session_id, "ascii_mode", false);
      break;
    case ID_WEASELTRAY_QUIT:
      stop();
      break;
    case ID_WEASELTRAY_DEPLOY:
      execute(install_dir() + LR"(\WeaselDeployer.exe)", L"/deploy");
      break;
    case ID_WEASELTRAY_SETTINGS:
      execute(install_dir() + LR"(\WeaselDeployer.exe)", {});
      break;
    case ID_WEASELTRAY_DICT_MANAGEMENT:
      execute(install_dir() + LR"(\WeaselDeployer.exe)", L"/dict");
      break;
    case ID_WEASELTRAY_SYNC:
      execute(install_dir() + LR"(\WeaselDeployer.exe)", L"/sync");
      break;
    case ID_WEASELTRAY_WIKI:
      open(L"https://rime.im/docs/");
      break;
    case ID_WEASELTRAY_HOMEPAGE:
      open(L"https://rime.im/");
      break;
    case ID_WEASELTRAY_FORUM:
      open(L"https://rime.im/discuss/");
      break;
    case ID_WEASELTRAY_CHECKUPDATE:
      check_update();
      break;
    case ID_WEASELTRAY_INSTALLDIR:
      explore(install_dir());
      break;
    case ID_WEASELTRAY_USERCONFIG:
      explore(WeaselUserDataPath());
      break;
    default:
      LOG(warn, "Unknown menu message({}, {})", msg->op, msg->session_id);
      return false;
    }
    return true;
  }
  else if(MSG_IS(pipe_message)) {
    // handle pipe msg
    auto out = msg->out;
    DWORD res = 0;
    auto data = msg->in_msg->data;
    auto session_id = msg->in_msg->session_id;
    
    // skip for result code
    weasel::ipc::write_buffer(*out, &res, 1);

    auto write_cb = [&](std::wstring msg) -> bool
    {
      weasel::ipc::write_buffer(*out, msg.c_str(), msg.length());
      // seems nobody cares about the return value
      return true;
    };

    using namespace weasel::ipc;
    switch (msg->in_msg->cmd)
    {
    case WEASEL_IPC_ECHO:
      DLOG(debug, "WEASEL_IPC_ECHO({},{})", msg->in_msg->data, msg->in_msg->session_id);
      if (handler_ == nullptr) { res = 0; break; }
      res = handler_->FindSession(session_id);
      break;
    case WEASEL_IPC_START_SESSION:
      DLOG(debug, "WEASEL_IPC_START_SESSION({},{})", msg->in_msg->data, msg->in_msg->session_id);
      if (handler_ == nullptr) { res = 0; break; }
      res = handler_->AddSession(reinterpret_cast<LPWSTR>(msg->in_data), write_cb); 
      break;
    case WEASEL_IPC_END_SESSION:
      DLOG(debug, "WEASEL_IPC_END_SESSION({},{})", msg->in_msg->data, msg->in_msg->session_id);
      if (handler_ == nullptr) { res = 0; break; }
      res = handler_->RemoveSession(session_id);
      break;
    case WEASEL_IPC_PROCESS_KEY_EVENT:
      DLOG(debug, "WEASEL_IPC_PROCESS_KEY_EVENT({},{})", msg->in_msg->data, msg->in_msg->session_id);
      {
        weasel::KeyEvent e(data);
      
        if (handler_ == nullptr) { res = 0; break; }
        res = handler_->ProcessKeyEvent(e, session_id, write_cb);
      }
      break;
    case WEASEL_IPC_SHUTDOWN_SERVER:
      DLOG(debug, "WEASEL_IPC_SHUTDOWN_SERVER({},{})", msg->in_msg->data, msg->in_msg->session_id);
      stop();
      break;
    case WEASEL_IPC_FOCUS_IN:
      DLOG(debug, "WEASEL_IPC_FOCUS_IN({},{})", msg->in_msg->data, msg->in_msg->session_id);
      {
        DWORD param = data;
      
        if (handler_ == nullptr) { res = 0; break; }
        handler_->FocusIn(param, session_id);
      }
      break;
    case WEASEL_IPC_FOCUS_OUT:
      DLOG(debug, "WEASEL_IPC_FOCUS_OUT({},{})", msg->in_msg->data, msg->in_msg->session_id);
      {
        DWORD param = data;
        
        if (handler_ == nullptr) { res = 0; break; }
        handler_->FocusOut(param, session_id);
      } 
      break;
    case WEASEL_IPC_UPDATE_INPUT_POS:
      DLOG(debug, "WEASEL_IPC_UPDATE_INPUT_POS({},{})", msg->in_msg->data, msg->in_msg->session_id);
      {
        if (handler_ == nullptr) { res = 0; break; }
        /*
         * 移位标志 = 1bit == 0
         * height: 0~127 = 7bit
         * top:-2048~2047 = 12bit（有符号）
         * left:-2048~2047 = 12bit（有符号）
         *
         * 高解析度下：
         * 移位标志 = 1bit == 1
         * height: 0~254 = 7bit（舍弃低1位）
         * top: -4096~4094 = 12bit（有符号，舍弃低1位）
         * left: -4096~4094 = 12bit（有符号，舍弃低1位）
         */
        RECT rc;
        constexpr int width = 6;
        int hi_res = (data >> 31) & 0x01;
        rc.left = ((data & 0x7ff) - (data & 0x800)) << hi_res;
        rc.top = (((data >> 12) & 0x7ff) - ((data >> 12) & 0x800)) << hi_res;
        int height = ((data >> 24) & 0x7f) << hi_res;
        rc.right = rc.left + width;
        rc.bottom = rc.top + height;

        POINT lt{rc.left, rc.top}, rb{rc.right, rc.bottom};
        PhysicalToLogicalPointForPerMonitorDPI(NULL, &lt);
        PhysicalToLogicalPointForPerMonitorDPI(NULL, &rb);
        rc = {lt.x, lt.y, rb.x, rb.y};
        handler_->UpdateInputPosition(rc, session_id);
      }
      break;
    case WEASEL_IPC_START_MAINTENANCE:
      DLOG(debug, "WEASEL_IPC_START_MAINTENANCE({},{})", msg->in_msg->data, msg->in_msg->session_id);
      if (handler_) handler_->StartMaintenance();
      break;
    case WEASEL_IPC_END_MAINTENANCE:
      DLOG(debug, "WEASEL_IPC_END_MAINTENANCE({},{})", msg->in_msg->data, msg->in_msg->session_id);
      if (handler_) handler_->EndMaintenance();
      break;
    case WEASEL_IPC_COMMIT_COMPOSITION:
      DLOG(debug, "WEASEL_IPC_COMMIT_COMPOSITION({},{})", msg->in_msg->data, msg->in_msg->session_id);
      if (handler_) handler_->CommitComposition(session_id);
      break;
    case WEASEL_IPC_CLEAR_COMPOSITION:
      DLOG(debug, "WEASEL_IPC_CLEAR_COMPOSITION({},{})", msg->in_msg->data, msg->in_msg->session_id);
      if (handler_) handler_->ClearComposition(session_id);
      break;
    case WEASEL_IPC_TRAY_COMMAND:
      DLOG(debug, "WEASEL_IPC_TRAY_COMMAND({},{})", msg->in_msg->data, msg->in_msg->session_id);
      {
        menu_message m(data, session_id);
        process_message(&m, false);
      }
      break;
    }

    *msg->out_res_code = res;
    return true;
  } else
  {
    DLOG(err, "Unknown message type {}", typeid(message).name());
  }
  
  return false;
}

void message_dispatcher::pipe_msg_converter(weasel::ipc::buffer& in, weasel::ipc::buffer& out, bool& should_run)
{
  out.clear();

  const pipe_message msg{in, out};
  process_message(&msg);
  
  in.clear();
}


#pragma region win32 message
LRESULT message_dispatcher::on_win32_create(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  ::SetWindowTextW(m_hWnd, WEASEL_IPC_WINDOW);
  return 0;
}

LRESULT message_dispatcher::on_win32_close(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  stop();
  return 0;
}

LRESULT message_dispatcher::on_win32_destroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  bHandled = FALSE;
  return 1;
}

LRESULT message_dispatcher::on_win32_queryendsession(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  return !running_;
}

LRESULT message_dispatcher::on_win32_endsession(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  // finalize request handler
  stop();
  return 0;
}

LRESULT message_dispatcher::on_win32_command(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  menu_message m(wParam, lParam);
  process_message(&m);
  return 0;
}





