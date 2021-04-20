#include "stdafx.h"
//#include "pch.h"
#include "TextRender.h"

namespace gfx {
namespace win {

    //class TextRender
    TextRender::TextRender() :m_fontMgr(SkFontMgr::RefDefault()), m_shaper(kDefaultFont) {
        m_fontName  = kDefaultFont;
        m_fontColor = kDefaultFontColor;
        m_bkColor   = kDefaultBkColor;
        m_bEnabled  = false;
        m_bCanDraw  = false;
        m_zoomLevel = GetZoomLevel();
        //m_zoomLevel = 1.0;
        //m_fontSize = 26.0f;
        SetFontSize(kDefaultFontNumber);
        m_cpCache      = std::make_unique<CodePointCache>(kCodePointCacheSize);
        m_surfaceCache = std::make_unique<SurfaceCache>(kTextBmpCacheSize);
    }
    TextRender::~TextRender() {
        _trace0("~TextRender");
    }
    double TextRender::GetZoomLevel() {
        HWND hWnd = ::GetDesktopWindow();
        HMONITOR hMonitor = ::MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFOEX miex;
        miex.cbSize = sizeof(miex);
        GetMonitorInfo(hMonitor, &miex);
        int cxLogical = (miex.rcMonitor.right  - miex.rcMonitor.left);
        int cyLogical = (miex.rcMonitor.bottom - miex.rcMonitor.top);

        DEVMODE dm = { 0 };
        dm.dmSize = sizeof(dm);
        dm.dmDriverExtra = 0;
        EnumDisplaySettings(miex.szDevice, ENUM_CURRENT_SETTINGS, &dm);
        int cxPhysical = dm.dmPelsWidth;
        int cyPhysical = dm.dmPelsHeight;

        double horzScale = ((double)cxPhysical / (double)cxLogical);
        double vertScale = ((double)cyPhysical / (double)cyLogical);
        assert(horzScale == vertScale); //应该相同
        return horzScale;
    }
    /*
    // Returns whether the codepoint has emoji properties.
    bool TextRender::IsEmojiRelated(UChar32 codepoint) {
        return u_hasBinaryProperty(codepoint, UCHAR_EMOJI) ||
               u_hasBinaryProperty(codepoint, UCHAR_EMOJI_PRESENTATION) ||
               u_hasBinaryProperty(codepoint, UCHAR_REGIONAL_INDICATOR);
    }
    // Returns whether the codepoint has the 'extended pictographic' property.
    bool TextRender::IsExtendedPictographic(UChar32 c) {
        return u_hasBinaryProperty(c, UCHAR_EXTENDED_PICTOGRAPHIC);
    }
    bool TextRender::isRegionalIndicator(UChar32 c) {
        return u_hasBinaryProperty(c, UCHAR_REGIONAL_INDICATOR);
    }
    bool TextRender::IsEmojiPresentation(UChar32 c) {
        return u_hasBinaryProperty(c, UCHAR_EMOJI_PRESENTATION);
    }
    // Returns true if c is emoji Character.
    bool TextRender::IsEmojiChar(UChar32 c) {
        return u_hasBinaryProperty(c, UCHAR_EMOJI);
    }
    // Returns true if c is emoji text.
    bool TextRender::IsEmojiText(UChar32 c) {
        return IsEmojiChar(c) && !IsEmojiPresentation(c);
    }
    // Returns true if c is emoji.
    bool TextRender::IsEmoji(UChar32 c) {
        return IsEmojiPresentation(c);
    }
    bool TextRender::IsKeycap(UChar32 c)
    {
        return (c >= '0' && c <= '9') || c == '#' || c == '*';
    }
    */
    /*
    int TextRender::test_endian(){
        union {
            wchar_t   s; //2 bytes
            char      c[sizeof(wchar_t)];
        }endian;
        endian.s = 0x0102;
        if(endian.c[0] == 1 && endian.c[1] == 2)       //big endian
            return  1;
        else if (endian.c[0] == 2 && endian.c[1] == 1) //little endian
            return  2;
        else //unknow
            return -1;
    }

    std::u32string TextRender::To_UTF32(const u8string& s) {
        std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;
        return conv.from_bytes(s);
    }
    std::u32string TextRender::To_UTF32(const std::wstring &s)
    {
        try{
            using conv_le_type = std::codecvt_utf16<char32_t, 0x10ffff,
                                static_cast<std::codecvt_mode>(std::generate_header | std::little_endian)>;
            using conv_be_type = std::codecvt_utf16<char32_t, 0x10ffff,
                                static_cast<std::codecvt_mode>(std::generate_header)>;

            const char16_t *pData = reinterpret_cast<const char16_t*>(s.c_str());
            const char* _first = reinterpret_cast<const char*>(pData);
            const char* _last  = reinterpret_cast<const char*>(pData+s.length());

            std::wstring_convert<conv_le_type,char32_t>  conv_le_cvt;     // little endian
            std::wstring_convert<conv_be_type,char32_t>  conv_be_cvt;     // default big endian
            return conv_le_cvt.from_bytes(_first,_last);
        }catch(...){
            return std::u32string();
        }
    }*/
    u8string TextRender::To_UTF8(const std::wstring& s) {

#if _HAS_CXX17 && defined(_WIN32)

        UINT nCodePage = CP_UTF8;
        //UINT nCodePage = GetACP();

        int nLen = WideCharToMultiByte(nCodePage, 0, s.c_str(), -1, NULL, NULL, NULL, NULL);
        char* pBuf = new char[nLen + 1];
        assert(pBuf);
        memset(pBuf, 0, nLen + 1);
        WideCharToMultiByte(nCodePage, 0, s.c_str(), s.length(), pBuf, nLen, NULL, NULL);
        u8string su8 = pBuf;
        delete[]pBuf;
        return su8;
#else
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> conv;
        return conv.to_bytes(s);
#endif

    }
    void TextRender::Enable(bool bEnabled) {
        m_bEnabled = bEnabled;
    }
    void TextRender::SetFontHeight(float height) {
        m_fontSize = height;
        m_zoomLevel = 1.0f;
    }
    void TextRender::SetFontSize(int fontNumber) {
        int n = (fontNumber < 8 || fontNumber>72) ? kDefaultFontNumber : fontNumber;
        m_fontSize = (n * 1.0 / 72.0) * 96.0 *(m_zoomLevel); //(13.0/72.0)*96.0*1.5; 
    }
    void TextRender::SetFontName(const std::wstring& name) {
        if (name.empty())
            m_fontName = kDefaultFont;
        else
            m_fontName = To_UTF8(name);
    }
    void TextRender::SetFontName(const std::string& name) {
        m_fontName = name.empty() ? kDefaultFont : name;
    }
    void TextRender::SetFontColor(SkColor fontColor, SkColor bkColor) {
        m_fontColor = fontColor;
        m_bkColor   = bkColor;
    }
    void TextRender::SetFontColor(COLORREF fontColor, COLORREF bkColor) {
        m_fontColor = COLORREFToSkColor(fontColor);
        m_bkColor   = COLORREFToSkColor(bkColor);
    }
    void TextRender::SetFontColor(COLORREF fontColor) {
        m_fontColor = COLORREFToSkColor(fontColor);
     }

