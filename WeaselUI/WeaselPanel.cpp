﻿#include "stdafx.h"
#include "WeaselPanel.h"
#include <WeaselCommon.h>
#include <ShellScalingApi.h>
#include "VersionHelpers.hpp"
#include "VerticalLayout.h"
#include "HorizontalLayout.h"
#include "FullScreenLayout.h"
#include "VHorizontalLayout.h"

// for IDI_ZH, IDI_EN
#include <resource.h>
#define COLORTRANSPARENT(color)		((color & 0xff000000) == 0)
#define COLORNOTTRANSPARENT(color)	((color & 0xff000000) != 0)
#define TRANS_COLOR		0x00000000
#define GDPCOLOR_FROM_COLORREF(color)	Gdiplus::Color::MakeARGB(((color >> 24) & 0xff), GetRValue(color), GetGValue(color), GetBValue(color))

#pragma comment(lib, "Shcore.lib")

template <class t0, class t1, class t2>
inline void LoadIconNecessary(t0& a, t1& b, t2& c, int d) {
	if(a == b) return;
	a = b;
	if (b.empty())	c.LoadIconW(d, STATUS_ICON_SIZE, STATUS_ICON_SIZE, LR_DEFAULTCOLOR);
	else			c = (HICON)LoadImage(NULL, b.c_str(), IMAGE_ICON, STATUS_ICON_SIZE, STATUS_ICON_SIZE, LR_LOADFROMFILE);
}

WeaselPanel::WeaselPanel(weasel::UI& ui)
	: m_layout(NULL),
	m_ctx(ui.ctx()),
	m_status(ui.status()),
	m_style(ui.style()),
	m_ostyle(ui.ostyle()),
	m_candidateCount(0),
	m_current_zhung_icon(),
	dpi(96),
	hide_candidates(false),
	pDWR(NULL),
	m_blurer(new GdiplusBlur()),
	pBrush(NULL),
	_m_gdiplusToken(0)
{
	m_iconDisabled.LoadIconW(IDI_RELOAD, STATUS_ICON_SIZE, STATUS_ICON_SIZE, LR_DEFAULTCOLOR);
	m_iconEnabled.LoadIconW(IDI_ZH, STATUS_ICON_SIZE, STATUS_ICON_SIZE, LR_DEFAULTCOLOR);
	m_iconAlpha.LoadIconW(IDI_EN, STATUS_ICON_SIZE, STATUS_ICON_SIZE, LR_DEFAULTCOLOR);
	m_iconFull.LoadIconW(IDI_FULL_SHAPE, STATUS_ICON_SIZE, STATUS_ICON_SIZE, LR_DEFAULTCOLOR);
	m_iconHalf.LoadIconW(IDI_HALF_SHAPE, STATUS_ICON_SIZE, STATUS_ICON_SIZE, LR_DEFAULTCOLOR);
	// for gdi+ drawings, initialization
	GdiplusStartup(&_m_gdiplusToken, &_m_gdiplusStartupInput, NULL);

	_InitFontRes();
	m_ostyle = m_style;
}

WeaselPanel::~WeaselPanel()
{
	Gdiplus::GdiplusShutdown(_m_gdiplusToken);
	CleanUp();
}

void WeaselPanel::_ResizeWindow()
{
	CDCHandle dc = GetDC();
	CSize size = m_layout->GetContentSize();
	// SetWindowPos with size info only if size changed
	if(size != m_osize) {
		SetWindowPos(NULL, 0, 0, size.cx, size.cy, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);
		m_osize = size;
	}
	ReleaseDC(dc);
}

void WeaselPanel::_CreateLayout()
{
	if (m_layout != NULL)
		delete m_layout;

	Layout* layout = NULL;
	if (m_style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT)
	{
		layout = new VHorizontalLayout(m_style, m_ctx, m_status);
	}
	else
	{
		if (m_style.layout_type == UIStyle::LAYOUT_VERTICAL || m_style.layout_type == UIStyle::LAYOUT_VERTICAL_FULLSCREEN)
		{
			layout = new VerticalLayout(m_style, m_ctx, m_status);
		}
		else if (m_style.layout_type == UIStyle::LAYOUT_HORIZONTAL || m_style.layout_type == UIStyle::LAYOUT_HORIZONTAL_FULLSCREEN)
		{
			layout = new HorizontalLayout(m_style, m_ctx, m_status);
		}

		if (IS_FULLSCREENLAYOUT(m_style))
		{
			layout = new FullScreenLayout(m_style, m_ctx, m_status, m_inputPos, layout);
		}
	}
	m_layout = layout;
}

//更新界面
void WeaselPanel::Refresh()
{
	bool should_show_icon = (m_status.ascii_mode || !m_status.composing || !m_ctx.aux.empty());
	m_candidateCount = (BYTE)m_ctx.cinfo.candies.size();
	// check if to hide candidates window
	// show tips status, two kind of situation: 1) only aux strings, don't care icon status; 2)only icon(ascii mode switching)
	bool show_tips = (!m_ctx.aux.empty() && m_ctx.cinfo.empty() && m_ctx.preedit.empty()) || (m_ctx.empty() && should_show_icon);
	// show schema menu status: always preedit start with "〔方案選單〕"
	bool show_schema_menu = std::regex_search(m_ctx.preedit.str, std::wsmatch(), std::wregex(L"^〔方案選單〕", std::wregex::icase));
	bool margin_negative = (m_style.margin_x < 0 || m_style.margin_y < 0);
	bool inline_no_candidates = m_style.inline_preedit && (m_ctx.cinfo.candies.size() == 0) && (!show_tips);
	// when to hide_cadidates?
	// 1. inline_no_candidates
	// or
	// 2. margin_negative, and not in show tips mode( ascii switching / half-full switching / simp-trad switching / error tips), and not in schema menu
	hide_candidates = inline_no_candidates || (margin_negative && !show_tips && !show_schema_menu);

	// only RedrawWindow if no need to hide candidates window
	if(!hide_candidates)
	{ 
		_InitFontRes();
		_CreateLayout();

		CDCHandle dc = GetDC();
		m_layout->DoLayout(dc, pDWR);
		ReleaseDC(dc);
		_ResizeWindow();
		_RepositionWindow();
		RedrawWindow();
	}
}

