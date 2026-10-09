#include "d2d.h"
#include <Dwmapi.h>
#include <ShellScalingApi.h>
#include <VersionHelpers.hpp>
#include <wincodec.h>
#include <algorithm>
#include <regex>

#pragma comment(lib, "dxguid.lib")

namespace weasel {
namespace {
UINT GetDpiForWindowCompat(HWND hwnd) {
  if (!hwnd)
    return 0;
  using GetDpiForWindowFunc = UINT(WINAPI*)(HWND);
  static auto const pGetDpiForWindow = reinterpret_cast<GetDpiForWindowFunc>(
      GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForWindow"));
  return pGetDpiForWindow ? pGetDpiForWindow(hwnd) : 0;
}
}  // namespace

// DeviceResources implementation moved from cpp to header; provide definitions
// here
DeviceResources& DeviceResources::Get() {
  static DeviceResources instance;
  return instance;
}

DeviceResources::DeviceResources() : initialized(false) {}

HRESULT DeviceResources::EnsureInitialized() {
  if (initialized)
    return S_OK;
  HRESULT hr = S_OK;
  // Prefer WARP to avoid loading the host process's hardware graphics driver.
  static const D3D_FEATURE_LEVEL featureLevels[] = {
      D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1,
      D3D_FEATURE_LEVEL_10_0, D3D_FEATURE_LEVEL_9_3,  D3D_FEATURE_LEVEL_9_2,
      D3D_FEATURE_LEVEL_9_1};
  hr = D3D11CreateDevice(
      nullptr, D3D_DRIVER_TYPE_WARP, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
      featureLevels, _countof(featureLevels), D3D11_SDK_VERSION,
      direct3dDevice.ReleaseAndGetAddressOf(), nullptr, nullptr);
  if (FAILED(hr)) {
    DEBUG << "D3D11CreateDevice WARP failed: " << HRESULTToString(hr)
          << ", retrying hardware";
    hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
                           D3D11_CREATE_DEVICE_BGRA_SUPPORT, featureLevels,
                           _countof(featureLevels), D3D11_SDK_VERSION,
                           direct3dDevice.ReleaseAndGetAddressOf(), nullptr,
                           nullptr);
    if (FAILED(hr)) {
      DEBUG << "D3D11CreateDevice hardware fallback failed: "
            << HRESULTToString(hr);
      return hr;
    }
  }
  hr = direct3dDevice.As(&dxgiDevice);
  if (FAILED(hr))
    return hr;
  hr = CreateDXGIFactory2(
      0, __uuidof(dxFactory.Get()),
      reinterpret_cast<void**>(dxFactory.ReleaseAndGetAddressOf()));
  if (FAILED(hr))
    return hr;
  D2D1_FACTORY_OPTIONS const options = {D2D1_DEBUG_LEVEL_INFORMATION};
  hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, options,
                         d2Factory.ReleaseAndGetAddressOf());
  if (FAILED(hr))
    return hr;
  hr = d2Factory->CreateDevice(dxgiDevice.Get(),
                               d2Device.ReleaseAndGetAddressOf());
  if (FAILED(hr))
    return hr;
  hr = DCompositionCreateDevice(
      dxgiDevice.Get(), __uuidof(dcompDevice.Get()),
      reinterpret_cast<void**>(dcompDevice.ReleaseAndGetAddressOf()));
  if (FAILED(hr))
    return hr;
  hr = DWriteCreateFactory(
      DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory2),
      reinterpret_cast<IUnknown**>(m_pWriteFactory.ReleaseAndGetAddressOf()));
  if (FAILED(hr))
    return hr;
  hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                        IID_PPV_ARGS(wicFactory.ReleaseAndGetAddressOf()));
  if (FAILED(hr))
    return hr;
  initialized = true;
  return S_OK;
}

void DeviceResources::Reset() {
  // release shared device resources so they can be recreated
  SafeReleaseAll(direct3dDevice, dxgiDevice, dxFactory, d2Factory, d2Device,
                 dcompDevice, m_pWriteFactory, wicFactory);
  initialized = false;
}

// implementation of D2D::ClearDeviceDependentCaches declared in header
void D2D::ClearDeviceDependentCaches() {
  std::lock_guard<std::mutex> lk(cacheMutex);
  // text formats created from IDWriteFactory are generally immutable and can
  // survive device reset; however if DWriteFactory is reset, clear cache
  if (!m_pWriteFactory) {
    for (auto& kv : textFormatCache) {
      kv.second.format.Reset();
    }
    textFormatCache.clear();
    textFormatCacheClock = 0;
  }
}

void D2D::ReleaseWindowResources() {
  // detach DC target first
  if (dc)
    dc->SetTarget(nullptr);
  bitmap.Reset();
  surface.Reset();
  visual.Reset();
  target.Reset();
  swapChain.Reset();
  m_hWnd = nullptr;
}

D2D::D2D(UIStyle& style)
    : m_style(style), m_hWnd(nullptr), m_dpiX(96.0f), m_dpiY(96.0f) {
  // Prepare shared device resources early so formats can be built before window
  DeviceResources::Get().EnsureInitialized();
  // initialize dpi scales even without window (defaults to 96 DPI)
  InitDpiInfo();
}

void D2D::AttachWindow(HWND hwnd) {
  if (m_hWnd == hwnd && swapChain) {
    // Keep DPI scale in sync when window stays the same but monitor DPI
    // changes.
    InitDpiInfo();
    return;
  }
  m_hWnd = hwnd;
  InitDpiInfo();
  InitDirect2D();
}

