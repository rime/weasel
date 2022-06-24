#include "stdafx.h"
#include "FullScreenLayout.h"

using namespace weasel;

FullScreenLayout::FullScreenLayout(const UIStyle &style, const Context &context, const Status &status, const CRect& inputPos, Layout* layout)
	: StandardLayout(style, context, status), mr_inputPos(inputPos), m_layout(layout)
{
}

FullScreenLayout::~FullScreenLayout()
{
	delete m_layout;
}

void FullScreenLayout::DoLayout(CDCHandle dc, GDIFonts* pFonts)
{
	if (_context.empty())
	{
		int width = 0, height = 0;
		UpdateStatusIconLayout(&width, &height);
		_contentSize.SetSize(width, height);
		return;
	}

	CRect workArea;
	HMONITOR hMonitor = MonitorFromRect(mr_inputPos, MONITOR_DEFAULTTONEAREST);
	if (hMonitor)
	{
		MONITORINFO info;
		info.cbSize = sizeof(MONITORINFO);
		if (GetMonitorInfo(hMonitor, &info))
		{
			workArea = info.rcWork;
		}
	}

	int fontPoint = _style.font_point;
	int step = 32;
	do {
		m_layout->DoLayout(dc, pFonts);
	}
	while (AdjustFontPoint(dc, workArea, pFonts, step));

	if (fontPoint < 4) fontPoint = 4;
	else if (fontPoint > 2048) fontPoint = 2048;
	const_cast<UIStyle*>(&_style)->font_point = fontPoint;

	int offsetX = (workArea.Width() - m_layout->GetContentSize().cx) / 2;
	int offsetY = (workArea.Height() - m_layout->GetContentSize().cy) / 2;
	_preeditRect = m_layout->GetPreeditRect();
	_preeditRect.OffsetRect(offsetX, offsetY);
	_auxiliaryRect = m_layout->GetAuxiliaryRect();
	_auxiliaryRect.OffsetRect(offsetX, offsetY);
	_highlightRect = m_layout->GetHighlightRect();
	_highlightRect.OffsetRect(offsetX, offsetY);
	for (int i = 0, n = (int)_context.cinfo.candies.size(); i < n && i < MAX_CANDIDATES_COUNT; ++i)
	{
		_candidateLabelRects[i] = m_layout->GetCandidateLabelRect(i);
		_candidateLabelRects[i].OffsetRect(offsetX, offsetY);
		_candidateTextRects[i] = m_layout->GetCandidateTextRect(i);
		_candidateTextRects[i].OffsetRect(offsetX, offsetY);
		_candidateCommentRects[i] = m_layout->GetCandidateCommentRect(i);
		_candidateCommentRects[i].OffsetRect(offsetX, offsetY);
	}
	_statusIconRect = m_layout->GetStatusIconRect();
	_statusIconRect.OffsetRect(offsetX, offsetY);

	_contentSize.SetSize(workArea.Width(), workArea.Height());
}

void weasel::FullScreenLayout::DoLayout(CDCHandle dc, DirectWriteResources* pDWR)
{
	if (_context.empty())
	{
		int width = 0, height = 0;
		UpdateStatusIconLayout(&width, &height);
		_contentSize.SetSize(width, height);
		return;
	}

	CRect workArea;
	HMONITOR hMonitor = MonitorFromRect(mr_inputPos, MONITOR_DEFAULTTONEAREST);
	if (hMonitor)
	{
		MONITORINFO info;
		info.cbSize = sizeof(MONITORINFO);
		if (GetMonitorInfo(hMonitor, &info))
		{
			workArea = info.rcWork;
		}
	}

	int fontPoint = _style.font_point;
	int step = 32;
	do {
		m_layout->DoLayout(dc, pDWR);
	}
	while (AdjustFontPoint(dc, workArea, pDWR, step));

	if (fontPoint < 4) fontPoint = 4;
	else if (fontPoint > 2048) fontPoint = 2048;
	const_cast<UIStyle*>(&_style)->font_point = fontPoint;

	int offsetX = (workArea.Width() - m_layout->GetContentSize().cx) / 2;
	int offsetY = (workArea.Height() - m_layout->GetContentSize().cy) / 2;
	_preeditRect = m_layout->GetPreeditRect();
	_preeditRect.OffsetRect(offsetX, offsetY);
	_auxiliaryRect = m_layout->GetAuxiliaryRect();
	_auxiliaryRect.OffsetRect(offsetX, offsetY);
	_highlightRect = m_layout->GetHighlightRect();
	_highlightRect.OffsetRect(offsetX, offsetY);
	for (int i = 0, n = (int)_context.cinfo.candies.size(); i < n && i < MAX_CANDIDATES_COUNT; ++i)
	{
		_candidateLabelRects[i] = m_layout->GetCandidateLabelRect(i);
		_candidateLabelRects[i].OffsetRect(offsetX, offsetY);
		_candidateTextRects[i] = m_layout->GetCandidateTextRect(i);
		_candidateTextRects[i].OffsetRect(offsetX, offsetY);
		_candidateCommentRects[i] = m_layout->GetCandidateCommentRect(i);
		_candidateCommentRects[i].OffsetRect(offsetX, offsetY);
	}
	_statusIconRect = m_layout->GetStatusIconRect();
	_statusIconRect.OffsetRect(offsetX, offsetY);

	_contentSize.SetSize(workArea.Width(), workArea.Height());
}