void WeaselPanel::_InitFontRes(void)
{
	HMONITOR hMonitor = MonitorFromRect(m_inputPos, MONITOR_DEFAULTTONEAREST);
	UINT dpiX = 0, dpiY = 0;
	if (hMonitor)
		GetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);
	// prepare d2d1 resources
	if (pDWR == NULL)
		pDWR = new DirectWriteResources(m_style, dpiX);
	// if style changed, re-initialize font resources
	else if (m_ostyle != m_style)
		pDWR->InitResources(m_style, dpiX);
	else if( dpiX != dpi)
	{
		pDWR->InitResources(m_style, dpiX);
	}
	// create color brush if null
	if (pBrush == NULL)
		pDWR->pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0, 1.0, 1.0, 1.0), &pBrush);
	m_ostyle = m_style;
	dpi = dpiX;
}

void WeaselPanel::CleanUp()
{
	delete m_layout;
	m_layout = NULL;

	delete pDWR;
	pDWR = NULL;

	delete m_blurer;
	m_blurer = NULL;

	SafeRelease(&pBrush);
	pBrush = NULL;
}

#ifdef USE_MOUSE_EVENTS
static HBITMAP CopyDCToBitmap(HDC hDC, LPRECT lpRect)
{
	if (!hDC || !lpRect || IsRectEmpty(lpRect)) return NULL;
	HDC hMemDC;
	HBITMAP hBitmap, hOldBitmap;
	int nX, nY, nX2, nY2;
	int nWidth, nHeight;

	nX = lpRect->left;
	nY = lpRect->top;
	nX2 = lpRect->right;
	nY2 = lpRect->bottom;
	nWidth = nX2 - nX;
	nHeight = nY2 - nY;

	hMemDC = CreateCompatibleDC(hDC);
	hBitmap = CreateCompatibleBitmap(hDC, nWidth, nHeight);
	hOldBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);
	StretchBlt(hMemDC, 0, 0, nWidth, nHeight, hDC, nX, nY, nWidth, nHeight, SRCCOPY);
	hBitmap = (HBITMAP)SelectObject(hMemDC, hOldBitmap);

	DeleteDC(hMemDC);
	DeleteObject(hOldBitmap);
	return hBitmap;
 }

void WeaselPanel::_CaptureRect(CRect& rect)
{
	HDC ScreenDC = ::GetDC(NULL);
	CRect rc;
	GetWindowRect(&rc);
	POINT WindowPosAtScreen = { rc.left, rc.top };
	rect.OffsetRect(WindowPosAtScreen);
	// capture input window
	if (OpenClipboard()) {
		HBITMAP bmp = CopyDCToBitmap(ScreenDC, LPRECT(rect));
		EmptyClipboard();
		SetClipboardData(CF_BITMAP, bmp);
		CloseClipboard();
		DeleteObject(bmp);
	}
	ReleaseDC(ScreenDC);
}

LRESULT WeaselPanel::OnMouseActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	bHandled = true;
	return MA_NOACTIVATE;
}

// simulating a key down and up
static void SendInputKey(WORD key)
{
	INPUT inputs[2];
	inputs[0].type = INPUT_KEYBOARD;
	inputs[0].ki = {key, 0,0,0,0};
	inputs[1].type = INPUT_KEYBOARD;
	inputs[1].ki = {key, 0,KEYEVENTF_KEYUP,0,0};
	::SendInput(sizeof(inputs) / sizeof(INPUT), inputs, sizeof(INPUT));
}

LRESULT WeaselPanel::OnMouseWheel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	int delta = GET_WHEEL_DELTA_WPARAM(wParam);
	if(delta > 0) SendInputKey(33);
	else	   SendInputKey(34);
	bHandled = true;
	return 0;
}

LRESULT WeaselPanel::OnLeftClicked(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if(hide_candidates)
	{
		bHandled = true;
		return 0;
	}
	CPoint point;
	point.x = GET_X_LPARAM(lParam);
	point.y = GET_Y_LPARAM(lParam);

	// capture
	{
		CRect recth = m_layout->GetCandidateRect((int)m_ctx.cinfo.highlighted);
		if(m_istorepos)	recth.OffsetRect(0, m_offsetys[m_ctx.cinfo.highlighted]);
		recth.InflateRect(m_style.hilite_padding, m_style.hilite_padding);
		// capture widow
		if (recth.PtInRect(point)) _CaptureRect(recth);
		else _CaptureRect(rcw);
	}
	// button response
	{
		if(!m_style.inline_preedit && m_candidateCount != 0 && COLORNOTTRANSPARENT(m_style.prevpage_color) && COLORNOTTRANSPARENT(m_style.nextpage_color)) {
			// click prepage
			if(m_ctx.cinfo.currentPage != 0 ) {
				CRect prc = m_layout->GetPrepageRect();
				if(m_istorepos)	prc.OffsetRect(0, m_offsety_preedit);
				if(prc.PtInRect(point)) {
					// to do send pgup
					SendInputKey(33);
					bHandled = true;
					return 0;
				}
			}
			// click nextpage
			if(!m_ctx.cinfo.is_last_page) {
				CRect prc = m_layout->GetNextpageRect();
				if(m_istorepos)	prc.OffsetRect(0, m_offsety_preedit);
				if(prc.PtInRect(point)) {
					// to do send pgdn
					SendInputKey(34);
					bHandled = true;
					return 0;
				}
			}
		}
		// select by click
		for (auto i = 0; i < m_candidateCount && i < MAX_CANDIDATES_COUNT; ++i) {
			CRect rect = m_layout->GetCandidateRect((int)i);
			if(m_istorepos)	rect.OffsetRect(0, m_offsetys[i]);
			rect.InflateRect(m_style.hilite_padding, m_style.hilite_padding);
			if (rect.PtInRect(point))
			{
				// if not select by number, to be test
				if(i < MAX_CANDIDATES_COUNT - 1) SendInputKey(0x31 + i);
				else	SendInputKey(0x30);
				break;
			}
		}
	}
	bHandled = true;
	return 0;
}

