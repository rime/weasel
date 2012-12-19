#include "stdafx.h"
#include <WeaselUI.h>
#include "WeaselPanel.h"
#include "WeaselTrayIcon.h"

using namespace weasel;

class weasel::UIImpl {
public:
	WeaselPanel panel;
	WeaselTrayIcon tray_icon;

	UIImpl(weasel::UI &ui)
		: panel(ui), tray_icon(ui) {}

	void Refresh() {
		panel.Refresh();
		tray_icon.Refresh();
	}
	void Show();
	void Hide();
	void ShowWithTimeout(DWORD millisec);

	static VOID CALLBACK OnTimer(
		  _In_  HWND hwnd,
		  _In_  UINT uMsg,
		  _In_  UINT_PTR idEvent,
		  _In_  DWORD dwTime
	);
	static const int AUTOHIDE_TIMER = 20121220;
	static UINT_PTR timer;
};

bool UI::Create(HWND parent)
{
	if (pimpl_)
		return true;

	pimpl_ = new UIImpl(*this);
	if (!pimpl_)
		return false;

	pimpl_->panel.Create(NULL);
	pimpl_->tray_icon.Create(parent);
	return true;
}

void UI::Destroy()
{
	if (pimpl_)
	{
		pimpl_->tray_icon.RemoveIcon();
		pimpl_->panel.DestroyWindow();
		delete pimpl_;
		pimpl_ = 0;
	}
}

void UI::Show()
{
	if (pimpl_)
	{
		pimpl_->Show();
	}
}

void UI::Hide()
{
	if (pimpl_)
	{
		pimpl_->Hide();
	}
}

void UI::ShowWithTimeout(DWORD millisec)
{
	if (pimpl_)
	{
		pimpl_->ShowWithTimeout(millisec);
	}
}

bool UI::IsCountingDown() const
{
	return pimpl_ && pimpl_->timer != 0;
}

void UI::Refresh()
{
	if (pimpl_)
	{
		pimpl_->Refresh();
	}
}

void UI::UpdateInputPosition(RECT const& rc)
{
	if (pimpl_)
	{
		pimpl_->panel.MoveTo(rc);
	}
}

void UI::Update(const Context &ctx, const Status &status)
{
	ctx_ = ctx;
	status_ = status;
	Refresh();
}

UINT_PTR UIImpl::timer = 0;

void UIImpl::Show()
{
	panel.ShowWindow(SW_SHOWNA);
	if (timer)
	{
		KillTimer(panel.m_hWnd, AUTOHIDE_TIMER);
		timer = 0;
	}
}

void UIImpl::Hide()
{
	panel.ShowWindow(SW_HIDE);
	if (timer)
	{
		KillTimer(panel.m_hWnd, AUTOHIDE_TIMER);
		timer = 0;
	}
}

void UIImpl::ShowWithTimeout(DWORD millisec)
{
	DLOG(INFO) << "ShowWithTimeout: " << millisec;
	panel.ShowWindow(SW_SHOWNA);
	SetTimer(panel.m_hWnd, AUTOHIDE_TIMER, millisec, &UIImpl::OnTimer);
	timer = UINT_PTR(this);
}

VOID CALLBACK UIImpl::OnTimer(
  _In_  HWND hwnd,
  _In_  UINT uMsg,
  _In_  UINT_PTR idEvent,
  _In_  DWORD dwTime
)
{
	DLOG(INFO) << "OnTimer:";
	KillTimer(hwnd, idEvent);
	UIImpl* self = (UIImpl*)timer;
	timer = 0;
	if (self)
	{
		self->Hide();
	}
}
