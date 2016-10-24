// WeaselServer.cpp : main source file for WeaselServer.exe
//
//	WTL MessageLoop 封装了消息循环. 实现了 getmessage/dispatchmessage....

#include "stdafx.h"
#include "resource.h"
#include <WeaselIPC.h>
#include <WeaselUI.h>
#include <RimeWithWeasel.h>
#include <WeaselUtility.h>
#include <winsparkle.h>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/scoped_ptr.hpp>

CAppModule _Module;


class WeaselServerApp {
public:
	static bool execute(const std::wstring &cmd, const std::wstring &args)
	{
		return (int)ShellExecuteW(NULL, NULL, cmd.c_str(), args.c_str(), NULL, SW_SHOWNORMAL) > 32;
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
		WCHAR exe_path[MAX_PATH] = {0};
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
	boost::scoped_ptr<RimeWithWeaselHandler> m_handler;
};

WeaselServerApp::WeaselServerApp()
{
	m_handler.reset(new RimeWithWeaselHandler(&m_ui));
	m_server.SetRequestHandler(m_handler.get());
	SetupMenuHandlers();
}

WeaselServerApp::~WeaselServerApp()
{
}

int WeaselServerApp::Run()
{
	if (!m_server.Start())
		return -1;

	//win_sparkle_set_appcast_url("http://localhost:8000/weasel/update/appcast.xml");
	win_sparkle_set_registry_path("Software\\Rime\\Weasel\\Updates");
	win_sparkle_init();
	m_ui.Create(m_server.GetHWnd());
	m_handler->Initialize();

	int ret = m_server.Run();

	m_handler->Finalize();
	m_ui.Destroy();
	win_sparkle_cleanup();

	return ret;
}

void WeaselServerApp::SetupMenuHandlers()
{
	std::wstring dir(install_dir());
	m_server.AddMenuHandler(ID_WEASELTRAY_QUIT, boost::lambda::bind(&weasel::Server::Stop, std::ref(m_server)) == 0);
	m_server.AddMenuHandler(ID_WEASELTRAY_DEPLOY, boost::lambda::bind(&execute, dir + L"\\WeaselDeployer.exe", std::wstring(L"/deploy")));
	m_server.AddMenuHandler(ID_WEASELTRAY_SETTINGS, boost::lambda::bind(&execute, dir + L"\\WeaselDeployer.exe", std::wstring()));
	m_server.AddMenuHandler(ID_WEASELTRAY_DICT_MANAGEMENT, boost::lambda::bind(&execute, dir + L"\\WeaselDeployer.exe", std::wstring(L"/dict")));
	m_server.AddMenuHandler(ID_WEASELTRAY_SYNC, boost::lambda::bind(&execute, dir + L"\\WeaselDeployer.exe", std::wstring(L"/sync")));
	m_server.AddMenuHandler(ID_WEASELTRAY_WIKI, boost::lambda::bind(&open, L"https://github.com/rime/home/wiki/UserGuide"));
	m_server.AddMenuHandler(ID_WEASELTRAY_HOMEPAGE, boost::lambda::bind(&open, L"http://rime.github.io/"));
	m_server.AddMenuHandler(ID_WEASELTRAY_FORUM, boost::lambda::bind(&open, L"http://tieba.baidu.com/f?kw=rime"));
	m_server.AddMenuHandler(ID_WEASELTRAY_CHECKUPDATE, check_update);
	m_server.AddMenuHandler(ID_WEASELTRAY_INSTALLDIR, boost::lambda::bind(&explore, dir));
	m_server.AddMenuHandler(ID_WEASELTRAY_USERCONFIG, boost::lambda::bind(&explore, WeaselUserDataPath()));
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{
	// 防止服务进程开启输入法
	ImmDisableIME(-1);

	{
		WCHAR user_name[20] = {0};
		DWORD size = _countof(user_name);
		GetUserName(user_name, &size);
		if (!_wcsicmp(user_name, L"SYSTEM"))
		{
			return 1;
		}
	}

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

	if (!wcscmp(L"/userdir", lpstrCmdLine))
	{
		CreateDirectory(WeaselUserDataPath().c_str(), NULL);
		WeaselServerApp::explore(WeaselUserDataPath());
		return 0;
	}
	if (!wcscmp(L"/weaseldir", lpstrCmdLine))
	{
		WeaselServerApp::explore(WeaselServerApp::install_dir());
		return 0;
	}

	// command line option /q stops the running server
	bool quit = !wcscmp(L"/q", lpstrCmdLine) || !wcscmp(L"/quit", lpstrCmdLine);
	// restart if already running
	{
		weasel::Client client;
		if (client.Connect())  // try to connect to running server
		{
			client.ShutdownServer();
		}
		if (quit)
			return 0;
	}

	bool check_updates = !wcscmp(L"/update", lpstrCmdLine);
	if (check_updates)
	{
		WeaselServerApp::check_update();
	}

	CreateDirectory(WeaselUserDataPath().c_str(), NULL);

	int nRet = 0;
	try
	{
		WeaselServerApp app;
		nRet = app.Run();
	}
	catch (...)
	{
		// bad luck...
		nRet = -1;
	}

	_Module.Term();
	::CoUninitialize();

	return nRet;
}
