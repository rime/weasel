#pragma once
#include "resource.h"
#include <WeaselUtility.h>
#include <ShellScalingApi.h>
#include <algorithm>
#include <d2d1_2.h>
#include <dwrite_2.h>
#include <map>
#include <regex>
#include <string>
#include <unordered_map>
#include <windows.h>
#include <wrl.h>
#include "UIStyleSettings.h"

using namespace Microsoft::WRL;
using namespace std;

#define STYLEORWEIGHT (L":[^:]*[^a-f0-9:]+[^:]*")

typedef ComPtr<IDWriteTextFormat1> PtTextFormat;
// ----------------------------------------------------------------------------
extern const wstring patStyle;
extern const wstring patWeight;
extern const map<wstring, DWRITE_FONT_STYLE> _mapStyle;
extern const map<wstring, DWRITE_FONT_WEIGHT> _mapWeight;

// ----------------------------------------------------------------------------
void RemoveSpaceAround(wstring& str);
void ParseFontFace(const wstring& fontFaceStr,
                   DWRITE_FONT_WEIGHT& fontWeight,
                   DWRITE_FONT_STYLE& fontStyle);
wstring MatchWordsOutLowerCaseTrim1st(const wstring& wstr, const wstring& pat);
std::vector<wstring> WstrSplit(const wstring& in, const wstring& delim);
// ----------------------------------------------------------------------------

struct D2D {
  D2D(HWND& hwnd,
      const wstring& text,
      COLORREF color = GetSysColor(COLOR_BTNFACE))
      : m_hWnd(hwnd), m_text(text) {
    m_color = D2D1::ColorF(GetRValue(color) / 255.0f, GetGValue(color) / 255.0f,
                           GetBValue(color) / 255.0f);
    GetClientRect(m_hWnd, &m_rect);
    InitializeDirect2D();
  }

  HRESULT InitFontFormat(const wstring& font_face, const int font_point) {
    DWRITE_WORD_WRAPPING wrapping = DWRITE_WORD_WRAPPING_WHOLE_WORD;
    DWRITE_FLOW_DIRECTION flow = DWRITE_FLOW_DIRECTION_LEFT_TO_RIGHT;
    GetDpi();

    auto init_font = [&](const wstring& font_face, int font_point,
                         PtTextFormat& _pTextFormat,
                         DWRITE_WORD_WRAPPING wrap) {
      std::vector<wstring> fontFaceStrVector;
      // set main font a invalid font name, to make every font range
      // customizable
      const wstring _mainFontFace = L"_InvalidFontName_";
      DWRITE_FONT_WEIGHT fontWeight = DWRITE_FONT_WEIGHT_NORMAL;
      DWRITE_FONT_STYLE fontStyle = DWRITE_FONT_STYLE_NORMAL;

      ParseFontFace(font_face, fontWeight, fontStyle);
      // text font text format set up
      fontFaceStrVector = WstrSplit(font_face, L",");
      fontFaceStrVector[0] = regex_replace(
          fontFaceStrVector[0], wregex(STYLEORWEIGHT, wregex::icase), L"");
      if (font_point <= 0) {
        _pTextFormat.Reset();
        return S_FALSE;
      }
      HR(m_pDWriteFactory->CreateTextFormat(
          _mainFontFace.c_str(), NULL, fontWeight, fontStyle,
          DWRITE_FONT_STRETCH_NORMAL, font_point * m_dpiScaleFontPoint, L"",
          reinterpret_cast<IDWriteTextFormat**>(
              _pTextFormat.ReleaseAndGetAddressOf())));

      _pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
      _pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

      _pTextFormat->SetWordWrapping(wrap);
      HR(SetFontFallback(_pTextFormat, fontFaceStrVector));
      decltype(fontFaceStrVector)().swap(fontFaceStrVector);
      return S_OK;
    };
    init_font(font_face, font_point, m_pTextFormat, wrapping);
    return S_OK;
  }

