#pragma once
#include <WeaselCommon.h>
#include <WeaselUI.h>
#include "StandardLayout.h"
#include "Layout.h"
#include "GdiplusBlur.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

using namespace weasel;

typedef CWinTraits<WS_POPUP|WS_CLIPSIBLINGS|WS_DISABLED, WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE  | WS_EX_LAYERED> CWeaselPanelTraits;

enum class BackType
{
	TEXT = 0,
	CAND = 1,
	BACKGROUND = 2	// background
};

class WeaselPanel : 
	public CWindowImpl<WeaselPanel, CWindow, CWeaselPanelTraits>,
	CDoubleBufferImpl<WeaselPanel>
{
public:
	BEGIN_MSG_MAP(WeaselPanel)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_DPICHANGED, OnDpiChanged)
#ifdef USE_MOUSE_EVENTS
		MESSAGE_HANDLER(WM_MOUSEACTIVATE, OnMouseActivate)
		MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLeftClicked)
		MESSAGE_HANDLER(WM_MOUSEWHEEL, OnMouseWheel)
#ifdef USE_MOUSE_HOVER
		MESSAGE_HANDLER(WM_MOUSEHOVER, OnMouseHover)
		MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
		MESSAGE_HANDLER(WM_MOUSELEAVE, OnMouseLeave)
#endif /*  USE_MOUSE_HOVER */
#endif /* USE_MOUSE_EVENTS */
		CHAIN_MSG_MAP(CDoubleBufferImpl<WeaselPanel>)
	END_MSG_MAP()

	LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) { m_osize = {0,0}; return 0; };
	LRESULT OnDpiChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
#ifdef USE_MOUSE_EVENTS
	LRESULT OnMouseActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnLeftClicked(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnMouseWheel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
#ifdef USE_MOUSE_HOVER
	LRESULT OnMouseHover(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnMouseLeave(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
#endif /*  USE_MOUSE_HOVER */
#endif	/* USE_MOUSE_EVENTS */

	WeaselPanel(weasel::UI &ui);
	~WeaselPanel();

	void MoveTo(RECT const& rc);
	void Refresh();
	void DoPaint(CDCHandle dc);
	void CleanUp();

private:
	void _InitFontRes(void);
#ifdef USE_MOUSE_EVENTS
	void _CaptureRect(CRect& rect);
#ifdef USE_MOUSE_HOVER
	bool m_mouse_entry = false;
#endif /*  USE_MOUSE_HOVER */
#endif	/* USE_MOUSE_EVENTS */
	void _CreateLayout();
	void _ResizeWindow();
	void _RepositionWindow(bool adj = false);
	bool _DrawPreedit(weasel::Text const& text, CDCHandle dc, CRect const& rc);
	bool _DrawPreeditBack(weasel::Text const& text, CDCHandle dc, CRect const& rc);
	bool _DrawCandidates(CDCHandle &dc, bool back = false);
	void _HighlightText(CDCHandle &dc, CRect rc, COLORREF color, COLORREF shadowColor, int radius, BackType type, IsToRoundStruct rd, COLORREF bordercolor);
	void _TextOut(CRect const& rc, std::wstring psz, size_t cch, int inColor, IDWriteTextFormat* pTextFormat = NULL);

	void _LayerUpdate(const CRect& rc, CDCHandle dc);

	weasel::Layout *m_layout;
	weasel::Context &m_ctx;
	weasel::Context &m_octx;
	weasel::Status &m_status;
	weasel::UIStyle &m_style;
	weasel::UIStyle &m_ostyle;

	CRect m_inputPos;
	CRect m_oinputPos;
	int  m_offsetys[MAX_CANDIDATES_COUNT];	// offset y for candidates when vertical layout over bottom
	int  m_offsety_preedit;
	int  m_offsety_aux;
	bool m_istorepos;
	CSize m_size;
	CSize m_osize;

	CIcon m_iconDisabled;
	CIcon m_iconEnabled;
	CIcon m_iconAlpha;
	CIcon m_iconFull;
	CIcon m_iconHalf;
	std::wstring m_current_zhung_icon;
	std::wstring m_current_ascii_icon;
	// for gdiplus drawings
	Gdiplus::GdiplusStartupInput _m_gdiplusStartupInput;
	ULONG_PTR _m_gdiplusToken;

	UINT dpi;

	CRect rcw;
	BYTE m_candidateCount;

	bool hide_candidates;
	// for multi font_face & font_point
	GdiplusBlur* m_blurer;
	DirectWriteResources* pDWR;
	ID2D1SolidColorBrush* pBrush;
};

