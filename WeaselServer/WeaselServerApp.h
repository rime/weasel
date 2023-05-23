#pragma once

#include "resource.h"
#include <WeaselIPC.h>
#include <WeaselUI.h>
#include <RimeWithWeasel.h>
#include <WeaselUtility.h>
#include <winsparkle.h>
#include <functional>
#include <memory>

#include "WeaselTrayIcon.h"

class WeaselServerApp {
public:
	static bool execute(const std::wstring &cmd, const std::wstring &args)
	{
		return (int)ShellExecuteW(NULL, NULL, cmd.c_str(), args.c_str(), NULL, SW_SHOWNORMAL) > 32;
	}

	static bool explore(const std::wstring &path)
	{
		return (int)ShellExecuteW(NULL, L"open", L"explorer",  (L"\"" + path + L"\"").c_str(), NULL, SW_SHOWNORMAL) > 32;
	}

	static bool open(const std::wstring &path)
	{
		return (int)ShellExecuteW(NULL, L"open", path.c_str(), NULL, NULL, SW_SHOWNORMAL) > 32;
	}

	static bool check_update()
	{
		// when checked manually, show testing versions too
		std::string feed_url = GetCustomResource("ManualUpdateFeedURL", "APPCAST");
		if (!feed_url.empty())
		{
			win_sparkle_set_appcast_url(feed_url.c_str());
		}
		win_sparkle_check_update_with_ui();
		return true;
	}

	static std::wstring install_dir()
	{
		WCHAR exe_path[MAX_PATH] = { 0 };
		GetModuleFileNameW(GetModuleHandle(NULL), exe_path, _countof(exe_path));
		std::wstring dir(exe_path);
		size_t pos = dir.find_last_of(L"\\");
		dir.resize(pos);
		return dir;
	}

public:
	WeaselServerApp();
	~WeaselServerApp();
	int Run();

protected:
	void SetupMenuHandlers();

	weasel::Server m_server;
	weasel::UI m_ui;
	WeaselTrayIcon tray_icon;
	std::unique_ptr<RimeWithWeaselHandler> m_handler;
};