D2D::~D2D() {
  SafeReleaseAll(direct3dDevice, dxgiDevice, dxFactory, swapChain, d2Factory,
                 d2Device, dc, surface, bitmap, dcompDevice, target, visual,
                 pPreeditFormat, pLabelFormat, pTextFormat, pCommentFormat,
                 m_pWriteFactory, m_pBrush);
  textFormatCache.clear();
}

void D2D::InitDirect2D() {
  // clear device-dependent caches before reinitializing
  ClearDeviceDependentCaches();

  // Use shared device resources to avoid recreating expensive objects per
  // window
  HRESULT hr = DeviceResources::Get().EnsureInitialized();
  if (FAILED(hr)) {
    DEBUG << "EnsureInitialized failed: " << HRESULTToString(hr);
    return;
  }

  // Adopt shared pointers
  direct3dDevice = DeviceResources::Get().direct3dDevice;
  dxgiDevice = DeviceResources::Get().dxgiDevice;
  dxFactory = DeviceResources::Get().dxFactory;
  d2Factory = DeviceResources::Get().d2Factory;
  d2Device = DeviceResources::Get().d2Device;
  dcompDevice = DeviceResources::Get().dcompDevice;
  m_pWriteFactory = DeviceResources::Get().m_pWriteFactory;

  // Create the Direct2D device context for this window (renders to
  // window-specific target)
  HR(d2Device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
                                   dc.ReleaseAndGetAddressOf()));
  dc->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
  dc->SetTextAntialiasMode((D2D1_TEXT_ANTIALIAS_MODE)m_style.antialias_mode);

  DXGI_SWAP_CHAIN_DESC1 description = {};
  description.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
  description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  description.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
  description.BufferCount = 2;
  description.SampleDesc.Count = 1;
  description.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;

  RECT rect = {};
  GetClientRect(m_hWnd, &rect);
  if (rect.right - rect.left <= 0 || rect.bottom - rect.top <= 0) {
    return;
  }
  description.Width = rect.right - rect.left;
  description.Height = rect.bottom - rect.top;

  hr = dxFactory->CreateSwapChainForComposition(
      dxgiDevice.Get(), &description, nullptr,
      swapChain.ReleaseAndGetAddressOf());
  if (FAILED(hr)) {
    DEBUG << "CreateSwapChainForComposition failed: " << HRESULTToString(hr);
    return;
  }

  // Retrieve the swap chain's back buffer
  hr = swapChain->GetBuffer(
      0, __uuidof(surface.Get()),
      reinterpret_cast<void**>(surface.ReleaseAndGetAddressOf()));
  if (FAILED(hr)) {
    DEBUG << "GetBuffer failed: " << HRESULTToString(hr);
    return;
  }
  // Create a Direct2D bitmap that points to the swap chain surface
  D2D1_BITMAP_PROPERTIES1 properties = {};
  properties.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
  properties.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
  properties.bitmapOptions =
      D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;
  hr = dc->CreateBitmapFromDxgiSurface(surface.Get(), properties,
                                       bitmap.ReleaseAndGetAddressOf());
  if (FAILED(hr)) {
    DEBUG << "CreateBitmapFromDxgiSurface failed: " << HRESULTToString(hr);
    return;
  }
  // Point the device context to the bitmap for rendering
  dc->SetTarget(bitmap.Get());

  target.Reset();
  hr = dcompDevice->CreateTargetForHwnd(m_hWnd,
                                        true,  // Top most
                                        target.ReleaseAndGetAddressOf());
  if (FAILED(hr)) {
    DEBUG << "CreateTargetForHwnd failed: " << HRESULTToString(hr);
    return;
  }
  hr = dcompDevice->CreateVisual(visual.ReleaseAndGetAddressOf());
  if (FAILED(hr)) {
    DEBUG << "CreateVisual failed: " << HRESULTToString(hr);
    return;
  }
  hr = visual->SetContent(swapChain.Get());
  if (FAILED(hr)) {
    DEBUG << "SetContent failed: " << HRESULTToString(hr);
    return;
  }
  hr = target->SetRoot(visual.Get());
  if (FAILED(hr)) {
    DEBUG << "SetRoot failed: " << HRESULTToString(hr);
    return;
  }
  hr = dcompDevice->Commit();
  if (FAILED(hr)) {
    DEBUG << "Commit failed: " << HRESULTToString(hr);
    return;
  }
  // create or reuse brush from cache
  static const D2D1_COLOR_F brushColor = D2D1::ColorF(D2D1::ColorF::Black);
  hr = dc->CreateSolidColorBrush(brushColor, m_pBrush.ReleaseAndGetAddressOf());
  if (FAILED(hr)) {
    DEBUG << "CreateSolidColorBrush failed: " << HRESULTToString(hr);
    return;
  }
}

