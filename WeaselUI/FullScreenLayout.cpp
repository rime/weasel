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

void FullScreenLayout::DoLayout(CDCHandle dc)
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
		long fontHeight = -MulDiv(fontPoint, dc.GetDeviceCaps(LOGPIXELSY), 72);
		CFont font;
		font.CreateFontW(fontHeight, 0, 0, 0, 0, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, _style.font_face.c_str());
		dc.SelectFont(font);

		m_layout->DoLayout(dc);
	}
	while (AdjustFontPoint(dc, workArea, fontPoint, step));

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
	for (int i = 0, n = (int)_context.cinfo.candies.size(); i < n; ++i)
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

bool FullScreenLayout::AdjustFontPoint(CDCHandle dc, const CRect& workArea, int& fontPoint, int& step)
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
		fontPoint += step;
		return true;
	}
	else if (sz.cx <= workArea.Width() * 31 / 32 && sz.cy <= workArea.Height() * 31 / 32)
	{
		if (step < 0)
		{
			step = -step >> 1;
		}
		fontPoint += step;
		return true;
	}

	return false;
}
