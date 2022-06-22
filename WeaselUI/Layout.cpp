#include "stdafx.h"
#include "Layout.h"

using namespace weasel;

Layout::Layout(const UIStyle &style, const Context &context, const Status &status)
	: _style(style), _context(context), _status(status)
{
}
DirectWriteResources::DirectWriteResources() :
	dpiScaleX_(0),
	dpiScaleY_(0),
	pD2d1Factory(NULL),
	pDWFactory(NULL),
	pRenderTarget(NULL),
	pTextFormat(NULL),
	pLabelTextFormat(NULL),
	pCommentTextFormat(NULL)
{
}

DirectWriteResources::~DirectWriteResources()
{
	SafeRelease(&pTextFormat);
	SafeRelease(&pLabelTextFormat);
	SafeRelease(&pCommentTextFormat);
	SafeRelease(&pRenderTarget);
	SafeRelease(&pDWFactory);
	SafeRelease(&pD2d1Factory);
}

HRESULT DirectWriteResources::InitResources(std::wstring label_font_face, int label_font_point,
	std::wstring font_face, int font_point,
	std::wstring comment_font_face, int comment_font_point) 
{
	// prepare d2d1 resources
	HRESULT hResult = S_OK;
	// create factory
	if(pD2d1Factory == NULL)
		hResult = ::D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, &pD2d1Factory);
	// create IDWriteFactory
	if(pDWFactory == NULL)
		hResult = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&pDWFactory));
	/* ID2D1HwndRenderTarget */
	if (pRenderTarget == NULL)
	{
		const D2D1_PIXEL_FORMAT format = D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED);
		const D2D1_RENDER_TARGET_PROPERTIES properties = D2D1::RenderTargetProperties( D2D1_RENDER_TARGET_TYPE_DEFAULT, format);
		pD2d1Factory->CreateDCRenderTarget(&properties, &pRenderTarget);
	}
    //get the dpi information
    HDC screen = ::GetDC(0);
    dpiScaleX_ = GetDeviceCaps(screen, LOGPIXELSX) / 72.0f;
    dpiScaleY_ = GetDeviceCaps(screen, LOGPIXELSY) / 72.0f;
    ::ReleaseDC(0, screen);
	if(pTextFormat == NULL)
		hResult = pDWFactory->CreateTextFormat(font_face.c_str(), NULL,
			DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
			font_point * dpiScaleX_ / 72.0f, L"", &pTextFormat);
	if( pTextFormat != NULL)
	{
		pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
		pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
		pTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
	}
	if(pLabelTextFormat == NULL)
		hResult = pDWFactory->CreateTextFormat(label_font_face.c_str(), NULL,
			DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
			label_font_point * dpiScaleX_ / 72.0f, L"", &pLabelTextFormat);
	if( pLabelTextFormat != NULL)
	{
		pLabelTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
		pLabelTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
		pLabelTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
	}
	if(pCommentTextFormat == NULL)
		hResult = pDWFactory->CreateTextFormat(comment_font_face.c_str(), NULL,
			DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
			comment_font_point * dpiScaleX_ / 72.0f, L"", &pCommentTextFormat);
	if( pCommentTextFormat != NULL)
	{
		pCommentTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
		pCommentTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
		pCommentTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
	}
	return hResult;
}
