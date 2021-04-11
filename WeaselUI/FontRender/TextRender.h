/******************************************/
/*  TextRender - render text by GDI       */
/*  ver: 0.1                              */
/*  Copyright (C) 2021,cnlog              */
/******************************************/

#ifndef UI_GFX_WIN_TEXT_RENDER_H_
#define UI_GFX_WIN_TEXT_RENDER_H_

#include "HbFont.hpp"
#include "graphemesplitter.hpp"
#include "xxhash.hpp"

#ifndef _DEBUG
//#   pragma comment(linker, "/MERGE:.data=.text")
//#   pragma comment(linker, "/MERGE:.rdata=.text")
#endif //_DEBUG

#pragma comment( lib , "user32.lib" )
#pragma comment( lib , "harfbuzz.lib" )
//#pragma comment( lib , "icu.lib" ) 
//#pragma comment( lib , "opengl32.lib" )

#define USE_SKDLL

#if defined(_WIN32) 
#   if defined(USE_SKDLL)
#       if defined(_WIN64)
#           pragma comment( lib , "skiax64.dll.lib" )
#       else
#           pragma comment( lib , "skiax86.dll.lib" )
#       endif
        static SkThreadID SkGetThreadID() { return GetCurrentThreadId(); }
#   else
#       pragma comment( lib , "skia.lib" )   
#   endif
#endif

namespace gfx {
namespace win {

#if defined(_WIN32) 
#   if defined(_WIN64)
#       define  _hash_t      xxh::hash64_t
#       define  _xxhash(v)   xxh::xxhash<64>(v)
#   else
#       define  _hash_t      xxh::hash32_t
#       define  _xxhash(v)   xxh::xxhash<32>(v)
#   endif
#endif

 constexpr int  kTextBmpCacheSize   = 256;
 constexpr int  kCodePointCacheSize = 1024;

 using  CodePointCache    = base::MRUCache<int32_t, sk_sp<SkTypeface>>;
 using  SurfaceCache      = base::MRUCache<_hash_t, sk_sp<SkSurface>>;
 using  CodePointCachePtr = std::unique_ptr<CodePointCache>;
 using  SurfaceCachePtr   = std::unique_ptr<SurfaceCache>;

 using Item = struct{
     sk_sp<SkTypeface> _face;
     u8string _s;
 };
 using ItemList = std::vector<Item> ;

 // class TextRender
 class TextRender{
    public:
        TextRender();
        virtual ~TextRender();
    public:
        bool IsEnable() { return m_bEnabled; }
        void Enable(bool bEnabled);
        void SetFontSize(int fontNumber);
        void SetFontHeight(float height);
        void SetFontName(std::string  name);
        void SetFontName(std::wstring name);
        void SetFontColor(SkColor  fontColor,SkColor  bkColor);
        void SetFontColor(COLORREF fontColor,COLORREF bkColor);
        void SetFontColor(COLORREF fontColo);
        bool CanDraw() {return m_bEnabled && m_bCanDraw;};
        bool OnDraw(std::wstring text, HDC  hdc,  const RECT& rc);
        bool Render(std::wstring text, HDC  hdc,  const RECT& rc);
        bool Render(std::wstring text, HWND hWnd, const RECT& rc);
        long CountCacheBytes_debug();
     protected:
        bool Render(std::wstring text, int width, int height);
        u8string To_UTF8(const std::wstring& s);
        SkBitmap GetBitMapFromSurface(const std::wstring text);
        SkColor  COLORREFToSkColor(COLORREF color);
        COLORREF SkColorToCOLORREF(SkColor  color);
        void ConvertSkiaToRGBA(const unsigned char* skia, int pixel_width, unsigned char* rgba);
    protected:
        ItemList TextAnalysis(std::wstring text);
        std::vector<u8string> split_graphemes(const char* str, size_t length);
        std::vector<u8string> split_graphemes(const u8string& str);
    protected:
        //c++17
        //std::u32string To_UTF32(const std::wstring  &s);
        //std::u32string To_UTF32(const u8string &s);
        //inline int test_endian(); //win32 api default Le. ret:1 big endian,2 little endian,-1 unknow
#if (_WIN32_WINNT >= NTDDI_WIN10_RS3)
        inline bool IsEmojiPresentation(UChar32 c);
        bool IsEmojiRelated(UChar32 c);
        bool IsEmojiChar(UChar32 c);
        bool IsEmojiText(UChar32 c);
        bool IsEmoji(UChar32 c);

        bool IsKeycap(UChar32 c);
        bool IsExtendedPictographic(UChar32 c);
        bool isRegionalIndicator(UChar32 c);  
#endif
    public:
        double GetZoomLevel();
        bool PixelsHaveAlpha(const uint32_t* pixels, size_t num_pixels);
    private:
        bool      m_bEnabled;
        bool      m_bCanDraw;
        float     m_fontSize;
        float     m_zoomLevel;
        u8string  m_fontName;
        SkColor   m_fontColor;
        SkColor   m_bkColor;
        const sk_sp<SkFontMgr> m_fontMgr;
        TextShaper m_shaper;
     private:
        SkMutex    m_cacheLock;
        CodePointCachePtr m_cpCache;
        SurfaceCachePtr   m_surfaceCache;

        sk_sp<SkTypeface> GetFontFaceFromCacheByCodePoint(int32_t cp);
        sk_sp<SkSurface>  GetSurfaceFromCacheByText(const std::wstring text);
        void SetSurfaceToCache(const std::wstring text, sk_sp<SkSurface> surface);
        //hash {text,textcolor,backcolor} 
        _hash_t GetHash(const std::wstring text);
    private:
        NOCOPY(TextRender);
 };
//
}  // namespace win
}  // namespace gfx	

#endif //UI_GFX_WIN_TEXT_RENDER_H_