#ifdef USE_MOUSE_HOVER
LRESULT WeaselPanel::OnMouseHover(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	CPoint point;
	point.x = GET_X_LPARAM(lParam);
	point.y = GET_Y_LPARAM(lParam);

	for (size_t i = 0; i < m_candidateCount && i < MAX_CANDIDATES_COUNT; ++i) {
		CRect rect = m_layout->GetCandidateRect((int)i);
		if (rect.PtInRect(point))
		{
			m_ctx.cinfo.highlighted = i;
			Refresh();
		}
	}
	bHandled = true;
	return 0;
}

LRESULT WeaselPanel::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if (m_mouse_entry == false)
	{
		TRACKMOUSEEVENT tme;
		tme.cbSize = sizeof(TRACKMOUSEEVENT);
		tme.dwFlags = TME_HOVER | TME_LEAVE;
		tme.dwHoverTime = 400; // 400 ms 
		tme.hwndTrack = m_hWnd;
		TrackMouseEvent(&tme);
	}
	return 0;
}

LRESULT WeaselPanel::OnMouseLeave(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	m_mouse_entry = false;
	Refresh();
	return 0;
}
#endif /*  USE_MOUSE_HOVER */
#endif /* USE_MOUSE_EVENTS */

void WeaselPanel::_HighlightText(CDCHandle &dc, CRect rc, COLORREF color, COLORREF shadowColor, int radius, BackType type = BackType::TEXT, IsToRoundStruct rd = IsToRoundStruct(), COLORREF bordercolor=TRANS_COLOR)
{
	// Graphics obj with SmoothingMode
	Gdiplus::Graphics g_back(dc);
	g_back.SetSmoothingMode(Gdiplus::SmoothingMode::SmoothingModeHighQuality);

	// blur buffer
	int blurMarginX = m_layout->offsetX * 3;
	int blurMarginY = m_layout->offsetY * 3;

	GraphicsRoundRectPath* hiliteBackPath;
	if (rd.Hemispherical && type!= BackType::BACKGROUND && NOT_FULLSCREENLAYOUT(m_style)) 
		hiliteBackPath = new GraphicsRoundRectPath(rc, m_style.round_corner_ex - m_style.border/2 + (m_style.border % 2), rd.IsTopLeftNeedToRound, rd.IsTopRightNeedToRound, rd.IsBottomRightNeedToRound, rd.IsBottomLeftNeedToRound);
	else // background or current candidate background not out of window background
		hiliteBackPath = new GraphicsRoundRectPath(rc, radius);

	// 必须shadow_color都是非完全透明色才做绘制, 全屏状态不绘制阴影保证响应速度
	if ( m_style.shadow_radius && COLORNOTTRANSPARENT(shadowColor) && NOT_FULLSCREENLAYOUT(m_style) ) {
		CRect rect(
			blurMarginX + m_style.shadow_offset_x,
			blurMarginY + m_style.shadow_offset_y,
			rc.Width()  + blurMarginX + m_style.shadow_offset_x,
			rc.Height() + blurMarginY + m_style.shadow_offset_y);
		BYTE r = GetRValue(shadowColor);
		BYTE g = GetGValue(shadowColor);
		BYTE b = GetBValue(shadowColor);
		BYTE alpha = (BYTE)((shadowColor >> 24) & 255);
		Gdiplus::Color shadow_color = Gdiplus::Color::MakeARGB(alpha, r, g, b);
		static Gdiplus::Bitmap* pBitmapDropShadow;
		pBitmapDropShadow = new Gdiplus::Bitmap((INT)rc.Width() + blurMarginX * 2, (INT)rc.Height() + blurMarginY * 2, PixelFormat32bppARGB);

		Gdiplus::Graphics g_shadow(pBitmapDropShadow);
		g_shadow.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
		// dropshadow, draw a roundrectangle to blur
		if (m_style.shadow_offset_x != 0 || m_style.shadow_offset_y != 0) {
			GraphicsRoundRectPath shadow_path(rect, radius);
			Gdiplus::SolidBrush shadow_brush(shadow_color);
			g_shadow.FillPath(&shadow_brush, &shadow_path);
		}
		// round shadow, draw multilines as base round line
		else {
			int step = alpha /  m_style.shadow_radius / 2;
			Gdiplus::Pen pen_shadow(shadow_color, (Gdiplus::REAL)1);
			for (int i = 0; i < m_style.shadow_radius; i++) {
				GraphicsRoundRectPath round_path(rect, radius + 1 + i);
				g_shadow.DrawPath(&pen_shadow, &round_path);
				shadow_color = Gdiplus::Color::MakeARGB(alpha - i * step, r, g, b);
				pen_shadow.SetColor(shadow_color);
				rect.InflateRect(1, 1);
			}
		}
		m_blurer->DoGaussianBlur(pBitmapDropShadow, (float)m_style.shadow_radius, (float)m_style.shadow_radius);

		g_back.DrawImage(pBitmapDropShadow, rc.left - blurMarginX, rc.top - blurMarginY);

		// free memory
		delete pBitmapDropShadow;
		pBitmapDropShadow = NULL;
	}

	// 必须back_color非完全透明才绘制
	if (COLORNOTTRANSPARENT(color))	{
		Gdiplus::Color back_color = GDPCOLOR_FROM_COLORREF(color);
		Gdiplus::SolidBrush back_brush(back_color);
		g_back.FillPath(&back_brush, hiliteBackPath);
	}
	// draw border, for bordercolor not transparent and border valid
	if(COLORNOTTRANSPARENT(bordercolor) && m_style.border > 0)
	{
		Gdiplus::Color border_color = GDPCOLOR_FROM_COLORREF(bordercolor);
		Gdiplus::Pen gPenBorder(border_color, (Gdiplus::REAL)m_style.border);
		// candidate window border
		if (type == BackType::BACKGROUND) {
			GraphicsRoundRectPath bgPath(rc, m_style.round_corner_ex);
			g_back.DrawPath(&gPenBorder, &bgPath);
		}
		else if (type != BackType::TEXT)	// hilited_candidate_border / candidate_border
			g_back.DrawPath(&gPenBorder, hiliteBackPath);
	}
	// free memory
	delete hiliteBackPath;
	hiliteBackPath = NULL;
}

