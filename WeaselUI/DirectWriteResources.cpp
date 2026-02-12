#include "stdafx.h"
#include <string>
#include <algorithm>
#include <map>
#include <WeaselUI.h>

using namespace weasel;

static vector<wstring> ws_split(const wstring& in, const wstring& delim) {
  // Optimization for simple character delimiters to avoid regex overhead
  if (delim.find_first_of(L"\\^$.|?*+()[]{}") == wstring::npos &&
      delim.length() > 0) {
    vector<wstring> result;
    size_t start = 0;
    size_t end = in.find(delim);
    while (end != wstring::npos) {
      result.push_back(in.substr(start, end - start));
      start = end + delim.length();
      end = in.find(delim, start);
    }
    result.push_back(in.substr(start));
    return result;
  }
  std::wregex re{delim};
  return vector<wstring>{
      std::wsregex_token_iterator(in.begin(), in.end(), re, -1),
      std::wsregex_token_iterator()};
}

DirectWriteResources::DirectWriteResources()
    : dpiScaleFontPoint(1.0f),
      dpiScaleLayout(1.0f),
      pD2d1Factory(NULL),
      pDWFactory(NULL),
      pRenderTarget(NULL),
      pBrush(NULL),
      pTextLayout(NULL),
      pPreeditTextFormat(NULL),
      pTextFormat(NULL),
      pLabelTextFormat(NULL),
      pCommentTextFormat(NULL) {
  // prepare d2d1 resources create factory
  HR(::D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED,
                         pD2d1Factory.ReleaseAndGetAddressOf()));
  // create IDWriteFactory
  HR(DWriteCreateFactory(
      DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
      reinterpret_cast<IUnknown**>(pDWFactory.ReleaseAndGetAddressOf())));
}

DirectWriteResources::~DirectWriteResources() {
  _textFormatCache.clear();
}

HRESULT DirectWriteResources::EnsureRenderTarget(int antialiasMode) {
  if (pRenderTarget) {
    if (pRenderTarget->GetTextAntialiasMode() !=
        (D2D1_TEXT_ANTIALIAS_MODE)antialiasMode)
      pRenderTarget->SetTextAntialiasMode(
          (D2D1_TEXT_ANTIALIAS_MODE)antialiasMode);
    return S_OK;
  }

  static const D2D1_PIXEL_FORMAT format = D2D1::PixelFormat(
      DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED);
  static const D2D1_RENDER_TARGET_PROPERTIES properties =
      D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT, format);
  HR(pD2d1Factory->CreateDCRenderTarget(&properties, &pRenderTarget));
  pRenderTarget->SetTextAntialiasMode((D2D1_TEXT_ANTIALIAS_MODE)antialiasMode);
  pRenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
  HR(pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f),
                                          pBrush.ReleaseAndGetAddressOf()));
  return S_OK;
}

void DirectWriteResources::ResetRenderTarget() {
  pRenderTarget.Reset();
  pBrush.Reset();
}