    std::vector<u8string> TextRender::split_graphemes(const char* str, size_t length) {
        std::vector<u8string> result;
        size_t pos = 0;
        while (pos < length) {
            size_t next_pos = graphemesplitter::next_grapheme(str, length, pos);
            result.push_back(u8string(str + pos, next_pos - pos));
            pos = next_pos;
        }
        return result;
    }
    std::vector<u8string> TextRender::split_graphemes(const u8string& str) {
        return split_graphemes(str.c_str(), str.size());
    }

    ItemList TextRender::TextAnalysis(const std::wstring& text) {
        if (text.empty())
            return ItemList();
        auto ItemIze = [](sk_sp<SkTypeface> f, const u8string& s) {
            Item l;
            SkASSERT(f);
            l._face = f;
            l._s += s;
            return l;
        };
        ItemList list;
        u8string su8 = To_UTF8(text);
        auto vec = split_graphemes(su8);
        for (const auto& it : vec)
        {
            int32_t cp = graphemesplitter::utf8_codepoint(it.c_str(), it.length(), 0);
            sk_sp<SkTypeface> face = GetFontFaceFromCacheByCodePoint(cp);
            if (nullptr == face) {
                vec.clear();
                return ItemList();
            }
            if (list.empty()) {
                list.push_back(ItemIze(face, it));
            }
            else {
                Item& l = list.back();
                if (l._face->uniqueID() == face->uniqueID())
                    l._s += it;
                else
                    list.push_back(ItemIze(face, it));
            }
#ifdef _DEBUG
            static int cc = 0;
            _trace("cp:%6x gslen:%2d cc:%d\r\n", cp, it.length(), cc++);
#endif
        } //for

        vec.clear();
        return list;
    }
 
