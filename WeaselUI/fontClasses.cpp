#include "stdafx.h"
#include <string>
#include "fontClasses.h"

DirectWriteResources::DirectWriteResources(weasel::UIStyle& style) :
	_style(style),
	dpiScaleX_(0),
	dpiScaleY_(0),
	pD2d1Factory(NULL),
	pDWFactory(NULL),
	pRenderTarget(NULL),
	pTextFormat(NULL),
	pPreeditTextFormat(NULL),
	pLabelTextFormat(NULL),
	pCommentTextFormat(NULL)
{
	// prepare d2d1 resources
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
		pRenderTarget->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
		pRenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
	}
	//get the dpi information
	pD2d1Factory->GetDesktopDpi(&dpiScaleX_, &dpiScaleY_);
	dpiScaleX_ /= 72.0f;
	dpiScaleY_ /= 72.0f;

	InitResources(style);
}

DirectWriteResources::~DirectWriteResources()
{
	SafeRelease(&pPreeditTextFormat);
	SafeRelease(&pTextFormat);
	SafeRelease(&pLabelTextFormat);
	SafeRelease(&pCommentTextFormat);
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
	DWRITE_WORD_WRAPPING wrapping = (_style.max_width == 0 || _style.max_height == 0) ? DWRITE_WORD_WRAPPING_NO_WRAP : DWRITE_WORD_WRAPPING_WHOLE_WORD;
	DWRITE_FLOW_DIRECTION flow = _style.vertical_text_left_to_right ? DWRITE_FLOW_DIRECTION_LEFT_TO_RIGHT : DWRITE_FLOW_DIRECTION_RIGHT_TO_LEFT;
	HRESULT hResult = S_OK;
	std::vector<std::wstring> fontFaceStrVector;
	// text font text format set up
	boost::algorithm::split(fontFaceStrVector, font_face, boost::algorithm::is_any_of(L","));
	std::wstring mainFontFace;
	DWRITE_FONT_WEIGHT fontWeight = DWRITE_FONT_WEIGHT_NORMAL;
	DWRITE_FONT_STYLE fontStyle = DWRITE_FONT_STYLE_NORMAL;
	_ParseFontFace(fontFaceStrVector[0], mainFontFace, fontWeight, fontStyle);
	hResult = pDWFactory->CreateTextFormat(mainFontFace.c_str(), NULL,
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
		if (fontFaceStrVector.size() > 1)
			_SetFontFallback(pTextFormat, fontFaceStrVector);
	}

	hResult = pDWFactory->CreateTextFormat(mainFontFace.c_str(), NULL,
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
		if (fontFaceStrVector.size() > 1)
			_SetFontFallback(pPreeditTextFormat, fontFaceStrVector);
	}
	fontFaceStrVector.swap(std::vector<std::wstring>());

	// label font text format set up
	boost::algorithm::split(fontFaceStrVector, label_font_face, boost::algorithm::is_any_of(L","));
	fontWeight = DWRITE_FONT_WEIGHT_NORMAL;
	fontStyle = DWRITE_FONT_STYLE_NORMAL;
	_ParseFontFace(fontFaceStrVector[0], mainFontFace, fontWeight, fontStyle);
	hResult = pDWFactory->CreateTextFormat(mainFontFace.c_str(), NULL,
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
		if (fontFaceStrVector.size() > 1)
			_SetFontFallback(pLabelTextFormat, fontFaceStrVector);
	}
	fontFaceStrVector.swap(std::vector<std::wstring>());

	// comment font text format set up
	boost::algorithm::split(fontFaceStrVector, comment_font_face, boost::algorithm::is_any_of(L","));
	fontWeight = DWRITE_FONT_WEIGHT_NORMAL;
	fontStyle = DWRITE_FONT_STYLE_NORMAL;
	_ParseFontFace(fontFaceStrVector[0], mainFontFace, fontWeight, fontStyle);
	hResult = pDWFactory->CreateTextFormat(mainFontFace.c_str(), NULL,
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
		if (fontFaceStrVector.size() > 1)
			_SetFontFallback(pCommentTextFormat, fontFaceStrVector);
	}
	fontFaceStrVector.swap(std::vector<std::wstring>());
	return hResult;
}

HRESULT DirectWriteResources::InitResources(UIStyle& style)
{
	_style = style;
	return InitResources(style.label_font_face, style.label_font_point, style.font_face, _style.font_point, 
		style.comment_font_face, style.comment_font_point, style.layout_type==UIStyle::LAYOUT_VERTICAL_TEXT);
}