// draw preedit text, text only
bool WeaselPanel::_DrawPreedit(Text const& text, CDCHandle dc, CRect const& rc)
{
	bool drawn = false;
	std::wstring const& t = text.str;
	IDWriteTextFormat1* txtFormat = pDWR->pPreeditTextFormat;

	if (!t.empty()) {
		weasel::TextRange range;
		std::vector<weasel::TextAttribute> const& attrs = text.attributes;
		for (size_t j = 0; j < attrs.size(); ++j)
			if (attrs[j].type == weasel::HIGHLIGHTED)
				range = attrs[j].range;

		if (range.start < range.end) {
			CSize beforeSz, hilitedSz, afterSz;
			std::wstring before_str = t.substr(0, range.start);
			std::wstring hilited_str = t.substr(range.start, range.end);
			std::wstring after_str = t.substr(range.end);
			m_layout->GetTextSizeDW(before_str, before_str.length(), txtFormat, pDWR, &beforeSz);
			m_layout->GetTextSizeDW(hilited_str, hilited_str.length(), txtFormat, pDWR, &hilitedSz);
			m_layout->GetTextSizeDW(after_str, after_str.length(), txtFormat, pDWR, &afterSz);

			int x = rc.left;
			int y = rc.top;

			if (range.start > 0) {
				// zzz
				std::wstring str_before(t.substr(0, range.start));
				CRect rc_before;
				if (m_style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT)
					rc_before = CRect(rc.left, y, rc.right, y + beforeSz.cy);
				else
					rc_before = CRect(x, rc.top, rc.left + beforeSz.cx, rc.bottom);
				_TextOut(rc_before, str_before.c_str(), str_before.length(), m_style.text_color, txtFormat);
				if (m_style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT)
					y += beforeSz.cy + m_style.hilite_spacing;
				else
					x += beforeSz.cx + m_style.hilite_spacing;
			}
			{
				// zzz[yyy]
				std::wstring str_highlight(t.substr(range.start, range.end - range.start));
				CRect rc_hi;
				
				if (m_style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT)
					rc_hi = CRect(rc.left, y, rc.right, y + hilitedSz.cy);
				else
					rc_hi = CRect(x, rc.top, x + hilitedSz.cx, rc.bottom);
				_TextOut(rc_hi, str_highlight.c_str(), str_highlight.length(), m_style.hilited_text_color, txtFormat);
				if (m_style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT)
					y += rc_hi.Height()+m_style.hilite_spacing;
				else
					x += rc_hi.Width()+m_style.hilite_spacing;
			}
			if (range.end < static_cast<int>(t.length())) {
				// zzz[yyy]xxx
				std::wstring str_after(t.substr(range.end));
				CRect rc_after;
				if (m_style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT)
					rc_after = CRect(rc.left, y, rc.right, y + afterSz.cy);
				else
					rc_after = CRect(x, rc.top, x + afterSz.cx, rc.bottom);
				_TextOut(rc_after, str_after.c_str(), str_after.length(), m_style.text_color, txtFormat);
			}
		}
		else {
			CRect rcText(rc.left, rc.top, rc.right, rc.bottom);
			_TextOut(rcText, t.c_str(), t.length(), m_style.text_color, txtFormat);
		}
		// draw pager mark if not inline_preedit if necessary
		if(m_candidateCount && !m_style.inline_preedit && COLORNOTTRANSPARENT(m_style.prevpage_color) && COLORNOTTRANSPARENT(m_style.nextpage_color))
		{
			const std::wstring pre = L"<";
			const std::wstring next = L">";
			CRect prc = m_layout->GetPrepageRect();
			// clickable color / disabled color
			int color = m_ctx.cinfo.currentPage ? m_style.prevpage_color : m_style.text_color;
			_TextOut(prc, pre.c_str(), pre.length(), color, txtFormat);

			CRect nrc = m_layout->GetNextpageRect();
			// clickable color / disabled color
			color = m_ctx.cinfo.is_last_page ? m_style.text_color : m_style.nextpage_color;
			_TextOut(nrc, next.c_str(), next.length(), color, txtFormat);
		}
		drawn = true;
	}
	return drawn;
}

