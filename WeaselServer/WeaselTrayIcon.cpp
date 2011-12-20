#include "stdafx.h"
#include "resource.h"
#include "WeaselTrayIcon.h"

WeaselTrayIcon::WeaselTrayIcon()
{
}

void WeaselTrayIcon::CustomizeMenu(HMENU hMenu)
{
}

void WeaselTrayIcon::AttachTo(weasel::Server &server)
{
	HMODULE hModule = GetModuleHandle(NULL);
	CIcon icon;
	icon.LoadIconW(IDI_ZHUNG);
	Create(hModule, NULL, WM_WEASEL_TRAY_NOTIFY, WEASEL_IME_NAME, icon, IDR_MENU_POPUP);
	SetTargetWnd(server.GetHWnd());
}
