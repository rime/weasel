#include "stdafx.h"
#include "WeaselPanel.h"
#include <WeaselCommon.h>
#include <vector>
#include <fstream>
#include <string>
#include <d2d1_1.h>
//#include <Usp10.h>

#include "VerticalLayout.h"
#include "HorizontalLayout.h"
#include "FullScreenLayout.h"

// for IDI_ZH, IDI_EN
#include <resource.h>

using namespace Gdiplus;
using namespace weasel;
using namespace std;

#define max3(a,b,c) max(max(a,b),c)
#define min3(a,b,c) min(min(a,b),c)

WeaselPanel::WeaselPanel(weasel::UI &ui)
	: m_layout(NULL), 
	  m_ctx(ui.ctx()), 
	  m_status(ui.status()), 
	  m_style(ui.style()),
	  _isVistaSp2OrGrater(false),
	  dpiScaleX_(0.0f),
	  dpiScaleY_(0.0f)
{
	m_iconDisabled.LoadIconW(IDI_RELOAD, STATUS_ICON_SIZE, STATUS_ICON_SIZE, LR_DEFAULTCOLOR);
	m_iconEnabled.LoadIconW(IDI_ZH, STATUS_ICON_SIZE, STATUS_ICON_SIZE, LR_DEFAULTCOLOR);
	m_iconAlpha.LoadIconW(IDI_EN, STATUS_ICON_SIZE, STATUS_ICON_SIZE, LR_DEFAULTCOLOR);
}

WeaselPanel::~WeaselPanel()
{
	if (m_layout != NULL)
		delete m_layout;
}

void WeaselPanel::_ResizeWindow()
{
	CDCHandle dc = GetDC();
	long fontHeight = -MulDiv(m_style.font_point, dc.GetDeviceCaps(LOGPIXELSY), 72);
	CFont font;
	font.CreateFontW(fontHeight, 0, 0, 0, 0, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, m_style.font_face.c_str());
	dc.SelectFont(font);

	CSize size = m_layout->GetContentSize();
	SetWindowPos(NULL, 0, 0, size.cx, size.cy, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);
	ReleaseDC(dc);
}

void WeaselPanel::_CreateLayout()
{
	if (m_layout != NULL)
		delete m_layout;

	Layout* layout = NULL;
	if (m_style.layout_type == UIStyle::LAYOUT_VERTICAL ||
		m_style.layout_type == UIStyle::LAYOUT_VERTICAL_FULLSCREEN)
	{
		layout = new VerticalLayout(m_style, m_ctx, m_status);
	}
	else if (m_style.layout_type == UIStyle::LAYOUT_HORIZONTAL ||
		m_style.layout_type == UIStyle::LAYOUT_HORIZONTAL_FULLSCREEN)
	{
		layout = new HorizontalLayout(m_style, m_ctx, m_status);
	}
	if (m_style.layout_type == UIStyle::LAYOUT_VERTICAL_FULLSCREEN ||
		m_style.layout_type == UIStyle::LAYOUT_HORIZONTAL_FULLSCREEN)
	{
		layout = new FullScreenLayout(m_style, m_ctx, m_status, m_inputPos, layout);
	}
	m_layout = layout;
}

//更新界面
void WeaselPanel::Refresh()
{
	_CreateLayout();

	CDCHandle dc = GetDC();
	long fontHeight = -MulDiv(m_style.font_point, dc.GetDeviceCaps(LOGPIXELSY), 72);
	CFont font;
	font.CreateFontW(fontHeight, 0, 0, 0, 0, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, m_style.font_face.c_str());
	dc.SelectFont(font);
	m_layout->DoLayout(dc);
	ReleaseDC(dc);

	_ResizeWindow();
	_RepositionWindow();
	RedrawWindow();
}

void WeaselPanel::_HighlightText(CDCHandle dc, CRect rc, COLORREF color)
{
	rc.InflateRect(m_style.hilite_padding, m_style.hilite_padding);
	{
		Graphics gBack(dc);
		gBack.SetSmoothingMode(SmoothingMode::SmoothingModeHighQuality);
		GraphicsRoundRectPath bgPath;
		bgPath.AddRoundRect(rc.left, rc.top, rc.Width(), rc.Height(), m_style.round_corner, m_style.round_corner);
		BYTE alpha = (BYTE)((color >> 24) & 255);
		Color back_color = Color::MakeARGB(alpha, GetRValue(color), GetGValue(color), GetBValue(color));
		SolidBrush gBrBack(back_color);
		if (m_style.shadow_radius)
		{
			BYTE r = GetRValue(color);
			BYTE g = GetGValue(color);
			BYTE b = GetBValue(color);
			Color centrColor = Color::MakeARGB(alpha/2, r, g, b);
			Color edgeColor = Color::MakeARGB(0, r, g, b);
			GraphicsRoundRectPath shadowPath;
			shadowPath.AddRoundRect(rc.left + m_style.shadow_radius, rc.top + m_style.shadow_radius,
				rc.Width(), rc.Height(), m_style.round_corner, m_style.round_corner);
			PathGradientBrush pathBr(&shadowPath);
			Color colors[] = { edgeColor, edgeColor,edgeColor, edgeColor };
			int count = 1;
			pathBr.SetCenterColor(centrColor);
			pathBr.SetSurroundColors(colors, &count);
			pathBr.SetFocusScales(
				((float)(rc.Width() - m_style.shadow_radius * 2)) / (float)rc.Width(),
				((float)(rc.Height() - m_style.shadow_radius * 2)) / (float)rc.Height()
			);
			gBack.FillPath(&pathBr, &shadowPath);
		}
		gBack.FillPath(&gBrBack, &bgPath);
	}
}

