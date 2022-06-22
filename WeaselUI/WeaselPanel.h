#pragma once
#include <WeaselCommon.h>
#include <WeaselUI.h>
#include "Layout.h"
#include <Usp10.h>

#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
typedef CWinTraits<WS_POPUP|WS_CLIPSIBLINGS|WS_DISABLED, WS_EX_TOOLWINDOW|WS_EX_TOPMOST> CWeaselPanelTraits;

class WeaselPanel : 
	public CWindowImpl<WeaselPanel, CWindow, CWeaselPanelTraits>,
	CDoubleBufferImpl<WeaselPanel>
{
public:
	BEGIN_MSG_MAP(WeaselPanel)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		CHAIN_MSG_MAP(CDoubleBufferImpl<WeaselPanel>)
	END_MSG_MAP()

	LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	void CloseDialog(int nVal);

	WeaselPanel(weasel::UI &ui);
	~WeaselPanel();

	void MoveTo(RECT const& rc);
	void Refresh();

	void DoPaint(CDCHandle dc);

private:
	void _CreateLayout();
	void _ResizeWindow();
	void _RepositionWindow();
	bool _DrawPreedit(weasel::Text const& text, CDCHandle dc, CRect const& rc);
	bool _DrawCandidates(CDCHandle dc);
	void _HighlightText(CDCHandle dc, CRect rc, COLORREF color);
	void _HighlightTextEx(CDCHandle dc, CRect rc, COLORREF color, COLORREF shadowColor, int blurOffsetX, int blurOffsetY, int radius );
	void _TextOut(CDCHandle dc, int x, int y, CRect const& rc, LPCWSTR psz, int cch);
	HBITMAP _CreateAlphaTextBitmap(LPCWSTR inText, HFONT inFont, COLORREF inColor, int cch);
	HRESULT _TextOutWithFallback_D2D(CDCHandle dc, CRect const rc, std::wstring psz, int cch, int font_point, COLORREF gdiColor, std::wstring fontface);

	weasel::Layout *m_layout;
	weasel::Context &m_ctx;
	weasel::Status &m_status;
	weasel::UIStyle &m_style;

	CRect m_inputPos;
	CIcon m_iconDisabled;
	CIcon m_iconEnabled;
	CIcon m_iconAlpha;

	Gdiplus::GdiplusStartupInput _m_gdiplusStartupInput;
	ULONG_PTR _m_gdiplusToken;
	bool _isVistaSp2OrGrater;
	
	float dpiScaleX_, dpiScaleY_;
	// variable for d2d dwrite
	ID2D1Factory* pD2d1Factory = NULL;
	IDWriteFactory* pDWFactory = NULL;
	ID2D1DCRenderTarget* pRenderTarget = NULL;
	IDWriteTextFormat* pTextFormat = NULL;
	DirectWriteResources* pDWR = NULL;
};

class GraphicsRoundRectPath : public Gdiplus::GraphicsPath
{
public:
	GraphicsRoundRectPath();
	GraphicsRoundRectPath(int left, int top, int width, int height, int cornerx, int cornery);
	GraphicsRoundRectPath(const CRect rc, int corner);

public:
	void AddRoundRect(int left, int top, int width, int height, int cornerx, int cornery);
};
