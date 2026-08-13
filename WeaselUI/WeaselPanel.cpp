#include "WeaselPanel.h"
#include "FullScreenLayout.h"
#include "HorizontalLayout.h"
#include "VHorizontalLayout.h"
#include "VerticalLayout.h"
#include <filesystem>
#include <memory>
#include <resource.h>
#include <windowsx.h>
#include <wrl/client.h>

namespace fs = std::filesystem;

using namespace weasel;

#define DPI_SCALE(t) (int)(t * m_pD2D->m_dpiScaleLayout)
#define COLORTRANSPARENT(color) ((color & 0xff000000) == 0)
#define COLORNOTTRANSPARENT(color) ((color & 0xff000000) != 0)
#define TRANS_COLOR 0x00000000
#define HALF_ALPHA_COLOR(color) \
  ((((color & 0xff000000) >> 25) & 0xff) << 24) | (color & 0x00ffffff)

namespace {
class ThreadDpiAwarenessScope {
 public:
  ThreadDpiAwarenessScope() {
    using SetThreadDpiAwarenessContextFunc =
        DPI_AWARENESS_CONTEXT(WINAPI*)(DPI_AWARENESS_CONTEXT);
    const auto pSetThreadDpiAwarenessContext =
        reinterpret_cast<SetThreadDpiAwarenessContextFunc>(GetProcAddress(
            GetModuleHandleW(L"user32.dll"), "SetThreadDpiAwarenessContext"));
    if (pSetThreadDpiAwarenessContext) {
      old_context_ = pSetThreadDpiAwarenessContext(
          DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
      setter_ = pSetThreadDpiAwarenessContext;
    }
  }

  ~ThreadDpiAwarenessScope() {
    if (setter_) {
      setter_(old_context_);
    }
  }

 private:
  DPI_AWARENESS_CONTEXT old_context_ = nullptr;
  DPI_AWARENESS_CONTEXT(WINAPI* setter_)(DPI_AWARENESS_CONTEXT) = nullptr;
};

wstring FormatCandidateLabel(const wstring& label, const wchar_t* format) {
  wchar_t buffer[128];
  swprintf_s<128>(buffer, format, label.c_str());
  return wstring(buffer);
}
}  // namespace

void LoadIconIfNeed(wstring& oicofile,
                    const wstring& icofile,
                    HICON& hIcon,
                    UINT id) {
  if (oicofile == icofile)
    return;
  if (!oicofile.empty() && hIcon)
    DestroyIcon(hIcon);
  oicofile = icofile;
  const int STATUS_ICON_SIZE = GetSystemMetrics(SM_CXICON);
  HINSTANCE hInstance = GetModuleHandle(NULL);
  if (!icofile.empty() && fs::exists(fs::path(icofile)))
    hIcon =
        (HICON)LoadImage(hInstance, icofile.c_str(), IMAGE_ICON,
                         STATUS_ICON_SIZE, STATUS_ICON_SIZE, LR_LOADFROMFILE);
  else
    hIcon = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(id), IMAGE_ICON,
                             STATUS_ICON_SIZE, STATUS_ICON_SIZE, LR_SHARED);
}

WeaselPanel::WeaselPanel(UI& ui)
    : m_hWnd(nullptr),
      m_ctx(ui.ctx()),
      m_octx(ui.octx()),
      m_layout(nullptr),
      m_pD2D(nullptr),
      m_status(ui.status()),
      m_in_server(ui.InServer()),
      m_style(ui.style()),
      m_uiCallback(ui.uiCallback()),
      m_ostyle(ui.ostyle()),
      m_candidateCount(0),
      m_lastCandidateCount(0),
      hide_candidates(false) {
  // Prepare shared graphics resources early to reduce first paint latency.
  m_pD2D = std::make_shared<D2D>(m_style);
  auto hInstance = GetModuleHandle(nullptr);
  m_iconAlpha = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_EN), IMAGE_ICON,
                                 STATUS_ICON_SIZE, STATUS_ICON_SIZE, LR_SHARED);
  m_iconEnabled =
      (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_ZH), IMAGE_ICON,
                       STATUS_ICON_SIZE, STATUS_ICON_SIZE, LR_SHARED);
  m_iconFull =
      (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_FULL_SHAPE), IMAGE_ICON,
                       STATUS_ICON_SIZE, STATUS_ICON_SIZE, LR_SHARED);
  m_iconHalf =
      (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_HALF_SHAPE), IMAGE_ICON,
                       STATUS_ICON_SIZE, STATUS_ICON_SIZE, LR_SHARED);
  m_iconDisabled =
      (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_RELOAD), IMAGE_ICON,
                       STATUS_ICON_SIZE, STATUS_ICON_SIZE, LR_SHARED);
}

BOOL WeaselPanel::IsWindow() const {
  return ::IsWindow(m_hWnd);
}

HWND WeaselPanel::hwnd() const {
  return m_hWnd;
}

void WeaselPanel::_ClearTimers() {
  if (!m_hWnd)
    return;
  if (m_clickTimer) {
    ::KillTimer(m_hWnd, AUTOREV_TIMER);
    m_clickTimer = 0;
  }
  if (m_autoHideTimer) {
    ::KillTimer(m_hWnd, AUTOHIDE_TIMER);
    m_autoHideTimer = 0;
  }
}

void WeaselPanel::ShowWithTimeout(size_t millisec) {
  if (!m_hWnd)
    return;
  ShowWindow(SW_SHOWNA);
  // cancel existing auto-hide timer if any
  if (m_autoHideTimer) {
    ::KillTimer(m_hWnd, m_autoHideTimer);
    m_autoHideTimer = 0;
  }
  m_autoHideTimer =
      ::SetTimer(m_hWnd, AUTOHIDE_TIMER, static_cast<UINT>(millisec), NULL);
}

bool WeaselPanel::IsCountingDown() const {
  return m_autoHideTimer != 0;
}

void WeaselPanel::ShowWindow(int nCmdShow) {
  ::ShowWindow(m_hWnd, nCmdShow);
  if (m_pD2D) {
    if (nCmdShow != SW_HIDE) {
      // ensure window resources exist when showing
      if (!m_pD2D->swapChain) {
        m_pD2D->AttachWindow(m_hWnd);
        if (!m_style.font_face.empty())
          m_pD2D->InitDirectWriteResources();
      }
    }
  }
}

void WeaselPanel::DestroyWindow() {
  // clear timers before releasing window resources
  _ClearTimers();

  if (m_pD2D) {
    m_pD2D->ReleaseWindowResources();
    // mark D2D as detached from any window
    m_pD2D->m_hWnd = nullptr;
  }
  ::DestroyWindow(m_hWnd);
  m_hWnd = nullptr;
}

