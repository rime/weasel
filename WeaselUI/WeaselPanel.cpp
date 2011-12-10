#include "stdafx.h"
#include "WeaselPanel.h"
#include <WeaselCommon.h>

using namespace weasel;

WeaselPanel::WeaselPanel()
{
	m_style.font_face = DEFAULT_FONT_FACE;
	m_style.font_point = DEFAULT_FONT_POINT;
	m_style.min_width = MIN_WIDTH;
	m_style.min_height = MIN_HEIGHT;
	m_style.border = BORDER;
	m_style.margin_x = MARGIN_X;
	m_style.margin_y = MARGIN_Y;
	m_style.spacing = SPACING;
	m_style.candidate_spacing = CAND_SPACING;
	m_style.hilite_spacing = HIGHLIGHT_SPACING;
	m_style.hilite_padding = HIGHLIGHT_PADDING;
	m_style.round_corner = ROUND_CORNER;

	m_style.text_color = TEXT_COLOR;
	m_style.candidate_text_color = CAND_TEXT_COLOR;
	m_style.back_color = BACK_COLOR;
	m_style.border_color = BORDER_COLOR;
	m_style.hilited_text_color = HIGHLIGHTED_TEXT_COLOR;
	m_style.hilited_back_color = HIGHLIGHTED_BACK_COLOR;
	m_style.hilited_candidate_text_color = HIGHLIGHTED_CAND_TEXT_COLOR;
	m_style.hilited_candidate_back_color = HIGHLIGHTED_CAND_BACK_COLOR;
}

void WeaselPanel::SetContext(const weasel::Context &ctx)
{
	m_ctx = ctx;
	Refresh();
}

void WeaselPanel::SetStatus(const weasel::Status &status)
{
	m_status = status;
	Refresh();
}

void WeaselPanel::_ResizeWindow()
{
	CDC dc = GetDC();
	long fontHeight = -MulDiv(m_style.font_point, dc.GetDeviceCaps(LOGPIXELSY), 72);

	CFont font;
	font.CreateFontW(fontHeight, 0, 0, 0, 0, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, m_style.font_face.c_str());
	CFontHandle oldFont = dc.SelectFont(font);

	long width = 0;
	long height = 0;
	CSize sz;

	// draw preedit string
	wstring const& preedit = m_ctx.preedit.str;
	vector<weasel::TextAttribute> const& attrs = m_ctx.preedit.attributes;
	if (!preedit.empty())
	{
		dc.GetTextExtent(preedit.c_str(), preedit.length(), &sz);
		for (size_t j = 0; j < attrs.size(); ++j)
		{
			if (attrs[j].type == weasel::HIGHLIGHTED)
			{
				const weasel::TextRange &range = attrs[j].range;
				if (range.start < range.end)
				{
					if (range.start > 0)
						sz.cx += m_style.hilite_spacing;
					else
						sz.cx += m_style.hilite_padding;
					if (range.end < static_cast<int>(preedit.length()))
						sz.cx += m_style.hilite_spacing;
					else
						sz.cx += m_style.hilite_padding;
				}
			}
		}
		width = max(width, sz.cx);
		height += sz.cy + m_style.spacing;
	}

	// draw aux string
	wstring const& aux = m_ctx.aux.str;
	if (!aux.empty())
	{
		dc.GetTextExtent(aux.c_str(), aux.length(), &sz);
		width = max(width, sz.cx);
		height += sz.cy + m_style.spacing;
	}

	// draw candidates
	vector<Text> const& candidates = m_ctx.cinfo.candies;
	for (size_t i = 0; i < candidates.size(); ++i, height += m_style.candidate_spacing)
	{
		wstring cand = (boost::wformat(CANDIDATE_PROMPT_PATTERN) % (i + 1) % candidates[i].str).str();
		dc.GetTextExtent(cand.c_str(), cand.length(), &sz);
		width = max(width, sz.cx);
		height += sz.cy;
	}
	if (!candidates.empty())
		height += m_style.spacing;

	//trim the last spacing
	if (height > 0)
		height -= m_style.spacing;

	width += 2 * m_style.margin_x;
	height += 2 * m_style.margin_y;

	width = max(width, m_style.min_width);
	height = max(height, m_style.min_height);
	
	SetWindowPos( NULL, 0, 0, width, height, SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOZORDER);
}

//更新界面
void WeaselPanel::Refresh()
{
	_ResizeWindow();
	_RepositionWindow();
	RedrawWindow();
}

