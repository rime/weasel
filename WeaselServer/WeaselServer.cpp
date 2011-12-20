// WeaselServer.cpp : main source file for WeaselServer.exe
//
//	WTL MessageLoop 封装了消息循环. 实现了 getmessage/dispatchmessage....

#include "stdafx.h"
#include "resource.h"
#include "WeaselTrayIcon.h"
#include <WeaselIPC.h>
#include <RimeWithWeasel.h>
#include <winsparkle.h>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>

CAppModule _Module;

static bool execute(const std::wstring &cmd, const std::wstring &args)
{
	return (int)ShellExecuteW(NULL, NULL, cmd.c_str(), args.c_str(), NULL, SW_HIDE) > 32;
}

static bool explore(const std::wstring &path)
{
	return (int)ShellExecuteW(NULL, L"explore", path.c_str(), NULL, NULL, SW_SHOWNORMAL) > 32;
}

static bool open(const std::wstring &path)
{
	return (int)ShellExecuteW(NULL, L"open", path.c_str(), NULL, NULL, SW_SHOWNORMAL) > 32;
}

static bool check_update()
{
	win_sparkle_check_update_with_ui();
	return true;
}

static void setup_menu_handlers(weasel::Server &server)
{
	WCHAR exe_path[MAX_PATH] = {0};
	WCHAR user_config_path[MAX_PATH] = {0};
	GetModuleFileNameW(GetModuleHandle(NULL), exe_path, _countof(exe_path));
	ExpandEnvironmentStringsW(L"%AppData%\\Rime", user_config_path, _countof(user_config_path));
	std::wstring install_dir(exe_path);
	size_t pos = install_dir.find_last_of(L"\\");
	install_dir.resize(pos);
	server.AddMenuHandler(ID_WEASELTRAY_QUIT, boost::lambda::bind(&weasel::Server::Stop, boost::ref(server)) == 0);
	server.AddMenuHandler(ID_WEASELTRAY_RELOAD, boost::lambda::bind(&execute, std::wstring(exe_path), std::wstring(L"/restart")));
	server.AddMenuHandler(ID_WEASELTRAY_DEPLOY, boost::lambda::bind(&execute, install_dir + L"\\WeaselDeploy.exe", std::wstring()));
	server.AddMenuHandler(ID_WEASELTRAY_HOMEPAGE, boost::lambda::bind(&open, L"http://code.google.com/p/rimeime/"));
	server.AddMenuHandler(ID_WEASELTRAY_CHECKUPDATE, check_update);
	server.AddMenuHandler(ID_WEASELTRAY_INSTALLDIR, boost::lambda::bind(&explore, install_dir));
	server.AddMenuHandler(ID_WEASELTRAY_USERCONFIG, boost::lambda::bind(&explore, std::wstring(user_config_path)));
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{
	// 防止服务进程开启输入法
	ImmDisableIME(-1);

	HRESULT hRes = ::CoInitialize(NULL);
	// If you are running on NT 4.0 or higher you can use the following call instead to 
	// make the EXE free threaded. This means that calls come in on a random RPC thread.
	//HRESULT hRes = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
	ATLASSERT(SUCCEEDED(hRes));

	// this resolves ATL window thunking problem when Microsoft Layer for Unicode (MSLU) is used
	::DefWindowProc(NULL, 0, 0, 0L);

	AtlInitCommonControls(ICC_BAR_CLASSES);	// add flags to support other controls

	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	// command line option /q stops the running server
	bool quit = !wcscmp(L"/q", lpstrCmdLine) || !wcscmp(L"/quit", lpstrCmdLine);
	bool restart = !wcscmp(L"/restart", lpstrCmdLine);
	if (quit || restart)
	{
		weasel::Client client;
		if (client.Connect())
			client.ShutdownServer();
		if (quit)
			return 0;
	}

	int nRet = 0;
	try
	{
		weasel::Server server(new RimeWithWeaselHandler);
		if (!server.Start())
			return -1;

		//win_sparkle_set_appcast_url("http://localhost:8000/weasel/update/appcast.xml");
		win_sparkle_set_registry_path("Software\\Rime\\Weasel\\Updates");
		win_sparkle_init();

		WeaselTrayIcon tray_icon;
		tray_icon.AttachTo(server);
		setup_menu_handlers(server);

		nRet = server.Run();

		tray_icon.RemoveIcon();
	}
	catch (...)
	{
		// bad luck...
		nRet = -1;
	}

	win_sparkle_cleanup();

	_Module.Term();
	::CoUninitialize();

	return nRet;
}
