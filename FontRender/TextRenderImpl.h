#ifndef UI_GFX_WIN_TEXT_RENDER_IMPL_H_
#define UI_GFX_WIN_TEXT_RENDER_IMPL_H_

#include "common.h"

//https://docs.microsoft.com/en-us/windows/win32/directwrite/render-to-a-gdi-surface?redirectedfrom=MSDN
//https://github.com/microsoft/Windows-classic-samples/blob/master/Samples/Win7Samples/multimedia/DirectWrite/PadWrite/RenderTarget.cpp

namespace gfx {
namespace win {
    //  The IDWriteTextRenderer interface is an input parameter to
    //  IDWriteTextLayout::Draw.  This interfaces defines a number of
    //  callback functions that the client application implements for
    //  custom text rendering.
    using namespace Microsoft::WRL;
    class TextRenderImpl : public IDWriteTextRenderer
    {
    public:
        TextRenderImpl(ComPtr<IDWriteBitmapRenderTarget> renderTarget);

        IFACEMETHOD(IsPixelSnappingDisabled)(
            _In_opt_ void* clientDrawingContext,
            _Out_ BOOL* isDisabled
            );

        IFACEMETHOD(GetCurrentTransform)(
            _In_opt_ void* clientDrawingContext,
            _Out_ DWRITE_MATRIX* transform
            );

        IFACEMETHOD(GetPixelsPerDip)(
            _In_opt_ void* clientDrawingContext,
            _Out_ FLOAT* pixelsPerDip
            );

        IFACEMETHOD(DrawGlyphRun)(
            _In_opt_ void* clientDrawingContext,
            FLOAT baselineOriginX,
            FLOAT baselineOriginY,
            DWRITE_MEASURING_MODE measuringMode,
            _In_ DWRITE_GLYPH_RUN const* glyphRun,
            _In_ DWRITE_GLYPH_RUN_DESCRIPTION const* glyphRunDescription,
            IUnknown* clientDrawingEffect
            );

        IFACEMETHOD(DrawUnderline)(
            _In_opt_ void* clientDrawingContext,
            FLOAT baselineOriginX,
            FLOAT baselineOriginY,
            _In_ DWRITE_UNDERLINE const* underline,
            IUnknown* clientDrawingEffect
            );

        IFACEMETHOD(DrawStrikethrough)(
            _In_opt_ void* clientDrawingContext,
            FLOAT baselineOriginX,
            FLOAT baselineOriginY,
            _In_ DWRITE_STRIKETHROUGH const* strikethrough,
            IUnknown* clientDrawingEffect
            );

        IFACEMETHOD(DrawInlineObject)(
            _In_opt_ void* clientDrawingContext,
            FLOAT originX,
            FLOAT originY,
            IDWriteInlineObject* inlineObject,
            BOOL isSideways,
            BOOL isRightToLeft,
            IUnknown* clientDrawingEffect
            );

    public:
        IFACEMETHOD_(unsigned long, AddRef) ();
        IFACEMETHOD_(unsigned long, Release) ();
        IFACEMETHOD(QueryInterface) (
            IID const& riid,
            void** ppvObject
            );

    private:
        unsigned long                        m_refCount;
        ComPtr<IDWriteFactory4>              m_dwriteFactory;
        ComPtr<IDWriteBitmapRenderTarget>    m_renderTarget;
    };


}  // namespace win
}  // namespace gfx	

#endif //UI_GFX_WIN_TEXT_RENDER_IMPL_H_