void WeaselPanel::_HighlightText(CDCHandle dc, CRect rc, COLORREF color)
{
	rc.InflateRect(m_style.hilite_padding, m_style.hilite_padding);
	CBrush brush;
	brush.CreateSolidBrush(color);
	CBrush oldBrush = dc.SelectBrush(brush);
	CPen pen;
	pen.CreatePen(PS_SOLID, 0, color);
	CPen oldPen = dc.SelectPen(pen);
	CPoint ptRoundCorner(m_style.round_corner, m_style.round_corner);
	dc.RoundRect(rc, ptRoundCorner);
	dc.SelectBrush(oldBrush);
	dc.SelectPen(oldPen);
}

bool WeaselPanel::_DrawText(Text const& text, CDCHandle dc, CRect const& rc, int& y)
{
	bool drawn = false;
	wstring const& t = text.str;
	if (!t.empty())
	{
		CSize szText;
		dc.GetTextExtent(t.c_str(), t.length(), &szText);
		weasel::TextRange range;
		vector<weasel::TextAttribute> const& attrs = text.attributes;
		for (size_t j = 0; j < attrs.size(); ++j)
		{
			if (attrs[j].type == weasel::HIGHLIGHTED)
			{
				range = attrs[j].range;
			}
		}
		if (range.start < range.end)
		{
			CSize selStart;
			dc.GetTextExtent(t.c_str(), range.start, &selStart);
			CSize selEnd;
			dc.GetTextExtent(t.c_str(), range.end, &selEnd);
			int x = rc.left;
			if (range.start > 0)
			{
				// zzz
				std::wstring str_before(t.substr(0, range.start));
				CRect rc_before(x, y, x + selStart.cx, y + szText.cy);
				dc.ExtTextOutW(x, y, ETO_CLIPPED | ETO_OPAQUE, &rc_before, str_before.c_str(), str_before.length(), 0);
				x += selStart.cx + m_style.hilite_spacing;
			}
			else
			{
				x += m_style.hilite_padding;
			}
			{
				// zzz[yyy]
				std::wstring str_hi(t.substr(range.start, range.end - range.start));
				CRect rc_hi(x, y, x + (selEnd.cx - selStart.cx), y + szText.cy);
				_HighlightText(dc, rc_hi, m_style.hilited_back_color);
				dc.SetTextColor(m_style.hilited_text_color);
				dc.SetBkColor(m_style.hilited_back_color);
				dc.ExtTextOutW(x, y, ETO_CLIPPED, &rc_hi, str_hi.c_str(), str_hi.length(), 0);
				dc.SetTextColor(m_style.text_color);
				dc.SetBkColor(m_style.back_color);
				x += (selEnd.cx - selStart.cx);
			}
			if (range.end < static_cast<int>(t.length()))
			{
				// zzz[yyy]xxx
				x += m_style.hilite_spacing;
				std::wstring str_after(t.substr(range.end));
				CRect rc_after(x, y, x + (szText.cx - selEnd.cx), y + szText.cy);
				dc.ExtTextOutW(x, y, ETO_CLIPPED | ETO_OPAQUE, &rc_after, str_after.c_str(), str_after.length(), 0);
				x += (szText.cx - selEnd.cx);
			}
			else
			{
				x += m_style.hilite_padding;
			}
			// done
			y += szText.cy;
		}
		else
		{
			CRect rcText(rc.left, y, rc.right, y + szText.cy);
			dc.ExtTextOutW(rc.left, y, ETO_CLIPPED | ETO_OPAQUE, &rcText, t.c_str(), t.length(), 0);
			y += szText.cy;
		}
		drawn = true;
	}
	return drawn;
}

bool WeaselPanel::_DrawCandidates(CandidateInfo const& cinfo, CDCHandle dc, CRect const& rc, int& y)
{
	bool drawn = false;
	dc.SetTextColor(m_style.candidate_text_color);
	vector<Text> const& candies = cinfo.candies;
	for (size_t i = 0; i < candies.size(); ++i, y += m_style.candidate_spacing)
	{
		if (y >= rc.bottom)
			break;
		wstring t = (boost::wformat(CANDIDATE_PROMPT_PATTERN) % (i + 1) % candies[i].str).str();
		CSize szText;
		dc.GetTextExtent(t.c_str(), t.length(), &szText);
		CRect rcText(rc.left + m_style.hilite_padding, y, rc.right - m_style.hilite_padding, y + szText.cy);
		if (i == cinfo.highlighted)
		{
			_HighlightText(dc, rcText, m_style.hilited_candidate_back_color);
			dc.SetTextColor(m_style.hilited_candidate_text_color);
			dc.ExtTextOutW(rcText.left, y, ETO_CLIPPED, &rcText, t.c_str(), t.length(), 0);
			dc.SetTextColor(m_style.candidate_text_color);
		}
		else
		{
			dc.ExtTextOutW(rcText.left, y, ETO_CLIPPED, &rcText, t.c_str(), t.length(), 0);
		}
		y += szText.cy;
		drawn = true;
	}
	dc.SetTextColor(m_style.text_color);
	return drawn;
}

