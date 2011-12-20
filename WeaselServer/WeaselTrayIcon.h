#pragma once
#include <WeaselIPC.h>
#include "SystemTraySDK.h"

#define	WM_WEASEL_TRAY_NOTIFY (WEASEL_IPC_LAST_COMMAND + 100)


class WeaselTrayIcon : public CSystemTray
{
public:
	WeaselTrayIcon();

	void AttachTo(weasel::Server &server);

protected:
	virtual void CustomizeMenu(HMENU hMenu);
};