void WeaselPanel::_HighlightTextEx(CDCHandle dc, CRect rc, COLORREF color, COLORREF shadowColor)
{
	rc.InflateRect(m_style.hilite_padding, m_style.hilite_padding);
	{
		Graphics gBack(dc);
		gBack.SetSmoothingMode(SmoothingMode::SmoothingModeHighQuality);
		GraphicsRoundRectPath bgPath;
		bgPath.AddRoundRect(rc.left, rc.top, rc.Width(), rc.Height(), m_style.round_corner, m_style.round_corner);
		Color back_color = Color::MakeARGB((color >> 24), GetRValue(color), GetGValue(color), GetBValue(color));
		SolidBrush gBrBack(back_color);
		if (m_style.shadow_radius)
		{
			BYTE r = GetRValue(shadowColor);
			BYTE g = GetGValue(shadowColor);
			BYTE b = GetBValue(shadowColor);
#if 0
			Color centrColor = Color::MakeARGB(shadowColor >> 24, r, g, b);
			Color edgeColor = Color::MakeARGB(0, r, g, b);
			GraphicsRoundRectPath shadowPath;
			shadowPath.AddRoundRect(rc.left + m_style.shadow_radius, rc.top + m_style.shadow_radius,
				rc.Width(), rc.Height(), m_style.round_corner, m_style.round_corner);
			Gdiplus::PathGradientBrush pathBr(&shadowPath);
			Color colors[] = { edgeColor, edgeColor,edgeColor, edgeColor };
			int count = 4;
			pathBr.SetCenterColor(centrColor);
			pathBr.SetSurroundColors(colors, &count);
			pathBr.SetFocusScales(
				((float)(rc.Width() - m_style.shadow_radius * 2)) / (float)rc.Width(),
				((float)(rc.Height() - m_style.shadow_radius * 2)) / (float)rc.Height()
			);
			gBack.FillPath(&pathBr, &shadowPath);
#else
			Color brc = Color::MakeARGB((shadowColor >> 24) / m_style.shadow_radius, r, g, b);

			for (int i = 0; i < m_style.shadow_radius; i++)
			{
				GraphicsRoundRectPath path;
				path.AddRoundRect(rc.left + m_style.shadow_radius + i, rc.top + m_style.shadow_radius + i,
					rc.Width() - 2 * i, rc.Height() - 2 * i, m_style.round_corner, m_style.round_corner);
				SolidBrush br(brc);
				gBack.FillPath(&br, &path);
				DeleteObject(&path);
				DeleteObject(&br);
			}
#endif
		}
		gBack.FillPath(&gBrBack, &bgPath);
	}
}
bool WeaselPanel::_DrawPreedit(Text const& text, CDCHandle dc, CRect const& rc)
{
	bool drawn = false;
	std::wstring const& t = text.str;
	if (!t.empty())
	{
		weasel::TextRange range;
		std::vector<weasel::TextAttribute> const& attrs = text.attributes;
		for (size_t j = 0; j < attrs.size(); ++j)
			if (attrs[j].type == weasel::HIGHLIGHTED)
				range = attrs[j].range;

		if (range.start < range.end)
		{
			CSize selStart, selEnd;
			m_layout->GetTextExtentDCMultiline(dc, t, range.start, &selStart);
			m_layout->GetTextExtentDCMultiline(dc, t, range.end, &selEnd);
			int x = rc.left;
			if (range.start > 0)
			{
				// zzz
				std::wstring str_before(t.substr(0, range.start));
				CRect rc_before(x, rc.top, rc.left + selStart.cx, rc.bottom);
				_TextOut(dc, x, rc.top, rc_before, str_before.c_str(), str_before.length());
				x += selStart.cx + m_style.hilite_spacing;
			}
			{
				// zzz[yyy]
				std::wstring str_highlight(t.substr(range.start, range.end - range.start));
				CRect rc_hi(x, rc.top, x + (selEnd.cx - selStart.cx), rc.bottom);
				_HighlightTextEx(dc, rc_hi, m_style.hilited_back_color, m_style.hilited_shadow_color);
				dc.SetTextColor(m_style.hilited_text_color);
				dc.SetBkColor(m_style.hilited_back_color);
				_TextOut(dc, x, rc.top, rc_hi, str_highlight.c_str(), str_highlight.length());
				dc.SetTextColor(m_style.text_color);
				dc.SetBkColor(m_style.back_color);
				x += (selEnd.cx - selStart.cx);
			}
			if (range.end < static_cast<int>(t.length()))
			{
				// zzz[yyy]xxx
				x += m_style.hilite_spacing;
				std::wstring str_after(t.substr(range.end));
				CRect rc_after(x, rc.top, rc.right, rc.bottom);
				_TextOut(dc, x, rc.top, rc_after, str_after.c_str(), str_after.length());
			}
		}
		else
		{
			CRect rcText(rc.left, rc.top, rc.right, rc.bottom);
			_TextOut(dc, rc.left, rc.top, rcText, t.c_str(), t.length());
		}
		drawn = true;
	}
	return drawn;
}