//draw client area
void WeaselPanel::DoPaint(CDCHandle dc)
{
	CRect rc;
	GetClientRect(&rc);

	// background
	{
		CBrush brush;
		brush.CreateSolidBrush(m_style.back_color);
		CRgn rgn;
		rgn.CreateRectRgnIndirect(&rc);
		dc.FillRgn(rgn, brush);

		CPen pen;
		pen.CreatePen(PS_SOLID | PS_INSIDEFRAME, m_style.border, m_style.border_color);
		CPenHandle oldPen = dc.SelectPen(pen);
		CBrushHandle oldBrush = dc.SelectBrush(brush);
		dc.Rectangle(&rc);
		dc.SelectPen(oldPen);
		dc.SelectBrush(oldBrush);
	}

	long height = -MulDiv(m_style.font_point, dc.GetDeviceCaps(LOGPIXELSY), 72);

	CFont font;
	font.CreateFontW(height, 0, 0, 0, 0, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, m_style.font_face.c_str());
	CFontHandle oldFont = dc.SelectFont(font);

	dc.SetTextColor(m_style.text_color);
	dc.SetBkColor(m_style.back_color);
	dc.SetBkMode(TRANSPARENT);

	rc.DeflateRect(m_style.margin_x, m_style.margin_y);

	int y = rc.top;

	// draw preedit string
	if (_DrawText(m_ctx.preedit, dc, rc, y))
		y += m_style.spacing;

	// draw aux string
	if (_DrawText(m_ctx.aux, dc, rc, y))
		y += m_style.spacing;

	// draw candidates
	if (_DrawCandidates(m_ctx.cinfo, dc, rc, y))
		y += m_style.spacing;

	// TODO: draw other parts

	if (y > rc.top)
		y -= m_style.spacing;

	dc.SelectFont(oldFont);	
}

LRESULT WeaselPanel::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	Refresh();
	//CenterWindow();
	GetWindowRect(&m_inputPos);
	return TRUE;
}

LRESULT WeaselPanel::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	return 0;
}

void WeaselPanel::CloseDialog(int nVal)
{
	
}

void WeaselPanel::MoveTo(RECT const& rc)
{
	const int distance = 6;
	m_inputPos = rc;
	m_inputPos.OffsetRect(0, distance);
	_RepositionWindow();
}

void WeaselPanel::_RepositionWindow()
{
	RECT rcWorkArea;
	//SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWorkArea, 0);
	memset(&rcWorkArea, 0, sizeof(rcWorkArea));
	HMONITOR hMonitor = MonitorFromRect(m_inputPos, MONITOR_DEFAULTTONEAREST);
	if (hMonitor)
	{
		MONITORINFO info;
		info.cbSize = sizeof(MONITORINFO);
		if (GetMonitorInfo(hMonitor, &info))
		{
			rcWorkArea = info.rcWork;
		}
	}
	RECT rcWindow;
	GetWindowRect(&rcWindow);
	int width = (rcWindow.right - rcWindow.left);
	int height = (rcWindow.bottom - rcWindow.top);
	// keep panel visible
	rcWorkArea.right -= width;
	rcWorkArea.bottom -= height;
	int x = m_inputPos.left;
	int y = m_inputPos.bottom;
	if (x > rcWorkArea.right)
		x = rcWorkArea.right;
	if (x < rcWorkArea.left)
		x = rcWorkArea.left;
	// show panel above the input focus if we're around the bottom
	if (y > rcWorkArea.bottom)
		y = m_inputPos.top - height;
	if (y > rcWorkArea.bottom)
		y = rcWorkArea.bottom;
	if (y < rcWorkArea.top)
		y = rcWorkArea.top;
	// memorize adjusted position (to avoid window bouncing on height change)
	m_inputPos.bottom = y;
	SetWindowPos(HWND_TOPMOST, x, y, 0, 0, SWP_NOSIZE|SWP_NOACTIVATE);
}
