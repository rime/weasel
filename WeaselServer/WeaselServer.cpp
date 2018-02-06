// WeaselServer.cpp : main source file for WeaselServer.exe
//
//	WTL MessageLoop 封装了消息循环. 实现了 getmessage/dispatchmessage....

#include "stdafx.h"
#include "resource.h"
#include "WeaselService.h"
#include <WeaselIPC.h>
#include <WeaselUI.h>
#include <RimeWithWeasel.h>
#include <WeaselUtility.h>
#include <winsparkle.h>
#include <functional>
#include <memory>

CAppModule _Module;

typedef enum PROCESS_DPI_AWARENESS {
	PROCESS_DPI_UNAWARE = 0,
	PROCESS_SYSTEM_DPI_AWARE = 1,
	PROCESS_PER_MONITOR_DPI_AWARE = 2
} PROCESS_DPI_AWARENESS;

typedef enum MONITOR_DPI_TYPE {
	MDT_EFFECTIVE_DPI = 0,
	MDT_ANGULAR_DPI = 1,
	MDT_RAW_DPI = 2,
	MDT_DEFAULT = MDT_EFFECTIVE_DPI
} MONITOR_DPI_TYPE;

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{
	InitVersion();

	// Set DPI awareness (Windows 8.1+)
	if (IsWindows8Point1OrGreater())
	{
		using PSPDA = HRESULT (WINAPI *)(PROCESS_DPI_AWARENESS);
		HMODULE shcore_module = ::LoadLibrary(_T("shcore.dll"));
		if (shcore_module != NULL)
		{
			PSPDA SetProcessDpiAwareness = (PSPDA)::GetProcAddress(shcore_module, "SetProcessDpiAwareness");
			SetProcessDpiAwareness(PROCESS_SYSTEM_DPI_AWARE);
			::FreeLibrary(shcore_module);
		}
	}

	// 防止服务进程开启输入法
	ImmDisableIME(-1);

	WCHAR user_name[20] = {0};
	DWORD size = _countof(user_name);
	GetUserName(user_name, &size);
	if (!_wcsicmp(user_name, L"SYSTEM"))
	{
		return 1;
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
		if (IsWindowsVistaOrGreater())
		{
			PRAR RegisterApplicationRestart = (PRAR)::GetProcAddress(::GetModuleHandle(_T("kernel32.dll")), "RegisterApplicationRestart");
			RegisterApplicationRestart(NULL, 0);
		}
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