void WeaselPanel::ReleaseAllResources() {
  // ensure timers are killed before releasing window/device resources
  _ClearTimers();

  if (m_pD2D) {
    m_pD2D->ReleaseWindowResources();
    // explicitly reset shared devices
    DeviceResources::Get().Reset();
    m_pD2D.reset();
  }
}

void WeaselPanel::MoveTo(RECT rc) {
  if (!m_hWnd || !m_layout)
    return;
  m_redraw_by_monitor_change = false;
  bool should_reset_sticky =
      (m_ctx.empty() || (abs(rc.left - m_inputPos.left) > 50) ||
       (abs(rc.top - m_inputPos.top) > 50));
  if (should_reset_sticky && m_sticky) {
    m_sticky = false;
    m_inputPos = rc;
    m_inputPos.bottom += 6;
    _Reposition(true);
    RedrawWindow();
    return;
  }
  if (m_style.ascii_tip_follow_cursor && m_ctx.empty() &&
      (!m_status.composing) && m_layout->ShouldDisplayStatusIcon()) {
    POINT p;
    ::GetCursorPos(&p);
    RECT irc{p.x - STATUS_ICON_SIZE, p.y - STATUS_ICON_SIZE, p.x, p.y};
    m_inputPos = irc;
    _Reposition(true);
    RedrawWindow();
  } else if (!(rc.left == m_inputPos.left && rc.bottom != m_inputPos.bottom &&
               abs(rc.bottom - m_inputPos.bottom) < 6) ||
             m_layout->ShouldDisplayStatusIcon()) {
    m_inputPos = rc;
    m_inputPos.bottom += 6;
    bool m_istorepos_buf = m_istorepos;
    _Reposition(true);
    if (m_redraw_by_monitor_change) {
      Refresh();
      return;
    }
    if (m_istorepos != m_istorepos_buf || !m_ctx.aux.empty() ||
        m_layout->ShouldDisplayStatusIcon()) {
      RedrawWindow();
    }
  }
}

void WeaselPanel::_ResizeWindow() {
  CSize& size = m_layout->GetContentSize();
  SetWindowPos(m_hWnd, 0, 0, 0, size.cx, size.cy,
               SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOREDRAW);
  m_pD2D->OnResize(size.cx, size.cy);
}

void WeaselPanel::_CreateLayout() {
  the<Layout> layout;
  if (m_style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT ||
      m_style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT_FULLSCREEN) {
    layout =
        std::make_unique<VHorizontalLayout>(m_style, m_ctx, m_status, m_pD2D);
  } else {
    if (m_style.layout_type == UIStyle::LAYOUT_VERTICAL ||
        m_style.layout_type == UIStyle::LAYOUT_VERTICAL_FULLSCREEN) {
      layout =
          std::make_unique<VerticalLayout>(m_style, m_ctx, m_status, m_pD2D);
    } else if (m_style.layout_type == UIStyle::LAYOUT_HORIZONTAL ||
               m_style.layout_type == UIStyle::LAYOUT_HORIZONTAL_FULLSCREEN) {
      layout =
          std::make_unique<HorizontalLayout>(m_style, m_ctx, m_status, m_pD2D);
    }
  }
  if (IS_FULLSCREENLAYOUT(m_style)) {
    layout = std::make_unique<FullScreenLayout>(
        m_style, m_ctx, m_status, m_inputPos, std::move(layout), m_pD2D);
  }
  m_layout = std::move(layout);
}

void WeaselPanel::_UpdateHideCandidates() {
  bool should_show_icon =
      (m_status.ascii_mode || !m_status.composing || !m_ctx.aux.empty());
  m_candidateCount = MIN((int)m_ctx.cinfo.candies.size(), MAX_CANDIDATES_COUNT);
  // When candidate list vanishes, release sticky top/bottom placement state.
  if (m_lastCandidateCount > 0 && m_candidateCount == 0) {
    m_sticky = false;
  }
  m_lastCandidateCount = m_candidateCount;
  // check if to hide candidates window
  // show tips status, two kind of situation: 1) only aux strings, don't care
  // icon status; 2)only icon(ascii mode switching)
  bool show_tips =
      m_in_server &&
      ((!m_ctx.aux.empty() && m_ctx.cinfo.empty() && m_ctx.preedit.empty()) ||
       (m_ctx.empty() && should_show_icon));
  // show schema menu status: schema_id == L".default"
  bool show_schema_menu = m_status.schema_id == L".default";
  bool margin_negative =
      (DPI_SCALE(m_style.margin_x) < 0 || DPI_SCALE(m_style.margin_y) < 0);
  // when to hide_cadidates?
  // 1. margin_negative, and not in show tips mode( ascii switching /
  // half-full switching / simp-trad switching / error tips), and not in
  // schema menu
  // 2. inline preedit without candidates
  bool inline_no_candidates =
      (m_style.inline_preedit && m_candidateCount == 0) && !show_tips;
  hide_candidates = inline_no_candidates ||
                    (margin_negative && !show_tips && !show_schema_menu);
}

void WeaselPanel::Refresh() {
  if (!m_hWnd)
    return;
  if (!m_pD2D)
    m_pD2D = std::make_shared<D2D>(m_style);
  m_pD2D->AttachWindow(m_hWnd);
  if (!m_style.font_face.empty() &&
      (m_ostyle != m_style || !m_pD2D->pTextFormat))
    m_pD2D->InitDirectWriteResources();
  m_ostyle = m_style;
  _UpdateHideCandidates();
  auto hr = m_pD2D->direct3dDevice
                ? m_pD2D->direct3dDevice->GetDeviceRemovedReason()
                : DXGI_ERROR_DEVICE_REMOVED;
  if (hr != S_OK) {
    DEBUG << "Device removed detected: " << HRESULTToString(hr);
    DeviceResources::Get().Reset();
    if (m_pD2D->swapChain)  // only reinit window resources if attached
      m_pD2D->InitDirect2D();
  }
  _CreateLayout();
  m_layout->DoLayout();
  _ResizeWindow();
  if (m_preview_mode) {
    if (!m_preview_positioned && !m_preview_detached)
      RepositionPreview();
  } else {
    _Reposition();
  }
  RedrawWindow();
}

