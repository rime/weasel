#pragma once
#include <WeaselCommon.h>
#include <WeaselUI.h>
#include <Usp10.h>
#include "Layout.h"
#include "GdiplusBlur.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")


using namespace weasel;
typedef enum _WINDOWCOMPOSITIONATTRIB
{
	WCA_UNDEFINED = 0,
	WCA_NCRENDERING_ENABLED = 1,
	WCA_NCRENDERING_POLICY = 2,
	WCA_TRANSITIONS_FORCEDISABLED = 3,
	WCA_ALLOW_NCPAINT = 4,
	WCA_CAPTION_BUTTON_BOUNDS = 5,
	WCA_NONCLIENT_RTL_LAYOUT = 6,
	WCA_FORCE_ICONIC_REPRESENTATION = 7,
	WCA_EXTENDED_FRAME_BOUNDS = 8,
	WCA_HAS_ICONIC_BITMAP = 9,
	WCA_THEME_ATTRIBUTES = 10,
	WCA_NCRENDERING_EXILED = 11,
	WCA_NCADORNMENTINFO = 12,
	WCA_EXCLUDED_FROM_LIVEPREVIEW = 13,
	WCA_VIDEO_OVERLAY_ACTIVE = 14,
	WCA_FORCE_ACTIVEWINDOW_APPEARANCE = 15,
	WCA_DISALLOW_PEEK = 16,
	WCA_CLOAK = 17,
	WCA_CLOAKED = 18,
	WCA_ACCENT_POLICY = 19,
	WCA_FREEZE_REPRESENTATION = 20,
	WCA_EVER_UNCLOAKED = 21,
	WCA_VISUAL_OWNER = 22,
	WCA_HOLOGRAPHIC = 23,
	WCA_EXCLUDED_FROM_DDA = 24,
	WCA_PASSIVEUPDATEMODE = 25,
	WCA_USEDARKMODECOLORS = 26,
	WCA_CORNER_STYLE = 27,
	WCA_PART_COLOR = 28,
	WCA_DISABLE_MOVESIZE_FEEDBACK = 29,
	WCA_LAST = 30
} WINDOWCOMPOSITIONATTRIB;

typedef struct _WINDOWCOMPOSITIONATTRIBDATA
{
	WINDOWCOMPOSITIONATTRIB Attrib;
	PVOID pvData;
	SIZE_T cbData;
} WINDOWCOMPOSITIONATTRIBDATA;

typedef enum _ACCENT_STATE
{
	ACCENT_DISABLED = 0,
	ACCENT_ENABLE_GRADIENT = 1,
	ACCENT_ENABLE_TRANSPARENTGRADIENT = 2,
	ACCENT_ENABLE_BLURBEHIND = 3,
	ACCENT_ENABLE_ACRYLICBLURBEHIND = 4, // RS4 1803
	ACCENT_ENABLE_HOSTBACKDROP = 5, // RS5 1809
 	ACCENT_INVALID_STATE = 6
} ACCENT_STATE;
typedef struct _ACCENT_POLICY
{
	ACCENT_STATE AccentState;
	DWORD AccentFlags;
	DWORD GradientColor;
	DWORD AnimationId;
} ACCENT_POLICY;
typedef BOOL (WINAPI *pfnGetWindowCompositionAttribute)(HWND, WINDOWCOMPOSITIONATTRIBDATA*);
typedef BOOL (WINAPI *pfnSetWindowCompositionAttribute)(HWND, WINDOWCOMPOSITIONATTRIBDATA*);

typedef CWinTraits<WS_POPUP|WS_CLIPSIBLINGS|WS_DISABLED, WS_EX_TOOLWINDOW|WS_EX_TOPMOST> CWeaselPanelTraits;

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
#ifdef USE_CAPTURE_BY_CLICK
		MESSAGE_HANDLER(WM_MOUSEACTIVATE, OnActivate)
		MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLeftClicked)
		MESSAGE_HANDLER(WM_MOUSEHOVER, OnMouseHover)
		MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
		MESSAGE_HANDLER(WM_MOUSELEAVE, OnMouseLeave)
#endif
		CHAIN_MSG_MAP(CDoubleBufferImpl<WeaselPanel>)
	END_MSG_MAP()

	LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) { return 0; };
	LRESULT OnDpiChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
#ifdef USE_CAPTURE_BY_CLICK
	LRESULT OnActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnLeftClicked(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnMouseHover(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnMouseLeave(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
#endif

	WeaselPanel(weasel::UI &ui);
	~WeaselPanel();

	void MoveTo(RECT const& rc);
	void Refresh();
	void InitFontRes(void);
	void DoPaint(CDCHandle dc);
	void CleanUp();

private:
#ifdef USE_CAPTURE_BY_CLICK
	void _CaptureRect(CRect& rect);
	bool m_mouse_entry = false;
#endif
	void _CreateLayout();
	void _ResizeWindow();
	void _RepositionWindow(bool adj = false);
	bool _DrawPreedit(weasel::Text const& text, CDCHandle dc, CRect const& rc);
	bool _DrawPreeditBack(weasel::Text const& text, CDCHandle dc, CRect const& rc);
	bool _DrawCandidates(CDCHandle &dc, bool back = false);
	void _HighlightText(CDCHandle &dc, CRect rc, COLORREF color, COLORREF shadowColor, int radius, BackType type, IsToRoundStruct rd, COLORREF bordercolor);
	void _TextOut(CRect const& rc, std::wstring psz, size_t cch, int inColor, IDWriteTextFormat* pTextFormat = NULL);

	void _LayerUpdate(const CRect& rc, CDCHandle dc);

#ifdef USE_BLUR_UNDER_WINDOWS10
	void _BlurBacktround(CRect& rc);
#endif

	weasel::Layout *m_layout;
	weasel::Context &m_ctx;
	weasel::Status &m_status;
	weasel::UIStyle &m_style;
	weasel::UIStyle &m_ostyle;

	CRect m_inputPos;
	CRect m_oinputPos;
	CSize m_osize;

	CIcon m_iconDisabled;
	CIcon m_iconEnabled;
	CIcon m_iconAlpha;
	CIcon m_iconFull;
	CIcon m_iconHalf;
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
#ifdef USE_BLUR_UNDER_WINDOWS10
	// for blur window
	HMODULE& hUser;
	pfnSetWindowCompositionAttribute setWindowCompositionAttribute;
	ACCENT_POLICY accent;
	WINDOWCOMPOSITIONATTRIBDATA data;
	bool m_isBlurAvailable;
#endif
};

