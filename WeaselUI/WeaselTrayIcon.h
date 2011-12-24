#pragma once
#include <WeaselUI.h>
#include <WeaselIPC.h>
#include "SystemTraySDK.h"

#define	WM_WEASEL_TRAY_NOTIFY (WEASEL_IPC_LAST_COMMAND + 100)


class WeaselTrayIcon : public CSystemTray
{
public:
	enum WeaselTrayMode {
		ZHUNG, ASCII, DISABLED,
	};

	WeaselTrayIcon(weasel::UI &ui);

	BOOL Create(HWND hTargetWnd);
	void Refresh();

protected:
	virtual void CustomizeMenu(HMENU hMenu);

	weasel::Status &m_status;
	WeaselTrayMode m_mode;
};