void WeaselPanel::RepositionPreview() {
  if (!m_hWnd || !m_preview_mode || m_preview_detached || !m_parent ||
      !m_layout)
    return;
  RECT parent_rect{};
  if (!GetWindowRect(m_parent, &parent_rect))
    return;
  const int width = m_layout->GetContentSize().cx;
  const int height = m_layout->GetContentSize().cy;
  const int x =
      parent_rect.left + (parent_rect.right - parent_rect.left - width) / 2;
  const int y = parent_rect.top - height;
  SetWindowPos(m_hWnd, HWND_TOP, x, y, 0, 0,
               SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
  m_preview_positioned = true;
}

BOOL WeaselPanel::Create(HWND parent, bool preview_mode) {
  if (m_hWnd)
    return !!m_hWnd;
  m_parent = parent;
  m_preview_mode = preview_mode;
  m_preview_positioned = false;
  m_preview_detached = false;
  m_hoverIndex = -1;
  POINT cursor_pos = {0, 0};
  if (::GetCursorPos(&cursor_pos)) {
    m_lastCursorPos = CPoint(cursor_pos.x, cursor_pos.y);
  } else {
    m_lastCursorPos = CPoint(-1, -1);
  }
  HINSTANCE hInstance = GetModuleHandle(nullptr);
  WNDCLASS wc = {};
  wc.lpfnWndProc = WeaselPanel::WindowProc;
  wc.hInstance = hInstance;
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.lpszClassName = L"WeaselPanel";
  ATOM atom = RegisterClass(&wc);
  if (!atom && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
    return FALSE;
  }
  ThreadDpiAwarenessScope dpi_scope;
  const DWORD ex_style = WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE |
                         (m_preview_mode ? 0 : WS_EX_TOPMOST) |
                         WS_EX_NOREDIRECTIONBITMAP;
  m_hWnd = CreateWindowEx(
      ex_style, L"WeaselPanel", L"WeaselPanel", WS_POPUP | WS_CLIPSIBLINGS,
      CW_USEDEFAULT, CW_USEDEFAULT, 10, 10, parent, nullptr, hInstance, this);
  if (m_hWnd) {
    if (!m_pD2D) {
      DEBUG << "m_style.font_face: " << m_style.font_face;
      m_pD2D = std::make_shared<D2D>(m_style);
    }
    m_pD2D->AttachWindow(m_hWnd);
    if (!m_style.font_face.empty())
      m_pD2D->InitDirectWriteResources();
    m_ostyle = m_style;
  }
  return !!m_hWnd;
}

void WeaselPanel::_UpdateOffsetY(CRect& arc, CRect& prc) {
  if (m_istorepos) {
    std::vector<CRect> rects(m_candidateCount);
    std::vector<int> btmys(m_candidateCount);
    for (int i = 0; i < m_candidateCount && i < MAX_CANDIDATES_COUNT; ++i) {
      rects[i] = m_layout->GetCandidateRect(i);
      btmys[i] = rects[i].bottom;
    }
    m_offsety_preedit = m_candidateCount && !m_layout->IsInlinePreedit() &&
                                !m_ctx.preedit.str.empty()
                            ? rects.back().bottom - prc.bottom
                            : 0;
    m_offsety_aux = m_candidateCount && !m_ctx.aux.str.empty()
                        ? rects.back().bottom - arc.bottom
                        : 0;
    int base_gap =
        !m_ctx.aux.str.empty()
            ? arc.Height() + DPI_SCALE(m_style.spacing)
            : (!m_layout->IsInlinePreedit() && !m_ctx.preedit.str.empty()
                   ? prc.Height() + DPI_SCALE(m_style.spacing)
                   : 0);
    for (int i = 0; i < m_candidateCount && i < MAX_CANDIDATES_COUNT; ++i) {
      m_offsetys[i] = i == 0 ? btmys.back() - base_gap - rects[i].bottom
                             : rects[i - 1].top + m_offsetys[i - 1] -
                                   DPI_SCALE(m_style.candidate_spacing) -
                                   rects[i].bottom;
    }
  }
}

void WeaselPanel::DoPaint() {
  if (!m_pD2D || !m_layout || !m_pD2D->dc || !m_pD2D->swapChain)
    return;
  auto hr = m_pD2D->direct3dDevice
                ? m_pD2D->direct3dDevice->GetDeviceRemovedReason()
                : DXGI_ERROR_DEVICE_REMOVED;
  if (FAILED(hr)) {
    DEBUG << HRESULTToString(hr);
    m_pD2D->InitDirect2D();
    if (!m_pD2D->dc || !m_pD2D->swapChain)
      return;
  }
  m_pD2D->dc->BeginDraw();
  m_pD2D->dc->Clear(D2D1::ColorF({0.0f, 0.0f, 0.0f, 0.0f}));
  if (!hide_candidates) {
    const bool should_draw_background =
        ((!m_ctx.empty() && !m_style.inline_preedit) ||
         (m_style.inline_preedit && (m_candidateCount || !m_ctx.aux.empty())));
    if (should_draw_background) {
      CRect& rc = m_layout->GetContentRect();
      IsToRoundStruct roundInfo;
      _HighlightRect(rc, DPI_SCALE(m_style.round_corner_ex),
                     DPI_SCALE(m_style.border), m_style.back_color,
                     m_style.shadow_color, m_style.border_color, roundInfo);
    }
    CRect& prc = m_layout->GetPreeditRect();
    CRect& arc = m_layout->GetAuxiliaryRect();
    // if vertical auto reverse triggered
    _UpdateOffsetY(arc, prc);
    if (!m_layout->IsInlinePreedit() && !m_ctx.preedit.empty()) {
      _DrawPreedit(m_ctx.preedit, true);
    }
    if (!m_ctx.aux.empty()) {
      _DrawPreedit(m_ctx.aux, false);
    }
    if (m_candidateCount) {
      _DrawCandidates();
    }
    if (m_layout->ShouldDisplayStatusIcon()) {
      ComPtr<ID2D1Bitmap1> pBitmap;
#define LOADICON(x, icon, id) LoadIconIfNeed(m_##x, m_style.##x, icon, id)
      LOADICON(current_ascii_icon, m_iconAlpha, IDI_EN);
      LOADICON(current_zhung_icon, m_iconEnabled, IDI_ZH);
      LOADICON(current_full_icon, m_iconFull, IDI_FULL_SHAPE);
      LOADICON(current_half_icon, m_iconHalf, IDI_HALF_SHAPE);
#undef LOADICON

      HICON& ico =
          m_status.disabled ? m_iconDisabled
          : m_status.ascii_mode
              ? m_iconAlpha
              : (m_status.type == SCHEMA
                     ? m_iconEnabled
                     : (m_status.full_shape ? m_iconFull : m_iconHalf));
      HRESULT hrIcon = m_pD2D->GetBmpFromIcon(ico, pBitmap);
      // Draw the bitmap
      if (SUCCEEDED(hrIcon) && pBitmap) {
        auto iconRect = m_layout->GetStatusIconRect();
        if (m_istorepos)
          iconRect.OffsetRect(0, m_offsety_preedit);
        D2D1_RECT_F iconRectf = D2D1::RectF(iconRect.left, iconRect.top,
                                            iconRect.right, iconRect.bottom);
        m_pD2D->dc->DrawBitmap(pBitmap.Get(), iconRectf);
      }
    }
  }

  auto hrEnd = m_pD2D->dc->EndDraw();
  if (FAILED(hrEnd)) {
    DEBUG << "EndDraw failed: " << HRESULTToString(hrEnd);
    DeviceResources::Get().Reset();
    m_pD2D->InitDirect2D();
    return;
  }
  // Make the swap chain available to the composition engine
  HRESULT hrPresent = m_pD2D->swapChain->Present(1, 0);  // sync
  if (hrPresent == DXGI_ERROR_DEVICE_REMOVED ||
      hrPresent == DXGI_ERROR_DEVICE_RESET) {
    DEBUG << "Device lost during Present: " << HRESULTToString(hrPresent);
    DeviceResources::Get().Reset();
    m_pD2D->InitDirect2D();
  } else if (FAILED(hrPresent)) {
    DEBUG << "Present failed: " << HRESULTToString(hrPresent);
    DeviceResources::Get().Reset();
    m_pD2D->InitDirect2D();
  }
}

bool WeaselPanel::_DrawPreedit(const Text& text, bool isPreedit) {
  bool drawn = false;
  wstring const& t = text.str;
  PtTextFormat& pTextFormat = m_pD2D->pPreeditFormat;

  if (!t.empty()) {
    const TextRange& range = m_layout->GetPreeditRange();
    if (range.start < range.end) {
      auto before_str = t.substr(0, range.start);
      auto hilited_str = t.substr(range.start, range.end - range.start);
      auto after_str = t.substr(range.end);

      // Use precomputed rectangles - make copies when we need to modify them
      CRect rc_before = isPreedit ? m_layout->GetPreeditBeforeRect()
                                  : m_layout->GetAuxBeforeRect();
      CRect rc_hi = isPreedit ? m_layout->GetPreeditHiliteRect()
                              : m_layout->GetAuxHiliteRect();
      CRect rc_after = isPreedit ? m_layout->GetPreeditAfterRect()
                                 : m_layout->GetAuxAfterRect();

      // Apply m_istorepos offset if needed
      if (m_istorepos) {
        int offsetY = isPreedit ? m_offsety_preedit : m_offsety_aux;
        rc_before.OffsetRect(0, offsetY);
        rc_hi.OffsetRect(0, offsetY);
        rc_after.OffsetRect(0, offsetY);
      }

      // Use DPI_SCALE macro for consistency with other code
      auto padx = DPI_SCALE(m_style.hilite_padding_x);
      auto pady = DPI_SCALE(m_style.hilite_padding_y);

      if (range.start > 0 && !rc_before.IsRectNull()) {
        _TextOut(rc_before, before_str, before_str.length(), m_style.text_color,
                 pTextFormat);
      }
      if (!rc_hi.IsRectNull()) {
        // zzz[yyy]
        CRect rc_hib = rc_hi;
        rc_hib.InflateRect(padx, pady);
        const IsToRoundStruct& roundInfo = m_layout->GetTextRoundInfo();
        _HighlightRect(rc_hib, DPI_SCALE(m_style.round_corner),
                       DPI_SCALE(m_style.border), m_style.hilited_back_color,
                       m_style.hilited_shadow_color, 0, roundInfo);
        _TextOut(rc_hi, hilited_str, hilited_str.length(),
                 m_style.hilited_text_color, pTextFormat);
      }
      if (range.end < static_cast<int>(t.length()) && !rc_after.IsRectNull()) {
        // zzz[yyy]xxx
        _TextOut(rc_after, after_str, after_str.length(), m_style.text_color,
                 pTextFormat);
      }
    } else {
      // No highlighted text, use the base rectangle from layout
      CRect rcText =
          isPreedit ? m_layout->GetPreeditRect() : m_layout->GetAuxiliaryRect();

      // Apply m_istorepos offset if needed
      if (m_istorepos) {
        int offsetY = isPreedit ? m_offsety_preedit : m_offsety_aux;
        rcText.OffsetRect(0, offsetY);
      }

      _TextOut(rcText, t.c_str(), t.length(), m_style.text_color, pTextFormat);
    }
    if (m_candidateCount && !m_style.inline_preedit &&
        COLORNOTTRANSPARENT(m_style.prevpage_color) &&
        COLORNOTTRANSPARENT(m_style.nextpage_color)) {
      const std::wstring pre = L"<";
      const std::wstring next = L">";
      CRect prc = m_layout->GetPrepageRect();
      if (m_istorepos)
        prc.OffsetRect(0, m_offsety_preedit);
      // clickable color / disabled color
      int color =
          m_ctx.cinfo.currentPage ? m_style.prevpage_color : m_style.text_color;
      _TextOut(prc, pre.c_str(), pre.length(), color, pTextFormat);

      CRect nrc = m_layout->GetNextpageRect();
      if (m_istorepos)
        nrc.OffsetRect(0, m_offsety_preedit);
      // clickable color / disabled color
      color = m_ctx.cinfo.is_last_page ? m_style.text_color
                                       : m_style.nextpage_color;
      _TextOut(nrc, next.c_str(), next.length(), color, pTextFormat);
    }
    drawn = true;
  }
  return drawn;
}

CRect WeaselPanel::_GetInflatedCandRect(int i) {
  CRect rc;
  if (!m_layout || i < 0 || i >= m_candidateCount ||
      i >= MAX_CANDIDATES_COUNT) {
    rc.SetRectEmpty();
    return rc;
  }
  rc = m_layout->GetCandidateRect(i);
  if (m_istorepos)
    rc.OffsetRect(0, m_offsetys[i]);
  const auto padx = DPI_SCALE(m_style.hilite_padding_x);
  const auto pady = DPI_SCALE(m_style.hilite_padding_y);
  rc.InflateRect(padx, pady);
  return rc;
}

bool WeaselPanel::_DrawCandidates() {
  bool drawn = false;
  if (m_candidateCount <= 0)
    return false;
  const int highlighted = (m_ctx.cinfo.highlighted >= 0 &&
                           m_ctx.cinfo.highlighted < m_candidateCount)
                              ? m_ctx.cinfo.highlighted
                              : -1;
  const vector<Text>& candidates(m_ctx.cinfo.candies);
  const vector<Text>& comments(m_ctx.cinfo.comments);
  const vector<Text>& labels(m_ctx.cinfo.labels);
  PtTextFormat& txtFormat = m_pD2D->pTextFormat;
  PtTextFormat& labeltxtFormat = m_pD2D->pLabelFormat;
  PtTextFormat& commenttxtFormat = m_pD2D->pCommentFormat;
  auto padx = DPI_SCALE(m_style.hilite_padding_x);
  auto pady = DPI_SCALE(m_style.hilite_padding_y);
  const auto hilitefunc = [&](int i, uint32_t back_color, uint32_t shadow_color,
                              uint32_t border_color, uint32_t border = 0) {
    auto rect = _GetInflatedCandRect(i);
    const IsToRoundStruct& roundInfo = m_layout->GetRoundInfo(i);
    _HighlightRect(rect, DPI_SCALE(m_style.round_corner), DPI_SCALE(border),
                   back_color, shadow_color, border_color, roundInfo);
  };
  for (auto i = 0; i < m_candidateCount; i++) {
    if (i == m_hoverIndex)
      continue;
    bool hilited = (i == highlighted);
    int shadow_color = hilited ? m_style.hilited_candidate_shadow_color
                               : m_style.candidate_shadow_color;
    if (COLORNOTTRANSPARENT(shadow_color))
      hilitefunc(i, 0, shadow_color, 0);
    drawn = true;
  }
  if (m_hoverIndex >= 0 && m_hoverIndex < m_candidateCount) {
    hilitefunc(m_hoverIndex,
               HALF_ALPHA_COLOR(m_style.hilited_candidate_back_color),
               HALF_ALPHA_COLOR(m_style.hilited_candidate_shadow_color),
               HALF_ALPHA_COLOR(m_style.hilited_candidate_border_color));
  }
  // draw highlighted background and text
  const auto drawText = [&](int i, const vector<Text>& texts, int color,
                            PtTextFormat& textFormat, CRect rc) {
    if (i < 0 || i >= (int)texts.size())
      return;
    const auto& text = texts[i].str;
    if (COLORTRANSPARENT(color) || rc.IsRectNull() || text.empty() ||
        !textFormat.Get())
      return;
    if (m_istorepos)
      rc.OffsetRect(0, m_offsetys[i]);
    _TextOut(rc, text, text.length(), color, textFormat);
  };
  for (auto i = 0; i < m_candidateCount; i++) {
    bool hilited = (i == highlighted);
    int label_text_color =
        hilited ? m_style.hilited_label_text_color : m_style.label_text_color;
    int candidate_text_color = hilited ? m_style.hilited_candidate_text_color
                                       : m_style.candidate_text_color;
    int comment_text_color = hilited ? m_style.hilited_comment_text_color
                                     : m_style.comment_text_color;
    int back_color = hilited ? m_style.hilited_candidate_back_color
                             : m_style.candidate_back_color;
    int border_color = hilited ? m_style.hilited_candidate_border_color
                               : m_style.candidate_border_color;
    hilitefunc(i, back_color, 0, border_color, m_style.border);
    if (i >= 0 && i < (int)labels.size()) {
      auto rc = m_layout->GetCandidateLabelRect(i);
      auto label = FormatCandidateLabel(labels[i].str,
                                        m_style.label_text_format.c_str());
      if (!COLORTRANSPARENT(label_text_color) && !rc.IsRectNull() &&
          !label.empty() && labeltxtFormat.Get()) {
        if (m_istorepos)
          rc.OffsetRect(0, m_offsetys[i]);
        _TextOut(rc, label, label.length(), label_text_color, labeltxtFormat);
      }
    }
    drawText(i, candidates, candidate_text_color, txtFormat,
             m_layout->GetCandidateTextRect(i));
    drawText(i, comments, comment_text_color, commenttxtFormat,
             m_layout->GetCandidateCommentRect(i));
    drawn = true;
  }
  // draw highlight mark
  if (COLORNOTTRANSPARENT(m_style.hilited_mark_color) && highlighted >= 0) {
    CRect rc = _GetInflatedCandRect(highlighted);
    if (!m_style.mark_text.empty()) {
      int vgap =
          m_layout->mark_height ? (rc.Height() - m_layout->mark_height) / 2 : 0;
      int hgap =
          m_layout->mark_width ? (rc.Width() - m_layout->mark_width) / 2 : 0;
      CRect hlRc;
      if (m_style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT ||
          m_style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT_FULLSCREEN)
        hlRc = CRect(rc.left + hgap, rc.top + pady,
                     rc.left + hgap + m_layout->mark_width,
                     rc.top + pady + m_layout->mark_height);
      else
        hlRc = CRect(rc.left + padx, rc.top + vgap,
                     rc.left + padx + m_layout->mark_width, rc.bottom - vgap);
      _TextOut(hlRc, m_style.mark_text.c_str(), m_style.mark_text.length(),
               m_style.hilited_mark_color, txtFormat);
    } else {
      int height = MIN(rc.Height() - pady * 2,
                       rc.Height() - DPI_SCALE(m_style.round_corner) * 2);
      int width = MIN(rc.Width() - padx * 2,
                      rc.Width() - DPI_SCALE(m_style.round_corner) * 2);
      width = MIN(width, static_cast<int>(rc.Width() * 0.618));
      height = MIN(height, static_cast<int>(rc.Height() * 0.618));
      if (m_bar_scale != 1.0f) {
        width = static_cast<int>(width * m_bar_scale);
        height = static_cast<int>(height * m_bar_scale);
      }

      CRect mkrc;
      int mark_radius;
      if (m_style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT ||
          m_style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT_FULLSCREEN) {
        int x = rc.left + (rc.Width() - width) / 2;
        mkrc = CRect(x, rc.top, x + width, rc.top + m_layout->mark_height);
        mark_radius = mkrc.Height() / 2;
      } else {
        int y = rc.top + (rc.Height() - height) / 2;
        mkrc = CRect(rc.left, y, rc.left + m_layout->mark_width, y + height);
        mark_radius = mkrc.Width() / 2;
      }
      IsToRoundStruct roundInfo;
      _HighlightRect(mkrc, mark_radius, 0, m_style.hilited_mark_color, 0, 0,
                     roundInfo);
    }
  }
  return drawn;
}

void WeaselPanel::_TextOut(CRect& rc,
                           const wstring& text,
                           size_t cch,
                           uint32_t color,
                           PtTextFormat& pTextFormat) {
  if (!pTextFormat.Get() || !m_pD2D || !m_pD2D->m_pWriteFactory)
    return;
  m_pD2D->SetBrushColor(color);

  ComPtr<IDWriteTextLayout> pTextLayout;
  HRESULT hr = m_pD2D->m_pWriteFactory->CreateTextLayout(
      text.c_str(), cch, pTextFormat.Get(), rc.Width(), rc.Height(),
      reinterpret_cast<IDWriteTextLayout**>(pTextLayout.GetAddressOf()));
  if (FAILED(hr) || !pTextLayout)
    return;
  if (m_style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT ||
      m_style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT_FULLSCREEN) {
    DWRITE_FLOW_DIRECTION flow = m_style.vertical_text_left_to_right
                                     ? DWRITE_FLOW_DIRECTION_LEFT_TO_RIGHT
                                     : DWRITE_FLOW_DIRECTION_RIGHT_TO_LEFT;
    pTextLayout->SetReadingDirection(DWRITE_READING_DIRECTION_TOP_TO_BOTTOM);
    pTextLayout->SetFlowDirection(flow);
  } else {
    pTextLayout->SetReadingDirection(DWRITE_READING_DIRECTION_LEFT_TO_RIGHT);
    pTextLayout->SetFlowDirection(DWRITE_FLOW_DIRECTION_TOP_TO_BOTTOM);
  }
  float offsetx = (float)rc.left;
  float offsety = (float)rc.top;

  DWRITE_OVERHANG_METRICS omt;
  pTextLayout->GetOverhangMetrics(&omt);
  if (m_style.layout_type != UIStyle::LAYOUT_VERTICAL_TEXT &&
      m_style.layout_type != UIStyle::LAYOUT_VERTICAL_TEXT_FULLSCREEN &&
      omt.left > 0)
    offsetx += omt.left;
  if ((m_style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT ||
       m_style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT_FULLSCREEN) &&
      omt.top > 0)
    offsety += omt.top;

  m_pD2D->DrawTextLayout(pTextLayout, offsetx, offsety, color);
  // draw rectangle for debug
  // m_pD2D->dc->DrawRectangle(
  //     D2D1::RectF((float)rc.left, (float)rc.top, (float)rc.right,
  //                 (float)rc.bottom),
  //     m_pD2D->m_pBrush.Get(), 1.0f); // 1.0f is the border width
}

void WeaselPanel::_HighlightRect(const RECT& rect,
                                 float radius,
                                 uint32_t border,
                                 uint32_t back_color,
                                 uint32_t shadow_color,
                                 uint32_t border_color,
                                 const IsToRoundStruct& roundInfo) {
  if (roundInfo.Hemispherical)
    radius = DPI_SCALE(m_style.round_corner_ex) - DPI_SCALE(border) / 2.0f;
  // draw shadow
  if (COLORNOTTRANSPARENT(shadow_color) && DPI_SCALE(m_style.shadow_radius))
    m_pD2D->FillGeometry(rect, shadow_color, radius, roundInfo, true);
  // draw back color
  if (COLORNOTTRANSPARENT(back_color))
    m_pD2D->FillGeometry(rect, back_color, radius, roundInfo);
  // draw border
  if (COLORNOTTRANSPARENT(border_color) && border) {
    float hb = -(float)border / 2;
    ComPtr<ID2D1PathGeometry> pGeometry;
    HR(m_pD2D->CreateRoundedRectanglePath(rect, radius + hb, roundInfo,
                                          pGeometry));
    m_pD2D->SetBrushColor(border_color);
    m_pD2D->dc->DrawGeometry(pGeometry.Get(), m_pD2D->m_pBrush.Get(), border);
  }
}

void WeaselPanel::_Reposition(bool adj) {
  if (!m_layout || !m_hWnd)
    return;
  RECT rcWorkArea;
  memset(&rcWorkArea, 0, sizeof(rcWorkArea));
  HMONITOR hMonitor = MonitorFromRect(&m_inputPos, MONITOR_DEFAULTTONEAREST);
  if (hMonitor) {
    MONITORINFO info;
    info.cbSize = sizeof(MONITORINFO);
    if (GetMonitorInfo(hMonitor, &info)) {
      rcWorkArea = info.rcWork;
    }
    if (hMonitor != m_hMonitor) {
      m_hMonitor = hMonitor;
      m_redraw_by_monitor_change = true;
    }
  }
  CRect rcWindow;
  GetWindowRect(m_hWnd, &rcWindow);
  int width = rcWindow.Width();
  int height = rcWindow.Height();
  rcWorkArea.right -= width;
  rcWorkArea.bottom -= height;
  int x = m_inputPos.left;
  int y = m_inputPos.bottom;
  if (DPI_SCALE(m_style.shadow_radius)) {
    x -= (DPI_SCALE(m_style.shadow_offset_x) >= 0 ||
          COLORTRANSPARENT(m_style.shadow_color))
             ? m_layout->offsetX
             : (m_layout->offsetX / 2);
    if (adj)
      y -= (DPI_SCALE(m_style.shadow_offset_y) > 0 ||
            COLORTRANSPARENT(m_style.shadow_color))
               ? m_layout->offsetY
               : (m_layout->offsetY / 2);
  }
  if (m_style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT &&
      !m_style.vertical_text_left_to_right) {
    x += m_layout->offsetX - width;
    if (DPI_SCALE(m_style.shadow_offset_x) < 0)
      x += m_layout->offsetX;
  }
  if (adj)
    m_istorepos = false;
  if (x > rcWorkArea.right)
    x = rcWorkArea.right;
  if (x < rcWorkArea.left)
    x = rcWorkArea.left;
  if (y > rcWorkArea.bottom || m_sticky) {
    if (!m_sticky)
      m_sticky = true;
    y = m_inputPos.top - height - 6;
    if (DPI_SCALE(m_style.shadow_radius) &&
        DPI_SCALE(m_style.shadow_offset_y) > 0)
      y -= DPI_SCALE(m_style.shadow_offset_y);
    m_istorepos = (m_style.vertical_auto_reverse &&
                   m_style.layout_type == UIStyle::LAYOUT_VERTICAL);
    if (DPI_SCALE(m_style.shadow_radius) > 0)
      y += (DPI_SCALE(m_style.shadow_offset_y) < 0 ||
            COLORTRANSPARENT(m_style.shadow_color))
               ? m_layout->offsetY
               : (m_layout->offsetY / 2);
  }
  if (y < rcWorkArea.top)
    y = rcWorkArea.top;
  m_inputPos.bottom = y;
  SetWindowPos(m_hWnd, HWND_TOPMOST, x, y, 0, 0,
               SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOREDRAW);
}

static HBITMAP CopyDCToBitmap(HDC hDC, LPRECT lpRect) {
  if (!hDC || !lpRect || IsRectEmpty(lpRect))
    return NULL;
  int nWidth = lpRect->right - lpRect->left;
  int nHeight = lpRect->bottom - lpRect->top;
  HDC hMemDC = CreateCompatibleDC(hDC);
  if (!hMemDC)
    return NULL;
  HBITMAP hBitmap = CreateCompatibleBitmap(hDC, nWidth, nHeight);
  if (!hBitmap) {
    DeleteDC(hMemDC);
    return NULL;
  }
  HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);
  StretchBlt(hMemDC, 0, 0, nWidth, nHeight, hDC, lpRect->left, lpRect->top,
             nWidth, nHeight, SRCCOPY);
  SelectObject(hMemDC, hOldBitmap);
  DeleteDC(hMemDC);
  return hBitmap;
}

