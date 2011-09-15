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

	SetStyle(GetUIStyleSettings());
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

void UI::SetStyle(UIStyle const& style)
{
	if (pimpl_)
	{
		if (!style.fontFace.empty())
			pimpl_->SetFontFace(style.fontFace);
		if (style.fontPoint > 0)
			pimpl_->SetFontPoint(style.fontPoint);
	}
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

void UI::UpdateContext(const weasel::Context &ctx)
{
	if (pimpl_)
	{
		pimpl_->SetContext(ctx);		
	}
}

void UI::UpdateStatus(const weasel::Status &status)
{
	if (pimpl_)
	{
		pimpl_->SetStatus(status);
	}
}

UIStyle const UI::GetUIStyleSettings()
{
  UIStyle style;

  HKEY hKey;
  LSTATUS ret = RegOpenKey(HKEY_LOCAL_MACHINE, WEASEL_REG_KEY, &hKey);
  if (ret != ERROR_SUCCESS)
  {
    return style;
  }

  {
    WCHAR value[100];
    DWORD len = sizeof(value);
    DWORD type = REG_SZ;
    ret = RegQueryValueEx(hKey, L"FontFace", NULL, &type, (LPBYTE)value, &len);
    if (ret == ERROR_SUCCESS)
      style.fontFace = value;
  }

  {
    DWORD dword = 0;
    DWORD len = sizeof(dword);
    DWORD type = REG_DWORD;
    ret = RegQueryValueEx(hKey, L"FontPoint", NULL, &type, (LPBYTE)&dword, &len);
    if (ret == ERROR_SUCCESS)
      style.fontPoint = (int)dword;
  }

  RegCloseKey(hKey);
  return style;
}