void D2D::OnResize(UINT width, UINT height) {
  if (width == 0 || height == 0)
    return;
  if (!dc || !swapChain || !visual || !target || !dcompDevice) {
    InitDirect2D();
    if (!dc || !swapChain || !visual || !target || !dcompDevice)
      return;
  }
  // Release Direct2D resources
  dc->SetTarget(nullptr);
  bitmap.Reset();
  surface.Reset();
  // Resize the swap chain
  HRESULT hr =
      swapChain->ResizeBuffers(2, width, height, DXGI_FORMAT_B8G8R8A8_UNORM, 0);
  if (hr == DXGI_ERROR_DEVICE_RESET || hr == DXGI_ERROR_DEVICE_REMOVED) {
    DEBUG << "Device lost during ResizeBuffers: " << HRESULTToString(hr);
    // attempt device recovery
    DeviceResources::Get().Reset();
    // per-window resources will be recreated by re-initializing
    InitDirect2D();
    return;
  }
  HR(hr);
  // Retrieve the new swap chain's back buffer
  HR(swapChain->GetBuffer(
      0, __uuidof(surface.Get()),
      reinterpret_cast<void**>(surface.ReleaseAndGetAddressOf())));
  // Create a new Direct2D bitmap pointing to the new swap chain surface
  D2D1_BITMAP_PROPERTIES1 properties = {};
  properties.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
  properties.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
  properties.bitmapOptions =
      D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;
  HR(dc->CreateBitmapFromDxgiSurface(surface.Get(), properties,
                                     bitmap.ReleaseAndGetAddressOf()));
  // Point the device context to the new bitmap for rendering
  dc->SetTarget(bitmap.Get());
  // Update DirectComposition target and visual
  HR(visual->SetContent(swapChain.Get()));
  HR(target->SetRoot(visual.Get()));
  HR(dcompDevice->Commit());
}

std::vector<std::wstring> ws_split(const std::wstring& in,
                                   const std::wstring& delim) {
  std::basic_regex<wchar_t> re{delim};
  return std::vector<std::wstring>{
      std::regex_token_iterator<std::wstring::const_iterator>(in.begin(),
                                                              in.end(), re, -1),
      std::regex_token_iterator<std::wstring::const_iterator>()};
}

static std::wstring MatchWordsOutLowerCaseTrim1st(
    const std::wstring& wstr,
    const std::basic_regex<wchar_t>& pattern) {
  std::wstring mat = L"";
  std::match_results<std::wstring::const_iterator> mc;
  std::wstring::const_iterator iter = wstr.cbegin();
  std::wstring::const_iterator end = wstr.cend();
  while (std::regex_search(iter, end, mc, pattern)) {
    for (const auto& m : mc) {
      mat = m;
      mat = mat.substr(1);
      break;
    }
    iter = mc.suffix().first;
  }
  std::wstring res;
  std::transform(mat.begin(), mat.end(), std::back_inserter(res), ::tolower);
  return res;
}

PtTextFormat D2D::GetOrCreateTextFormat(const std::wstring& face,
                                        int point,
                                        DWRITE_WORD_WRAPPING wrap) {
  // safety checks: require write factory and valid point
  if (!m_pWriteFactory || point <= 0 || face.empty()) {
    return PtTextFormat();
  }
  bool vertical_text =
      (m_style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT ||
       m_style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT_FULLSCREEN);
  DWRITE_FLOW_DIRECTION flow = m_style.vertical_text_left_to_right
                                   ? DWRITE_FLOW_DIRECTION_LEFT_TO_RIGHT
                                   : DWRITE_FLOW_DIRECTION_RIGHT_TO_LEFT;
  const std::wstring key =
      face + L"|" + std::to_wstring(point) + L"|" + std::to_wstring((int)wrap);

  PtTextFormat pFormat;
  {
    std::lock_guard<std::mutex> lk(cacheMutex);
    auto it = textFormatCache.find(key);
    if (it != textFormatCache.end()) {
      it->second.last_used = ++textFormatCacheClock;
      pFormat = it->second.format;
    }
  }

  if (!pFormat) {
    // create new text format
    static const std::wstring _mainFontFace = L"_InvalidFontName_";
    DWRITE_FONT_WEIGHT fontWeight = DWRITE_FONT_WEIGHT_NORMAL;
    DWRITE_FONT_STYLE fontStyle = DWRITE_FONT_STYLE_NORMAL;
    DWRITE_FONT_STRETCH fontStretch = DWRITE_FONT_STRETCH_NORMAL;
    ParseFontFace(face, fontWeight, fontStyle, fontStretch);
    ComPtr<IDWriteTextFormat> pFormatBase;
    HRESULT hr = m_pWriteFactory->CreateTextFormat(
        _mainFontFace.c_str(), NULL, fontWeight, fontStyle, fontStretch,
        point * m_dpiScaleFontPoint, L"", pFormatBase.ReleaseAndGetAddressOf());
    if (FAILED(hr) || !pFormatBase) {
      return PtTextFormat();
    }
    pFormatBase.As(&pFormat);
    pFormat->SetWordWrapping(wrap);

    std::vector<std::wstring> fontFaceStrVector;
    fontFaceStrVector = ws_split(face, L",");
    static const std::basic_regex<wchar_t> styleorweight(
        (L":[^:]*[^a-f0-9:]+[^:]*"), std::basic_regex<wchar_t>::icase);
    fontFaceStrVector[0] =
        std::regex_replace(fontFaceStrVector[0], styleorweight, L"");
    SetFontFallback(pFormat, fontFaceStrVector);

    {
      std::lock_guard<std::mutex> lk(cacheMutex);
      if (textFormatCache.size() >= kMaxTextFormatCacheEntries) {
        auto oldest = std::min_element(
            textFormatCache.begin(), textFormatCache.end(),
            [](const auto& lhs, const auto& rhs) {
              return lhs.second.last_used < rhs.second.last_used;
            });
        textFormatCache.erase(oldest);
      }
      textFormatCache[key] = {pFormat, ++textFormatCacheClock};
    }
  }

  // Update dynamic styles (always apply these as they might change with
  // m_style)
  if (vertical_text) {
    pFormat->SetFlowDirection(flow);
    pFormat->SetReadingDirection(DWRITE_READING_DIRECTION_TOP_TO_BOTTOM);
  } else {
    // Reset to default for horizontal
    pFormat->SetFlowDirection(DWRITE_FLOW_DIRECTION_TOP_TO_BOTTOM);
    pFormat->SetReadingDirection(DWRITE_READING_DIRECTION_LEFT_TO_RIGHT);
  }
  // Text alignment is the same for both orientations
  pFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
  pFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

  // convert percentage to float
  float linespacing =
      m_dpiScaleFontPoint * ((float)m_style.linespacing / 100.0f);
  float baseline = m_dpiScaleFontPoint * ((float)m_style.baseline / 100.0f);
  if (vertical_text)
    baseline = linespacing / 2;

  if (m_style.linespacing && m_style.baseline) {
    pFormat->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM,
                            point * linespacing, point * baseline);
  } else {
    pFormat->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_DEFAULT, 0, 0);
  }

  return pFormat;
}

