#pragma once
#include <WeaselCommon.h>
#include <WeaselUI.h>
#include "Layout.h"
#include <Usp10.h>

#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

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
	void _TextOut(CDCHandle dc, int x, int y, CRect const& rc, LPCWSTR psz, int cch);
	HBITMAP _CreateAlphaTextBitmap(LPCWSTR inText, HFONT inFont, COLORREF inColor, int cch);
	HBITMAP _CreateAlphaTextBitmapSSO(LPCWSTR inText, HFONT inFont, COLORREF inColor, int cch, HRESULT* hr, SCRIPT_STRING_ANALYSIS* ssa, CRect const& rc);
	HRESULT _TextOutWithFallback_ULW(CDCHandle dc, int x, int y, CRect const& rc, LPCWSTR psz, int cch);

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
};

class GraphicsRoundRectPath : public Gdiplus::GraphicsPath
{
public:
	GraphicsRoundRectPath();
	GraphicsRoundRectPath(int left, int top, int width, int height, int cornerx, int cornery);

public:
	void AddRoundRect(int left, int top, int width, int height, int cornerx, int cornery);
};
