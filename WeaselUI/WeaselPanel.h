#pragma once
#include <WeaselCommon.h>
#include <WeaselUI.h>

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
static const COLORREF BACK_COLOR                  = 0xffffff;
static const COLORREF BORDER_COLOR                = 0x000000;
static const COLORREF HIGHLIGHTED_TEXT_COLOR      = 0x000000;
static const COLORREF HIGHLIGHTED_BACK_COLOR      = 0x7fffff;
static const COLORREF HIGHLIGHTED_CAND_TEXT_COLOR = 0xffffff;
static const COLORREF HIGHLIGHTED_CAND_BACK_COLOR = 0x000000;

static WCHAR CANDIDATE_PROMPT_PATTERN[] = L"%1%. %2%";

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

	WeaselPanel();
	void SetContext(const weasel::Context &ctx);
	void SetStatus(const weasel::Status &status);
	void MoveTo(RECT const& rc);
	void Refresh();

	void DoPaint(CDCHandle dc);

	weasel::UIStyle* GetStyle() { return &m_style; }

private:
	void _ResizeWindow();
	void _RepositionWindow();
	bool _DrawText(weasel::Text const& text, CDCHandle dc, CRect const& rc, int& y);
	bool _DrawCandidates(weasel::CandidateInfo const& cinfo, CDCHandle dc, CRect const& rc, int& y);
	void _HighlightText(CDCHandle dc, CRect rc, COLORREF color);

	wstring m_fontFace;
	int m_fontPoint;
	CRect m_inputPos;
	weasel::Context m_ctx;
	weasel::Status m_status;
	weasel::UIStyle m_style;
};