// draw hilited back color, back only
bool WeaselPanel::_DrawPreeditBack(Text const& text, CDCHandle dc, CRect const& rc)
{
	bool drawn = false;
	std::wstring const& t = text.str;
	IDWriteTextFormat1* txtFormat = pDWR->pPreeditTextFormat;

	if (!t.empty()) {
		weasel::TextRange range;
		std::vector<weasel::TextAttribute> const& attrs = text.attributes;
		for (size_t j = 0; j < attrs.size(); ++j)
			if (attrs[j].type == weasel::HIGHLIGHTED)
				range = attrs[j].range;

		if (range.start < range.end) {
			CSize beforeSz, hilitedSz;
			std::wstring before_str = t.substr(0, range.start);
			std::wstring hilited_str = t.substr(range.start, range.end);
			m_layout->GetTextSizeDW(before_str, before_str.length(), txtFormat, pDWR, &beforeSz);
			m_layout->GetTextSizeDW(hilited_str, hilited_str.length(), txtFormat, pDWR, &hilitedSz);

			int x = rc.left;
			int y = rc.top;

			if (range.start > 0) {
				if (m_style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT)
					y += beforeSz.cy + m_style.hilite_spacing;
				else
					x += beforeSz.cx + m_style.hilite_spacing;
			}
			{
				CRect rc_hi;
				if (m_style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT)
					rc_hi = CRect(rc.left, y, rc.right, y + hilitedSz.cy);
				else
					rc_hi = CRect(x, rc.top, x + hilitedSz.cx, rc.bottom);
				// if preedit rect size smaller than icon, fill the gap to STATUS_ICON_SIZE
				if(m_layout->ShouldDisplayStatusIcon())
				{
					if((m_style.layout_type == UIStyle::LAYOUT_HORIZONTAL || m_style.layout_type == UIStyle::LAYOUT_VERTICAL) && hilitedSz.cy < STATUS_ICON_SIZE)
						rc_hi.InflateRect(0, (STATUS_ICON_SIZE - hilitedSz.cy) / 2);
					if(m_style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT && hilitedSz.cx < STATUS_ICON_SIZE)
						rc_hi.InflateRect((STATUS_ICON_SIZE - hilitedSz.cx) / 2, 0);
				}

				rc_hi.InflateRect(m_style.hilite_padding, m_style.hilite_padding);
				IsToRoundStruct rd = m_layout->GetTextRoundInfo();
				_HighlightText(dc, rc_hi, m_style.hilited_back_color, m_style.hilited_shadow_color, m_style.round_corner, BackType::TEXT, rd);
			}
		}
		drawn = true;
	}
	return drawn;
}