void WeaselPanel::_CaptureRect(CRect& rect) {
  HDC ScreenDC = ::GetDC(NULL);
  RECT rc_screen = rect;
  ::MapWindowPoints(m_hWnd, HWND_DESKTOP, reinterpret_cast<LPPOINT>(&rc_screen),
                    2);
  // capture input window
  if (::OpenClipboard(m_hWnd)) {
    HBITMAP bmp = CopyDCToBitmap(ScreenDC, &rc_screen);
    EmptyClipboard();
    HANDLE result = SetClipboardData(CF_BITMAP, bmp);
    CloseClipboard();
    if (!result && bmp) {
      DeleteObject(bmp);
    }
  }
  ::ReleaseDC(NULL, ScreenDC);
}

void WeaselPanel::OnDestroy() {
  // ensure timers are cleared and then reset state
  _ClearTimers();
  if (GetCapture() == m_hWnd)
    ReleaseCapture();

  m_hoverIndex = -1;
  m_layout.reset();
  m_sticky = false;
  m_dragging = false;
}

HRESULT WeaselPanel::OnScroll(UINT uMsg, WPARAM wParam, LPARAM lParam) {
  if (m_preview_mode)
    return 0;
  int delta = GET_WHEEL_DELTA_WPARAM(wParam);
  if (m_uiCallback && delta != 0) {
    bool scroll_down = delta < 0;
    m_uiCallback(NULL, NULL, NULL, &scroll_down);
  }
  return 0;
}

