#pragma once
#include <WeaselCommon.h>

static LPCWSTR DEFAULT_FONT_FACE = L"Arial";
static const int DEFAULT_FONT_POINT = 12;

static const int MIN_WIDTH = 200;
static const int MIN_HEIGHT = 0;
static const int BORDER = 1;
static const int MARGIN_X = 12;
static const int MARGIN_Y = 10;
static const int SPACING = 9;
static const int CAND_SPACING = 3;
static const int HIGHLIGHT_PADDING = 0;
static const int ROUND_CORNER = 5;
static const COLORREF HIGHLIGHTED_TEXT_COLOR = RGB(255, 255, 128);
static const COLORREF HIGHLIGHTED_CAND_COLOR = RGB(0, 0, 0);

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

	WeaselPanel() : m_fontFace(DEFAULT_FONT_FACE), m_fontPoint(DEFAULT_FONT_POINT) {}
	~WeaselPanel() {}
	void SetContext(const weasel::Context &ctx);
	void SetStatus(const weasel::Status &status);
	void MoveTo(RECT const& rc);
	void DoPaint(CDCHandle dc);

	LPCWSTR GetFontFace() const { return m_fontFace.c_str(); }
	void SetFontFace(wstring const& fontFace) { m_fontFace = fontFace; }
	int GetFontPoint() const { return m_fontPoint; }
	void SetFontPoint(int fontPoint) { m_fontPoint = fontPoint; }

private:
	void _Refresh();
	void _ResizeWindow();
	void _RepositionWindow();
	bool _DrawText(weasel::Text const& text, CDCHandle dc, CRect const& rc, int& y);
	bool _DrawCandidates(weasel::CandidateInfo const& cinfo, CDCHandle dc, CRect const& rc, int& y);
	void _HighlightText(CDCHandle dc, CRect rc, COLORREF color, DWORD dwRop);

	wstring m_fontFace;
	int m_fontPoint;
	CRect m_inputPos;
	weasel::Context m_ctx;
	weasel::Status m_status;
};
