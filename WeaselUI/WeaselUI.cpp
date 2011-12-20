#include "stdafx.h"
#include <WeaselUI.h>
#include "WeaselPanel.h"

using namespace weasel;

class weasel::UIImpl : public WeaselPanel {};

bool UI::Create(HWND parent)
{
	if (pimpl_)
		return true;

	pimpl_ = new UIImpl();
	if (!pimpl_)
		return false;

	pimpl_->Create(parent);
	return true;
}

void UI::Destroy()
{
	if (pimpl_)
	{
		pimpl_->DestroyWindow();
		delete pimpl_;
		pimpl_ = 0;
	}
}

void UI::Show()
{
	if (pimpl_)
		pimpl_->ShowWindow(SW_SHOWNA);
}

void UI::Hide()
{
	if (pimpl_)
		pimpl_->ShowWindow(SW_HIDE);

}

UIStyle* UI::GetStyle() const
{
	if (pimpl_)
	{
		return pimpl_->GetStyle();
	}
	return NULL;
}

void UI::UpdateInputPosition(RECT const& rc)
{
	if (pimpl_)
	{
		pimpl_->MoveTo(rc);
	}
}

void UI::Refresh()
{
	if (pimpl_)
	{
		pimpl_->Refresh();
	}
}

void UI::Update(const weasel::Context &ctx, const weasel::Status &status)
{
	if (pimpl_)
	{
		pimpl_->SetContext(ctx);		
		pimpl_->SetStatus(status);
		pimpl_->Refresh();
	}
}
