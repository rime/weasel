#include "stdafx.h"
#include <string>
#include <WeaselUI.h>
//#include "fontClasses.h"

using namespace weasel;
#define STYLEORWEIGHT	(L":[^:]*[^a-f0-9:]+[^:]*")	

std::vector<std::wstring> ws_split(const std::wstring& in, const std::wstring& delim) 
{
	std::wregex re{ delim };
	return std::vector<std::wstring> {
		std::wsregex_token_iterator(in.begin(), in.end(), re, -1),
			std::wsregex_token_iterator()
	};
}

std::vector<std::string> c_split(const char* in, const char* delim) 
{
	std::regex re{ delim };
	return std::vector<std::string> {
		std::cregex_token_iterator(in, in + strlen(in),re, -1),
			std::cregex_token_iterator()
	};
}

std::vector<std::wstring> wc_split(const wchar_t* in, const wchar_t* delim) 
{
	std::wregex re{ delim };
	return std::vector<std::wstring> {
		std::wcregex_token_iterator(in, in + wcslen(in),re, -1),
			std::wcregex_token_iterator()
	};
}

DirectWriteResources::DirectWriteResources(weasel::UIStyle& style, UINT dpi = 0) :
	_style(style),
	dpiScaleX_(0),
	dpiScaleY_(0),
	pD2d1Factory(NULL),
	pDWFactory(NULL),
	pRenderTarget(NULL),
	pPreeditTextFormat(NULL),
	pTextFormat(NULL),
	pLabelTextFormat(NULL),
	pCommentTextFormat(NULL),
	pTextLayout(NULL)
{
	D2D1_TEXT_ANTIALIAS_MODE mode = _style.antialias_mode <= 3 ? (D2D1_TEXT_ANTIALIAS_MODE)(_style.antialias_mode) : D2D1_TEXT_ANTIALIAS_MODE_FORCE_DWORD; // prepare d2d1 resources
	HRESULT hResult = S_OK;
	// create factory
	if (pD2d1Factory == NULL)
		hResult = ::D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, &pD2d1Factory);
	// create IDWriteFactory
	if (pDWFactory == NULL)
		hResult = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&pDWFactory));
	/* ID2D1HwndRenderTarget */
	if (pRenderTarget == NULL)
	{
		const D2D1_PIXEL_FORMAT format = D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED);
		const D2D1_RENDER_TARGET_PROPERTIES properties = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT, format);
		pD2d1Factory->CreateDCRenderTarget(&properties, &pRenderTarget);
		pRenderTarget->SetTextAntialiasMode(mode);
		pRenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
	}
	pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0, 1.0, 1.0, 1.0), &pBrush);
	//get the dpi information
	if (dpi == 0)
	{
		pD2d1Factory->GetDesktopDpi(&dpiScaleX_, &dpiScaleY_);
		dpiScaleX_ /= 72.0f;
		dpiScaleY_ /= 72.0f;
	}
	else
	{
		dpiScaleX_ = dpi / 72.0f;
		dpiScaleY_ = dpi / 72.0f;
	}

	InitResources(style, dpi);
}

DirectWriteResources::~DirectWriteResources()
{
	SafeRelease(&pPreeditTextFormat);
	SafeRelease(&pTextFormat);
	SafeRelease(&pLabelTextFormat);
	SafeRelease(&pCommentTextFormat);
	SafeRelease(&pBrush);
	SafeRelease(&pRenderTarget);
	SafeRelease(&pDWFactory);
	SafeRelease(&pD2d1Factory);
	SafeRelease(&pTextLayout);
}