HRESULT DirectWriteResources::InitResources(const UIStyle& style,
                                            const int& label_font_point,
                                            const int& font_point,
                                            const int& comment_font_point) {
  // prepare d2d1 resources
  const bool vertical_text = style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT;
  const DWRITE_WORD_WRAPPING wrapping =
      ((style.max_width == 0 &&
        style.layout_type != UIStyle::LAYOUT_VERTICAL_TEXT) ||
       (style.max_height == 0 &&
        style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT))
          ? DWRITE_WORD_WRAPPING_NO_WRAP
          : DWRITE_WORD_WRAPPING_WHOLE_WORD;
  const DWRITE_WORD_WRAPPING wrapping_preedit =
      ((style.max_width == 0 &&
        style.layout_type != UIStyle::LAYOUT_VERTICAL_TEXT) ||
       (style.max_height == 0 &&
        style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT))
          ? DWRITE_WORD_WRAPPING_NO_WRAP
          : DWRITE_WORD_WRAPPING_CHARACTER;
  const DWRITE_FLOW_DIRECTION flow = style.vertical_text_left_to_right
                                         ? DWRITE_FLOW_DIRECTION_LEFT_TO_RIGHT
                                         : DWRITE_FLOW_DIRECTION_RIGHT_TO_LEFT;

  // set main font a invalid font name, to make every font range customizable
  static const wstring _mainFontFace = L"_InvalidFontName_";
  DWRITE_FONT_WEIGHT fontWeight = DWRITE_FONT_WEIGHT_NORMAL;
  DWRITE_FONT_STYLE fontStyle = DWRITE_FONT_STYLE_NORMAL;
  // convert percentage to float
  float linespacing = dpiScaleFontPoint * ((float)style.linespacing / 100.0f);
  float baseline = dpiScaleFontPoint * ((float)style.baseline / 100.0f);
  if (style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT)
    baseline = linespacing / 2;

  auto init_font = [&](const wstring& fontface, int fontpoint,
                       ComPtr<IDWriteTextFormat1>& _pTextFormat,
                       DWRITE_WORD_WRAPPING wrap) {
    const wstring key = fontface + L"|" + std::to_wstring(fontpoint) + L"|" +
                        (vertical_text ? L"1" : L"0") + L"|" +
                        std::to_wstring((int)wrap) + L"|" +
                        std::to_wstring(style.linespacing) + L"|" +
                        std::to_wstring(style.baseline) + L"|" +
                        std::to_wstring(dpiScaleFontPoint);
    if (_textFormatCache.find(key) != _textFormatCache.end()) {
      _pTextFormat = _textFormatCache[key];
      return;
    }
    vector<wstring> fontFaceStrVector;
    // text font text format set up
    fontFaceStrVector = ws_split(fontface, L",");
    // setup weight and style by the first unit of fontface setting string
    _ParseFontFace(fontface, fontWeight, fontStyle);
    static const std::wregex styleOrWeightRegex(L":[^:]*[^a-f0-9:]+[^:]*",
                                                std::wregex::icase);
    fontFaceStrVector[0] =
        std::regex_replace(fontFaceStrVector[0], styleOrWeightRegex, L"");
    // create text format with invalid font point will 'FAILED', no HR
    pDWFactory->CreateTextFormat(_mainFontFace.c_str(), NULL, fontWeight,
                                 fontStyle, DWRITE_FONT_STRETCH_NORMAL,
                                 fontpoint * dpiScaleFontPoint, L"",
                                 reinterpret_cast<IDWriteTextFormat**>(
                                     _pTextFormat.ReleaseAndGetAddressOf()));
    if (_pTextFormat != NULL) {
      if (vertical_text) {
        HR(_pTextFormat->SetFlowDirection(flow));
        HR(_pTextFormat->SetReadingDirection(
            DWRITE_READING_DIRECTION_TOP_TO_BOTTOM));
        HR(_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING));
      } else
        HR(_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING));

      HR(_pTextFormat->SetParagraphAlignment(
          DWRITE_PARAGRAPH_ALIGNMENT_CENTER));
      HR(_pTextFormat->SetWordWrapping(wrap));
      _SetFontFallback(_pTextFormat, fontFaceStrVector);
      if (style.linespacing && style.baseline)
        _pTextFormat->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM,
                                     fontpoint * linespacing,
                                     fontpoint * baseline);
      _textFormatCache[key] = _pTextFormat;
    }
  };
  init_font(style.font_face, font_point, pTextFormat, wrapping);
  init_font(style.font_face, font_point, pPreeditTextFormat, wrapping_preedit);
  init_font(style.label_font_face, label_font_point, pLabelTextFormat,
            wrapping);
  init_font(style.comment_font_face, comment_font_point, pCommentTextFormat,
            wrapping);
  return S_OK;
}

HRESULT DirectWriteResources::InitResources(const UIStyle& style,
                                            const UINT& dpi = 96) {
  if (dpi) {
    dpiScaleFontPoint = dpi / 72.0f;
    dpiScaleLayout = dpi / 96.0f;
  }
  return InitResources(style, style.label_font_point, style.font_point,
                       style.comment_font_point);
}

static wstring _MatchWordsOutLowerCaseTrim1st(const wstring& wstr,
                                              const std::wregex& pattern) {
  wstring mat = L"";
  std::wsmatch mc;
  wstring::const_iterator iter = wstr.cbegin();
  wstring::const_iterator end = wstr.cend();
  while (regex_search(iter, end, mc, pattern)) {
    for (const auto& m : mc) {
      mat = m;
      mat = mat.substr(1);
      break;
    }
    iter = mc.suffix().first;
  }
  wstring res;
  std::transform(mat.begin(), mat.end(), std::back_inserter(res), ::tolower);
  return res;
}