void D2D::InitFontFormats() {
  InitFontFormats(m_style.label_font_face, m_style.label_font_point,
                  m_style.font_face, m_style.font_point,
                  m_style.comment_font_face, m_style.comment_font_point);
}

void D2D::InitFontFormats(const wstring& label_font_face,
                          const int label_font_point,
                          const wstring& font_face,
                          const int font_point,
                          const wstring& comment_font_face,
                          const int comment_font_point) {
  bool vertical_text_layout =
      (m_style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT ||
       m_style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT_FULLSCREEN);
  DWRITE_WORD_WRAPPING wrapping =
      ((m_style.max_width == 0 && !vertical_text_layout) ||
       (m_style.max_height == 0 && vertical_text_layout))
          ? DWRITE_WORD_WRAPPING_NO_WRAP
          : DWRITE_WORD_WRAPPING_WHOLE_WORD;
  DWRITE_WORD_WRAPPING wrapping_preedit =
      ((m_style.max_width == 0 && !vertical_text_layout) ||
       (m_style.max_height == 0 && vertical_text_layout))
          ? DWRITE_WORD_WRAPPING_NO_WRAP
          : DWRITE_WORD_WRAPPING_CHARACTER;
  pPreeditFormat =
      GetOrCreateTextFormat(font_face, font_point, wrapping_preedit);
  pTextFormat = GetOrCreateTextFormat(font_face, font_point, wrapping);
  pLabelFormat =
      GetOrCreateTextFormat(label_font_face, label_font_point, wrapping);
  pCommentFormat =
      GetOrCreateTextFormat(comment_font_face, comment_font_point, wrapping);
}

void D2D::InitDirectWriteResources() {
  // dwrite factory is provided by DeviceResources; ensure initialized
  HRESULT hr = DeviceResources::Get().EnsureInitialized();
  if (FAILED(hr))
    return;
  m_pWriteFactory = DeviceResources::Get().m_pWriteFactory;
  // guard against invalid style
  if (m_style.font_face.empty() || m_style.font_point <= 0)
    return;
  InitFontFormats();
}

void D2D::SetBrushColor(uint32_t color) {
  if (!m_pBrush)
    return;
  float a = ((color >> 24) & 0xFF) / 255.0f;
  float b = ((color >> 16) & 0xFF) / 255.0f;
  float g = ((color >> 8) & 0xFF) / 255.0f;
  float r = (color & 0xFF) / 255.0f;
  auto colorf = D2D1::ColorF(r, g, b, a);
  m_pBrush->SetColor(colorf);
}

void D2D::InitDpiInfo() {
  const auto oldDpiScaleFontPoint = m_dpiScaleFontPoint;
  const auto oldDpiScaleLayout = m_dpiScaleLayout;
  if (m_hWnd) {
    const UINT dpi = GetDpiForWindowCompat(m_hWnd);
    if (dpi > 0) {
      m_dpiX = static_cast<float>(dpi);
      m_dpiY = static_cast<float>(dpi);
    } else if (IsWindows8Point1OrGreater()) {
      HMONITOR const mon = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
      UINT x = 0, y = 0;
      HR(GetDpiForMonitor(mon, MDT_EFFECTIVE_DPI, &x, &y));
      m_dpiX = static_cast<float>(x);
      m_dpiY = static_cast<float>(y);
      if (m_dpiY == 0)
        m_dpiX = m_dpiY = 96.0f;
    } else {
      m_dpiY = m_dpiX = 96.0f;
    }
  } else {
    m_dpiY = m_dpiX = 96.0f;
  }
  m_dpiScaleFontPoint = m_dpiY / 72.0f;
  m_dpiScaleLayout = m_dpiY / 96.0;
  if (m_dpiScaleFontPoint != oldDpiScaleFontPoint ||
      m_dpiScaleLayout != oldDpiScaleLayout) {
    std::lock_guard<std::mutex> lk(cacheMutex);
    textFormatCache.clear();
    textFormatCacheClock = 0;
    pPreeditFormat.Reset();
    pTextFormat.Reset();
    pLabelFormat.Reset();
    pCommentFormat.Reset();
  }
}

