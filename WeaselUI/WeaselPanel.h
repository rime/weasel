#include <WeaselUI.h>

#include "Layout.h"
#include "d2d.h"
#include <WeaselUtility.h>

namespace weasel {

class WeaselPanel {
 public:
  WeaselPanel(UI& ui);
  ~WeaselPanel() {
    if (!m_current_zhung_icon.empty())
      DestroyIcon(m_iconEnabled);
    if (!m_current_ascii_icon.empty())
      DestroyIcon(m_iconAlpha);
    if (!m_current_half_icon.empty())
      DestroyIcon(m_iconHalf);
    if (!m_current_full_icon.empty())
      DestroyIcon(m_iconFull);
  }
  void MoveTo(RECT rc);
  void Refresh();

  BOOL IsWindow() const;
  void ShowWindow(int nCmdShow);
  void DestroyWindow();
  // release all resources associated with this panel (window + shared devices)
  void ReleaseAllResources();
  BOOL Create(HWND parent);
  bool GetIsReposition() { return m_istorepos; }

  static const int AUTOREV_TIMER = 20241209;
  static const int AUTOHIDE_TIMER = 20241107;

  HWND hwnd() const;

 private:
  void RedrawWindow() { InvalidateRect(m_hWnd, nullptr, true); }
  void _CreateLayout();
  bool _DrawPreedit(const Text& text, bool isPreedit);
  bool _DrawCandidates();
  void _ResizeWindow();
  void _Reposition(bool adj = false);
  void _TextOut(CRect& rc,
                const wstring& text,
                size_t cch,
                uint32_t color,
                ComPtr<IDWriteTextFormat1>& pTextFormat);
  void _HighlightRect(const RECT& rect,
                      float radius,
                      uint32_t border,
                      uint32_t back_color,
                      uint32_t shadow_color,
                      uint32_t border_color,
                      const IsToRoundStruct& roundInfo);
  CRect _GetInflatedCandRect(int i);
  void _CaptureRect(CRect& rect);
  void _UpdateHideCandidates();

  void _UpdateOffsetY(CRect& arc, CRect& prc);

  void DoPaint();
  void OnDestroy();
  HRESULT OnScroll(UINT uMsg, WPARAM wParam, LPARAM lParam);
  LRESULT OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam);
  LRESULT OnMouseActive(UINT uMsg, WPARAM wParam, LPARAM lParam);
  LRESULT OnLeftClickUp(UINT uMsg, WPARAM wParam, LPARAM lParam);
  LRESULT OnLeftClickDown(UINT uMsg, WPARAM wParam, LPARAM lParam);

  LRESULT MsgHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  static LRESULT CALLBACK WindowProc(HWND hwnd,
                                     UINT uMsg,
                                     WPARAM wParam,
                                     LPARAM lParam);

  // helper to clear any active timers
  void _ClearTimers();

  RECT m_inputPos{0, 0, 0, 0};
  CPoint m_lastCursorPos = {-1, -1};

  Context& m_ctx;
  Context& m_octx;
  Status& m_status;
  const bool& m_in_server;
  UIStyle& m_style;
  UIStyle& m_ostyle;

  int m_candidateCount;
  int m_lastCandidateCount = 0;
  int m_hoverIndex = -1;
  bool hide_candidates;

  int m_offsetys[MAX_CANDIDATES_COUNT];  // offset y for candidates when
                                         // vertical layout over bottom
  int m_offsety_preedit = 0;
  int m_offsety_aux = 0;
  bool m_istorepos = false;
  bool m_sticky = false;
  float m_bar_scale = 1.0f;
  HMONITOR m_hMonitor = NULL;
  bool m_redraw_by_monitor_change = false;
  // ------------------------------------------------------------
  an<D2D> m_pD2D;
  the<Layout> m_layout;
  HICON m_iconAlpha;
  HICON m_iconEnabled;
  HICON m_iconFull;
  HICON m_iconHalf;
  HICON m_iconDisabled;
  wstring m_current_zhung_icon;
  wstring m_current_ascii_icon;
  wstring m_current_half_icon;
  wstring m_current_full_icon;
  UICallbackFunc& m_uiCallback;
  // window handle and per-instance click timer
  HWND m_hWnd;
  UINT_PTR m_clickTimer = 0;
  UINT_PTR m_autoHideTimer = 0;

 public:
  void ShowWithTimeout(size_t millisec);
  bool IsCountingDown() const;
};
}  // namespace weasel
