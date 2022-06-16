#include "stdafx.h"
#include "StandardLayout.h"

using namespace weasel;

StandardLayout::StandardLayout(const UIStyle &style, const Context &context, const Status &status)
	: Layout(style, context, status)
{
}
std::wstring StandardLayout::GetLabelText(const std::vector<Text> &labels, int id, const wchar_t *format) const
{
	wchar_t buffer[128];
	swprintf_s<128>(buffer, format, labels.at(id).str.c_str());
	return std::wstring(buffer);
}

// std::wstring版本
//static std::vector<std::wstring> ws_split(const std::wstring& in, const std::wstring& delim) {
//    std::wregex re{ delim };
//    return std::vector<std::wstring> {
//        std::wsregex_token_iterator(in.begin(), in.end(), re, -1),
//            std::wsregex_token_iterator()
//    };
//}
// from https://www.wabiapp.com/WabiSampleSource/windows/convert_crlf_w.html
std::wstring StandardLayout::ConvertCRLF(std::wstring strString, std::wstring strCRLF) const
{
	std::wstring strRet;
	std::wstring::iterator ite = strString.begin();
	std::wstring::iterator iteEnd = strString.end();
	if (0 < strString.size()) {
		wchar_t wNextChar = *ite++;
		while (1) {
			if ('\r' == wNextChar) {
				strRet += strCRLF;
				if (ite == iteEnd) { break; }
				wNextChar = *ite++;
				if ('\n' == wNextChar) {
					if (ite == iteEnd) { break; }
					wNextChar = *ite++;
				}
			}
			else if ('\n' == wNextChar) {
				strRet += strCRLF;
				if (ite == iteEnd) { break; }
				wNextChar = *ite++;
				if ('\r' == wNextChar) {
					if (ite == iteEnd) { break; }
					wNextChar = *ite++;
				}
			}
			else {
				strRet += wNextChar;
				if (ite == iteEnd) { break; }
				wNextChar = *ite++;
			}
		};
	}
	return(strRet);
}

void weasel::StandardLayout::GetTextExtentDCMultiline(CDCHandle dc, std::wstring wszString, int nCount, LPSIZE lpSize) const
{
	RECT TextArea = { 0, 0, 0, 0 };
	wszString = ConvertCRLF(wszString,  L"\r");
	DrawText(dc, wszString.c_str(), nCount, &TextArea, DT_CALCRECT);
	lpSize->cx = TextArea.right - TextArea.left;
	lpSize->cy = TextArea.bottom - TextArea.top;
}

void weasel::StandardLayout::GetTextSizeDW(const std::wstring text, int nCount, IDWriteTextFormat* pTextFormat, IDWriteFactory* pDWFactory, LPSIZE lpSize) const
{
	D2D1_SIZE_F sz;
	HRESULT hr = S_OK;
	IDWriteTextLayout* pTextLayout = NULL;
	if(pDWFactory == NULL)
		::MessageBox(NULL, L"factory null", L"omg factory", 0 );
	if(pTextFormat == NULL)
		::MessageBox(NULL, L"format null", L"omg format standardlayout.cpp", 0 );

	// 创建文本布局 
	if(pTextFormat != NULL)
		hr = pDWFactory->CreateTextLayout(text.c_str(), nCount, pTextFormat, 0.0f, 0.0f, &pTextLayout);
	if (SUCCEEDED(hr))
	{
		// 获取文本尺寸  
		DWRITE_TEXT_METRICS textMetrics;
		hr = pTextLayout->GetMetrics(&textMetrics);
		sz = D2D1::SizeF(ceil(textMetrics.width), ceil(textMetrics.height));
		lpSize->cx = (int)sz.width;
		lpSize->cy = (int)sz.height;
	}
	else
	{
		::MessageBox(NULL, L"failed create textlayout", L"ooo", 0 );
	}
	SafeRelease(&pTextLayout);
}

