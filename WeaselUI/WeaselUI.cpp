#include "stdafx.h"
#include <WeaselUI.h>
#include <ShellScalingApi.h>
#include "WeaselPanel.h"

#pragma comment(lib, "Shcore.lib")

using namespace weasel;

static UINT GetDefaultDpiForPrewarm() {
  RECT rc = {};
  HMONITOR hMonitor = MonitorFromRect(&rc, MONITOR_DEFAULTTONEAREST);
  UINT dpiX = 96, dpiY = 96;
  if (hMonitor) {
    GetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);
  }
  return dpiX;
}

static bool WarmUpDwrEndDraw(PDWR& pDWR) {
  if (!pDWR || !pDWR->pRenderTarget) {
    return false;
  }

  constexpr int kWidth = 96;
  constexpr int kHeight = 32;
  HDC screen_dc = ::GetDC(NULL);
  HDC mem_dc = screen_dc ? ::CreateCompatibleDC(screen_dc) : NULL;
  HBITMAP bitmap =
      screen_dc ? ::CreateCompatibleBitmap(screen_dc, kWidth, kHeight) : NULL;
  HGDIOBJ old_bitmap = NULL;
  bool ok = false;

  if (mem_dc && bitmap) {
    old_bitmap = ::SelectObject(mem_dc, bitmap);
    RECT rc = {0, 0, kWidth, kHeight};
    HRESULT hr = pDWR->pRenderTarget->BindDC(mem_dc, &rc);
    if (SUCCEEDED(hr)) {
      pDWR->pRenderTarget->BeginDraw();
      if (!pDWR->pBrush) {
        pDWR->CreateBrush(D2D1::ColorF(0, 0, 0, 1));
      } else {
        pDWR->SetBrushColor(D2D1::ColorF(0, 0, 0, 1));
      }

      IDWriteTextFormat1* format = pDWR->pTextFormat.Get();
      if (!format) {
        format = pDWR->pPreeditTextFormat.Get();
      }
      if (format) {
        hr = pDWR->CreateTextLayout(L"warm", 4, format, kWidth, kHeight);
        if (SUCCEEDED(hr) && pDWR->pTextLayout) {
          pDWR->DrawTextLayoutAt({0, 0});
        }
      }
      ok = SUCCEEDED(pDWR->pRenderTarget->EndDraw());
      pDWR->ResetLayout();
    }
  }

  if (old_bitmap) {
    ::SelectObject(mem_dc, old_bitmap);
  }
  if (bitmap) {
    ::DeleteObject(bitmap);
  }
  if (mem_dc) {
    ::DeleteDC(mem_dc);
  }
  if (screen_dc) {
    ::ReleaseDC(NULL, screen_dc);
  }

  return ok;
}

class weasel::UIImpl {
 public:
  WeaselPanel panel;

  UIImpl(weasel::UI& ui) : panel(ui), shown(false) {}
  ~UIImpl() {}
  void Refresh() {
    if (!panel.IsWindow())
      return;
    if (timer) {
      Hide();
      KillTimer(panel.m_hWnd, AUTOHIDE_TIMER);
      timer = 0;
    }
    panel.Refresh();
  }
  void Show();
  void Hide();
  void ShowWithTimeout(size_t millisec);
  bool IsShown() const { return shown; }

  static VOID CALLBACK OnTimer(_In_ HWND hwnd,
                               _In_ UINT uMsg,
                               _In_ UINT_PTR idEvent,
                               _In_ DWORD dwTime);
  static const int AUTOHIDE_TIMER = 20121220;
  static UINT_PTR timer;
  bool shown;
};

UINT_PTR UIImpl::timer = 0;

void UIImpl::Show() {
  if (!panel.IsWindow())
    return;
  panel.ShowWindow(SW_SHOWNA);
  shown = true;
  if (timer) {
    KillTimer(panel.m_hWnd, AUTOHIDE_TIMER);
    timer = 0;
  }
}

void UIImpl::Hide() {
  if (!panel.IsWindow())
    return;
  panel.ShowWindow(SW_HIDE);
  shown = false;
  if (timer) {
    KillTimer(panel.m_hWnd, AUTOHIDE_TIMER);
    timer = 0;
  }
}