bool WeaselPanel::_DrawCandidates(CDCHandle &dc, bool back)
{
	bool drawn = false;
	const std::vector<Text> &candidates(m_ctx.cinfo.candies);
	const std::vector<Text> &comments(m_ctx.cinfo.comments);
	const std::vector<Text> &labels(m_ctx.cinfo.labels);

	IDWriteTextFormat1* txtFormat = pDWR->pTextFormat;
	IDWriteTextFormat1* labeltxtFormat = pDWR->pLabelTextFormat;
	IDWriteTextFormat1* commenttxtFormat = pDWR->pCommentTextFormat;
	BackType bkType = BackType::CAND;


	CRect rect;	
	// draw back color and shadow color, with gdi+
	if (back) {
		// if candidate_shadow_color not transparent, draw candidate shadow first
		if (COLORNOTTRANSPARENT(m_style.candidate_shadow_color)) {
			for (auto i = 0; i < m_candidateCount && i < MAX_CANDIDATES_COUNT; ++i) {
				if (i == m_ctx.cinfo.highlighted) continue;	// draw non hilited candidates only 
				rect = m_layout->GetCandidateRect((int)i);
				if(m_istorepos) rect.OffsetRect(0, m_offsetys[i]);
				rect.InflateRect(m_style.hilite_padding, m_style.hilite_padding);
				IsToRoundStruct rd = m_layout->GetRoundInfo(i);
				_HighlightText(dc, rect, 0x00000000, m_style.candidate_shadow_color, m_style.round_corner, bkType, rd);
				drawn = true;
			}
		}
		// draw non highlighted candidates, without shadow
		if (COLORNOTTRANSPARENT(m_style.candidate_back_color) || COLORNOTTRANSPARENT(m_style.candidate_border_color)
				)	// if transparent not to draw
		{
			for (auto i = 0; i < m_candidateCount && i < MAX_CANDIDATES_COUNT; ++i) {
				if (i == m_ctx.cinfo.highlighted) continue;
				rect = m_layout->GetCandidateRect((int)i);
				if(m_istorepos) rect.OffsetRect(0, m_offsetys[i]);
				rect.InflateRect(m_style.hilite_padding, m_style.hilite_padding);
				IsToRoundStruct rd = m_layout->GetRoundInfo(i);
				_HighlightText(dc, rect, m_style.candidate_back_color, 0x00000000, m_style.round_corner, bkType, rd, m_style.candidate_border_color);
				drawn = true;
			}
		}
		// draw highlighted back ground and shadow
		{
			rect = m_layout->GetHighlightRect();
			if(m_istorepos) rect.OffsetRect(0, m_offsetys[m_ctx.cinfo.highlighted]);
			rect.InflateRect(m_style.hilite_padding, m_style.hilite_padding);
			IsToRoundStruct rd = m_layout->GetRoundInfo(m_ctx.cinfo.highlighted);
			_HighlightText(dc, rect, m_style.hilited_candidate_back_color, m_style.hilited_candidate_shadow_color, m_style.round_corner, bkType, rd, m_style.hilited_candidate_border_color);
			drawn = true;
		}
	}
	// draw text with direct write
	else
	{
		// begin draw candidate texts
		int label_text_color, candidate_text_color, comment_text_color;
		for (auto i = 0; i < m_candidateCount && i < MAX_CANDIDATES_COUNT; ++i) {
			if (i == m_ctx.cinfo.highlighted)
			{
				label_text_color = m_style.hilited_label_text_color;
				candidate_text_color = m_style.hilited_candidate_text_color;
				comment_text_color = m_style.hilited_comment_text_color;
			}
			else
			{
				label_text_color = m_style.label_text_color;
				candidate_text_color = m_style.candidate_text_color;
				comment_text_color = m_style.comment_text_color;
			}
			// draw highlight mark
			if (!m_style.mark_text.empty() && COLORNOTTRANSPARENT(m_style.hilited_mark_color))
			{
				CRect rc = m_layout->GetHighlightRect();
				if(m_istorepos) rc.OffsetRect(0, m_offsetys[m_ctx.cinfo.highlighted]);
				rc.InflateRect(m_style.hilite_padding, m_style.hilite_padding);
				int vgap = m_layout->MARK_HEIGHT ? (rc.Height() - m_layout->MARK_HEIGHT) / 2 : 0;
				int hgap = m_layout->MARK_WIDTH ? (rc.Width() - m_layout->MARK_WIDTH) / 2 : 0;
				CRect hlRc;
				if(m_style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT)
					hlRc = CRect(rc.left + hgap, rc.top + m_style.hilite_padding + (m_layout->MARK_GAP - m_layout->MARK_HEIGHT) / 2 + 1,
					rc.left + hgap + m_layout->MARK_WIDTH, rc.top + m_style.hilite_padding + (m_layout->MARK_GAP - m_layout->MARK_HEIGHT) / 2 + 1 + m_layout->MARK_HEIGHT);
				else
					hlRc = CRect(rc.left + m_style.hilite_padding + (m_layout->MARK_GAP - m_layout->MARK_WIDTH) / 2 + 1, rc.top + vgap,
					rc.left + m_style.hilite_padding + (m_layout->MARK_GAP - m_layout->MARK_WIDTH) / 2 + 1 + m_layout->MARK_WIDTH, rc.bottom - vgap);
				_TextOut(hlRc, m_style.mark_text.c_str(), m_style.mark_text.length(), m_style.hilited_mark_color, pDWR->pTextFormat);
			}
			// Draw label
			std::wstring label = m_layout->GetLabelText(labels, (int)i, m_style.label_text_format.c_str());
			if (!label.empty()) {
				rect = m_layout->GetCandidateLabelRect((int)i);
				if(m_istorepos) rect.OffsetRect(0, m_offsetys[i]);
				_TextOut(rect, label.c_str(), label.length(), label_text_color, labeltxtFormat);
			}
			// Draw text
			std::wstring text = candidates.at(i).str;
			if (!text.empty()) {
				rect = m_layout->GetCandidateTextRect((int)i);
				if(m_istorepos) rect.OffsetRect(0, m_offsetys[i]);
				_TextOut(rect, text.c_str(), text.length(), candidate_text_color, txtFormat);
			}
			// Draw comment
			std::wstring comment = comments.at(i).str;
			if (!comment.empty()) {
				rect = m_layout->GetCandidateCommentRect((int)i);
				if(m_istorepos) rect.OffsetRect(0, m_offsetys[i]);
				_TextOut(rect, comment.c_str(), comment.length(), comment_text_color, commenttxtFormat);
			}
			drawn = true;
		}
	}
	return drawn;
}

