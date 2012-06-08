#include "stdafx.h"
#include "WeaselPanel.h"
#include <WeaselCommon.h>

// for IDI_ZHUNG, IDI_ALPHA
#include "../WeaselServer/resource.h"

using namespace weasel;

static LPCWSTR DEFAULT_FONT_FACE = L"";
static const int DEFAULT_FONT_POINT = 16;

static const int MIN_WIDTH = 160;
static const int MIN_HEIGHT = 0;
static const int BORDER = 3;
static const int MARGIN_X = 12;
static const int MARGIN_Y = 12;
static const int SPACING = 10;
static const int CAND_SPACING = 5;
static const int HIGHLIGHT_SPACING = 4;
static const int HIGHLIGHT_PADDING = 2;
static const int ROUND_CORNER = 4;

static const COLORREF TEXT_COLOR                  = 0x000000;
static const COLORREF CAND_TEXT_COLOR             = 0x000000;
static const COLORREF BACK_COLOR                  = 0xffffff;
static const COLORREF BORDER_COLOR                = 0x000000;
static const COLORREF HIGHLIGHTED_TEXT_COLOR      = 0x000000;
static const COLORREF HIGHLIGHTED_BACK_COLOR      = 0x7fffff;
static const COLORREF HIGHLIGHTED_CAND_TEXT_COLOR = 0xffffff;
static const COLORREF HIGHLIGHTED_CAND_BACK_COLOR = 0x000000;

static const int STATUS_ICON_SIZE = 16;

static WCHAR LABEL_PATTERN[] = L"%1%. ";

WeaselPanel::WeaselPanel(weasel::UI &ui)
	: m_ctx(ui.ctx()), m_status(ui.status()), m_style(ui.style())
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
	m_style.label_text_color = CAND_TEXT_COLOR;
	m_style.comment_text_color = CAND_TEXT_COLOR;
	m_style.back_color = BACK_COLOR;
	m_style.border_color = BORDER_COLOR;
	m_style.hilited_text_color = HIGHLIGHTED_TEXT_COLOR;
	m_style.hilited_back_color = HIGHLIGHTED_BACK_COLOR;
	m_style.hilited_candidate_text_color = HIGHLIGHTED_CAND_TEXT_COLOR;
	m_style.hilited_candidate_back_color = HIGHLIGHTED_CAND_BACK_COLOR;
	m_style.hilited_label_text_color = HIGHLIGHTED_CAND_TEXT_COLOR;
	m_style.hilited_comment_text_color = HIGHLIGHTED_CAND_TEXT_COLOR;

	m_iconZhung.LoadIconW(IDI_ZHUNG, STATUS_ICON_SIZE, STATUS_ICON_SIZE, LR_DEFAULTCOLOR);
	m_iconAlpha.LoadIconW(IDI_ALPHA, STATUS_ICON_SIZE, STATUS_ICON_SIZE, LR_DEFAULTCOLOR);
}