HRESULT DirectWriteResources::InitResources(std::wstring label_font_face, int label_font_point,
	std::wstring font_face, int font_point,
	std::wstring comment_font_face, int comment_font_point, bool vertical_text) 
{
	// prepare d2d1 resources
	SafeRelease(&pPreeditTextFormat);
	SafeRelease(&pTextFormat);
	SafeRelease(&pLabelTextFormat);
	SafeRelease(&pCommentTextFormat);
	DWRITE_WORD_WRAPPING wrapping = ((_style.max_width == 0 && _style.layout_type != UIStyle::LAYOUT_VERTICAL_TEXT) 
			|| (_style.max_height == 0 && _style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT)) ?
		DWRITE_WORD_WRAPPING_NO_WRAP : DWRITE_WORD_WRAPPING_CHARACTER;
	DWRITE_FLOW_DIRECTION flow = _style.vertical_text_left_to_right ? DWRITE_FLOW_DIRECTION_LEFT_TO_RIGHT : DWRITE_FLOW_DIRECTION_RIGHT_TO_LEFT;

	HRESULT hResult = S_OK;
	std::vector<std::wstring> fontFaceStrVector;

	// text font text format set up
	fontFaceStrVector = ws_split(font_face, L",");
	// set main font a invalid font name, to make every font range customizable
	const std::wstring _mainFontFace = L"_InvalidFontName_";
	DWRITE_FONT_WEIGHT fontWeight = DWRITE_FONT_WEIGHT_NORMAL;
	DWRITE_FONT_STYLE fontStyle = DWRITE_FONT_STYLE_NORMAL;
	// setup font weight and font style by the first unit of font_face setting string
	_ParseFontFace(fontFaceStrVector[0], fontWeight, fontStyle);
	fontFaceStrVector[0] = std::regex_replace(fontFaceStrVector[0], std::wregex(STYLEORWEIGHT, std::wregex::icase), L"");
	hResult = pDWFactory->CreateTextFormat(_mainFontFace.c_str(), NULL,
			fontWeight, fontStyle, DWRITE_FONT_STRETCH_NORMAL,
			font_point * dpiScaleX_, L"", reinterpret_cast<IDWriteTextFormat**>(&pTextFormat));
	if( pTextFormat != NULL)
	{
		if (vertical_text)
		{
			pTextFormat->SetFlowDirection(flow);
			pTextFormat->SetReadingDirection(DWRITE_READING_DIRECTION_TOP_TO_BOTTOM);
			pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
		}
		else
			pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);

		pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
		pTextFormat->SetWordWrapping(wrapping);
		_SetFontFallback(pTextFormat, fontFaceStrVector);
	}
	fontFaceStrVector.swap(std::vector<std::wstring>());

	fontFaceStrVector = ws_split(font_face, L",");
	//_ParseFontFace(fontFaceStrVector[0], fontWeight, fontStyle);
	fontFaceStrVector[0] = std::regex_replace(fontFaceStrVector[0], std::wregex(STYLEORWEIGHT, std::wregex::icase), L"");
	hResult = pDWFactory->CreateTextFormat(_mainFontFace.c_str(), NULL,
			fontWeight, fontStyle, DWRITE_FONT_STRETCH_NORMAL,
			font_point * dpiScaleX_, L"", reinterpret_cast<IDWriteTextFormat**>(&pPreeditTextFormat));
	if( pPreeditTextFormat != NULL)
	{
		if (vertical_text)
		{
			pPreeditTextFormat->SetFlowDirection(flow);
			pPreeditTextFormat->SetReadingDirection(DWRITE_READING_DIRECTION_TOP_TO_BOTTOM);
			pPreeditTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
		}
		else
			pPreeditTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
		pPreeditTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
		pPreeditTextFormat->SetWordWrapping(wrapping);
		_SetFontFallback(pPreeditTextFormat, fontFaceStrVector);
	}
	fontFaceStrVector.swap(std::vector<std::wstring>());

	// label font text format set up
	fontFaceStrVector = ws_split(label_font_face, L",");
	// setup weight and style of label_font_face
	_ParseFontFace(fontFaceStrVector[0], fontWeight, fontStyle);
	fontFaceStrVector[0] = std::regex_replace(fontFaceStrVector[0], std::wregex(STYLEORWEIGHT, std::wregex::icase), L"");
	hResult = pDWFactory->CreateTextFormat(_mainFontFace.c_str(), NULL,
			fontWeight, fontStyle, DWRITE_FONT_STRETCH_NORMAL,
			label_font_point * dpiScaleX_, L"", reinterpret_cast<IDWriteTextFormat**>(&pLabelTextFormat));
	if( pLabelTextFormat != NULL)
	{
		if (vertical_text)
		{
			pLabelTextFormat->SetFlowDirection(flow);
			pLabelTextFormat->SetReadingDirection(DWRITE_READING_DIRECTION_TOP_TO_BOTTOM);
			pLabelTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
		}
		else
			pLabelTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
		pLabelTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
		pLabelTextFormat->SetWordWrapping(wrapping);
		_SetFontFallback(pLabelTextFormat, fontFaceStrVector);
	}
	fontFaceStrVector.swap(std::vector<std::wstring>());

	// comment font text format set up
	fontFaceStrVector = ws_split(comment_font_face, L",");
	// setup weight and style of label_font_face
	_ParseFontFace(fontFaceStrVector[0], fontWeight, fontStyle);
	fontFaceStrVector[0] = std::regex_replace(fontFaceStrVector[0], std::wregex(STYLEORWEIGHT, std::wregex::icase), L"");
	hResult = pDWFactory->CreateTextFormat(_mainFontFace.c_str(), NULL,
			fontWeight, fontStyle, DWRITE_FONT_STRETCH_NORMAL,
			comment_font_point * dpiScaleX_, L"", reinterpret_cast<IDWriteTextFormat**>(&pCommentTextFormat));
	if( pCommentTextFormat != NULL)
	{
		if (vertical_text)
		{
			pCommentTextFormat->SetFlowDirection(flow);
			pCommentTextFormat->SetReadingDirection(DWRITE_READING_DIRECTION_TOP_TO_BOTTOM);
			pCommentTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
		}
		else
			pCommentTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
		pCommentTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
		pCommentTextFormat->SetWordWrapping(wrapping);
		_SetFontFallback(pCommentTextFormat, fontFaceStrVector);
	}
	fontFaceStrVector.swap(std::vector<std::wstring>());
	return hResult;
}