bool WeaselPanel::_DrawCandidates(CDCHandle dc)
{
	bool drawn = false;
	const std::vector<Text> &candidates(m_ctx.cinfo.candies);
	const std::vector<Text> &comments(m_ctx.cinfo.comments);
	const std::vector<Text> &labels(m_ctx.cinfo.labels);

	for (size_t i = 0; i < candidates.size() && i < MAX_CANDIDATES_COUNT; ++i)
	{
		CRect rect;
		if (i == m_ctx.cinfo.highlighted)
		{
			_HighlightTextEx(dc, m_layout->GetHighlightRect(), m_style.hilited_candidate_back_color, m_style.hilited_candidate_shadow_color);
			dc.SetTextColor(m_style.hilited_label_text_color);
		}
		else
		{
			CRect candidateBackRect;
			if(m_style.layout_type == m_style.LAYOUT_HORIZONTAL_FULLSCREEN || m_style.layout_type == m_style.LAYOUT_HORIZONTAL)
			{
				candidateBackRect.left = m_layout->GetCandidateLabelRect(i).left;
				candidateBackRect.right = m_layout->GetCandidateCommentRect(i).right;
			}
			else
			{
				candidateBackRect.left = m_layout->GetHighlightRect().left;
				candidateBackRect.right = m_layout->GetHighlightRect().right;
			}
			candidateBackRect.top =m_layout->GetCandidateTextRect(i).top;
			candidateBackRect.bottom = m_layout->GetCandidateTextRect(i).bottom;
			_HighlightTextEx(dc, candidateBackRect, m_style.candidate_back_color, m_style.candidate_shadow_color);
			dc.SetTextColor(m_style.label_text_color);
		}

		// Draw label
		std::wstring label = m_layout->GetLabelText(labels, i, m_style.label_text_format.c_str());
		rect = m_layout->GetCandidateLabelRect(i);
		_TextOut(dc, rect.left, rect.top, rect, label.c_str(), label.length());

		// Draw text
		std::wstring text = candidates.at(i).str;
		if (i == m_ctx.cinfo.highlighted)
			dc.SetTextColor(m_style.hilited_candidate_text_color);
		else
			dc.SetTextColor(m_style.candidate_text_color);
		rect = m_layout->GetCandidateTextRect(i);
		_TextOut(dc, rect.left, rect.top, rect, text.c_str(), text.length());
		
		// Draw comment
		std::wstring comment = comments.at(i).str;
		if (!comment.empty())
		{
			if (i == m_ctx.cinfo.highlighted)
				dc.SetTextColor(m_style.hilited_comment_text_color);
			else
				dc.SetTextColor(m_style.comment_text_color);
			rect = m_layout->GetCandidateCommentRect(i);
			_TextOut(dc, rect.left, rect.top, rect, comment.c_str(), comment.length());
		}
		drawn = true;
	}

	dc.SetTextColor(m_style.text_color);
	return drawn;
}

