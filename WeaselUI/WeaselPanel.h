#pragma once
#include <WeaselCommon.h>
#include <WeaselUI.h>

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

	void MoveTo(RECT const& rc);
	void Refresh();

	void DoPaint(CDCHandle dc);

private:
	void _ResizeWindow();
	void _RepositionWindow();
	bool _DrawText(weasel::Text const& text, CDCHandle dc, CRect const& rc, int& y);
	bool _DrawCandidates(weasel::CandidateInfo const& cinfo, CDCHandle dc, CRect const& rc, int& y);
	void _HighlightText(CDCHandle dc, CRect rc, COLORREF color);

	weasel::Context &m_ctx;
	weasel::Status &m_status;
	weasel::UIStyle &m_style;

	CRect m_inputPos;
	CIcon m_iconZhung;
	CIcon m_iconAlpha;
};
