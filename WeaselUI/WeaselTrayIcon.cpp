#include "stdafx.h"
#include "WeaselTrayIcon.h"

// nasty
#include "../WeaselServer/resource.h"

static UINT mode_icon[] = { IDI_ZHUNG, IDI_ALPHA, IDI_ALPHA };
static const WCHAR *mode_label[] = { L"中文", L"西文", L"維護中" };

WeaselTrayIcon::WeaselTrayIcon(weasel::UI &ui)
	: m_status(ui.status()), m_mode(ZHUNG)
{
}

void WeaselTrayIcon::CustomizeMenu(HMENU hMenu)
{
}

BOOL WeaselTrayIcon::Create(HWND hTargetWnd)
{
	HMODULE hModule = GetModuleHandle(NULL);
	CIcon icon;
	icon.LoadIconW(IDI_ZHUNG);
	BOOL bRet = CSystemTray::Create(hModule, NULL, WM_WEASEL_TRAY_NOTIFY, 
		WEASEL_IME_NAME, icon, IDR_MENU_POPUP);
	if (hTargetWnd)
	{
		SetTargetWnd(hTargetWnd);
	}
	return bRet;
}

void WeaselTrayIcon::Refresh()
{
	WeaselTrayMode mode = m_status.disabled ? DISABLED : 
		m_status.ascii_mode ? ASCII : ZHUNG;

	if (mode != m_mode)
	{
		m_mode = mode;
		SetIcon(mode_icon[mode]);
		ShowBalloon(mode_label[mode], WEASEL_IME_NAME);
	}
}