//draw client area
void WeaselPanel::DoPaint(CDCHandle dc)
{
	// background start
	CRect rc;
	GetClientRect(&rc);

	SIZE sz = { rc.right - rc.left, rc.bottom - rc.top };
	CDCHandle hdc = ::GetDC(m_hWnd);
	CDCHandle memDC = ::CreateCompatibleDC(hdc);
	HBITMAP memBitmap = ::CreateCompatibleBitmap(hdc, sz.cx, sz.cy);
	::SelectObject(memDC, memBitmap);
	// 获取候选数，消除当候选数为0，又处于inline_preedit状态时出现一個空白的小方框或者圆角矩形的情况
	//const std::vector<Text> &candidates(m_ctx.cinfo.candies);
	//if(!(candidates.size()==0 && m_style.inline_preedit))
	//{
		Graphics gBack(memDC);
		gBack.SetSmoothingMode(SmoothingMode::SmoothingModeHighQuality);
		GraphicsRoundRectPath bgPath;

		bgPath.AddRoundRect(rc.left+m_style.border, rc.top+m_style.border, rc.Width()-m_style.border*2, rc.Height()-m_style.border*2, m_style.round_corner_ex, m_style.round_corner_ex);

		int alpha = ((m_style.border_color >> 24) & 255);
		Color border_color = Color::MakeARGB(alpha, GetRValue(m_style.border_color), GetGValue(m_style.border_color), GetBValue(m_style.border_color));
		alpha = (m_style.back_color >> 24) & 255;
		Color back_color = Color::MakeARGB(alpha, GetRValue(m_style.back_color), GetGValue(m_style.back_color), GetBValue(m_style.back_color));
		SolidBrush gBrBack(back_color);
		Pen gPenBorder(border_color, m_style.border);

		gBack.DrawPath(&gPenBorder, &bgPath);
		gBack.FillPath(&gBrBack, &bgPath);
		gBack.ReleaseHDC(memDC);
	//}
	// background end
	long height = -MulDiv(m_style.font_point, memDC.GetDeviceCaps(LOGPIXELSY), 72);

	CFont font;
	font.CreateFontW(height, 0, 0, 0, 0, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, m_style.font_face.c_str());
	CFontHandle oldFont = memDC.SelectFont(font);

	memDC.SetTextColor(m_style.text_color);
	memDC.SetBkColor(m_style.back_color);
	memDC.SetBkMode(TRANSPARENT);
	
	bool drawn = false;

	// draw preedit string
	if (!m_layout->IsInlinePreedit())
		drawn |= _DrawPreedit(m_ctx.preedit, memDC, m_layout->GetPreeditRect());
	
	// draw auxiliary string
	drawn |= _DrawPreedit(m_ctx.aux, memDC, m_layout->GetAuxiliaryRect());

	// status icon (I guess Metro IME stole my idea :)
	if (m_layout->ShouldDisplayStatusIcon())
	{
		const CRect iconRect(m_layout->GetStatusIconRect());
		CIcon& icon(m_status.disabled ? m_iconDisabled : m_status.ascii_mode ? m_iconAlpha : m_iconEnabled);
		memDC.DrawIconEx(iconRect.left, iconRect.top, icon, 0, 0);
		drawn = true;
	}

	// draw candidates
	drawn |= _DrawCandidates(memDC);

	/* Nothing drawn, hide candidate window */
	if (!drawn)
		ShowWindow(SW_HIDE);

	memDC.SelectFont(oldFont);	

	HDC screenDC = ::GetDC(NULL);
	CRect rect;
	GetWindowRect(&rect);
	POINT ptSrc = { rect.left, rect.top };
	POINT ptDest = { rc.left, rc.top };

	BLENDFUNCTION bf;
	bf.AlphaFormat = AC_SRC_ALPHA;
	bf.BlendFlags = 0;
	bf.BlendOp = AC_SRC_OVER;
	bf.SourceConstantAlpha = 255;
	::UpdateLayeredWindow(m_hWnd, screenDC, &ptSrc, &sz, memDC, &ptDest, RGB(0,0,0), &bf, ULW_ALPHA);
	::DeleteDC(memDC);
	::DeleteObject(memBitmap);
	ReleaseDC(screenDC);

}

LRESULT WeaselPanel::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	LONG t = ::GetWindowLong(m_hWnd, GWL_EXSTYLE);
	t |= WS_EX_LAYERED;
	::SetWindowLong(m_hWnd, GWL_EXSTYLE, t);
	Refresh();
	//CenterWindow();
	GdiplusStartup(&_m_gdiplusToken, &_m_gdiplusStartupInput, NULL);
	GetWindowRect(&m_inputPos);

	_isVistaSp2OrGrater = IsWindowsVistaSP2OrGreater();

    //get the dpi information
    //HDC screen = ::GetDC(0);
    //dpiScaleX_ = GetDeviceCaps(screen, LOGPIXELSX) / 96.0f;
    //dpiScaleY_ = GetDeviceCaps(screen, LOGPIXELSY) / 96.0f;
    //::ReleaseDC(0, screen);

	// prepare d2d1 resources
	HRESULT hResult = S_OK;
	// create factory
	hResult = ::D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, &pD2d1Factory);
	// create IDWriteFactory
	hResult = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&pDWFactory));
	/* ID2D1HwndRenderTarget */
	const D2D1_PIXEL_FORMAT format =
		D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,
			D2D1_ALPHA_MODE_PREMULTIPLIED);
	const D2D1_RENDER_TARGET_PROPERTIES properties =
		D2D1::RenderTargetProperties(
			D2D1_RENDER_TARGET_TYPE_DEFAULT,
			format);
	pD2d1Factory->CreateDCRenderTarget(&properties, &pRenderTarget);
	//pRenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
	pD2d1Factory->GetDesktopDpi(&dpiScaleX_, &dpiScaleY_);
	return TRUE;
}

LRESULT WeaselPanel::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	GdiplusShutdown(_m_gdiplusToken);
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