void D2D::SetFontFallback(PtTextFormat textFormat,
                          const std::vector<std::wstring>& fontVector) {
  if (!m_pWriteFactory || !textFormat)
    return;
  ComPtr<IDWriteFontFallback> pSysFallback;
  HR(m_pWriteFactory->GetSystemFontFallback(
      pSysFallback.ReleaseAndGetAddressOf()));
  ComPtr<IDWriteFontFallback> pFontFallback = NULL;
  ComPtr<IDWriteFontFallbackBuilder> pFontFallbackBuilder = NULL;
  HR(m_pWriteFactory->CreateFontFallbackBuilder(
      pFontFallbackBuilder.ReleaseAndGetAddressOf()));
  std::vector<std::wstring> fallbackFontsVector;
  for (UINT32 i = 0; i < fontVector.size(); i++) {
    fallbackFontsVector = ws_split(fontVector[i], L":");
    std::wstring _fontFaceWstr, firstWstr, lastWstr;
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
        return std::stoul(wstr.c_str(), 0, 16);
      } catch (...) {
        return fallback;
      }
    };
    UINT first = func(firstWstr, 0), last = func(lastWstr, 0x10ffff);
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

void D2D::ParseFontFace(const std::wstring& fontFaceStr,
                        DWRITE_FONT_WEIGHT& fontWeight,
                        DWRITE_FONT_STYLE& fontStyle,
                        DWRITE_FONT_STRETCH& fontStretch) {
  static const std::basic_regex<wchar_t> patWeight(
      L"(:thin|:extra_light|:ultra_light|:light|:semi_light|:medium|:demi_bold|"
      L":semi_bold|:bold|:extra_bold|:ultra_bold|:black|:heavy|:extra_black|:"
      L"ultra_black)",
      std::basic_regex<wchar_t>::icase);
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
  const wstring weight = MatchWordsOutLowerCaseTrim1st(fontFaceStr, patWeight);
  const auto it = _mapWeight.find(weight);
  fontWeight =
      (it != _mapWeight.end()) ? it->second : DWRITE_FONT_WEIGHT_NORMAL;

  static const std::basic_regex<wchar_t> patStyle(
      L"(:italic|:oblique|:normal)", std::basic_regex<wchar_t>::icase);
  static const std::map<wstring, DWRITE_FONT_STYLE> _mapStyle = {
      {L"italic", DWRITE_FONT_STYLE_ITALIC},
      {L"oblique", DWRITE_FONT_STYLE_OBLIQUE},
      {L"normal", DWRITE_FONT_STYLE_NORMAL},
  };
  const wstring style = MatchWordsOutLowerCaseTrim1st(fontFaceStr, patStyle);
  const auto it2 = _mapStyle.find(style);
  fontStyle = (it2 != _mapStyle.end()) ? it2->second : DWRITE_FONT_STYLE_NORMAL;

  static const std::basic_regex<wchar_t> patStretch(
      L"(:undefined|:ultra_condensed|:extra_condensed|"
      L":condensed|:semi_condensed|"
      L":normal_stretch|:medium_stretch|:semi_expanded|"
      L":expanded|:extra_expanded|:ultra_expanded)",
      std::basic_regex<wchar_t>::icase);
  const std::map<wstring, DWRITE_FONT_STRETCH> _mapStretch = {
      {L"undefined", DWRITE_FONT_STRETCH_UNDEFINED},
      {L"ultra_condensed", DWRITE_FONT_STRETCH_ULTRA_CONDENSED},
      {L"extra_condensed", DWRITE_FONT_STRETCH_EXTRA_CONDENSED},
      {L"condensed", DWRITE_FONT_STRETCH_CONDENSED},
      {L"semi_condensed", DWRITE_FONT_STRETCH_SEMI_CONDENSED},
      {L"normal_stretch", DWRITE_FONT_STRETCH_NORMAL},
      {L"medium_stretch", DWRITE_FONT_STRETCH_MEDIUM},
      {L"semi_expanded", DWRITE_FONT_STRETCH_SEMI_EXPANDED},
      {L"expanded", DWRITE_FONT_STRETCH_EXPANDED},
      {L"extra_expanded", DWRITE_FONT_STRETCH_EXTRA_EXPANDED},
      {L"ultra_expanded", DWRITE_FONT_STRETCH_ULTRA_EXPANDED}};
  const wstring stretch =
      MatchWordsOutLowerCaseTrim1st(fontFaceStr, patStretch);
  const auto it3 = _mapStretch.find(stretch);
  fontStretch =
      (it3 != _mapStretch.end()) ? it3->second : DWRITE_FONT_STRETCH_NORMAL;
  if (fontStretch == DWRITE_FONT_STRETCH_UNDEFINED)
    fontStretch = DWRITE_FONT_STRETCH_NORMAL;
}