//draw client area
void WeaselPanel::DoPaint(CDCHandle dc)
{
	GetClientRect(&rcw);
	// prepare memDC
	CDCHandle hdc = ::GetDC(m_hWnd);
	CDCHandle memDC = ::CreateCompatibleDC(hdc);
	HBITMAP memBitmap = ::CreateCompatibleBitmap(hdc, rcw.Width(), rcw.Height());
	::SelectObject(memDC, memBitmap);
	ReleaseDC(hdc);
	bool drawn = false;
	if(!hide_candidates){
		bool over_bottom = false;
		CRect auxrc = m_layout->GetAuxiliaryRect();
		CRect preeditrc = m_layout->GetPreeditRect();
		if(m_style.vertical_auto_reverse && m_style.layout_type == UIStyle::LAYOUT_VERTICAL)
		{
			RECT rcWorkArea;
			memset(&rcWorkArea, 0, sizeof(rcWorkArea));
			HMONITOR hMonitor = MonitorFromRect(m_inputPos, MONITOR_DEFAULTTONEAREST);
			if (hMonitor) {
				MONITORINFO info;
				info.cbSize = sizeof(MONITORINFO);
				if (GetMonitorInfo(hMonitor, &info)) {
					rcWorkArea = info.rcWork;
				}
			}
			CSize size = m_layout->GetContentSize();
			rcWorkArea.right -= size.cx;
			rcWorkArea.bottom -= size.cy;
			int x = m_ocursurPos.left;
			int y = m_ocursurPos.bottom;
			y -= (m_style.shadow_offset_y >= 0) ? m_layout->offsetY : (COLORNOTTRANSPARENT(m_style.shadow_color)? 0 : (m_style.margin_y - m_style.hilite_padding));
			y -= m_style.shadow_radius / 2;
			over_bottom = (y > rcWorkArea.bottom);
			if (over_bottom)
			{
				CRect* rects = new CRect[m_candidateCount];
				int* btmys = new int[m_candidateCount];
				for (auto i = 0; i < m_candidateCount && i < MAX_CANDIDATES_COUNT; ++i) {
					rects[i] = m_layout->GetCandidateRect(i);
					btmys[i] = rects[i].bottom;
				}
				if (m_candidateCount) {
					if (!m_layout->IsInlinePreedit() && !m_ctx.preedit.str.empty())
						m_offsety_preedit = rects[m_candidateCount - 1].bottom - preeditrc.bottom;
					if (!m_ctx.aux.str.empty())
						m_offsety_aux = rects[m_candidateCount - 1].bottom - auxrc.bottom;
				} else {
					m_offsety_preedit = 0;
					m_offsety_aux = 0;
				}
				int base_gap = 0;
				if (!m_ctx.aux.str.empty())
					base_gap = auxrc.Height() + m_style.spacing;
				else if (!m_layout->IsInlinePreedit() && !m_ctx.preedit.str.empty())
					base_gap = preeditrc.Height() + m_style.spacing;

				for (auto i = 0; i < m_candidateCount && i < MAX_CANDIDATES_COUNT; ++i) {
					if (i == 0)
						m_offsetys[i] = btmys[m_candidateCount - i - 1] - base_gap - rects[i].bottom;
					else
						m_offsetys[i] = (rects[i - 1].top + m_offsetys[i - 1] - m_style.candidate_spacing) - rects[i].bottom;
				}
				delete[] rects;
				delete[] btmys;
			}
		}

		// background and candidates back, hilite back drawing start
		if (!m_ctx.empty()) {
			CRect backrc = m_layout->GetContentRect();
			_HighlightText(memDC, backrc, m_style.back_color, m_style.shadow_color, m_style.round_corner_ex, BackType::BACKGROUND, IsToRoundStruct(),  m_style.border_color);
		}
		m_istorepos = over_bottom;
		if (!m_ctx.aux.str.empty())
		{
			if(m_istorepos)
				auxrc.OffsetRect(0, m_offsety_aux);
			drawn |= _DrawPreeditBack(m_ctx.aux, memDC, auxrc);
		}
		if (!m_layout->IsInlinePreedit() && !m_ctx.preedit.str.empty())
		{
			if(m_istorepos)
				preeditrc.OffsetRect(0, m_offsety_preedit);
			drawn |= _DrawPreeditBack(m_ctx.preedit, memDC, preeditrc);
		}
		if (m_candidateCount)
			drawn |= _DrawCandidates(memDC, true);
		// background and candidates back, hilite back drawing end

		// begin  texts drawing
		pDWR->pRenderTarget->BindDC(memDC, &rcw);
		pDWR->pRenderTarget->BeginDraw();
		// draw auxiliary string
		if (!m_ctx.aux.str.empty())
			drawn |= _DrawPreedit(m_ctx.aux, memDC, auxrc);
		// draw preedit string
		if (!m_layout->IsInlinePreedit() && !m_ctx.preedit.str.empty())
			drawn |= _DrawPreedit(m_ctx.preedit, memDC, preeditrc);
		// draw candidates string
		if(m_candidateCount)
			drawn |= _DrawCandidates(memDC);
		pDWR->pRenderTarget->EndDraw();
		// end texts drawing

		// status icon (I guess Metro IME stole my idea :)
		if (m_layout->ShouldDisplayStatusIcon()) {
			// decide if custom schema zhung icon to show
			LoadIconNecessary(m_current_zhung_icon, m_style.current_zhung_icon, m_iconEnabled, IDI_ZH);
			LoadIconNecessary(m_current_ascii_icon, m_style.current_ascii_icon, m_iconAlpha, IDI_EN);
			CRect iconRect(m_layout->GetStatusIconRect());
			if (m_istorepos && !m_ctx.aux.str.empty())
				iconRect.OffsetRect(0, m_offsety_aux);
			else if (m_istorepos && !m_layout->IsInlinePreedit() && !m_ctx.preedit.str.empty())
				iconRect.OffsetRect(0, m_offsety_preedit);

			CIcon& icon(m_status.disabled ? m_iconDisabled : m_status.ascii_mode ? m_iconAlpha :
				((m_ctx.aux.str != L"全角" && m_ctx.aux.str != L"半角") ? m_iconEnabled : (m_status.full_shape ? m_iconFull : m_iconHalf)) );
			memDC.DrawIconEx(iconRect.left, iconRect.top, icon, 0, 0);
			drawn = true;
		}
		/* Nothing drawn, hide candidate window */
		if (!drawn)
			ShowWindow(SW_HIDE);
	}
	_LayerUpdate(rcw, memDC);

#ifdef USE_MOUSE_EVENTS
	// turn off WS_EX_TRANSPARENT after drawings, for better resp performance
	::SetWindowLong(m_hWnd, GWL_EXSTYLE, ::GetWindowLong(m_hWnd, GWL_EXSTYLE) & (~WS_EX_TRANSPARENT));
#endif
	// clean objs
	::DeleteDC(memDC);
	::DeleteObject(memBitmap);
}

