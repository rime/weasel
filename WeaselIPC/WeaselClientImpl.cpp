#include "stdafx.h"
#include "WeaselClientImpl.h"
#include <StringAlgorithm.hpp>
#include <future>
#include <chrono>

using namespace weasel;

ClientImpl::ClientImpl()
    : session_id(0), channel(GetPipeName()), is_ime(false) {
  _InitializeClientInfo();
}

ClientImpl::~ClientImpl() {
  if (channel.Connected())
    Disconnect();
}

// http://stackoverflow.com/questions/557081/how-do-i-get-the-hmodule-for-the-currently-executing-code
HMODULE GetCurrentModule() {  // NB: XP+ solution!
  HMODULE hModule = NULL;
  GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                    (LPCTSTR)GetCurrentModule, &hModule);

  return hModule;
}

void ClientImpl::_InitializeClientInfo() {
  // get app name
  WCHAR exe_path[MAX_PATH] = {0};
  GetModuleFileName(NULL, exe_path, MAX_PATH);
  std::wstring path = exe_path;
  size_t separator_pos = path.find_last_of(L"\\/");
  if (separator_pos < path.size())
    app_name = path.substr(separator_pos + 1);
  else
    app_name = path;
  to_lower(app_name);
  // determine client type
  GetModuleFileName(GetCurrentModule(), exe_path, MAX_PATH);
  path = exe_path;
  to_lower(path);
  is_ime = ends_with(path, L".ime");
}

bool ClientImpl::Connect(ServerLauncher const& launcher) {
  return channel.Connect();
}

void ClientImpl::Disconnect() {
  if (_Active())
    EndSession();
  channel.Disconnect();
}

void ClientImpl::ShutdownServer() {
  _SendMessage(WEASEL_IPC_SHUTDOWN_SERVER, 0, 0);
}

bool ClientImpl::ProcessKeyEvent(KeyEvent const& keyEvent) {
  if (!_Active())
    return false;

  LRESULT ret =
      _SendMessage(WEASEL_IPC_PROCESS_KEY_EVENT, keyEvent, session_id);
  return ret != 0;
}

bool ClientImpl::CommitComposition() {
  if (!_Active())
    return false;

  LRESULT ret = _SendMessage(WEASEL_IPC_COMMIT_COMPOSITION, 0, session_id);
  return ret != 0;
}

bool ClientImpl::ClearComposition() {
  if (!_Active())
    return false;

  LRESULT ret = _SendMessage(WEASEL_IPC_CLEAR_COMPOSITION, 0, session_id);
  return ret != 0;
}

bool ClientImpl::SelectCandidateOnCurrentPage(size_t index) {
  if (!_Active())
    return false;
  LRESULT ret = _SendMessage(WEASEL_IPC_SELECT_CANDIDATE_ON_CURRENT_PAGE, index,
                             session_id);
  return ret != 0;
}

bool ClientImpl::HighlightCandidateOnCurrentPage(size_t index) {
  if (!_Active())
    return false;
  LRESULT ret = _SendMessage(WEASEL_IPC_HIGHLIGHT_CANDIDATE_ON_CURRENT_PAGE,
                             index, session_id);
  return ret != 0;
}

bool ClientImpl::ChangePage(bool backward) {
  if (!_Active())
    return false;
  LRESULT ret = _SendMessage(WEASEL_IPC_CHANGE_PAGE, backward, session_id);
  return ret != 0;
}

void ClientImpl::UpdateInputPosition(RECT const& rc) {
  if (!_Active())
    return;
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
  int hi_res =
      static_cast<int>(rc.bottom - rc.top >= 128 || rc.left < -2048 ||
                       rc.left >= 2048 || rc.top < -2048 || rc.top >= 2048);
  int left = max(-2048, min(2047, rc.left >> hi_res));
  int top = max(-2048, min(2047, rc.top >> hi_res));
  int height = max(0, min(127, (rc.bottom - rc.top) >> hi_res));
  DWORD compressed_rect = ((hi_res & 0x01) << 31) | ((height & 0x7f) << 24) |
                          ((top & 0xfff) << 12) | (left & 0xfff);
  _SendMessage(WEASEL_IPC_UPDATE_INPUT_POS, compressed_rect, session_id);
}

void ClientImpl::FocusIn() {
  DWORD client_caps = 0; /* TODO */
  _SendMessage(WEASEL_IPC_FOCUS_IN, client_caps, session_id);
}