void D2D::GetTextSize(const wstring& text,
                      size_t nCount,
                      PtTextFormat& pTextFormat,
                      LPSIZE lpSize) {
  D2D1_SIZE_F sz;

  if (!pTextFormat) {
    lpSize->cx = 0;
    lpSize->cy = 0;
    return;
  }
  ComPtr<IDWriteTextLayout> pTextLayout;
  bool vertical_text_layout =
      (m_style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT ||
       m_style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT_FULLSCREEN);
  DWRITE_FLOW_DIRECTION flow = m_style.vertical_text_left_to_right
                                   ? DWRITE_FLOW_DIRECTION_LEFT_TO_RIGHT
                                   : DWRITE_FLOW_DIRECTION_RIGHT_TO_LEFT;
  if (vertical_text_layout) {
    HR(m_pWriteFactory->CreateTextLayout(
        text.c_str(), (int)nCount, pTextFormat.Get(), 0.0f,
        (float)m_style.max_height, pTextLayout.ReleaseAndGetAddressOf()));
    HR(pTextLayout->SetReadingDirection(
        DWRITE_READING_DIRECTION_TOP_TO_BOTTOM));
    HR(pTextLayout->SetFlowDirection(flow));
  } else
    HR(m_pWriteFactory->CreateTextLayout(
        text.c_str(), (int)nCount, pTextFormat.Get(), (float)m_style.max_width,
        0, pTextLayout.ReleaseAndGetAddressOf()));

  DWRITE_TEXT_METRICS textMetrics;
  HR(pTextLayout->GetMetrics(&textMetrics));
  sz = D2D1::SizeF(ceil(textMetrics.widthIncludingTrailingWhitespace),
                   ceil(textMetrics.height));
  lpSize->cx = (int)sz.width;
  lpSize->cy = (int)sz.height;

  if (vertical_text_layout) {
    auto max_height =
        m_style.max_height == 0 ? textMetrics.height : m_style.max_height;
    HR(m_pWriteFactory->CreateTextLayout(
        text.c_str(), (int)nCount, pTextFormat.Get(),
        textMetrics.widthIncludingTrailingWhitespace, max_height,
        pTextLayout.ReleaseAndGetAddressOf()));
    HR(pTextLayout->SetReadingDirection(
        DWRITE_READING_DIRECTION_TOP_TO_BOTTOM));
    HR(pTextLayout->SetFlowDirection(flow));
  } else {
    auto max_width = m_style.max_width == 0
                         ? textMetrics.widthIncludingTrailingWhitespace
                         : m_style.max_width;
    HR(m_pWriteFactory->CreateTextLayout(
        text.c_str(), (int)nCount, pTextFormat.Get(), max_width,
        textMetrics.height, pTextLayout.ReleaseAndGetAddressOf()));
  }

  DWRITE_OVERHANG_METRICS overhangMetrics;
  HR(pTextLayout->GetOverhangMetrics(&overhangMetrics));
  {
    if (overhangMetrics.left > 0)
      lpSize->cx += (LONG)(overhangMetrics.left + 1);
    if (overhangMetrics.right > 0)
      lpSize->cx += (LONG)(overhangMetrics.right + 1);
    if (overhangMetrics.top > 0)
      lpSize->cy += (LONG)(overhangMetrics.top + 1);
    if (overhangMetrics.bottom > 0)
      lpSize->cy += (LONG)(overhangMetrics.bottom + 1);
  }
}

// Helper function to convert IWICBitmap to a format compatible with Direct2D
static HRESULT ConvertWicBitmapToSupportedFormat(
    IWICBitmap* pWicBitmap,
    IWICImagingFactory* pWicFactory,
    IWICFormatConverter** ppConvertedBitmap) {
  if (!pWicBitmap || !pWicFactory || !ppConvertedBitmap)
    return E_POINTER;

  // Create a format converter to convert to a Direct2D supported format
  ComPtr<IWICFormatConverter> pWicFormatConverter;
  HRESULT hr = pWicFactory->CreateFormatConverter(
      pWicFormatConverter.ReleaseAndGetAddressOf());
  if (FAILED(hr))
    return hr;
  // Convert the bitmap to a format supported by Direct2D (e.g.,
  // DXGI_FORMAT_B8G8R8A8_UNORM)
  hr = pWicFormatConverter->Initialize(
      pWicBitmap,                      // Source bitmap
      GUID_WICPixelFormat32bppPBGRA,   // Supported pixel format for Direct2D
                                       // (BGRA, 32bpp)
      WICBitmapDitherTypeNone,         // No dithering
      nullptr,                         // No palette
      0.0f,                            // No alpha threshold
      WICBitmapPaletteTypeMedianCut);  // Robust default palette type

  if (FAILED(hr))
    return hr;
  *ppConvertedBitmap = pWicFormatConverter.Detach();
  return S_OK;
}

HRESULT D2D::GetBmpFromIcon(HICON hIcon, ComPtr<ID2D1Bitmap1>& pBitmap) {
  if (!hIcon)
    return S_FALSE;  // Failed to load icon
  pBitmap.Reset();
  // Get icon info and HBITMAP
  ICONINFO iconInfo{};
  if (!GetIconInfo(hIcon, &iconInfo))
    return S_FALSE;  // Failed to get icon info

  auto cleanupIconInfo = [&]() {
    if (iconInfo.hbmColor) {
      DeleteObject(iconInfo.hbmColor);
      iconInfo.hbmColor = nullptr;
    }
    if (iconInfo.hbmMask) {
      DeleteObject(iconInfo.hbmMask);
      iconInfo.hbmMask = nullptr;
    }
  };

  HBITMAP hBitmap = iconInfo.hbmColor ? iconInfo.hbmColor : iconInfo.hbmMask;
  if (!hBitmap) {
    cleanupIconInfo();
    return E_INVALIDARG;  // Failed to get bitmap from icon
  }

  // Use cached WIC factory
  IWICImagingFactory* pWicFactory = DeviceResources::Get().wicFactory.Get();
  if (!pWicFactory) {
    cleanupIconInfo();
    return E_POINTER;
  }
  // Create WIC Bitmap from HBITMAP
  ComPtr<IWICBitmap> pWicBitmap;
  HRESULT hr = pWicFactory->CreateBitmapFromHBITMAP(
      hBitmap, nullptr, WICBitmapUseAlpha, pWicBitmap.ReleaseAndGetAddressOf());
  if (FAILED(hr)) {
    cleanupIconInfo();
    return hr;
  }
  // Convert the bitmap to a Direct2D compatible format
  ComPtr<IWICFormatConverter> pConvertedBitmap;
  hr = ConvertWicBitmapToSupportedFormat(
      pWicBitmap.Get(), pWicFactory, pConvertedBitmap.ReleaseAndGetAddressOf());
  if (FAILED(hr)) {
    cleanupIconInfo();
    return hr;
  }
  if (!dc) {
    cleanupIconInfo();
    return E_POINTER;
  }
  hr = dc->CreateBitmapFromWicBitmap(pConvertedBitmap.Get(), nullptr,
                                     pBitmap.ReleaseAndGetAddressOf());
  cleanupIconInfo();
  return hr;
}

