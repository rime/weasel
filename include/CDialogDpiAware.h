#pragma once

#include <shellscalingapi.h>
#include <unordered_map>

template <typename T>
class CDialogDpiAware : public CDialogImpl<T> {
 public:
  ~CDialogDpiAware() {
    if (m_currentFont)
      DeleteObject(m_currentFont);
  }

 protected:
  BEGIN_MSG_MAP(T)
  MESSAGE_HANDLER(WM_DPICHANGED, OnDpiChanged)
  END_MSG_MAP()

  void InitCtrlRects() {
    ::GetWindowRect(m_hWnd, &m_rect);
    EnumChildWindows(
        m_hWnd,
        [](HWND hWnd, LPARAM lParam) {
          auto pThis = reinterpret_cast<T*>(lParam);
          RECT rect, rcDlg;
          ::GetClientRect(pThis->m_hWnd, &rcDlg);
          ::ClientToScreen(pThis->m_hWnd,
                           reinterpret_cast<LPPOINT>(&rcDlg.left));
          ::ClientToScreen(pThis->m_hWnd,
                           reinterpret_cast<LPPOINT>(&rcDlg.right));
          ::GetWindowRect(hWnd, &rect);
          rect.left -= rcDlg.left;
          rect.right -= rcDlg.left;
          rect.top -= rcDlg.top;
          rect.bottom -= rcDlg.top;
          pThis->m_controlOriginalRects[hWnd] = rect;
          return TRUE;
        },
        reinterpret_cast<LPARAM>(this));

    m_initialDpi = GetWindowDpi();
    m_currentDpi = m_initialDpi;

    HFONT font =
        reinterpret_cast<HFONT>(::SendMessage(m_hWnd, WM_GETFONT, 0, 0));
    if (font) {
      m_hasOriginalFont =
          GetObject(font, sizeof(m_originalFont), &m_originalFont) != 0;
    }
  }

 private:
  UINT GetWindowDpi() {
    HMONITOR monitor = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
    UINT dpiX = 96, dpiY = 96;
    if (SUCCEEDED(GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY)) &&
        dpiX != 0) {
      return dpiX;
    }
    return 96;
  }

  void ScaleControlsAndFonts(UINT newDpi) {
    const float scaleFactor =
        static_cast<float>(newDpi) / static_cast<float>(m_initialDpi);
    for (const auto& [hWnd, originalRect] : m_controlOriginalRects) {
      int width = static_cast<int>((originalRect.right - originalRect.left) *
                                   scaleFactor);
      int height = static_cast<int>((originalRect.bottom - originalRect.top) *
                                    scaleFactor);
      ::SetWindowPos(hWnd, nullptr,
                     static_cast<int>(originalRect.left * scaleFactor),
                     static_cast<int>(originalRect.top * scaleFactor), width,
                     height, SWP_NOZORDER | SWP_NOACTIVATE);
    }
    if (!m_hasOriginalFont)
      return;

    LOGFONT font = m_originalFont;
    font.lfHeight = static_cast<int>(font.lfHeight * scaleFactor);
    HFONT newFont = CreateFontIndirect(&font);
    if (!newFont)
      return;
    if (m_currentFont)
      DeleteObject(m_currentFont);
    ::SendMessage(m_hWnd, WM_SETFONT, reinterpret_cast<WPARAM>(newFont), TRUE);
    for (const auto& [hWnd, rect] : m_controlOriginalRects) {
      ::SendMessage(hWnd, WM_SETFONT, reinterpret_cast<WPARAM>(newFont), TRUE);
    }
    m_currentFont = newFont;
  }

  LRESULT OnDpiChanged(UINT, WPARAM wParam, LPARAM lParam, BOOL&) {
    const UINT newDpi = HIWORD(wParam);
    if (newDpi == m_currentDpi)
      return 0;
    const auto* rect = reinterpret_cast<const RECT*>(lParam);
    ::SetWindowPos(m_hWnd, nullptr, rect->left, rect->top,
                   rect->right - rect->left, rect->bottom - rect->top,
                   SWP_NOZORDER | SWP_NOACTIVATE);
    ScaleControlsAndFonts(newDpi);
    m_currentDpi = newDpi;
    this->Invalidate();
    return 0;
  }

  UINT m_currentDpi = 96;
  UINT m_initialDpi = 96;
  RECT m_rect{};
  std::unordered_map<HWND, RECT> m_controlOriginalRects;
  HFONT m_currentFont = nullptr;
  LOGFONT m_originalFont{};
  bool m_hasOriginalFont = false;
};