static HRESULT _TextOutWithFallback(CDCHandle dc, int x, int y, CRect const& rc, LPCWSTR psz, int cch)
{
    SCRIPT_STRING_ANALYSIS ssa;
    HRESULT hr;

    hr = ScriptStringAnalyse(
        dc,
        psz, cch,
        2 * cch + 16,
        -1,
        SSA_GLYPHS|SSA_FALLBACK|SSA_LINK,
        0,
        NULL, // control
        NULL, // state
        NULL, // piDx
        NULL,
        NULL, // pbInClass
        &ssa);

    if (SUCCEEDED(hr))
    {
        hr = ScriptStringOut(
            ssa, x, y, 0,
            &rc,
            0, 0, FALSE);
    }

	hr = ScriptStringFree(&ssa);
	return hr;
}

HBITMAP WeaselPanel::_CreateAlphaTextBitmap(LPCWSTR inText, HFONT inFont, COLORREF inColor, int cch)
{
	HDC hTextDC = CreateCompatibleDC(NULL);
	HFONT hOldFont = (HFONT)SelectObject(hTextDC, inFont);
	HBITMAP hMyDIB = NULL;
	// get text area
	RECT TextArea = { 0, 0, 0, 0 };
	DrawText(hTextDC, inText, cch, &TextArea, DT_CALCRECT);
	if ((TextArea.right > TextArea.left) && (TextArea.bottom > TextArea.top))
	{
		BITMAPINFOHEADER BMIH;
		memset(&BMIH, 0x0, sizeof(BITMAPINFOHEADER));
		void* pvBits = NULL;
		// dib setup
		BMIH.biSize = sizeof(BMIH);
		BMIH.biWidth = TextArea.right - TextArea.left;
		BMIH.biHeight = TextArea.bottom - TextArea.top;
		BMIH.biPlanes = 1;
		BMIH.biBitCount = 32;
		BMIH.biCompression = BI_RGB;

		// create and select dib into dc
		hMyDIB = CreateDIBSection(hTextDC, (LPBITMAPINFO)&BMIH, 0, (LPVOID*)&pvBits, NULL, 0);
		HBITMAP hOldBMP = (HBITMAP)SelectObject(hTextDC, hMyDIB);
		if (hOldBMP != NULL)
		{
			SetTextColor(hTextDC, 0x00FFFFFF);
			SetBkColor(hTextDC, 0x00000000);
			SetBkMode(hTextDC, OPAQUE);
			// draw text to buffer
			DrawText(hTextDC, inText, cch, &TextArea, DT_NOCLIP);
			BYTE* DataPtr = (BYTE*)pvBits;
			BYTE FillR = GetRValue(inColor);
			BYTE FillG = GetGValue(inColor);
			BYTE FillB = GetBValue(inColor);
			BYTE ThisA;

			for (int LoopY = 0; LoopY < BMIH.biHeight; LoopY++)
			{
				for (int LoopX = 0; LoopX < BMIH.biWidth; LoopX++)
				{
					ThisA = *DataPtr; // move alpha and premutiply with rgb
					*DataPtr++ = (FillB * ThisA) >> 8;
					*DataPtr++ = (FillG * ThisA) >> 8;
					*DataPtr++ = (FillR * ThisA) >> 8;
					*DataPtr++ = ThisA;
				}
			}
			SelectObject(hTextDC, hOldBMP);
		}
	}
	SelectObject(hTextDC, hOldFont);
	DeleteDC(hTextDC);
	return hMyDIB;
}