void UIImpl::ShowWithTimeout(size_t millisec) {
  if (!panel.IsWindow())
    return;
  DLOG(INFO) << "ShowWithTimeout: " << millisec;
  panel.ShowWindow(SW_SHOWNA);
  shown = true;
  SetTimer(panel.m_hWnd, AUTOHIDE_TIMER, static_cast<UINT>(millisec),
           &UIImpl::OnTimer);
  timer = UINT_PTR(this);
}
VOID CALLBACK UIImpl::OnTimer(_In_ HWND hwnd,
                              _In_ UINT uMsg,
                              _In_ UINT_PTR idEvent,
                              _In_ DWORD dwTime) {
  DLOG(INFO) << "OnTimer:";
  KillTimer(hwnd, idEvent);
  UIImpl* self = (UIImpl*)timer;
  timer = 0;
  if (self) {
    self->Hide();
    self->shown = false;
  }
}

bool UI::Create(HWND parent) {
  if (pimpl_) {
    pimpl_->panel.Create(
        parent, 0, 0, WS_POPUP,
        WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE | WS_EX_TRANSPARENT,
        0U, 0);
    return true;
  }

  pimpl_ = new UIImpl(*this);
  if (!pimpl_)
    return false;

  pimpl_->panel.Create(
      parent, 0, 0, WS_POPUP,
      WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE | WS_EX_TRANSPARENT,
      0U, 0);
  return true;
}

bool UI::Prewarm() {
  UINT dpi = GetDefaultDpiForPrewarm();
  UINT cached_dpi =
      pDWR ? static_cast<UINT>(pDWR->dpiScaleLayout * 96.0f + 0.5f) : 0;
  bool need_rebuild = !pDWR || (ostyle_ != style_) || (cached_dpi != dpi);

  if (!need_rebuild) {
    if (!dwr_warm_drawn_) {
      dwr_warm_drawn_ = WarmUpDwrEndDraw(pDWR);
    }
    return true;
  }

  pDWR.reset();
  dwr_warm_drawn_ = false;
  pDWR = std::make_shared<DirectWriteResources>(style_, dpi);
  if (pDWR && pDWR->pRenderTarget) {
    pDWR->pRenderTarget->SetTextAntialiasMode(
        (D2D1_TEXT_ANTIALIAS_MODE)style_.antialias_mode);
  }
  ostyle_ = style_;
  dwr_warm_drawn_ = WarmUpDwrEndDraw(pDWR);

  return pDWR != nullptr;
}

void UI::Destroy(bool full) {
  if (pimpl_) {
    // destroy panel
    if (pimpl_->panel.IsWindow()) {
      pimpl_->panel.DestroyWindow();
    }
    if (full) {
      delete pimpl_;
      pimpl_ = 0;
      pDWR.reset();
      dwr_warm_drawn_ = false;
    }
  }
}

bool UI::GetIsReposition() {
  if (pimpl_)
    return pimpl_->panel.GetIsReposition();
  else
    return false;
}

void UI::Show() {
  if (pimpl_) {
    pimpl_->Show();
  }
}

void UI::Hide() {
  if (pimpl_) {
    pimpl_->Hide();
  }
}

void UI::ShowWithTimeout(size_t millisec) {
  if (pimpl_) {
    pimpl_->ShowWithTimeout(millisec);
  }
}

bool UI::IsCountingDown() const {
  return pimpl_ && pimpl_->timer != 0;
}

bool UI::IsShown() const {
  return pimpl_ && pimpl_->IsShown();
}

void UI::Refresh() {
  if (pimpl_) {
    pimpl_->Refresh();
  }
}

void UI::UpdateInputPosition(RECT const& rc) {
  if (pimpl_ && pimpl_->panel.IsWindow()) {
    pimpl_->panel.MoveTo(rc);
  }
}

void UI::Update(const Context& ctx, const Status& status) {
  if (ctx_ == ctx && status_ == status)
    return;
  ctx_ = ctx;
  status_ = status;
  if (style_.candidate_abbreviate_length > 0) {
    for (auto& c : ctx_.cinfo.candies) {
      if (c.str.length() > (size_t)style_.candidate_abbreviate_length) {
        c.str =
            c.str.substr(0, (size_t)style_.candidate_abbreviate_length - 1) +
            L"..." + c.str.substr(c.str.length() - 1);
      }
    }
  }
  Refresh();
}