void WeaselPanel::_ResizeWindow()
{
	if (!m_status.composing)
	{
		SetWindowPos( NULL, 0, 0, STATUS_ICON_SIZE, STATUS_ICON_SIZE, SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOZORDER);
		return;
	}

	CDC dc = GetDC();
	long fontHeight = -MulDiv(m_style.font_point, dc.GetDeviceCaps(LOGPIXELSY), 72);

	CFont font;
	font.CreateFontW(fontHeight, 0, 0, 0, 0, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, m_style.font_face.c_str());
	CFontHandle oldFont = dc.SelectFont(font);

	long width = 0;
	long height = 0;
	CSize sz;

	// measure preedit string
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

	// ascii mode icon
	if (m_status.ascii_mode && width > 0)
	{
		width += m_style.spacing + STATUS_ICON_SIZE;
	}

	// measure aux string
	wstring const& aux = m_ctx.aux.str;
	if (!aux.empty())
	{
		dc.GetTextExtent(aux.c_str(), aux.length(), &sz);
		width = max(width, sz.cx);
		height += sz.cy + m_style.spacing;
	}

	// measure candidates
	vector<Text> const& candies(m_ctx.cinfo.candies);
	vector<Text> const& comments(m_ctx.cinfo.comments);
	std::string const& labels(m_ctx.cinfo.labels);
	long label_width = 0;
	long cand_width = 0;
	for (size_t i = 0; i < candies.size(); ++i, height += m_style.candidate_spacing)
	{
		wstring label_text;
		if (i < labels.size())
			label_text = (boost::wformat(LABEL_PATTERN) % labels[i]).str();
		else
			label_text = (boost::wformat(LABEL_PATTERN) % ((i + 1) % 10)).str();
		dc.GetTextExtent(label_text.c_str(), label_text.length(), &sz);
		label_width = max(label_width, sz.cx);

		wstring cand = candies[i].str;
		if (!comments[i].str.empty())
		{
			cand += L" " + comments[i].str;
		}
		dc.GetTextExtent(cand.c_str(), cand.length(), &sz);
		cand_width = max(cand_width, sz.cx);

		height += sz.cy;
	}
	width = max(width, label_width + cand_width + 2 * m_style.hilite_padding);
	if (!candies.empty())
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
	CSize sz;
	vector<Text> const& candies(cinfo.candies);
	vector<Text> const& comments(cinfo.comments);
	std::string const& labels(cinfo.labels);
	vector<wstring> label_text;
	long label_width = 0;
	long comment_shift_width = 0;
	long line_height = 0;
	for (size_t i = 0; i < candies.size(); ++i)
	{
		if (i < labels.size())
			label_text.push_back((boost::wformat(LABEL_PATTERN) % labels[i]).str());
		else
			label_text.push_back((boost::wformat(LABEL_PATTERN) % ((i + 1) % 10)).str());
		dc.GetTextExtent(label_text.back().c_str(), label_text.back().length(), &sz);
		label_width = max(label_width, sz.cx);
		line_height = max(line_height, sz.cy);
		wstring cand_text = candies[i].str;
		wstring comment_text;
		dc.GetTextExtent(cand_text.c_str(), cand_text.length(), &sz);
		long cand_width = sz.cx;
		line_height = max(line_height, sz.cy);
		if (!comments[i].str.empty())
		{
			comment_text = L" " + comments[i].str;
			comment_shift_width = max(comment_shift_width, cand_width);
		}
		dc.GetTextExtent(comment_text.c_str(), comment_text.length(), &sz);
		line_height = max(line_height, sz.cy);
	}
	for (size_t i = 0; i < candies.size(); ++i, y += m_style.candidate_spacing)
	{
		if (y >= rc.bottom)
			break;
		wstring cand_text = candies[i].str;
		wstring comment_text;
		if (!comments[i].str.empty())
		{
			comment_text = L" " + comments[i].str;
		}
		CRect rc_out(rc.left + m_style.hilite_padding, y, rc.right - m_style.hilite_padding, y + line_height);
		if (i == cinfo.highlighted)
		{
			_HighlightText(dc, rc_out, m_style.hilited_candidate_back_color);
			dc.SetTextColor(m_style.hilited_label_text_color);
		}
		else
		{
			dc.SetTextColor(m_style.label_text_color);
		}
		// draw label
		dc.ExtTextOutW(rc_out.left, y, ETO_CLIPPED, &rc_out, label_text[i].c_str(), label_text[i].length(), 0);
		rc_out.DeflateRect(label_width, 0, 0, 0);
		// draw candidate text
		if (i == cinfo.highlighted)
		{
			dc.SetTextColor(m_style.hilited_candidate_text_color);
			dc.ExtTextOutW(rc_out.left, y, ETO_CLIPPED, &rc_out, cand_text.c_str(), cand_text.length(), 0);
			dc.SetTextColor(m_style.hilited_comment_text_color);
		}
		else
		{
			dc.SetTextColor(m_style.candidate_text_color);
			dc.ExtTextOutW(rc_out.left, y, ETO_CLIPPED, &rc_out, cand_text.c_str(), cand_text.length(), 0);
			dc.SetTextColor(m_style.comment_text_color);
		}
		// draw comment text
		if (!comment_text.empty())
		{
			rc_out.DeflateRect(comment_shift_width, 0, 0, 0);
			dc.ExtTextOutW(rc_out.left, y, ETO_CLIPPED, &rc_out, comment_text.c_str(), comment_text.length(), 0);
		}
		y += line_height;
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

	if (!m_status.composing)
	{
		if (m_status.ascii_mode)
			dc.DrawIconEx(0, 0, m_iconAlpha, 0, 0);
		else
			dc.DrawIconEx(0, 0, m_iconZhung, 0, 0); 
		return;
	}

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

	// ascii mode icon
	if (m_status.ascii_mode && y > rc.top)
	{
		int icon_x = rc.right - STATUS_ICON_SIZE;
		int icon_y = (rc.top + y - m_style.spacing - STATUS_ICON_SIZE) / 2;
		dc.DrawIconEx(icon_x, icon_y, m_iconAlpha, 0, 0);
	}

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
