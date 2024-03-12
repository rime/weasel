#include "stdafx.h"
#include "WeaselTrayIcon.h"

// nasty
#include <resource.h>

static UINT mode_icon[] = {IDI_ZH, IDI_ZH, IDI_EN, IDI_RELOAD};
static const WCHAR* mode_label[] = {NULL, /*L"中文"*/ NULL, /*L"西文"*/ NULL,
                                    L"維護中"};

WeaselTrayIcon::WeaselTrayIcon(weasel::UI& ui)
    : m_style(ui.style()),
      m_status(ui.status()),
      m_mode(INITIAL),
      m_schema_zhung_icon(),
      m_schema_ascii_icon(),
      m_disabled(false) {}

void WeaselTrayIcon::CustomizeMenu(HMENU hMenu) {}

BOOL WeaselTrayIcon::Create(HWND hTargetWnd) {
  HMODULE hModule = GetModuleHandle(NULL);
  CIcon icon;
  icon.LoadIconW(IDI_ZH);
  BOOL bRet = CSystemTray::Create(hModule, NULL, WM_WEASEL_TRAY_NOTIFY,
                                  WEASEL_IME_NAME, icon, IDR_MENU_POPUP);
  if (hTargetWnd) {
    SetTargetWnd(hTargetWnd);
  }
  if (!m_style.display_tray_icon) {
    RemoveIcon();
  }
  return bRet;
}

void WeaselTrayIcon::Refresh() {
  if (!m_style.display_tray_icon &&
      !m_status.disabled)  // display notification when deploying
  {
    if (m_mode != INITIAL) {
      RemoveIcon();
      m_mode = INITIAL;
    }
    m_disabled = false;
    return;
  }
  WeaselTrayMode mode = m_status.disabled     ? DISABLED
                        : m_status.ascii_mode ? ASCII
                                              : ZHUNG;
  /* change icon, when
          1,mode changed
          2,icon changed
          3,both m_schema_zhung_icon and m_style.current_zhung_icon empty(for
     initialize) 4,both m_schema_ascii_icon and m_style.current_ascii_icon
     empty(for initialize)
  */
  if (mode != m_mode || m_schema_zhung_icon != m_style.current_zhung_icon ||
      (m_schema_zhung_icon.empty() && m_style.current_zhung_icon.empty()) ||
      m_schema_ascii_icon != m_style.current_ascii_icon ||
      (m_schema_ascii_icon.empty() && m_style.current_ascii_icon.empty())) {
    ShowIcon();
    m_mode = mode;
    m_schema_zhung_icon = m_style.current_zhung_icon;
    m_schema_ascii_icon = m_style.current_ascii_icon;
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
      if (GetThreadUILanguage() ==
          MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL))
        ShowBalloon(mode_label[mode], WEASEL_IME_NAME);
      else
        ShowBalloon(L"维护中", WEASEL_IME_NAME);
      m_disabled = true;
    }
    if (m_mode != DISABLED)
      m_disabled = false;
  } else if (!Visible()) {
    ShowIcon();
  }
}