HRESULT DirectWriteResources::InitResources(UIStyle& style, UINT dpi = 0)
{
	_style = style;
	if(dpi)
	{
		dpiScaleX_ = dpi / 72.0f;
		dpiScaleY_ = dpi / 72.0f;
	}
	return InitResources(style.label_font_face, style.label_font_point, style.font_face, style.font_point, 
		style.comment_font_face, style.comment_font_point, style.layout_type==UIStyle::LAYOUT_VERTICAL_TEXT);
}

void weasel::DirectWriteResources::SetDpi(UINT dpi)
{
	dpiScaleX_ = dpi / 72.0f;
	dpiScaleY_ = dpi / 72.0f;
	
	SafeRelease(&pPreeditTextFormat);
	SafeRelease(&pTextFormat);
	SafeRelease(&pLabelTextFormat);
	SafeRelease(&pCommentTextFormat);
	InitResources(_style);
}

void DirectWriteResources::_ParseFontFace(const std::wstring fontFaceStr, DWRITE_FONT_WEIGHT& fontWeight, DWRITE_FONT_STYLE& fontStyle)
{
	if (std::regex_search(fontFaceStr, std::wsmatch(), std::wregex(L":thin", std::wregex::icase)))
		fontWeight = DWRITE_FONT_WEIGHT_THIN;
	else if (std::regex_search(fontFaceStr, std::wsmatch(), std::wregex(L":extra_light", std::wregex::icase)))
		fontWeight = DWRITE_FONT_WEIGHT_EXTRA_LIGHT;
	else if (std::regex_search(fontFaceStr, std::wsmatch(), std::wregex(L":ultra_light", std::wregex::icase)))
		fontWeight = DWRITE_FONT_WEIGHT_ULTRA_LIGHT;
	else if (std::regex_search(fontFaceStr, std::wsmatch(), std::wregex(L":light", std::wregex::icase)))
		fontWeight = DWRITE_FONT_WEIGHT_LIGHT;
	else if (std::regex_search(fontFaceStr, std::wsmatch(), std::wregex(L":semi_light", std::wregex::icase)))
		fontWeight = DWRITE_FONT_WEIGHT_SEMI_LIGHT;
	else if (std::regex_search(fontFaceStr, std::wsmatch(), std::wregex(L":medium", std::wregex::icase)))
		fontWeight = DWRITE_FONT_WEIGHT_MEDIUM;
	else if (std::regex_search(fontFaceStr, std::wsmatch(), std::wregex(L":demi_bold", std::wregex::icase)))
		fontWeight = DWRITE_FONT_WEIGHT_DEMI_BOLD;
	else if (std::regex_search(fontFaceStr, std::wsmatch(), std::wregex(L":semi_bold", std::wregex::icase)))
		fontWeight = DWRITE_FONT_WEIGHT_SEMI_BOLD;
	else if (std::regex_search(fontFaceStr, std::wsmatch(), std::wregex(L":bold", std::wregex::icase)))
		fontWeight = DWRITE_FONT_WEIGHT_BOLD;
	else if (std::regex_search(fontFaceStr, std::wsmatch(), std::wregex(L":extra_bold", std::wregex::icase)))
		fontWeight = DWRITE_FONT_WEIGHT_EXTRA_BOLD;
	else if (std::regex_search(fontFaceStr, std::wsmatch(), std::wregex(L":ultra_bold", std::wregex::icase)))
		fontWeight = DWRITE_FONT_WEIGHT_ULTRA_BOLD;
	else if (std::regex_search(fontFaceStr, std::wsmatch(), std::wregex(L":black", std::wregex::icase)))
		fontWeight = DWRITE_FONT_WEIGHT_BLACK;
	else if (std::regex_search(fontFaceStr, std::wsmatch(), std::wregex(L":heavy", std::wregex::icase)))
		fontWeight = DWRITE_FONT_WEIGHT_HEAVY;
	else if (std::regex_search(fontFaceStr, std::wsmatch(), std::wregex(L":extra_black", std::wregex::icase)))
		fontWeight = DWRITE_FONT_WEIGHT_EXTRA_BLACK;
	else if (std::regex_search(fontFaceStr, std::wsmatch(), std::wregex(L":ultra_black", std::wregex::icase)))
		fontWeight = DWRITE_FONT_WEIGHT_ULTRA_BLACK;
	else
		fontWeight = DWRITE_FONT_WEIGHT_NORMAL;

	if (std::regex_search(fontFaceStr, std::wsmatch(), std::wregex(L":italic", std::wregex::icase)))
		fontStyle = DWRITE_FONT_STYLE_ITALIC;
	else if (std::regex_search(fontFaceStr, std::wsmatch(), std::wregex(L":oblique", std::wregex::icase)))
		fontStyle = DWRITE_FONT_STYLE_OBLIQUE;
	else
		fontStyle = DWRITE_FONT_STYLE_NORMAL;
}