void DirectWriteResources::_ParseFontFace(const wstring& fontFaceStr,
                                          DWRITE_FONT_WEIGHT& fontWeight,
                                          DWRITE_FONT_STYLE& fontStyle) {
  static const std::wregex patWeight(
      L"(:thin|:extra_light|:ultra_light|:light|:semi_light|:medium|:demi_bold|"
      L":semi_bold|:bold|:extra_bold|:ultra_bold|:black|:heavy|:extra_black|:"
      L"ultra_black)",
      std::wregex::icase);
  static const std::map<wstring, DWRITE_FONT_WEIGHT> _mapWeight = {
      {L"thin", DWRITE_FONT_WEIGHT_THIN},
      {L"extra_light", DWRITE_FONT_WEIGHT_EXTRA_LIGHT},
      {L"ultra_light", DWRITE_FONT_WEIGHT_ULTRA_LIGHT},
      {L"light", DWRITE_FONT_WEIGHT_LIGHT},
      {L"semi_light", DWRITE_FONT_WEIGHT_SEMI_LIGHT},
      {L"medium", DWRITE_FONT_WEIGHT_MEDIUM},
      {L"demi_bold", DWRITE_FONT_WEIGHT_DEMI_BOLD},
      {L"semi_bold", DWRITE_FONT_WEIGHT_SEMI_BOLD},
      {L"bold", DWRITE_FONT_WEIGHT_BOLD},
      {L"extra_bold", DWRITE_FONT_WEIGHT_EXTRA_BOLD},
      {L"ultra_bold", DWRITE_FONT_WEIGHT_ULTRA_BOLD},
      {L"black", DWRITE_FONT_WEIGHT_BLACK},
      {L"heavy", DWRITE_FONT_WEIGHT_HEAVY},
      {L"extra_black", DWRITE_FONT_WEIGHT_EXTRA_BLACK},
      {L"normal", DWRITE_FONT_WEIGHT_NORMAL},
      {L"ultra_black", DWRITE_FONT_WEIGHT_ULTRA_BLACK}};
  const wstring weight = _MatchWordsOutLowerCaseTrim1st(fontFaceStr, patWeight);
  auto it = _mapWeight.find(weight);
  fontWeight =
      (it != _mapWeight.end()) ? it->second : DWRITE_FONT_WEIGHT_NORMAL;

  static const std::wregex patStyle(L"(:italic|:oblique|:normal)",
                                    std::wregex::icase);
  static const std::map<wstring, DWRITE_FONT_STYLE> _mapStyle = {
      {L"italic", DWRITE_FONT_STYLE_ITALIC},
      {L"oblique", DWRITE_FONT_STYLE_OBLIQUE},
      {L"normal", DWRITE_FONT_STYLE_NORMAL},
  };
  const wstring style = _MatchWordsOutLowerCaseTrim1st(fontFaceStr, patStyle);
  auto it2 = _mapStyle.find(style);
  fontStyle = (it2 != _mapStyle.end()) ? it2->second : DWRITE_FONT_STYLE_NORMAL;
}

static UINT GetValue(const wstring& wstr, const int fallback) {
  try {
    return std::stoul(wstr.c_str(), 0, 16);
  } catch (...) {
    return fallback;
  }
}

void DirectWriteResources::_SetFontFallback(
    ComPtr<IDWriteTextFormat1>& textFormat,
    const vector<wstring>& fontVector) {
  ComPtr<IDWriteFontFallback> pSysFallback;
  HR(pDWFactory->GetSystemFontFallback(pSysFallback.ReleaseAndGetAddressOf()));
  ComPtr<IDWriteFontFallback> pFontFallback = NULL;
  ComPtr<IDWriteFontFallbackBuilder> pFontFallbackBuilder = NULL;
  HR(pDWFactory->CreateFontFallbackBuilder(
      pFontFallbackBuilder.ReleaseAndGetAddressOf()));
  vector<wstring> fallbackFontsVector;
  for (UINT32 i = 0; i < fontVector.size(); i++) {
    fallbackFontsVector = ws_split(fontVector[i], L":");
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
    UINT first = GetValue(firstWstr, 0), last = GetValue(lastWstr, 0x10ffff);
    DWRITE_UNICODE_RANGE range = {first, last};
    const WCHAR* familys = {_fontFaceWstr.c_str()};
    HR(pFontFallbackBuilder->AddMapping(&range, 1, &familys, 1));
  }
  // add system defalt font fallback
  HR(pFontFallbackBuilder->AddMappings(pSysFallback.Get()));
  HR(pFontFallbackBuilder->CreateFontFallback(
      pFontFallback.ReleaseAndGetAddressOf()));
  HR(textFormat->SetFontFallback(pFontFallback.Get()));
}
