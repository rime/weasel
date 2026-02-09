#pragma once

#include <WeaselIPCData.h>
#include <vector>
#include <regex>
#include <iterator>
#include <d2d1.h>
#include <dwrite_2.h>
#include <memory>
#include <functional>
#include <map>
#include <WeaselUtility.h>

namespace weasel {

enum ClientCapabilities {
  INLINE_PREEDIT_CAPABLE = 1,
};

class UIImpl;
class DirectWriteResources;
template <class T>
using an = std::shared_ptr<T>;

using UICallback =
    std::function<void(size_t* const, size_t* const, bool* const, bool* const)>;

//
// 输入法界面接口类
//
class UI {
 public:
  UI() : pimpl_(0), in_server_(false) {}

  virtual ~UI();

  // 创建输入法界面
  bool Create(HWND parent);

  // 销毁界面
  void Destroy(bool full = false);

  // 界面显隐
  void Show();
  void Hide();
  void ShowWithTimeout(size_t millisec);
  bool IsCountingDown() const;
  bool IsShown() const;

  // 重绘界面
  void Refresh();

  // 置输入焦点位置（光标跟随时移动候选窗）但不重绘
  void UpdateInputPosition(RECT const& rc);

  // 更新界面显示内容
  void Update(Context const& ctx, Status const& status);

  Context& ctx() { return ctx_; }
  Context& octx() { return octx_; }
  Status& status() { return status_; }
  UIStyle& style() { return style_; }
  UIStyle& ostyle() { return ostyle_; }
  DirectWriteResources* pdwr();
  bool GetIsReposition();
  bool& InServer() { return in_server_; }

  UICallback& uiCallback() { return ui_callback_; }
  void SetUICallBack(UICallback const& func) { ui_callback_ = func; }

 private:
  UIImpl* pimpl_;
  std::unique_ptr<DirectWriteResources> pDWR;

  Context ctx_;
  Context octx_;
  Status status_;
  UIStyle style_;
  UIStyle ostyle_;
  bool in_server_;
  UICallback ui_callback_;
};

class DirectWriteResources {
 public:
  DirectWriteResources();
  ~DirectWriteResources();

  HRESULT InitResources(const UIStyle& style,
                        const int& label_font_point,
                        const int& font_point,
                        const int& comment_font_point);
  HRESULT InitResources(const UIStyle& style, const UINT& dpi);

  HRESULT EnsureRenderTarget(int antialiasMode);
  void ResetRenderTarget();

  HRESULT CreateTextLayout(const std::wstring& text,
                           const int& nCount,
                           IDWriteTextFormat1* const txtFormat,
                           const float& width,
                           const float& height) {
    return pDWFactory->CreateTextLayout(
        text.c_str(), nCount, txtFormat, width, height,
        reinterpret_cast<IDWriteTextLayout**>(
            pTextLayout.ReleaseAndGetAddressOf()));
  }
  void DrawRect(D2D1_RECT_F* const rect,
                const float& strokeWidth = 1.0f,
                ID2D1StrokeStyle* const sstyle = (ID2D1StrokeStyle*)0) {
    pRenderTarget->DrawRectangle(rect, pBrush.Get(), strokeWidth, sstyle);
  }
  HRESULT GetLayoutOverhangMetrics(DWRITE_OVERHANG_METRICS* overhangMetrics) {
    return pTextLayout->GetOverhangMetrics(overhangMetrics);
  }
  HRESULT GetLayoutMetrics(DWRITE_TEXT_METRICS* metrics) {
    return pTextLayout->GetMetrics(metrics);
  }
  HRESULT SetLayoutReadingDirection(const DWRITE_READING_DIRECTION& direct) {
    return pTextLayout->SetReadingDirection(direct);
  }
  HRESULT SetLayoutFlowDirection(const DWRITE_FLOW_DIRECTION& direct) {
    return pTextLayout->SetFlowDirection(direct);
  }
  void DrawTextLayoutAt(const D2D1_POINT_2F& point) {
    pRenderTarget->DrawTextLayout(point, pTextLayout.Get(), pBrush.Get(),
                                  D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
  }
  void ResetLayout() { pTextLayout.Reset(); }
  HRESULT SetBrushColor(const D2D1_COLOR_F& color) {
    if (pBrush) {
      pBrush->SetColor(color);
      return S_OK;
    } else {
      return pRenderTarget->CreateSolidColorBrush(
          color, pBrush.ReleaseAndGetAddressOf());
    }
  }

  float dpiScaleFontPoint, dpiScaleLayout;
  ComPtr<ID2D1Factory> pD2d1Factory;
  ComPtr<IDWriteFactory2> pDWFactory;
  ComPtr<ID2D1DCRenderTarget> pRenderTarget;
  ComPtr<IDWriteTextFormat1> pPreeditTextFormat;
  ComPtr<IDWriteTextFormat1> pTextFormat;
  ComPtr<IDWriteTextFormat1> pLabelTextFormat;
  ComPtr<IDWriteTextFormat1> pCommentTextFormat;
  ComPtr<IDWriteTextLayout2> pTextLayout;
  ComPtr<ID2D1SolidColorBrush> pBrush;

 private:
  void _ParseFontFace(const std::wstring& fontFaceStr,
                      DWRITE_FONT_WEIGHT& fontWeight,
                      DWRITE_FONT_STYLE& fontStyle);
  void _SetFontFallback(ComPtr<IDWriteTextFormat1>& pTextFormat,
                        const std::vector<std::wstring>& fontVector);
  std::map<std::wstring, ComPtr<IDWriteTextFormat1>> _textFormatCache;
};
}  // namespace weasel