void DirectWriteResources::_SetFontFallback(IDWriteTextFormat1* textFormat, std::vector<std::wstring> fontVector)
{
	IDWriteFontFallback* pSysFallback;
	pDWFactory->GetSystemFontFallback(&pSysFallback);
	IDWriteFontFallback* pFontFallback = NULL;
	IDWriteFontFallbackBuilder* pFontFallbackBuilder = NULL;
	pDWFactory->CreateFontFallbackBuilder(&pFontFallbackBuilder);
	std::vector<std::wstring> fallbackFontsVector;
	for (UINT32 i = 0; i < fontVector.size(); i++)
	{
		fallbackFontsVector = ws_split(fontVector[i], L":");
		std::wstring _fontFaceWstr, firstWstr, lastWstr;
		if (fallbackFontsVector.size() == 3)
		{
			_fontFaceWstr = fallbackFontsVector[0];
			firstWstr = fallbackFontsVector[1];
			lastWstr = fallbackFontsVector[2];
			if (lastWstr.empty())
				lastWstr = L"10ffff";
			if (firstWstr.empty())
				firstWstr = L"0";
		}
		else if (fallbackFontsVector.size() == 2)	// fontName : codepoint
		{
			_fontFaceWstr = fallbackFontsVector[0];
			firstWstr = fallbackFontsVector[1];
			if (firstWstr.empty())
				firstWstr = L"0";
			lastWstr = L"10ffff";
		}
		else if (fallbackFontsVector.size() == 1)	// if only font defined, use all range
		{
			_fontFaceWstr = fallbackFontsVector[0];
			firstWstr = L"0";
			lastWstr = L"10ffff";
		}
		UINT first = 0, last = 0x10ffff;
		try {
			first = std::stoi(firstWstr.c_str(), 0, 16);
		}
		catch(...){
			first = 0;
		}
		try {
			last = std::stoi(lastWstr.c_str(), 0, 16);
		}
		catch(...){
			last = 0x10ffff;
		}
		DWRITE_UNICODE_RANGE range = { first, last };
		const  WCHAR* familys = { _fontFaceWstr.c_str() };
		pFontFallbackBuilder->AddMapping(&range, 1, &familys, 1);
		fallbackFontsVector.swap(std::vector<std::wstring>());
	}
	// add system defalt font fallback
	pFontFallbackBuilder->AddMappings(pSysFallback);
	pFontFallbackBuilder->CreateFontFallback(&pFontFallback);
	textFormat->SetFontFallback(pFontFallback);
	fallbackFontsVector.swap(std::vector<std::wstring>());
	SafeRelease(&pFontFallback);
	SafeRelease(&pSysFallback);
	SafeRelease(&pFontFallbackBuilder);
}