  HRESULT SetFontFallback(PtTextFormat textFormat,
                          const std::vector<wstring>& fontVector) {
    ComPtr<IDWriteFontFallback> pSysFallback;
    HR(m_pDWriteFactory->GetSystemFontFallback(
        pSysFallback.ReleaseAndGetAddressOf()));
    ComPtr<IDWriteFontFallback> pFontFallback = NULL;
    ComPtr<IDWriteFontFallbackBuilder> pFontFallbackBuilder = NULL;
    HR(m_pDWriteFactory->CreateFontFallbackBuilder(
        pFontFallbackBuilder.ReleaseAndGetAddressOf()));
    std::vector<wstring> fallbackFontsVector;
    for (UINT32 i = 0; i < fontVector.size(); i++) {
      fallbackFontsVector = WstrSplit(fontVector[i], L":");
      wstring _fontFaceWstr, firstWstr, lastWstr;
      if (fallbackFontsVector.size() == 3) {
        _fontFaceWstr = fallbackFontsVector[0];
        firstWstr = fallbackFontsVector[1];
        lastWstr = fallbackFontsVector[2];
        if (lastWstr.empty())
          lastWstr = L"10ffff";
        if (firstWstr.empty())
          firstWstr = L"0";
      } else if (fallbackFontsVector.size() == 2)  // fontName : codepoint
      {
        _fontFaceWstr = fallbackFontsVector[0];
        firstWstr = fallbackFontsVector[1];
        if (firstWstr.empty())
          firstWstr = L"0";
        lastWstr = L"10ffff";
      } else if (fallbackFontsVector.size() ==
                 1)  // if only font defined, use all range
      {
        _fontFaceWstr = fallbackFontsVector[0];
        firstWstr = L"0";
        lastWstr = L"10ffff";
      }
      const auto func = [](wstring wstr, UINT fallback) -> UINT {
        try {
          return stoul(wstr.c_str(), 0, 16);
        } catch (...) {
          return fallback;
        }
      };
      UINT first = func(firstWstr, 0), last = func(lastWstr, 0x10ffff);
      DWRITE_UNICODE_RANGE range = {first, last};
      const WCHAR* familys = {_fontFaceWstr.c_str()};
      HR(pFontFallbackBuilder->AddMapping(&range, 1, &familys, 1));
      decltype(fallbackFontsVector)().swap(fallbackFontsVector);
    }
    // add system defalt font fallback
    HR(pFontFallbackBuilder->AddMappings(pSysFallback.Get()));
    HR(pFontFallbackBuilder->CreateFontFallback(
        pFontFallback.ReleaseAndGetAddressOf()));
    HR(textFormat->SetFontFallback(pFontFallback.Get()));
    decltype(fallbackFontsVector)().swap(fallbackFontsVector);
    return S_OK;
  }

  bool GetDpi() {
    HMONITOR const mon = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
    UINT x = 0, y = 0;
    HR(GetDpiForMonitor(mon, MDT_EFFECTIVE_DPI, &x, &y));
    auto m_dpiX = static_cast<float>(x);
    auto m_dpiY = static_cast<float>(y);
    if (m_dpiY == 0)
      m_dpiX = m_dpiY = 96.0;
    auto dpiScaleFontPoint = m_dpiY / 72.0f;
    bool ret = (dpiScaleFontPoint == m_dpiScaleFontPoint);
    m_dpiScaleFontPoint = dpiScaleFontPoint;
    m_dpiScaleLayout = m_dpiX / 96.0f;
    return ret;
  }

