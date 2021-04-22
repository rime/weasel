#include "stdafx.h"
#include "WeaselTrayIcon.h"

// nasty
#include <resource.h>

static UINT mode_icon[] = { IDI_ZH, IDI_ZH, IDI_EN, IDI_RELOAD };
static const WCHAR *mode_label[] = { NULL, /*L"中文"*/ NULL, /*L"西文"*/ NULL, L"維護中" };

WeaselTrayIcon::WeaselTrayIcon(weasel::UI &ui)
	: m_style(ui.style()), m_status(ui.status()), m_mode(INITIAL)
{
}

void WeaselTrayIcon::CustomizeMenu(HMENU hMenu)
{
}

BOOL WeaselTrayIcon::Create(HWND hTargetWnd)
{
	HMODULE hModule = GetModuleHandle(NULL);
	CIcon icon;
	icon.LoadIconW(IDI_ZH);
	BOOL bRet = CSystemTray::Create(hModule, NULL, WM_WEASEL_TRAY_NOTIFY, 
		WEASEL_IME_NAME, icon, IDR_MENU_POPUP,
		TRUE); 
	if (hTargetWnd)
	{
		SetTargetWnd(hTargetWnd);
	}
	if (!m_style.display_tray_icon)
	{
		RemoveIcon();
	}
	return bRet;
}

void WeaselTrayIcon::Refresh(int state)
{
	if (!m_style.display_tray_icon && !m_status.disabled) // display notification when deploying
	{
		if (m_mode != INITIAL)
		{
			RemoveIcon();
			m_mode = INITIAL;
		}
		return;
	}
#ifdef _DEBUG
	bool bTsf = false;
	bool bAddSession = false;
	static int current = 0;
	// state==0 ?,other state ?
	if (state) 
	{
		bTsf = (state & (1 << 1)) != 0;
		bAddSession = (state & (1 << 2)) != 0;

		if (!bTsf && bAddSession)		 //ime enter
			current = 1;
		else if (bTsf && bAddSession)	 //tsf enter
			current = 2;
		else if (bTsf && !bAddSession)	 //tsf leave
			current = 3;
		else if (!bTsf && !bAddSession)	 //ime leave
			current = 4;

		char buf[128] = { 0 };
		sprintf(buf, "tsf:%d addsession:%d current:%d\r\n", bTsf, bAddSession,current);
		OutputDebugStringA(buf);
	}
#endif
	WeaselTrayMode mode = m_status.disabled ? DISABLED : 
		m_status.ascii_mode ? ASCII : ZHUNG;
	if (mode != m_mode )
	{
		m_mode = mode;
		ShowIcon();
		SetIcon(mode_icon[mode]);
		if (mode_label[mode])
		{
			ShowBalloon(mode_label[mode], WEASEL_IME_NAME);
		}
	}
	else if (!Visible())
	{
		ShowIcon();
	}
}
