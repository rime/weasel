#pragma once
#include <WeaselUI.h>
#include <WeaselIPC.h>
#include "SystemTraySDK.h"

#include <condition_variable>
#include <mutex>
#include <thread>

#define WM_WEASEL_TRAY_NOTIFY (WEASEL_IPC_LAST_COMMAND + 100)

// Snapshot of the tray-relevant UI state, computed on the pipe worker thread
// and applied on a dedicated tray thread. Keeps Shell_NotifyIcon off the pipe
// worker threads (and away from g_api_mutex) and off the server message thread:
// Shell_NotifyIcon waits on the taskbar UI thread with SMTO_BLOCK,
// and while the taskbar UI thread itself is waiting on the pipe,
// a pipe worker doing a cross-thread SetWindowPos/ShowWindow on the candidate
// window would otherwise wait on the blocked message thread, closing the
// deadlock loop.
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
  ~WeaselTrayIcon();

  BOOL Create(HWND hTargetWnd);

  // Captures the tray-relevant state and wakes the tray thread.
  // Never calls Shell_NotifyIcon itself, so it is safe from any thread and
  // under any lock.
  void RequestRefresh();
  // Stops the tray thread; waits for an in-flight refresh to finish.
  void DisableRefresh();

 protected:
  virtual void CustomizeMenu(HMENU hMenu);

  void Refresh(const WeaselTrayIconState& state);
  void RefreshThreadProc();

  weasel::UIStyle& m_style;
  weasel::Status& m_status;
  WeaselTrayMode m_mode;
  std::wstring m_schema_zhung_icon;
  std::wstring m_schema_ascii_icon;
  bool m_disabled;

  // Guarded by m_state_mutex.
  bool m_refresh_enabled = true;
  bool m_refresh_pending = false;
  WeaselTrayIconState m_pending_state;
  std::mutex m_state_mutex;
  std::condition_variable m_state_cv;
  std::thread m_refresh_thread;
};