void ClientImpl::FocusOut() {
  _SendMessage(WEASEL_IPC_FOCUS_OUT, 0, session_id);
}

void ClientImpl::TrayCommand(UINT menuId) {
  _SendMessage(WEASEL_IPC_TRAY_COMMAND, menuId, session_id);
}

void ClientImpl::StartSession() {
  if (_Active() && Echo())
    return;

  _WriteClientInfo();
  UINT ret = _SendMessage(WEASEL_IPC_START_SESSION, 0, 0);
  session_id = ret;
}

void ClientImpl::EndSession() {
  _SendMessage(WEASEL_IPC_END_SESSION, 0, session_id);
  session_id = 0;
}

void ClientImpl::StartMaintenance() {
  _SendMessage(WEASEL_IPC_START_MAINTENANCE, 0, 0);
  session_id = 0;
}

void ClientImpl::EndMaintenance() {
  _SendMessage(WEASEL_IPC_END_MAINTENANCE, 0, 0);
  session_id = 0;
}

bool ClientImpl::Echo() {
  if (!_Active())
    return false;

  UINT serverEcho = _SendMessage(WEASEL_IPC_ECHO, 0, session_id);
  return (serverEcho == session_id);
}

bool ClientImpl::GetResponseData(ResponseHandler const& handler) {
  if (!handler) {
    return false;
  }

  return channel.HandleResponseData(handler);
}

bool ClientImpl::_WriteClientInfo() {
  channel << L"action=session\n";
  channel << L"session.client_app=" << app_name.c_str() << L"\n";
  channel << L"session.client_type=" << (is_ime ? L"ime" : L"tsf") << L"\n";
  channel << L".\n";
  return true;
}

LRESULT ClientImpl::_SendMessage(WEASEL_IPC_COMMAND Msg,
                                 DWORD wParam,
                                 DWORD lParam) {
  try {
    PipeMessage req{Msg, wParam, lParam};
    auto future = std::async(std::launch::async,
                             [this, &req]() { return channel.Transact(req); });

    // wait Transact complete or overtime
    if (future.wait_for(std::chrono::seconds(2)) ==
        std::future_status::timeout) {
      // Transact overtime
      return 0;
    } else {
      // Transact complete
      return future.get();
    }
  } catch (DWORD /* ex */) {
    return 0;
  }
}

Client::Client() : m_pImpl(new ClientImpl()) {}

Client::~Client() {
  if (m_pImpl)
    delete m_pImpl;
}

bool Client::Connect(ServerLauncher launcher) {
  return m_pImpl->Connect(launcher);
}

void Client::Disconnect() {
  m_pImpl->Disconnect();
}

void Client::ShutdownServer() {
  m_pImpl->ShutdownServer();
}

bool Client::ProcessKeyEvent(KeyEvent const& keyEvent) {
  return m_pImpl->ProcessKeyEvent(keyEvent);
}

bool Client::CommitComposition() {
  return m_pImpl->CommitComposition();
}

bool Client::ClearComposition() {
  return m_pImpl->ClearComposition();
}

bool Client::SelectCandidateOnCurrentPage(size_t index) {
  return m_pImpl->SelectCandidateOnCurrentPage(index);
}

bool Client::HighlightCandidateOnCurrentPage(size_t index) {
  return m_pImpl->HighlightCandidateOnCurrentPage(index);
}

bool Client::ChangePage(bool backward) {
  return m_pImpl->ChangePage(backward);
}

void Client::UpdateInputPosition(RECT const& rc) {
  m_pImpl->UpdateInputPosition(rc);
}

void Client::FocusIn() {
  m_pImpl->FocusIn();
}

void Client::FocusOut() {
  m_pImpl->FocusOut();
}

void Client::StartSession() {
  m_pImpl->StartSession();
}

void Client::EndSession() {
  m_pImpl->EndSession();
}

void Client::StartMaintenance() {
  m_pImpl->StartMaintenance();
}

void Client::EndMaintenance() {
  m_pImpl->EndMaintenance();
}

void Client::TrayCommand(UINT menuId) {
  m_pImpl->TrayCommand(menuId);
}

bool Client::Echo() {
  return m_pImpl->Echo();
}

bool Client::GetResponseData(ResponseHandler handler) {
  return m_pImpl->GetResponseData(handler);
}