void DirectWriteResources::_ParseFontFace(const std::wstring fontFaceStr,
		std::wstring& fontFace, DWRITE_FONT_WEIGHT& fontWeight, DWRITE_FONT_STYLE& fontStyle)
{
	std::vector<std::wstring> parsedStrV; 
	boost::algorithm::split(parsedStrV, fontFaceStr, boost::algorithm::is_any_of(L":"));
	fontFace = parsedStrV[0];

	if (boost::regex_search(fontFaceStr, boost::wsmatch(), boost::wregex(L":thin", boost::wregex::icase)))
		fontWeight = DWRITE_FONT_WEIGHT_THIN;
	else if (boost::regex_search(fontFaceStr, boost::wsmatch(), boost::wregex(L":extra_light", boost::wregex::icase)))
		fontWeight = DWRITE_FONT_WEIGHT_EXTRA_LIGHT;
	else if (boost::regex_search(fontFaceStr, boost::wsmatch(), boost::wregex(L":ultra_light", boost::wregex::icase)))
		fontWeight = DWRITE_FONT_WEIGHT_ULTRA_LIGHT;
	else if (boost::regex_search(fontFaceStr, boost::wsmatch(), boost::wregex(L":light", boost::wregex::icase)))
		fontWeight = DWRITE_FONT_WEIGHT_LIGHT;
	else if (boost::regex_search(fontFaceStr, boost::wsmatch(), boost::wregex(L":semi_light", boost::wregex::icase)))
		fontWeight = DWRITE_FONT_WEIGHT_SEMI_LIGHT;
	else if (boost::regex_search(fontFaceStr, boost::wsmatch(), boost::wregex(L":medium", boost::wregex::icase)))
		fontWeight = DWRITE_FONT_WEIGHT_MEDIUM;
	else if (boost::regex_search(fontFaceStr, boost::wsmatch(), boost::wregex(L":demi_bold", boost::wregex::icase)))
		fontWeight = DWRITE_FONT_WEIGHT_DEMI_BOLD;
	else if (boost::regex_search(fontFaceStr, boost::wsmatch(), boost::wregex(L":semi_bold", boost::wregex::icase)))
		fontWeight = DWRITE_FONT_WEIGHT_SEMI_BOLD;
	else if (boost::regex_search(fontFaceStr, boost::wsmatch(), boost::wregex(L":bold", boost::wregex::icase)))
		fontWeight = DWRITE_FONT_WEIGHT_BOLD;
	else if (boost::regex_search(fontFaceStr, boost::wsmatch(), boost::wregex(L":extra_bold", boost::wregex::icase)))
		fontWeight = DWRITE_FONT_WEIGHT_EXTRA_BOLD;
	else if (boost::regex_search(fontFaceStr, boost::wsmatch(), boost::wregex(L":ultra_bold", boost::wregex::icase)))
		fontWeight = DWRITE_FONT_WEIGHT_ULTRA_BOLD;
	else if (boost::regex_search(fontFaceStr, boost::wsmatch(), boost::wregex(L":black", boost::wregex::icase)))
		fontWeight = DWRITE_FONT_WEIGHT_BLACK;
	else if (boost::regex_search(fontFaceStr, boost::wsmatch(), boost::wregex(L":heavy", boost::wregex::icase)))
		fontWeight = DWRITE_FONT_WEIGHT_HEAVY;
	else if (boost::regex_search(fontFaceStr, boost::wsmatch(), boost::wregex(L":extra_black", boost::wregex::icase)))
		fontWeight = DWRITE_FONT_WEIGHT_EXTRA_BLACK;
	else if (boost::regex_search(fontFaceStr, boost::wsmatch(), boost::wregex(L":ultra_black", boost::wregex::icase)))
		fontWeight = DWRITE_FONT_WEIGHT_ULTRA_BLACK;
	else
		fontWeight = DWRITE_FONT_WEIGHT_NORMAL;

	if (boost::regex_search(fontFaceStr, boost::wsmatch(), boost::wregex(L":italic", boost::wregex::icase)))
		fontStyle = DWRITE_FONT_STYLE_ITALIC;
	else if (boost::regex_search(fontFaceStr, boost::wsmatch(), boost::wregex(L":oblique", boost::wregex::icase)))
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
	for (UINT32 i = 1; i < fontVector.size(); i++)
	{
		boost::algorithm::split(fallbackFontsVector, fontVector[i], boost::algorithm::is_any_of(L":"));
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
			first = 0x10ffff;
		}
		DWRITE_UNICODE_RANGE range = { first, last };
		const  WCHAR* familys = { _fontFaceWstr.c_str() };
		pFontFallbackBuilder->AddMapping(&range, 1, &familys, 1);
		fallbackFontsVector.swap(std::vector<std::wstring>());
	}
	pFontFallbackBuilder->AddMappings(pSysFallback);
	pFontFallbackBuilder->CreateFontFallback(&pFontFallback);
	textFormat->SetFontFallback(pFontFallback);
	fallbackFontsVector.swap(std::vector<std::wstring>());
	SafeRelease(&pFontFallback);
	SafeRelease(&pSysFallback);
	SafeRelease(&pFontFallbackBuilder);
}