HRESULT D2D::GetIconFromFile(const wstring& iconPath,
                             ComPtr<ID2D1Bitmap1>& pD2DBitmap) {
  IWICImagingFactory* pWicFactory = DeviceResources::Get().wicFactory.Get();
  if (!pWicFactory) {
    return E_POINTER;
  }
  if (!dc) {
    return E_POINTER;
  }

  HRESULT hr;
  // Step 2: Load the image from file into a WICBitmapDecoder
  ComPtr<IWICBitmapDecoder> pDecoder;
  hr = pWicFactory->CreateDecoderFromFilename(
      iconPath.c_str(), nullptr, GENERIC_READ,
      WICDecodeOptions::WICDecodeMetadataCacheOnLoad,
      pDecoder.ReleaseAndGetAddressOf());
  if (FAILED(hr)) {
    DEBUG << "Failed to load image from file, HRESULT: " << std::hex << hr;
    return hr;
  }

  // Step 3: Get the first frame of the image
  ComPtr<IWICBitmapFrameDecode> pFrame;
  hr = pDecoder->GetFrame(0, pFrame.ReleaseAndGetAddressOf());
  if (FAILED(hr)) {
    DEBUG << "Failed to get frame from decoder, HRESULT: " << std::hex << hr;
    return hr;
  }

  // Step 4: Convert the frame to a supported format using IWICFormatConverter
  ComPtr<IWICFormatConverter> pConvertedBitmap;
  hr = pWicFactory->CreateFormatConverter(
      pConvertedBitmap.ReleaseAndGetAddressOf());
  if (FAILED(hr)) {
    DEBUG << "Failed to create IWICFormatConverter, HRESULT: " << std::hex
          << hr;
    return hr;
  }

  // Initialize the format converter
  hr = pConvertedBitmap->Initialize(
      pFrame.Get(),  // The source bitmap (IWICBitmapFrameDecode)
      GUID_WICPixelFormat32bppPBGRA,  // Target format (Direct2D supported
                                      // format)
      WICBitmapDitherTypeNone,        // Dithering type
      nullptr,                        // Palette (nullptr to use default)
      0.0f,                       // Alpha threshold (0.0 for no transparency)
      WICBitmapPaletteTypeCustom  // Palette type (Custom)
  );
  if (FAILED(hr)) {
    DEBUG << "Failed to initialize IWICFormatConverter, HRESULT: " << std::hex
          << hr;
    return hr;
  }

  // Step 5: Create a Direct2D bitmap from the converted WIC bitmap
  hr = dc->CreateBitmapFromWicBitmap(pConvertedBitmap.Get(), nullptr,
                                     pD2DBitmap.ReleaseAndGetAddressOf());
  if (FAILED(hr)) {
    DEBUG << "Failed to create Direct2D bitmap from WIC bitmap, HRESULT: "
          << std::hex << hr;
    return hr;
  }

  return S_OK;
}

HRESULT
D2D::CreateRoundedRectanglePath(const RECT& rc,
                                float radius,
                                const IsToRoundStruct& roundInfo,
                                ComPtr<ID2D1PathGeometry>& pPathGeometry) {
#define PT2F(x, y) D2D1::Point2F((float)x, (float)y)
  if (!d2Factory)
    return E_POINTER;
  // 创建路径几何对象
  HRESULT hr =
      d2Factory->CreatePathGeometry(pPathGeometry.ReleaseAndGetAddressOf());
  if (FAILED(hr))
    return hr;
  // 打开路径几何
  ComPtr<ID2D1GeometrySink> pSink = nullptr;
  hr = pPathGeometry->Open(pSink.ReleaseAndGetAddressOf());
  if (FAILED(hr))
    return hr;
  pSink->SetFillMode(D2D1_FILL_MODE_WINDING);
  // if radius is 0, return a rectangle path
  if (radius <= 0.0f) {
    pSink->BeginFigure(PT2F(rc.left, rc.top), D2D1_FIGURE_BEGIN_FILLED);
    pSink->AddLine(PT2F(rc.right, rc.top));
    pSink->AddLine(PT2F(rc.right, rc.bottom));
    pSink->AddLine(PT2F(rc.left, rc.bottom));
    pSink->AddLine(PT2F(rc.left, rc.top));
  } else {
    D2D1_RECT_F rectf{(float)rc.left, (float)rc.top, (float)rc.right,
                      (float)rc.bottom};
    radius = MIN(radius, (rc.right - rc.left) / 2.0f);
    radius = MIN(radius, (rc.bottom - rc.top) / 2.0f);
    const float good_radius = 0.382f * radius;
    // 从左上角开始
    if (roundInfo.IsTopLeftNeedToRound) {
      pSink->BeginFigure(PT2F(rectf.left + radius, rectf.top),
                         D2D1_FIGURE_BEGIN_FILLED);
    } else
      pSink->BeginFigure(PT2F(rectf.left, rectf.top), D2D1_FIGURE_BEGIN_FILLED);
    // 顶边
    pSink->AddLine(PT2F(rectf.right - radius, rectf.top));
    // 右上角圆角
    if (roundInfo.IsTopRightNeedToRound) {
      D2D1_BEZIER_SEGMENT seg{PT2F(rectf.right - good_radius, rectf.top),
                              PT2F(rectf.right, rectf.top + good_radius),
                              PT2F(rectf.right, rectf.top + radius)};
      pSink->AddBezier(seg);
    } else
      pSink->AddLine(PT2F(rectf.right, rectf.top));
    // 右边
    pSink->AddLine(PT2F(rectf.right, rectf.bottom - radius));
    // 右下角圆角
    if (roundInfo.IsBottomRightNeedToRound) {
      D2D1_BEZIER_SEGMENT seg{PT2F(rectf.right, rectf.bottom - good_radius),
                              PT2F(rectf.right - good_radius, rectf.bottom),
                              PT2F(rectf.right - radius, rectf.bottom)};
      pSink->AddBezier(seg);
    } else
      pSink->AddLine(PT2F(rectf.right, rectf.bottom));
    // 底边
    pSink->AddLine(PT2F(rectf.left + radius, rectf.bottom));
    // 左下角圆角
    if (roundInfo.IsBottomLeftNeedToRound) {
      D2D1_BEZIER_SEGMENT seg{PT2F(rectf.left + good_radius, rectf.bottom),
                              PT2F(rectf.left, rectf.bottom - good_radius),
                              PT2F(rectf.left, rectf.bottom - radius)};
      pSink->AddBezier(seg);
    } else
      pSink->AddLine(PT2F(rectf.left, rectf.bottom));
    // 左边
    pSink->AddLine(PT2F(rectf.left, rectf.top + radius));
    // 左上角圆角
    if (roundInfo.IsTopLeftNeedToRound) {
      D2D1_BEZIER_SEGMENT seg{PT2F(rectf.left, rectf.top + good_radius),
                              PT2F(rectf.left + good_radius, rectf.top),
                              PT2F(rectf.left + radius, rectf.top)};
      pSink->AddBezier(seg);
    } else {
      pSink->AddLine(PT2F(rectf.left, rectf.top));
    }
  }
  // 闭合路径
  pSink->EndFigure(D2D1_FIGURE_END_CLOSED);
  hr = pSink->Close();
#undef PT2F
  return hr;
}