  void InitializeDirect2D() {
    // 创建 Direct2D 工厂
    HR(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
                         __uuidof(ID2D1Factory), nullptr, &m_pD2DFactory));
    GetDpi();
    // 创建渲染目标
    auto scale = (initialized && m_dpiScaleLayout == 1) ? 1 : m_dpiScaleLayout;
    HR(m_pD2DFactory->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(),
        D2D1::HwndRenderTargetProperties(
            m_hWnd, D2D1::SizeU(scale * (m_rect.right - m_rect.left),
                                scale * (m_rect.bottom - m_rect.top))),
        &m_pRenderTarget));

    // 创建画刷
    HR(m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black),
                                              &m_pBrush));

    // 初始化 DirectWrite 工厂
    HR(DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(m_pDWriteFactory.GetAddressOf())));
    initialized = true;
  }

  void Draw() {
    if (!m_pRenderTarget)
      InitializeDirect2D();
    m_pRenderTarget->BeginDraw();
    m_pRenderTarget->Clear(m_color);
    if (m_pTextFormat && !m_text.empty()) {
      m_pRenderTarget->DrawText(
          m_text.c_str(), static_cast<UINT32>(m_text.length()),
          m_pTextFormat.Get(),
          D2D1::RectF(m_rect.left + 10, m_rect.top, m_rect.right - 10,
                      m_rect.bottom),
          m_pBrush.Get(), D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
    }
    m_pRenderTarget->EndDraw();
  }

  ComPtr<ID2D1Factory> m_pD2DFactory;
  ComPtr<ID2D1HwndRenderTarget> m_pRenderTarget;
  ComPtr<ID2D1SolidColorBrush> m_pBrush;
  ComPtr<IDWriteFactory2> m_pDWriteFactory;
  ComPtr<IDWriteTextFormat1> m_pTextFormat;

  D2D1::ColorF m_color = D2D1::ColorF::White;
  const wstring& m_text;
  float m_dpiScaleFontPoint = 1.0f;
  float m_dpiScaleLayout = 1.0f;
  bool initialized = false;
  HWND m_hWnd;
  RECT m_rect;
};

class FontSettingDialog {
 public:
  FontSettingDialog(UIStyleSettings* settings, HWND parent = NULL)
      : hInstance_(nullptr),
        hDlg_(nullptr),
        hParent(parent),
        m_font_face(settings->font_face),
        m_font_point(settings->font_point),
        m_label_font_face(settings->label_font_face),
        m_label_font_point(settings->label_font_point),
        m_comment_font_face(settings->comment_font_face),
        m_comment_font_point(settings->comment_font_point) {}
  INT_PTR ShowDialog();

  wstring m_font_face = L"微软雅黑";
  wstring m_label_font_face = L"微软雅黑";
  wstring m_comment_font_face = L"微软雅黑";
  int m_font_point;
  int m_label_font_point;
  int m_comment_font_point;

 private:
  static INT_PTR CALLBACK DialogProc(HWND hDlg,
                                     UINT message,
                                     WPARAM wParam,
                                     LPARAM lParam);
  INT_PTR HandleMsg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
  INT_PTR OnInitDialog();
  LRESULT OnDpiChanged(UINT, WPARAM wParam, LPARAM lParam, BOOL&);
  UINT GetWindowDpi();
  wstring GetComboBoxSelectStr(HWND target);
  wstring GetTextOfEdit(HWND& hwndEdit);
  void InitCtrlRects();
  void ScaleControlsAndFonts(UINT newDpi);
  void UpdatePreview();
  void OnAddFont();
  void OnChangeFontFace();
  void OnRangeToggle();
  INT_PTR OnChangeFontPoint();
  void OnChangeFontFaceEdit();
  void HandleRangeInput(int controlId, const wchar_t* defaultValue);
  void OnStyleOrWeightChanged(WPARAM wParam);
  INT_PTR OnCommand(WPARAM wParam);
  void InitFontPickerItems();
  void ResetRange();

  HWND m_hFontPickerCbb = nullptr;
  HWND m_hFontStyleCbb = nullptr;
  HWND m_hFontWeightCbb = nullptr;
  HWND m_hFontFaceNameCbb = nullptr;
  HWND m_hComboBoxFontPoint = nullptr;
  HWND m_hCheckBoxRangeEn = nullptr;
  HWND m_hEditRangeStart = nullptr;
  HWND m_hEditRangeEnd = nullptr;
  HWND m_hEditFontFace = nullptr;
  HWND m_hEditPreviewText = nullptr;
  HWND m_hPreview = nullptr;
  wstring* m_font_face_ptr = nullptr;
  int* m_font_point_ptr = nullptr;

  wstring m_text = L"Weasel Powered by Rime 聰明的輸入法懂我心意🎉🎉🎉";
  std::unique_ptr<D2D> m_pD2D;

  HINSTANCE hInstance_;
  HWND hDlg_;
  HWND hParent;

  UINT m_currentDpi = 96;
  RECT m_rect;
  std::unordered_map<HWND, RECT> m_controlOriginalRects;
  HFONT m_currentFont = nullptr;
};
