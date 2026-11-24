#include "stdafx.h"
#include "WeaselTrayIcon.h"
#include <atlstr.h>

// nasty
#include <resource.h>

static UINT mode_icon[] = {IDI_ZH, IDI_ZH, IDI_EN, IDI_RELOAD};
static const WCHAR* mode_label[] = {NULL, /*L"中文"*/ NULL, /*L"西文"*/ NULL,
                                    L"Under maintenance"};

WeaselTrayIcon::WeaselTrayIcon(weasel::UI& ui)
    : m_style(ui.style()),
      m_status(ui.status()),
      m_mode(INITIAL),
      m_schema_zhung_icon(),
      m_schema_ascii_icon(),
      m_disabled(false) {}

WeaselTrayIcon::~WeaselTrayIcon() {
  DisableRefresh();
}

void WeaselTrayIcon::CustomizeMenu(HMENU hMenu) {}

BOOL WeaselTrayIcon::Create(HWND hTargetWnd) {
  std::promise<BOOL> created;
  std::future<BOOL> result = created.get_future();
  bool display_tray_icon = m_style.display_tray_icon;
  m_tray_thread = std::thread([this, hTargetWnd, display_tray_icon, &created] {
    TrayThreadProc(hTargetWnd, display_tray_icon, created);
  });
  return result.get();
}

void WeaselTrayIcon::TrayThreadProc(HWND hTargetWnd,
                                    bool display_tray_icon,
                                    std::promise<BOOL>& created) {
  HMODULE hModule = GetModuleHandle(NULL);
  CIcon icon;
  icon.LoadIconW(IDI_ZH);
  BOOL bRet =
      CSystemTray::Create(hModule, NULL, WM_WEASEL_TRAY_NOTIFY,
                          get_weasel_ime_name().c_str(), icon, IDR_MENU_POPUP);
  HWND hWnd = GetSafeHwnd();
  bool run = false;
  if (hWnd) {
    m_base_wndproc = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(
        hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(TrayWindowProc)));
    if (hTargetWnd) {
      SetTargetWnd(hTargetWnd);
    }
    if (!display_tray_icon) {
      RemoveIcon();
    } else {
      AddIcon();
    }
    std::lock_guard<std::mutex> lock(m_state_mutex);
    run = m_refresh_enabled;
    if (run) {
      m_tray_hwnd = hWnd;
      if (m_refresh_pending) {
        ::PostMessage(hWnd, WM_WEASEL_TRAY_REFRESH, 0, 0);
      }
    }
  }
  created.set_value(bRet);
  if (!hWnd) {
    return;
  }
  MSG msg;
  while (run && ::GetMessage(&msg, NULL, 0, 0) > 0) {
    ::TranslateMessage(&msg);
    ::DispatchMessage(&msg);
  }
  {
    std::lock_guard<std::mutex> lock(m_state_mutex);
    m_tray_hwnd = NULL;
  }
  RemoveIcon();
  ::DestroyWindow(m_hWnd);
  m_hWnd = NULL;
}

LRESULT CALLBACK WeaselTrayIcon::TrayWindowProc(HWND hWnd,
                                                UINT message,
                                                WPARAM wParam,
                                                LPARAM lParam) {
  WeaselTrayIcon* self = static_cast<WeaselTrayIcon*>(m_pThis);
  switch (message) {
    case WM_WEASEL_TRAY_REFRESH:
      self->ApplyPendingRefresh();
      return 0;
    case WM_WEASEL_TRAY_QUIT:
      ::PostQuitMessage(0);
      return 0;
  }
  return ::CallWindowProc(self->m_base_wndproc, hWnd, message, wParam, lParam);
}

void WeaselTrayIcon::RequestRefresh() {
  HWND hWnd = NULL;
  {
    std::lock_guard<std::mutex> lock(m_state_mutex);
    if (!m_refresh_enabled) {
      return;
    }
    m_pending_state = WeaselTrayIconState::From(m_style, m_status);
    if (m_refresh_pending) {
      return;
    }
    m_refresh_pending = true;
    hWnd = m_tray_hwnd;
  }
  if (hWnd) {
    ::PostMessage(hWnd, WM_WEASEL_TRAY_REFRESH, 0, 0);
  }
}

// Pending requests are coalesced: only the latest snapshot is applied.
void WeaselTrayIcon::ApplyPendingRefresh() {
  WeaselTrayIconState state;
  {
    std::lock_guard<std::mutex> lock(m_state_mutex);
    if (!m_refresh_pending || !m_refresh_enabled) {
      return;
    }
    state = m_pending_state;
    m_refresh_pending = false;
  }
  Refresh(state);
}

void WeaselTrayIcon::DisableRefresh() {
  HWND hWnd = NULL;
  {
    std::lock_guard<std::mutex> lock(m_state_mutex);
    m_refresh_enabled = false;
    m_refresh_pending = false;
    hWnd = m_tray_hwnd;
  }
  if (hWnd) {
    ::PostMessage(hWnd, WM_WEASEL_TRAY_QUIT, 0, 0);
  }
  if (m_tray_thread.joinable()) {
    m_tray_thread.join();
  }
}

void WeaselTrayIcon::Refresh(const WeaselTrayIconState& state) {
  if (!state.display_tray_icon &&
      !state.disabled)  // display notification when deploying
  {
    if (m_mode != INITIAL) {
      RemoveIcon();
      m_mode = INITIAL;
    }
    m_disabled = false;
    return;
  }
  WeaselTrayMode mode = state.disabled     ? DISABLED
                        : state.ascii_mode ? ASCII
                                           : ZHUNG;
  /* change icon, when
          1,mode changed
          2,icon changed
          3,both m_schema_zhung_icon and state.current_zhung_icon empty(for
     initialize) 4,both m_schema_ascii_icon and state.current_ascii_icon
     empty(for initialize)
  */
  if (mode != m_mode || m_schema_zhung_icon != state.current_zhung_icon ||
      (m_schema_zhung_icon.empty() && state.current_zhung_icon.empty()) ||
      m_schema_ascii_icon != state.current_ascii_icon ||
      (m_schema_ascii_icon.empty() && state.current_ascii_icon.empty())) {
    ShowIcon();
    m_mode = mode;
    m_schema_zhung_icon = state.current_zhung_icon;
    m_schema_ascii_icon = state.current_ascii_icon;
    if (mode == ASCII) {
      if (m_schema_ascii_icon.empty())
        SetIcon(mode_icon[mode]);
      else
        SetIcon(m_schema_ascii_icon.c_str());
    } else if (mode == ZHUNG) {
      if (m_schema_zhung_icon.empty())
        SetIcon(mode_icon[mode]);
      else
        SetIcon(m_schema_zhung_icon.c_str());
    } else
      SetIcon(mode_icon[mode]);

    if (mode_label[mode] && m_disabled == false) {
      CString info;
      info.LoadStringW(IDS_STR_UNDER_MAINTENANCE);
      ShowBalloon(info, get_weasel_ime_name().c_str());
      m_disabled = true;
    }
    if (m_mode != DISABLED)
      m_disabled = false;
  } else if (!Visible()) {
    ShowIcon();
  }
}