    _hash_t TextRender::GetHash(const std::wstring& text) {
        std::array< uint32_t, 3> font = { m_fontColor,m_bkColor,SkScalarRoundToInt(m_fontSize) };
        _hash_t hash_text  = _xxhash(text);
        _hash_t hash_font  = _xxhash(font);
        std::array<_hash_t, 2> hash_result = { hash_text ,hash_font };
        _hash_t hash = _xxhash(hash_result);
        return hash;
    }

    sk_sp<SkTypeface> TextRender::GetFontFaceFromCacheByCodePoint(int32_t cp) {
        CodePointCache* cache = m_cpCache.get();
        CodePointCache::iterator iter = cache->Get(cp);
        if (iter == cache->end()) {
            SkTypeface* fc = m_fontMgr->matchFamilyStyleCharacter(m_fontName.c_str(), SkFontStyle(),
                nullptr, 0, static_cast<SkUnichar>(cp));
            sk_sp<SkTypeface> face(fc);
            if (nullptr == face) {
                _trace("GetFontFaceFromCacheByCodePoint: no font, cp:%d\r\n", cp);
                //SkASSERT(face);
                return nullptr;
            }
            auto result = cache->Put(cp, std::move(face));
            iter = result;
        }
        return iter->second;
    }

    sk_sp<SkSurface> TextRender::GetSurfaceFromCacheByText(const std::wstring& text) {
        if (text.empty())
            return nullptr;
        _hash_t hash = GetHash(text);
        SurfaceCache* cache = m_surfaceCache.get();
        SurfaceCache::iterator iter = cache->Get(hash);
        if (iter == cache->end()) {
            return nullptr;
        }
         return iter->second;
    }
    void TextRender::SetSurfaceToCache(const std::wstring& text, sk_sp<SkSurface> surface) {
        if (text.empty())
            return;
        SurfaceCache* cache = m_surfaceCache.get();
        _hash_t hash = GetHash(text);
        SurfaceCache::iterator iter = cache->Get(hash);
        if (iter == cache->end()) {
            auto result = cache->Put(hash, std::move(surface));
        }
    }
    long TextRender::CountCacheBytes_debug() _Acquires_exclusive_lock_(m_cacheLock) {
        {
#ifdef _DEBUG
            long nSize = 0;
            SkAutoMutexExclusive lock(m_cacheLock);
            SurfaceCache* cache = m_surfaceCache.get();
            for_each(cache->begin(), cache->end(), [&](auto it) {
                nSize += it.second->imageInfo().computeMinByteSize();
                });
            nSize = (nSize / 1024) / 1024;
            return nSize;
#endif // DEBUG
        }
        return 0;
    }

    SkBitmap TextRender::GetBitMapFromSurface(const std::wstring& text) {
        SkBitmap bmp;
        SkPixmap pm;
        sk_sp<SkSurface> surface = GetSurfaceFromCacheByText(text);
        if (surface && surface->peekPixels(&pm)) {
            bmp.installPixels(pm);
        }
        return bmp;
    }
    bool TextRender::OnDraw(const std::wstring& text,
                            HDC hdc, const RECT& rc) _Acquires_exclusive_lock_(m_cacheLock) {
        SkASSERT(hdc);
        if (!hdc) {  return false; }
        if (!m_bEnabled) { return false; }
        if (!m_bCanDraw) { return false; }
                   
        SkAutoMutexExclusive lock(m_cacheLock);
        SkBitmap m_bmpFont = GetBitMapFromSurface(text);
        if (m_bmpFont.isNull() || m_bmpFont.empty()) {
            return false;
        }
        
        m_bCanDraw = false;
        BITMAPINFO bmi = { 0 };
        memset(&bmi, 0, sizeof(bmi));
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth  =  m_bmpFont.width();
        bmi.bmiHeader.biHeight = -m_bmpFont.height(); // top-down image
        bmi.bmiHeader.biPlanes =  1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
        bmi.bmiHeader.biSizeImage = 0;

        RGBQUAD* pBits = nullptr;
        HBITMAP hBmp = ::CreateDIBSection(nullptr, &bmi, DIB_RGB_COLORS, (void**)&pBits, nullptr, 0);
        SkASSERT(hBmp);
        // SetMapMode( hdc, MM_TEXT );

        if (hBmp && pBits) {
            int orgW = m_bmpFont.width();
            int orgH = m_bmpFont.height();
            int w    = SkScalarRoundToInt(orgW / m_zoomLevel);
            int h    = SkScalarRoundToInt(orgH / m_zoomLevel);

            ::SetStretchBltMode(hdc, HALFTONE);
            ::SetBrushOrgEx(hdc, 0, 0, nullptr);

            memcpy(pBits, m_bmpFont.getPixels(), m_bmpFont.rowBytes() * m_bmpFont.height());
            HDC bmpDC   = ::CreateCompatibleDC(hdc);
            if (!bmpDC) {
                ::DeleteObject(hBmp);
                return false;
            }
            HDC blendDC = ::CreateCompatibleDC(hdc);
            if (!blendDC) {
                ::DeleteObject(hBmp);
                ::DeleteDC(bmpDC);
                return false;
            }
            HBITMAP hBlendBmp = ::CreateCompatibleBitmap(hdc, orgW, orgH);
            if (!hBlendBmp) {
                ::DeleteObject(hBmp);
                ::DeleteDC(bmpDC);
                ::DeleteDC(blendDC);
                return false;
            }

            ::SelectObject(bmpDC, hBmp);
            ::SelectObject(blendDC, hBlendBmp);
            ::StretchBlt(blendDC, 0, 0, orgW, orgH, hdc, rc.left, rc.top, w, h, SRCCOPY);
            
            BLENDFUNCTION blend_function = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
            ::GdiAlphaBlend(blendDC,0,0, orgW, orgH, bmpDC,0,0,orgW,orgH,blend_function);
           
            ::StretchBlt(hdc, rc.left, rc.top, w, h,blendDC, 0, 0, orgW, orgH, SRCCOPY);
            ::DeleteObject(hBmp);
            ::DeleteObject(hBlendBmp);
            ::DeleteDC(bmpDC);
            ::DeleteDC(blendDC);

            _trace("bmpw:%d bmph:%d bmpBytes:%d\r\n", 
                     m_bmpFont.width(), m_bmpFont.height(),
                     m_bmpFont.rowBytes() * m_bmpFont.height());
            return true;
        }
        return false;
    }