CSize StandardLayout::GetPreeditSize(CDCHandle dc) const
{
	const std::wstring &preedit = _context.preedit.str;
	const std::vector<weasel::TextAttribute> &attrs = _context.preedit.attributes;
	CSize size(0, 0);
	if (!preedit.empty())
	{
		GetTextExtentDCMultiline(dc, preedit, preedit.length(), &size);
		for (size_t i = 0; i < attrs.size(); i++)
		{
			if (attrs[i].type == weasel::HIGHLIGHTED)
			{
				const weasel::TextRange &range = attrs[i].range;
				if (range.start < range.end)
				{
					if (range.start > 0)
						size.cx += _style.hilite_spacing;
					else
						size.cx += _style.hilite_padding;
					if (range.end < static_cast<int>(preedit.length()))
						size.cx += _style.hilite_spacing;
					else
						size.cx += _style.hilite_padding;
				}
			}
		}
	}
	return size;
}

CSize StandardLayout::GetPreeditSize(CDCHandle dc, IDWriteTextFormat* pTextFormat, IDWriteFactory* pDWFactory) const
{
	const std::wstring &preedit = _context.preedit.str;
	const std::vector<weasel::TextAttribute> &attrs = _context.preedit.attributes;
	CSize size(0, 0);
	if (!preedit.empty())
	{
		GetTextSizeDW(preedit, preedit.length(), pTextFormat, pDWFactory, &size);
		for (size_t i = 0; i < attrs.size(); i++)
		{
			if (attrs[i].type == weasel::HIGHLIGHTED)
			{
				const weasel::TextRange &range = attrs[i].range;
				if (range.start < range.end)
				{
					if (range.start > 0)
						size.cx += _style.hilite_spacing;
					else
						size.cx += _style.hilite_padding;
					if (range.end < static_cast<int>(preedit.length()))
						size.cx += _style.hilite_spacing;
					else
						size.cx += _style.hilite_padding;
				}
			}
		}
	}
	return size;
}
void StandardLayout::UpdateStatusIconLayout(int* width, int* height)
{
	// rule 1. status icon is middle-aligned with preedit text or auxiliary text, whichever comes first
	// rule 2. there is a spacing between preedit/aux text and the status icon
	// rule 3. status icon is right aligned in WeaselPanel, when [margin_x + width(preedit/aux) + spacing + width(icon) + margin_x] < style.min_width
	if (ShouldDisplayStatusIcon())
	{
		int left = 0, middle = 0;
		if (!_preeditRect.IsRectNull())
		{
			left = _preeditRect.right + _style.spacing;
			middle = (_preeditRect.top + _preeditRect.bottom) / 2;
		}
		else if (!_auxiliaryRect.IsRectNull())
		{
			left = _auxiliaryRect.right + _style.spacing;
			middle = (_auxiliaryRect.top + _auxiliaryRect.bottom) / 2;
		}
		if (left && middle)
		{
			int right_alignment = *width - _style.margin_x - STATUS_ICON_SIZE;
			if (left > right_alignment)
			{
				*width = left + STATUS_ICON_SIZE + _style.margin_x;
			}
			else
			{
				left = right_alignment;
			}
			_statusIconRect.SetRect(left, middle - STATUS_ICON_SIZE / 2, left + STATUS_ICON_SIZE, middle + STATUS_ICON_SIZE / 2);
		}
		else
		{
			_statusIconRect.SetRect(0, 0, STATUS_ICON_SIZE, STATUS_ICON_SIZE);
			*width = *height = STATUS_ICON_SIZE;
		}
	}
}

bool StandardLayout::IsInlinePreedit() const
{
	return _style.inline_preedit && (_style.client_caps & weasel::INLINE_PREEDIT_CAPABLE) != 0 &&
		_style.layout_type != UIStyle::LAYOUT_VERTICAL_FULLSCREEN && _style.layout_type != UIStyle::LAYOUT_HORIZONTAL_FULLSCREEN;
}

bool StandardLayout::ShouldDisplayStatusIcon() const
{
	// rule 1. emphasis ascii mode
	// rule 2. show status icon when switching mode
	// rule 3. always show status icon with tips 
	return _status.ascii_mode || !_status.composing || !_context.aux.empty();
}