bool FullScreenLayout::AdjustFontPoint(CDCHandle dc, const CRect& workArea, DirectWriteResources* pDWR, int& step)
{
	if (_context.empty() || step == 0)
		return false;
	int fontPointLabel		= pDWR->pLabelTextFormat->GetFontSize();
	int fontPoint			= pDWR->pTextFormat->GetFontSize();
	int fontPointComment	= pDWR->pCommentTextFormat->GetFontSize();
	CSize sz = m_layout->GetContentSize();
	if (sz.cx > workArea.Width() || sz.cy > workArea.Height())
	{
		if (step > 0)
		{
			step = - (step >> 1);
		}
		fontPoint += step* pDWR->dpiScaleX_ / 72.0f;
		fontPointLabel += step* pDWR->dpiScaleX_ / 72.0f;
		fontPointComment += step* pDWR->dpiScaleX_ / 72.0f;
		pDWR->pDWFactory->CreateTextFormat(_style.font_face.c_str(), NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 
				fontPoint, L"", &pDWR->pTextFormat);
		pDWR->pDWFactory->CreateTextFormat(_style.label_font_face.c_str(), NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 
				fontPointLabel, L"", &pDWR->pLabelTextFormat);
		pDWR->pDWFactory->CreateTextFormat(_style.comment_font_face.c_str(), NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 
				fontPointComment, L"", &pDWR->pCommentTextFormat);
		pDWR->pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
		pDWR->pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
		pDWR->pTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
		pDWR->pLabelTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
		pDWR->pLabelTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
		pDWR->pLabelTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
		pDWR->pCommentTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
		pDWR->pCommentTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
		pDWR->pCommentTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
		return true;
	}
	else if (sz.cx <= workArea.Width() * 31 / 32 && sz.cy <= workArea.Height() * 31 / 32)
	{
		if (step < 0)
		{
			step = -step >> 1;
		}
		fontPoint += step* pDWR->dpiScaleX_ / 72.0f;
		fontPointLabel += step* pDWR->dpiScaleX_ / 72.0f;
		fontPointComment += step* pDWR->dpiScaleX_ / 72.0f;
		pDWR->pDWFactory->CreateTextFormat(_style.font_face.c_str(), NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 
				fontPoint, L"", &pDWR->pTextFormat);
		pDWR->pDWFactory->CreateTextFormat(_style.label_font_face.c_str(), NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 
				fontPointLabel, L"", &pDWR->pLabelTextFormat);
		pDWR->pDWFactory->CreateTextFormat(_style.comment_font_face.c_str(), NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 
				fontPointComment, L"", &pDWR->pCommentTextFormat);
		pDWR->pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
		pDWR->pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
		pDWR->pTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
		pDWR->pLabelTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
		pDWR->pLabelTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
		pDWR->pLabelTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
		pDWR->pCommentTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
		pDWR->pCommentTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
		pDWR->pCommentTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
		return true;
	}

	return false;
}

bool weasel::FullScreenLayout::AdjustFontPoint(CDCHandle dc, const CRect& workArea, GDIFonts* pFonts, int& step)
{
	if (_context.empty() || step == 0)
		return false;

	CSize sz = m_layout->GetContentSize();
	if (sz.cx > workArea.Width() || sz.cy > workArea.Height())
	{
		if (step > 0)
		{
			step = - (step >> 1);
		}
		pFonts->_LabelFontPoint += step;
		pFonts->_TextFontPoint += step;
		pFonts->_CommentFontPoint += step;
		return true;
	}
	else if (sz.cx <= workArea.Width() * 31 / 32 && sz.cy <= workArea.Height() * 31 / 32)
	{
		if (step < 0)
		{
			step = -step >> 1;
		}
		pFonts->_LabelFontPoint += step;
		pFonts->_TextFontPoint += step;
		pFonts->_CommentFontPoint += step;
		return true;
	}

	return false;
}