    bool TextRender::Render(const std::wstring& text, HWND hWnd, const RECT& rc) {
        SkASSERT(hWnd);
        if (!m_bEnabled)  { return false; }
        if (!hWnd)        { return false; }
        if (text.empty()) { return false; }
        HDC hdc = ::GetDC(hWnd);
        SkASSERT(hdc);
        if (!hdc) { return false; }
        bool b = Render(text, hdc, rc);
        ::ReleaseDC(hWnd, hdc);
        return b;
    }
    bool TextRender::Render(const std::wstring& text,HDC hdc, 
                            const RECT& rc) _Acquires_exclusive_lock_(m_cacheLock) {
        SkASSERT(hdc);
        if (!m_bEnabled)  { return false; } 
        if (text.empty()) { return false; }
        SkASSERT((rc.bottom - rc.top) > m_fontSize);
        RECT rc0(rc);
        _trace("rect | left:%d top:%d right:%d bottom:%d w:%d h:%d fontSize:%.2f\r\n", 
                rc.left, rc.top, rc.right, rc.bottom,rc.right-rc.left,rc.bottom-rc.top,m_fontSize);
        if (-1 == rc.bottom) {
            rc0.bottom = SkScalarRoundToInt(m_fontSize + m_fontSize / 2.0f) ;
        }
        bool b = false;
        {
            SkAutoMutexExclusive lock(m_cacheLock);
            b = Render(text, abs(rc0.right - rc0.left), abs(rc0.bottom - rc0.top));
        }
        if (!b) { return false; }
        //HWND hWnd = ::WindowFromDC(hdc);
        //assert(hWnd);
        //b = ::InvalidateRect(hWnd, &rc0, FALSE);
        return b;
    }
    bool TextRender::Render(const std::wstring& text, int width, int height) {
        SkASSERT(width  > 0);
        SkASSERT(height > 0);
        if (width <= 0 || height <= 0)
            return false;

        if (!m_bEnabled) { return false; }
        sk_sp<SkSurface> surface = GetSurfaceFromCacheByText(text);
        if (surface) {
            m_bCanDraw = true;
            return m_bCanDraw;
        }
        ItemList list = TextAnalysis(text);
        if (list.empty()) { return false; }
        m_bCanDraw = false;
        //SkImageInfo imageInfo = SkImageInfo::Make(width,height,  kRGBA_8888_SkColorType,kPremul_SkAlphaType);
        SkImageInfo imageInfo = SkImageInfo::Make(width, height, kBGRA_8888_SkColorType, kPremul_SkAlphaType);
        surface = SkSurface::MakeRaster(imageInfo);
        SkCanvas *canvas= surface->getCanvas();
        canvas->clear(SK_AlphaTRANSPARENT);// 背景为透明色
        //canvas->clear(m_bkColor);
        SkPaint paint;
        paint.setAntiAlias(true);
        //paint.setFilterQuality(SkFilterQuality::kHigh_SkFilterQuality);
        //paint.setAlpha(10);
        //paint.setBlendMode(SkBlendMode::kSrc);

        //SkPath path;
        //paint.setColor(SK_ColorGREEN);
        //SkRect rc = { 0,0,width,height };
        //path.addRoundRect(rc, SkIntToScalar(5), SkIntToScalar(5));
        //canvas->drawPath(path, paint);
        //canvas->scale(SkDoubleToScalar(1.1), SkDoubleToScalar(1.1));
        paint.setColor(m_fontColor);
        SkScalar y_adjust = (height * 1.0f - m_fontSize) / 4.0f + m_fontSize;
        float advance = 0.0f;
        for (auto& it : list) {
            SkFont   font(it._face, m_fontSize);
            font.setEdging(SkFont::Edging::kSubpixelAntiAlias);
            font.setSubpixel(true);

            int32_t cp = graphemesplitter::utf8_codepoint(it._s.c_str(), it._s.length(), 0);
            auto blob = m_shaper.Shape(it._s.c_str(), it._face, font);
            SkASSERT(blob);
            if (blob.get() == nullptr) {
                _trace0("nullptr blob");
                surface = nullptr;
                list.clear();
                return false;
            }
            SkScalar ll  = font.measureText(it._s.c_str(), it._s.length(), SkTextEncoding::kUTF8);
            SkScalar llw = blob->bounds().width();
            SkScalar llh = blob->bounds().height();
            canvas->drawTextBlob(blob.get(), advance, m_fontSize, paint);
            //advance += IsEmojiPresentation(cp) ? llw : ll;
            advance +=  ll;
#ifdef _DEBUG
            SkString  fs, fs2, fs3;
            it._face->getFamilyName(&fs);
            it._face->getPostScriptName(&fs2);
            _trace("cp:%6x slen:%2d family:%s script:%s ll:%.2f w:%.2f h:%.2f\r\n",
                cp, it._s.length(), fs.c_str(), fs2.c_str(), ll, llw, llh);
#endif // _DEBUG
            
            blob = nullptr;
        }
        SetSurfaceToCache(text, surface);
        list.clear();
        m_bCanDraw = true;
        return true;
    }


SkColor  TextRender::COLORREFToSkColor(COLORREF color) {
#ifndef _MSC_VER
    return SkColorSetRGB(GetRValue(color), GetGValue(color), GetBValue(color));
#else
    // ARGB = 0xFF000000 | ((0BGR -> RGB0) >> 8)
    return 0xFF000000u | (_byteswap_ulong(color) >> 8);
#endif
}
COLORREF TextRender::SkColorToCOLORREF(SkColor  color) {
#ifndef _MSC_VER
    return RGB(SkColorGetR(color), SkColorGetG(color), SkColorGetB(color));
#else
    // 0BGR = ((ARGB -> BGRA) >> 8)
    return (_byteswap_ulong(color) >> 8);
#endif
}

bool TextRender::PixelsHaveAlpha(const uint32_t* pixels, size_t num_pixels) {
    for (const uint32_t* end = pixels + num_pixels; pixels != end; ++pixels) {
        if ((*pixels & 0xff000000) != 0)
            return true;
    }
    return false;
}

void TextRender::ConvertSkiaToRGBA(const unsigned char* skia,
                                   int pixel_width,unsigned char* rgba) {
    int total_length = pixel_width * 4;
    for (int i = 0; i < total_length; i += 4) {
        const uint32_t pixel_in = *reinterpret_cast<const uint32_t*>(&skia[i]);

        // Pack the components here.
        SkAlpha alpha = SkGetPackedA32(pixel_in);
        if (alpha != 0 && alpha != 255) {
            SkColor unmultiplied = SkUnPreMultiply::PMColorToColor(pixel_in);
            rgba[i + 0] = SkColorGetR(unmultiplied);
            rgba[i + 1] = SkColorGetG(unmultiplied);
            rgba[i + 2] = SkColorGetB(unmultiplied);
            rgba[i + 3] = alpha;
        }
        else {
            rgba[i + 0] = SkGetPackedR32(pixel_in);
            rgba[i + 1] = SkGetPackedG32(pixel_in);
            rgba[i + 2] = SkGetPackedB32(pixel_in);
            rgba[i + 3] = alpha;
        }
    }
}


}  // namespace win
}  // namespace gfx