static HRESULT _TextOutWithFallback_ULW(CDCHandle dc, int x, int y, CRect const rc, LPCWSTR psz, int cch, long height, std::wstring fontface)
{
    SCRIPT_STRING_ANALYSIS ssa;
    HRESULT hr;
	CFont font;
	font.CreateFontW(height, 0, 0, 0, 0, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, fontface.c_str());
	int TextLength = cch;
	HDC hTextDC = CreateCompatibleDC(NULL);
	HFONT hOldFont = (HFONT)SelectObject(hTextDC, font);
	HBITMAP MyBMP = NULL;

    hr = ScriptStringAnalyse(
        hTextDC,
        psz, cch,
        2 * cch + 16,
        -1,
        SSA_GLYPHS|SSA_FALLBACK|SSA_LINK,
        0,
        NULL, // control
        NULL, // state
        NULL, // piDx
        NULL,
        NULL, // pbInClass
        &ssa);

    if (SUCCEEDED(hr))
    {

		BITMAPINFOHEADER BMIH;
		memset(&BMIH, 0x0, sizeof(BITMAPINFOHEADER));
		void* pvBits = NULL;
		BMIH.biSize = sizeof(BMIH);
		BMIH.biWidth = rc.right - rc.left;
		BMIH.biHeight = rc.bottom - rc.top;
		BMIH.biPlanes = 1;
		BMIH.biBitCount = 32;
		BMIH.biCompression = BI_RGB;
		MyBMP = CreateDIBSection(hTextDC, (LPBITMAPINFO)&BMIH, 0, (LPVOID*)&pvBits, NULL, 0);
		HBITMAP hOldBMP = (HBITMAP)SelectObject(hTextDC, MyBMP);
		COLORREF inColor = dc.GetTextColor();
		BYTE alpha = (inColor >> 24) & 255 ;
		if (hOldBMP != NULL)
		{
			SetTextColor(hTextDC, 0x00FFFFFF);
			SetBkColor(hTextDC, 0x00000000);
			SetBkMode(hTextDC, OPAQUE);
			// draw text to buffer
			hr = ScriptStringOut(ssa, 0, 0, 0, rc, 0, 0, FALSE);
			BYTE* DataPtr = (BYTE*)pvBits;
			BYTE FillR = GetRValue(inColor);
			BYTE FillG = GetGValue(inColor);
			BYTE FillB = GetBValue(inColor);
			BYTE ThisA;
			for (int LoopY = 0; LoopY < BMIH.biHeight; LoopY++)
			{
				for (int LoopX = 0; LoopX < BMIH.biWidth; LoopX++)
				{
					ThisA = *DataPtr; // move alpha and premutiply with rgb
					*DataPtr++ = (FillB * ThisA) >> 8;
					*DataPtr++ = (FillG * ThisA) >> 8;
					*DataPtr++ = (FillR * ThisA) >> 8;
					*DataPtr++ = ThisA;
				}
			}
			SelectObject(hTextDC, hOldBMP);
		}
		SelectObject(hTextDC, hOldFont);
		DeleteDC(hTextDC);
		DeleteObject(font);
		if (MyBMP)
		{
			// temporary dc select bmp into it
			HDC hTempDC = CreateCompatibleDC(dc);
			HBITMAP hOldBMP = (HBITMAP)SelectObject(hTempDC, MyBMP);
			if (hOldBMP)
			{
				BITMAP BMInf;
				GetObject(MyBMP, sizeof(BITMAP), &BMInf);
				// fill blend function and blend new text to window
				BLENDFUNCTION bf;
				bf.BlendOp = AC_SRC_OVER;
				bf.BlendFlags = 0;
				bf.SourceConstantAlpha = alpha;
				bf.AlphaFormat = AC_SRC_ALPHA;
				AlphaBlend(dc, x, y, BMInf.bmWidth, BMInf.bmHeight, hTempDC, 0, 0, BMInf.bmWidth, BMInf.bmHeight, bf);
				// clean up
				SelectObject(hTempDC, hOldBMP);
				DeleteObject(MyBMP);
				DeleteDC(hTempDC);
			}
		}
    }

	hr = ScriptStringFree(&ssa);
	return hr;
}

// utf16* to unicode and return length of utf16
inline static size_t Utf16ToUnicode(const wchar_t* src, ULONG& des)
{
	if (!src || (*src) == 0) return 0;

	wchar_t w1 = src[0];
	if (w1 >= 0xD800 && w1 <= 0xDFFF)
	{
		if (w1 < 0xDC00)
		{
			wchar_t w2 = src[1];
			if (w2 >= 0xDC00 && w2 <= 0xDFFF)
			{
				des = (w2 & 0x03FF) + (((w1 & 0x03FF) + 0x40) << 10);
				return 2;
			}
		}
		return 0; // the src is invalid  
	}
	else
	{
		des = w1;
		return 1;
	}
}

