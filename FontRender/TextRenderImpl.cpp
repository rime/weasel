#include "TextRenderImpl.h"

namespace gfx {
namespace win {

// The constructor stores the Direct2D factory and device context
// and creates resources the renderer will use.
TextRenderImpl::TextRenderImpl(ComPtr<IDWriteBitmapRenderTarget> renderTarget) :
    m_refCount(0),m_renderTarget(renderTarget)
{
    HRESULT hr = DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED,
            __uuidof(IDWriteFactory),
            &m_dwriteFactory
            );

    //m_dwriteFactory->CreateMonitorRenderingParams();
}

// Decomposes the received glyph run into smaller color glyph runs
// using IDWriteFactory4::TranslateColorGlyphRun. Depending on the
// type of each color run, the renderer uses Direct2D to draw the
// outlines, SVG content, or bitmap content.
HRESULT TextRenderImpl::DrawGlyphRun(
    _In_opt_ void* clientDrawingContext,
    FLOAT baselineOriginX,
    FLOAT baselineOriginY,
    DWRITE_MEASURING_MODE measuringMode,
    _In_ DWRITE_GLYPH_RUN const* glyphRun,
    _In_ DWRITE_GLYPH_RUN_DESCRIPTION const* glyphRunDescription,
    IUnknown* clientDrawingEffect
)
{
    HRESULT hr = DWRITE_E_NOCOLOR;
    // The list of glyph image formats this renderer is prepared to support.
    DWRITE_GLYPH_IMAGE_FORMATS supportedFormats =
            DWRITE_GLYPH_IMAGE_FORMATS_TRUETYPE |
            DWRITE_GLYPH_IMAGE_FORMATS_CFF |
            DWRITE_GLYPH_IMAGE_FORMATS_COLR |
            DWRITE_GLYPH_IMAGE_FORMATS_SVG |
            DWRITE_GLYPH_IMAGE_FORMATS_PNG |
            DWRITE_GLYPH_IMAGE_FORMATS_JPEG |
            DWRITE_GLYPH_IMAGE_FORMATS_TIFF |
            DWRITE_GLYPH_IMAGE_FORMATS_PREMULTIPLIED_B8G8R8A8;

    // Determine whether there are any color glyph runs within glyphRun. If
    // there are, glyphRunEnumerator can be used to iterate through them.
    D2D1_POINT_2F baselineOrigin = D2D1::Point2F(baselineOriginX, baselineOriginY);
    ComPtr<IDWriteColorGlyphRunEnumerator1> glyphRunEnumerator;
    hr = m_dwriteFactory->TranslateColorGlyphRun(
            baselineOrigin,
            glyphRun,
            glyphRunDescription,
            supportedFormats,
            measuringMode,
            nullptr,
            0,
            &glyphRunEnumerator
            );

    // Pass on the drawing call to the render target to do the real work.
    RECT dirtyRect = {0};
    hr = m_renderTarget->DrawGlyphRun(
            baselineOriginX,
            baselineOriginY,
            measuringMode,
            glyphRun,
            pRenderingParams_,
            RGB(0,200,255),
            &dirtyRect
            );
   
    return hr;
}

IFACEMETHODIMP TextRenderImpl::DrawUnderline(
    _In_opt_ void* clientDrawingContext,
    FLOAT baselineOriginX,
    FLOAT baselineOriginY,
    _In_ DWRITE_UNDERLINE const* underline,
    IUnknown* clientDrawingEffect
    )
{
    // Not implemented
    return E_NOTIMPL;
}

IFACEMETHODIMP TextRenderImpl::DrawStrikethrough(
    _In_opt_ void* clientDrawingContext,
    FLOAT baselineOriginX,
    FLOAT baselineOriginY,
    _In_ DWRITE_STRIKETHROUGH const* strikethrough,
    IUnknown* clientDrawingEffect
    )
{
    // Not implemented
    return E_NOTIMPL;
}

IFACEMETHODIMP TextRenderImpl::DrawInlineObject(
    _In_opt_ void* clientDrawingContext,
    FLOAT originX,
    FLOAT originY,
    IDWriteInlineObject* inlineObject,
    BOOL isSideways,
    BOOL isRightToLeft,
    IUnknown* clientDrawingEffect
    )
{
    // Not implemented
    return E_NOTIMPL;
}

IFACEMETHODIMP_(unsigned long) TextRenderImpl::AddRef()
{
    return InterlockedIncrement(&m_refCount);
}

IFACEMETHODIMP_(unsigned long) TextRenderImpl::Release()
{
    unsigned long newCount = InterlockedDecrement(&m_refCount);
    if (newCount == 0)
    {
        delete this;
        return 0;
    }

    return newCount;
}

IFACEMETHODIMP TextRenderImpl::IsPixelSnappingDisabled(
    _In_opt_ void* clientDrawingContext,
    _Out_ BOOL* isDisabled
    )
{
    return E_NOTIMPL;
}

IFACEMETHODIMP TextRenderImpl::GetCurrentTransform(
    _In_opt_ void* clientDrawingContext,
    _Out_ DWRITE_MATRIX* transform
    )
{
    return E_NOTIMPL;
}

IFACEMETHODIMP TextRenderImpl::GetPixelsPerDip(
    _In_opt_ void* clientDrawingContext,
    _Out_ FLOAT* pixelsPerDip
    )
{
    return E_NOTIMPL;
}

IFACEMETHODIMP TextRenderImpl::QueryInterface(
    IID const& riid,
    void** ppvObject
    )
{
    if (__uuidof(IDWriteTextRenderer) == riid)
    {
        *ppvObject = this;
    }
    else if (__uuidof(IDWritePixelSnapping) == riid)
    {
        *ppvObject = this;
    }
    else if (__uuidof(IUnknown) == riid)
    {
        *ppvObject = this;
    }
    else
    {
        *ppvObject = nullptr;
        return E_FAIL;
    }

    this->AddRef();

    return S_OK;
}

}  // namespace win
}  // namespace gfx