HRESULT D2D::FillGeometry(const CRect& rect,
                          uint32_t color,
                          uint32_t radius,
                          IsToRoundStruct roundInfo,
                          bool to_blur) {
  if (!dc || !d2Factory)
    return E_POINTER;
  if (((color >> 24) & 0xFF) == 0)
    return S_OK;
  if (!to_blur && radius == 0) {
    // draw simple rectangle without path/blur
    D2D1_RECT_F rf{(float)rect.left, (float)rect.top, (float)rect.right,
                   (float)rect.bottom};
    SetBrushColor(color);
    dc->FillRectangle(&rf, m_pBrush.Get());
    return S_OK;
  }
  SetBrushColor(color);
  ComPtr<ID2D1PathGeometry> pGeometry;
  HRESULT hr;
  if (to_blur) {
    CRect rc = rect;
    rc.OffsetRect(m_dpiScaleLayout * m_style.shadow_offset_x,
                  m_dpiScaleLayout * m_style.shadow_offset_y);
    CreateRoundedRectanglePath(rc, radius, roundInfo, pGeometry);
    ComPtr<ID2D1BitmapRenderTarget> bitmapRenderTarget;
    hr = dc->CreateCompatibleRenderTarget(&bitmapRenderTarget);
    if (FAILED(hr)) {
      DEBUG << "CreateCompatibleRenderTarget failed: " << HRESULTToString(hr);
      return hr;
    }
    bitmapRenderTarget->BeginDraw();
    bitmapRenderTarget->Clear(D2D1::ColorF(0, 0.0f));
    bitmapRenderTarget->FillGeometry(pGeometry.Get(), m_pBrush.Get());
    bitmapRenderTarget->EndDraw();
    // Get the bitmap from the bitmap render target
    ComPtr<ID2D1Bitmap> bitmap;
    hr = bitmapRenderTarget->GetBitmap(&bitmap);
    if (FAILED(hr)) {
      DEBUG << "GetBitmap failed: " << HRESULTToString(hr);
      return hr;
    }
    //// Create a Gaussian blur effect
    ComPtr<ID2D1Effect> blurEffect;
    hr = dc->CreateEffect(CLSID_D2D1GaussianBlur, &blurEffect);
    if (FAILED(hr)) {
      DEBUG << "CreateEffect(GaussianBlur) failed: " << HRESULTToString(hr);
      return hr;
    }
    blurEffect->SetInput(0, bitmap.Get());
    blurEffect->SetValue(D2D1_GAUSSIANBLUR_PROP_STANDARD_DEVIATION,
                         (float)(m_dpiScaleLayout * m_style.shadow_radius));
    // Draw the blurred rounded rectangle onto the main render target
    dc->DrawImage(blurEffect.Get());
  } else {
    hr = CreateRoundedRectanglePath(rect, radius, roundInfo, pGeometry);
    if (FAILED(hr)) {
      DEBUG << "CreateRoundedRectanglePath failed: " << HRESULTToString(hr);
      return hr;
    }
    dc->FillGeometry(pGeometry.Get(), m_pBrush.Get());
  }
  return S_OK;
}

HRESULT D2D::DrawTextLayout(ComPtr<IDWriteTextLayout> pTextLayout,
                            float x,
                            float y,
                            uint32_t color) {
  SetBrushColor(color);
  dc->DrawTextLayout({x, y}, pTextLayout.Get(), m_pBrush.Get(),
                     D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
  return S_OK;
}
}  // namespace weasel
