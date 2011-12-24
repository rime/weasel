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
		pimpl_->panel.ShowWindow(SW_SHOWNA);
}

void UI::Hide()
{
	if (pimpl_)
		pimpl_->panel.ShowWindow(SW_HIDE);

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