LRESULT WeaselPanel::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam) {
  if (m_preview_mode) {
    if (m_dragging) {
      POINT point{};
      GetCursorPos(&point);
      const int dx = point.x - m_drag_start.x;
      const int dy = point.y - m_drag_start.y;
      SetWindowPos(m_hWnd, nullptr, m_drag_window.left + dx,
                   m_drag_window.top + dy, 0, 0,
                   SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
    }
    return 0;
  }
  if (m_style.hover_type == UIStyle::NONE)
    return 0;
  CPoint point(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
  bool hovered = false;
  bool hover_index_change = false;
  CPoint ptScreen = point;
  ClientToScreen(m_hWnd, &ptScreen);
  if (ptScreen == m_lastCursorPos)
    return 0;
  m_lastCursorPos = ptScreen;
  for (int i = 0; i < m_candidateCount; i++) {
    CRect rect = _GetInflatedCandRect(i);
    if (rect.PtInRect(point)) {
      hovered = true;
      if (i != m_ctx.cinfo.highlighted) {
        if (m_style.hover_type == UIStyle::HoverType::HILITE) {
          if (m_uiCallback) {
            size_t hover_index = i;
            m_uiCallback(nullptr, &hover_index, nullptr, nullptr);
          }
        } else if (m_hoverIndex != i) {
          m_hoverIndex = i;
          hover_index_change = true;
        }
      } else if (m_style.hover_type == UIStyle::HoverType::SEMI_HILITE &&
                 m_hoverIndex != -1) {
        m_hoverIndex = -1;
        hover_index_change = true;
      }
    }
  }
  if (!hovered && m_hoverIndex >= 0) {
    m_hoverIndex = -1;
    hover_index_change = true;
  }
  if (hover_index_change)
    RedrawWindow();
  return 0;
}

LRESULT WeaselPanel::OnMouseActive(UINT uMsg, WPARAM wParam, LPARAM lParam) {
  return MA_NOACTIVATE;
}

LRESULT WeaselPanel::OnLeftClickUp(UINT uMsg, WPARAM wParam, LPARAM lParam) {
  if (m_preview_mode) {
    if (m_dragging) {
      m_dragging = false;
      ReleaseCapture();
      // A click without a drag reattaches the preview to its parent dialog.
      POINT point{};
      GetCursorPos(&point);
      const int dx = point.x - m_drag_start.x;
      const int dy = point.y - m_drag_start.y;
      const int cx = GetSystemMetrics(SM_CXDRAG);
      const int cy = GetSystemMetrics(SM_CYDRAG);
      if (dx > -cx && dx < cx && dy > -cy && dy < cy) {
        m_preview_detached = false;
        RepositionPreview();
      }
    }
    return 0;
  }
  if (hide_candidates || m_candidateCount <= 0)
    return 0;
  const int highlighted = (m_ctx.cinfo.highlighted >= 0 &&
                           m_ctx.cinfo.highlighted < m_candidateCount)
                              ? m_ctx.cinfo.highlighted
                              : -1;
  if (highlighted < 0)
    return 0;
  CPoint point(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

  ::KillTimer(m_hWnd, AUTOREV_TIMER);
  m_bar_scale = 1.0;
  m_clickTimer = 0;

  auto rect = _GetInflatedCandRect(highlighted);
  if (rect.PtInRect(point)) {
    size_t i = (size_t)highlighted;
    if (m_uiCallback) {
      m_uiCallback(&i, nullptr, nullptr, nullptr);
      if (!m_status.composing)
        DestroyWindow();
    }
  } else {
    RedrawWindow();
  }
  return 0;
}

LRESULT WeaselPanel::OnLeftClickDown(UINT uMsg, WPARAM wParam, LPARAM lParam) {
  if (m_preview_mode) {
    GetCursorPos(&m_drag_start);
    GetWindowRect(m_hWnd, &m_drag_window);
    m_dragging = true;
    m_preview_detached = true;
    SetCapture(m_hWnd);
    return 0;
  }
  if (hide_candidates || m_candidateCount <= 0)
    return 0;
  const int highlighted = (m_ctx.cinfo.highlighted >= 0 &&
                           m_ctx.cinfo.highlighted < m_candidateCount)
                              ? m_ctx.cinfo.highlighted
                              : -1;
  CPoint point(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
  auto padx = DPI_SCALE(m_style.hilite_padding_x);
  auto pady = DPI_SCALE(m_style.hilite_padding_y);
  // capture
  if (m_style.click_to_capture) {
    CRect rcw;
    GetClientRect(m_hWnd, &rcw);
    auto recth = highlighted >= 0 ? _GetInflatedCandRect(highlighted) : CRect();
    if (recth.PtInRect(point))
      _CaptureRect(recth);
    else {
      CRect crc(rcw);
      // if shadow_color transparent, decrease the capture rectangle size
      if (COLORTRANSPARENT(m_style.shadow_color) &&
          DPI_SCALE(m_style.shadow_radius) != 0) {
        int shadow_gap =
            (m_style.shadow_offset_x == 0 && m_style.shadow_offset_y == 0)
                ? 2 * DPI_SCALE(m_style.shadow_radius)
                : DPI_SCALE(m_style.shadow_radius) +
                      DPI_SCALE(m_style.shadow_radius) / 2;
        int ofx = padx + abs(DPI_SCALE(m_style.shadow_offset_x)) + shadow_gap >
                          abs(DPI_SCALE(m_style.margin_x))
                      ? padx + abs(DPI_SCALE(m_style.shadow_offset_x)) +
                            shadow_gap - abs(DPI_SCALE(m_style.margin_x))
                      : 0;
        int ofy = pady + abs(DPI_SCALE(m_style.shadow_offset_y)) + shadow_gap >
                          abs(DPI_SCALE(m_style.margin_y))
                      ? pady + abs(DPI_SCALE(m_style.shadow_offset_y)) +
                            shadow_gap - abs(DPI_SCALE(m_style.margin_y))
                      : 0;
        crc.DeflateRect(m_layout->offsetX - ofx, m_layout->offsetY - ofy);
      }
      _CaptureRect(crc);
    }
  }
  // page buttons, and click to select
  {
    if (!m_style.inline_preedit && m_candidateCount != 0 &&
        COLORNOTTRANSPARENT(m_style.prevpage_color) &&
        COLORNOTTRANSPARENT(m_style.nextpage_color)) {
      // click prepage
      if (m_ctx.cinfo.currentPage != 0) {
        CRect prc = m_layout->GetPrepageRect();
        if (m_istorepos)
          prc.OffsetRect(0, m_offsety_preedit);
        if (prc.PtInRect(point)) {
          bool nextPage = false;
          if (m_uiCallback)
            m_uiCallback(NULL, NULL, &nextPage, NULL);
          return 0;
        }
      }
      // click nextpage
      if (!m_ctx.cinfo.is_last_page) {
        CRect prc = m_layout->GetNextpageRect();
        if (m_istorepos)
          prc.OffsetRect(0, m_offsety_preedit);
        if (prc.PtInRect(point)) {
          bool nextPage = true;
          if (m_uiCallback)
            m_uiCallback(NULL, NULL, &nextPage, NULL);
          return 0;
        }
      }
    }
    // select by click relative actions
    for (size_t i = 0; i < m_candidateCount && i < MAX_CANDIDATES_COUNT; ++i) {
      auto rect = _GetInflatedCandRect(i);
      if (rect.PtInRect(point)) {
        m_bar_scale = 0.8f;
        //  modify highlighted
        if (i != m_ctx.cinfo.highlighted) {
          if (m_uiCallback)
            m_uiCallback(NULL, &i, NULL, NULL);
        } else {
          RedrawWindow();
        }
        m_clickTimer = ::SetTimer(m_hWnd, AUTOREV_TIMER, 1000, NULL);
        return 0;
      }
    }
  }
  return 0;
}

LRESULT CALLBACK WeaselPanel::WindowProc(HWND hwnd,
                                         UINT uMsg,
                                         WPARAM wParam,
                                         LPARAM lParam) {
  if (uMsg == WM_NCCREATE) {
    auto self = static_cast<WeaselPanel*>(
        reinterpret_cast<LPCREATESTRUCT>(lParam)->lpCreateParams);
    SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
  }
  auto self =
      reinterpret_cast<WeaselPanel*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
  if (self)
    return self->MsgHandler(hwnd, uMsg, wParam, lParam);
  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT WeaselPanel::MsgHandler(HWND hwnd,
                                UINT uMsg,
                                WPARAM wParam,
                                LPARAM lParam) {
  switch (uMsg) {
    case WM_PAINT:
      DoPaint();
      break;
    case WM_DESTROY:
      OnDestroy();
      return 0;
    case WM_NCDESTROY:
      SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
      return DefWindowProc(hwnd, uMsg, wParam, lParam);
    case WM_MOUSEMOVE:
      OnMouseMove(uMsg, wParam, lParam);
      break;
    case WM_MOUSEACTIVATE:
      return OnMouseActive(uMsg, wParam, lParam);
    case WM_MOUSEWHEEL:
      return OnScroll(uMsg, wParam, lParam);
    case WM_LBUTTONUP:
      return OnLeftClickUp(uMsg, wParam, lParam);
    case WM_LBUTTONDOWN:
      return OnLeftClickDown(uMsg, wParam, lParam);
    case WM_DPICHANGED:
      if (lParam) {
        const auto* rc = reinterpret_cast<const RECT*>(lParam);
        ::SetWindowPos(hwnd, nullptr, rc->left, rc->top, rc->right - rc->left,
                       rc->bottom - rc->top,
                       SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW);
      }
      if (m_pD2D) {
        m_pD2D->InitDpiInfo();
        if (!m_style.font_face.empty())
          m_pD2D->InitDirectWriteResources();
      }
      Refresh();
      return 0;
    case WM_TIMER:
      if (wParam == AUTOREV_TIMER) {
        ::KillTimer(m_hWnd, AUTOREV_TIMER);
        m_clickTimer = 0;
        m_bar_scale = 1.0f;
        InvalidateRect(m_hWnd, nullptr, TRUE);
        return 0;
      } else if (wParam == AUTOHIDE_TIMER) {
        ::KillTimer(m_hWnd, AUTOHIDE_TIMER);
        m_autoHideTimer = 0;
        // hide the panel on auto-hide
        ShowWindow(SW_HIDE);
        return 0;
      }
  }
  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