// wstring中查找所有emoji字符的range
static vector<DWRITE_TEXT_RANGE> CheckEmojiRange(wstring str)
{
	vector<DWRITE_TEXT_RANGE> rng;
	int i = 0;
	wchar_t* utf16 = &str[0];
	ULONG unicode = 0;
	UINT32 sc = 0, ec = 0;
	size_t sz = 0;
	BOOL isEmjtmp = FALSE;
	BOOL isEmoji = FALSE;
	while (i < str.size())
	{
		sz = Utf16ToUnicode(utf16, unicode);
		if ( (unicode >= 0x2000 && unicode <= 0x27bf) || (unicode >= 0x1f000 && unicode <= 0x1fbff) )
			isEmjtmp = TRUE;    /* 当前字符是emoji 字符长度为sz */
		else
			isEmjtmp = FALSE;
		if (isEmoji == TRUE && (*utf16 == 0x200d)) /* 如果前面的字符是Emoji 后面链接符号也认为是Emoji的一部分 */
		{
			isEmjtmp = TRUE;
			sz = 1;
		}
		if (isEmoji == FALSE && isEmjtmp == TRUE)    /* 如果前面不是emoji而当前文字为emoji 则这是emoji字符串的开始 */
		{
			sc = i;
		}
		if (isEmoji == TRUE && isEmjtmp == FALSE)    /* 如果前面是emoji 而当前文字不是emoji，则这是emoji字符串的结尾 */
		{
			ec = i;
			rng.push_back(DWRITE_TEXT_RANGE{ sc, ec - sc });
		}
		isEmoji = isEmjtmp;
		if ((i == str.size() - sz) && isEmjtmp == TRUE)  /* 最后一个字符，刚好为emoji， 这是emoji字符串结尾*/
		{
			ec = i + sz;
			rng.push_back(DWRITE_TEXT_RANGE{ sc, ec - sc });
		}
		if (i < str.size())
		{
			i += sz;
			utf16 += sz;
		}
	}
	return rng;
}
// wstring中查找所有非emoji字符的range
static vector<DWRITE_TEXT_RANGE> CheckNotEmojiRange(wstring str)
{
	vector<DWRITE_TEXT_RANGE> rng;
	size_t i = 0;
	wchar_t* utf16 = &str[0];
	ULONG unicode = 0;
	UINT32 sc = 0, ec = 0;
	size_t sz = 0;
	BOOL isEmjtmp = FALSE;
	BOOL isEmoji = TRUE;
	while (i < str.size())
	{
		sz = Utf16ToUnicode(utf16, unicode);
		if ( (unicode >= 0x2000 && unicode <= 0x27bf) || (unicode >= 0x1f000 && unicode <= 0x1fbff) )
			isEmjtmp = TRUE;    /* 当前字符是emoji 字符长度为sz */
		else
			isEmjtmp = FALSE;
		if (isEmoji == TRUE && (*utf16 == 0x200d)) /* 如果前面的字符是Emoji 后面链接符号也认为是Emoji的一部分 */
		{
			isEmjtmp = TRUE;
			sz = 1;
		}
		if (isEmoji == TRUE && isEmjtmp == FALSE)    /* 如果前面不是emoji而当前文字不是emoji 则这是非emoji字符串的开始 */
		{
			sc = i;
		}
		if (isEmoji == FALSE && isEmjtmp == TRUE)    /* 如果前面是非emoji 而当前文字是emoji，则这是非emoji字符串的结尾 */
		{
			ec = i;
			rng.push_back(DWRITE_TEXT_RANGE{ sc, ec - sc });
		}
		isEmoji = isEmjtmp;
		if ((i == str.size() - sz) && isEmjtmp == FALSE)  /* 最后一个字符，刚好不是emoji， 这是非emoji字符串结尾*/
		{
			ec = i + sz;
			rng.push_back(DWRITE_TEXT_RANGE{ sc, ec - sc });
		}
		if (i < str.size())
		{
			i += sz;
			utf16 += sz;
		}
	}
	return rng;
}
static HRESULT _TextOutWithFallback_D2D 
(
		CDCHandle dc, 
		CRect const rc, wstring psz, int cch, int font_point, float scaleX, float scaleY, 
		COLORREF gdiColor,
		std::wstring fontface, 
		ID2D1DCRenderTarget* pRenderTarget,
		IDWriteFactory* pDWFactory)
{
	float r = (float)(GetRValue(gdiColor))/255.0f;
	float g = (float)(GetGValue(gdiColor))/255.0f;
	float b = (float)(GetBValue(gdiColor))/255.0f;

	// alpha 
	float alpha = (float)((gdiColor >> 24) & 255) / 255.0f;
	ID2D1SolidColorBrush* pBrush = NULL;
	pRenderTarget->CreateSolidColorBrush( D2D1::ColorF(r,g,b,alpha), &pBrush);
	// create text format
	IDWriteTextFormat* pTextFormat = NULL;
	pDWFactory->CreateTextFormat(
		fontface.c_str(), NULL,
		DWRITE_FONT_WEIGHT_NORMAL, 
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		font_point * scaleX / 72.0f, L"", &pTextFormat);
	pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
	pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
	pTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

	if (NULL != pBrush && NULL != pTextFormat)
	{
		IDWriteTextLayout* pTextLayout = NULL;
		pDWFactory->CreateTextLayout( ((wstring)psz).c_str(), ((wstring)psz).size(), pTextFormat, 0, 0, &pTextLayout);
		vector<DWRITE_TEXT_RANGE> rng;
		rng = CheckEmojiRange(psz);
		for (auto r : rng)
			pTextLayout->SetFontSize(font_point * scaleX / 96.0f , r);
		DWRITE_TEXT_METRICS txtMetrics;
		pTextLayout->GetMetrics(&txtMetrics);
		D2D1_SIZE_F size;
		size = D2D1::SizeF(ceil(txtMetrics.widthIncludingTrailingWhitespace), ceil(txtMetrics.height));
		// maybe bug here in the future
		CRect rect(rc.left, rc.top, rc.left + max(size.width, rc.Width()), rc.top + max(size.height, rc.Height()));
		pTextLayout->SetMaxWidth(rect.Width());
		pTextLayout->SetMaxHeight(rect.Height());
		// offset calc start
		IDWriteFontCollection* collection;
		WCHAR name[64];
		UINT32 findex;
		BOOL exists;
		pTextFormat->GetFontFamilyName(name, 64);
		pTextFormat->GetFontCollection(&collection);
		collection->FindFamilyName(name, &findex, &exists);
		IDWriteFontFamily* ffamily;
		collection->GetFontFamily(findex, &ffamily);
		IDWriteFont* font;
		ffamily->GetFirstMatchingFont(pTextFormat->GetFontWeight(), pTextFormat->GetFontStretch(), pTextFormat->GetFontStyle(), &font);
		DWRITE_FONT_METRICS metrics;
		font->GetMetrics(&metrics);
		float ratio = pTextFormat->GetFontSize() / (float)metrics.designUnitsPerEm;
		int offset = static_cast<int>((-metrics.strikethroughPosition + metrics.descent) * ratio);
		ffamily->Release();
		collection->Release();
		font->Release();
		// offset calc end
		pRenderTarget->BindDC(dc, &rect);
		pRenderTarget->BeginDraw();
		if(pTextLayout != NULL)
			pRenderTarget->DrawTextLayout({ 0.0f, 0.0f + offset}, pTextLayout, pBrush, D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
		pRenderTarget->EndDraw();
	}
	pTextFormat->Release();
	pBrush->Release();
	return S_OK;
}
static std::vector<std::wstring> ws_split(const std::wstring& in, const std::wstring& delim) 
{
    std::wregex re{ delim };
    return std::vector<std::wstring> {
        std::wsregex_token_iterator(in.begin(), in.end(), re, -1),
            std::wsregex_token_iterator()
    };
}
void WeaselPanel::_TextOut(CDCHandle dc, int x, int y, CRect const& rc, LPCWSTR psz, int cch)
{
	long height = -MulDiv(m_style.font_point, dc.GetDeviceCaps(LOGPIXELSY), 72);
	if (_isVistaSp2OrGrater && m_style.color_font )
	{
		std::vector<DWRITE_TEXT_RANGE> rgn = CheckEmojiRange(psz);
		if (rgn.size())
			_TextOutWithFallback_D2D(dc, rc, psz, cch, m_style.font_point, dpiScaleX_, dpiScaleY_,
				dc.GetTextColor(), m_style.font_face, pRenderTarget, pDWFactory);
		else
			goto GDI_TextOut;
	}
	else
	{ 
	GDI_TextOut:
		std::vector<std::wstring> lines;
		lines = ws_split(psz, L"\r");
		int offset = 0;
		for (wstring line : lines)
		{
			CSize size;
			TEXTMETRIC tm;
			dc.GetTextExtent(line.c_str(), line.length(), &size);
			if (FAILED(_TextOutWithFallback_ULW(dc, x, y+offset, rc, line.c_str(), line.length(), height, m_style.font_face ))) 
			{
				CFont font;
				font.CreateFontW(height, 0, 0, 0, 0, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, m_style.font_face.c_str());

				HBITMAP MyBMP = _CreateAlphaTextBitmap(psz, font, dc.GetTextColor(), cch);
				DeleteObject(font);
				if (MyBMP)
				{
					BYTE alpha = (BYTE)((dc.GetTextColor() >> 24) & 255) ;
					// temporary dc select bmp into it
					HDC hTempDC = CreateCompatibleDC(dc);
					HBITMAP hOldBMP = (HBITMAP)SelectObject(hTempDC, MyBMP);
					if (hOldBMP)
					{
						BITMAP BMInf;
						GetObject(MyBMP, sizeof(BITMAP), &BMInf);
						// fill blend function and blend new text to window
						BLENDFUNCTION bf;
						bf.BlendOp = AC_SRC_OVER;
						bf.BlendFlags = 0;
						bf.SourceConstantAlpha = alpha;
						bf.AlphaFormat = AC_SRC_ALPHA;
						AlphaBlend(dc, x, y, BMInf.bmWidth, BMInf.bmHeight, hTempDC, 0, 0, BMInf.bmWidth, BMInf.bmHeight, bf);
						// clean up
						SelectObject(hTempDC, hOldBMP);
						DeleteObject(MyBMP);
						DeleteDC(hTempDC);
					}
				}
			}
			offset += size.cy;
		}
	}
}

GraphicsRoundRectPath::GraphicsRoundRectPath(void) : Gdiplus::GraphicsPath()
{

}
GraphicsRoundRectPath::GraphicsRoundRectPath(int left, int top, int width, int height, int cornerx, int cornery) 
	: Gdiplus::GraphicsPath()
{
	AddRoundRect(left, top, width, height, cornerx, cornery);
}

void GraphicsRoundRectPath::AddRoundRect(int left, int top, int width, int height, int cornerx, int cornery)
{
	if(cornery > 0 && cornerx >0)
	{
		int cnx = ((cornerx*2 <= width) ? cornerx : (width/2));
		int cny = ((cornery*2 <= height) ? cornery : (height/2));
		int elWid = 2 * cnx;
		int elHei = 2 * cny;

		AddArc(left, top, elWid, elHei, 180, 90);
		AddLine(left + cnx , top, left + width - cnx , top);

		AddArc(left + width - elWid, top, elWid, elHei, 270, 90);
		AddLine(left + width, top + cny, left + width, top + height - cny);

		AddArc(left + width - elWid, top + height - elHei, elWid, elHei, 0, 90);
		AddLine(left + width - cnx, top + height, left + cnx, top + height);

		AddArc(left, top + height - elHei, elWid, elHei, 90, 90);
		AddLine(left, top + cny, left, top + height - cny);
	}
	else
	{
		Gdiplus::Rect& rc = Rect(left, top, width, height);
		AddRectangle(rc);
	}
}
