#pragma once
#include <WeaselUI.h>
#include <WeaselIPC.h>
#include "SystemTraySDK.h"

#include <condition_variable>
#include <mutex>

#define WM_WEASEL_TRAY_NOTIFY (WEASEL_IPC_LAST_COMMAND + 100)

// Snapshot of the tray-relevant UI state, computed on the pipe worker thread
// and applied on the server message thread. Keeps Shell_NotifyIcon off the
// pipe worker threads (and away from g_api_mutex), avoiding the deadlock loop
// where the taskbar UI thread waits on the pipe while the server waits for the
// taskbar UI thread inside Shell_NotifyIcon.
struct WeaselTrayIconState {
  WeaselTrayIconState()
      : valid(false),
        display_tray_icon(false),
        disabled(false),
        ascii_mode(false) {}

  static WeaselTrayIconState From(const weasel::UIStyle& style,
                                  const weasel::Status& status) {
    WeaselTrayIconState state;
    state.valid = true;
    state.display_tray_icon = style.display_tray_icon;
    state.disabled = status.disabled;
    state.ascii_mode = status.ascii_mode;
    state.current_zhung_icon = style.current_zhung_icon;
    state.current_ascii_icon = style.current_ascii_icon;
    return state;
  }

  bool operator==(const WeaselTrayIconState& rhs) const {
    return valid == rhs.valid && display_tray_icon == rhs.display_tray_icon &&
           disabled == rhs.disabled && ascii_mode == rhs.ascii_mode &&
           current_zhung_icon == rhs.current_zhung_icon &&
           current_ascii_icon == rhs.current_ascii_icon;
  }

  bool operator!=(const WeaselTrayIconState& rhs) const {
    return !(*this == rhs);
  }

  bool valid;
  bool display_tray_icon;
  bool disabled;
  bool ascii_mode;
  std::wstring current_zhung_icon;
  std::wstring current_ascii_icon;
};

class WeaselTrayIcon : public CSystemTray {
 public:
  enum WeaselTrayMode {
    INITIAL,
    ZHUNG,
    ASCII,
    DISABLED,
  };

  WeaselTrayIcon(weasel::UI& ui);

  BOOL Create(HWND hTargetWnd);

  // Captures the tray-relevant state and posts a refresh request to the server
  // message thread. Never calls Shell_NotifyIcon itself.
  void RequestRefresh();
  void DisableRefresh();

  // Runs on the server message thread (no g_api_mutex held).
  void ApplyRefresh();

 protected:
  virtual void CustomizeMenu(HMENU hMenu);

  void Refresh(const WeaselTrayIconState& state);

  weasel::UIStyle& m_style;
  weasel::Status& m_status;
  WeaselTrayMode m_mode;
  std::wstring m_schema_zhung_icon;
  std::wstring m_schema_ascii_icon;
  bool m_disabled;

  // Guarded by m_state_mutex.
  bool m_refresh_enabled = true;
  bool m_refresh_pending = false;
  bool m_refresh_in_progress = false;
  WeaselTrayIconState m_pending_state;
  std::mutex m_state_mutex;
  std::condition_variable m_state_cv;
};