void WeaselPanel::_LayerUpdate(const CRect& rc, CDCHandle dc)
{
	HDC ScreenDC = ::GetDC(NULL);
	CRect rect;
	GetWindowRect(&rect);
	POINT WindowPosAtScreen = { rect.left, rect.top };
	POINT PointOriginal = { 0, 0 };
	SIZE sz = { rc.Width(), rc.Height() };

	BLENDFUNCTION bf = {AC_SRC_OVER, 0, 0XFF, AC_SRC_ALPHA};
	UpdateLayeredWindow(m_hWnd, ScreenDC, &WindowPosAtScreen, &sz, dc, &PointOriginal, RGB(0,0,0), &bf, ULW_ALPHA);
	ReleaseDC(ScreenDC);
}

LRESULT WeaselPanel::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	GetWindowRect(&m_inputPos);
	Refresh();
	return TRUE;
}

LRESULT WeaselPanel::OnDpiChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	Refresh();
	return LRESULT();
}

void WeaselPanel::MoveTo(RECT const& rc)
{
	m_inputPos = rc;
	m_ocursurPos = m_inputPos;
	if (m_style.shadow_offset_y >= 0)	m_inputPos.OffsetRect(0, 10);
	// with parameter to avoid vertical flicker
	_RepositionWindow(true);
	RedrawWindow();
}

void WeaselPanel::_RepositionWindow(bool adj)
{
	RECT rcWorkArea;
	memset(&rcWorkArea, 0, sizeof(rcWorkArea));
	HMONITOR hMonitor = MonitorFromRect(m_inputPos, MONITOR_DEFAULTTONEAREST);
	if (hMonitor) {
		MONITORINFO info;
		info.cbSize = sizeof(MONITORINFO);
		if (GetMonitorInfo(hMonitor, &info)) {
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
	if (m_style.shadow_radius > 0) {
		x -= (m_style.shadow_offset_x >= 0) ? m_layout->offsetX : (COLORNOTTRANSPARENT(m_style.shadow_color)? 0 : (m_style.margin_x - m_style.hilite_padding));
		// avoid flickering in MoveTo
		if(adj) {
			y -= (m_style.shadow_offset_y >= 0) ? m_layout->offsetY : (COLORNOTTRANSPARENT(m_style.shadow_color)? 0 : (m_style.margin_y - m_style.hilite_padding));
			y -= m_style.shadow_radius / 2;
		}
	}
	// for vertical text layout, flow right to left, make window left side
	if(m_style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT && !m_style.vertical_text_left_to_right) {
		x -= width;
		x += m_layout->offsetX;
	}
	if (x > rcWorkArea.right) x = rcWorkArea.right;		// over workarea right
	if (x < rcWorkArea.left) x = rcWorkArea.left;		// over workarea left
	// show panel above the input focus if we're around the bottom
	if (y > rcWorkArea.bottom) y = m_inputPos.top - height; // over workarea bottom
	//if (y > rcWorkArea.bottom) y = rcWorkArea.bottom;
	if (y < rcWorkArea.top) y = rcWorkArea.top;		// over workarea top
	// memorize adjusted position (to avoid window bouncing on height change)
	m_inputPos.bottom = y;
	// reposition window only if the position changed
	if (m_oinputPos.left == x && m_oinputPos.bottom == y) return;
	m_oinputPos.left = x;
	m_oinputPos.bottom = y;
	SetWindowPos(HWND_TOPMOST, x, y, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
}

void WeaselPanel::_TextOut(CRect const& rc, std::wstring psz, size_t cch, int inColor, IDWriteTextFormat* pTextFormat)
{
	if (pTextFormat == NULL) return;
	float r = (float)(GetRValue(inColor))/255.0f;
	float g = (float)(GetGValue(inColor))/255.0f;
	float b = (float)(GetBValue(inColor))/255.0f;
	float alpha = (float)((inColor >> 24) & 255) / 255.0f;
	pBrush->SetColor(D2D1::ColorF(r, g, b, alpha));

	if (NULL != pBrush && NULL != pTextFormat) {
		pDWR->pDWFactory->CreateTextLayout( psz.c_str(), (UINT32)psz.size(), pTextFormat, (float)rc.Width(), (float)rc.Height(), reinterpret_cast<IDWriteTextLayout**>(&pDWR->pTextLayout));
		if (m_style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT) {
			DWRITE_FLOW_DIRECTION flow = m_style.vertical_text_left_to_right ? DWRITE_FLOW_DIRECTION_LEFT_TO_RIGHT : DWRITE_FLOW_DIRECTION_RIGHT_TO_LEFT;
			pDWR->pTextLayout->SetReadingDirection(DWRITE_READING_DIRECTION_TOP_TO_BOTTOM);
			pDWR->pTextLayout->SetFlowDirection(flow);
		}

		// offsetx for font glyph over left
		float offsetx = rc.left;
		float offsety = rc.top;
		// prepare for space when first character overhanged
		DWRITE_OVERHANG_METRICS omt;
		pDWR->pTextLayout->GetOverhangMetrics(&omt);
		if (m_style.layout_type != UIStyle::LAYOUT_VERTICAL_TEXT && omt.left > 0)
			offsetx += omt.left;
		if (m_style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT && omt.top > 0)
			offsety += omt.top;

		if (pDWR->pTextLayout != NULL) {
			pDWR->pRenderTarget->DrawTextLayout({ offsetx, offsety }, pDWR->pTextLayout, pBrush, D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
#if 0
			D2D1_RECT_F rectf =  D2D1::RectF(offsetx, offsety, offsetx + rc.Width(), offsety + rc.Height());
			pDWR->pRenderTarget->DrawRectangle(&rectf, pBrush);
#endif
		}
		SafeRelease(&pDWR->pTextLayout);
	}
}

