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
#include <ShellScalingApi.h>
#include <WinUser.h>
#include <memory>
#pragma comment(lib, "Shcore.lib")
CAppModule _Module;

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{
	if( !IsWindowsBlueOrLaterEx() )
	{
		::MessageBox(NULL, L"僅支持Windows 8.1或更高版本系統", L"系統版本過低", MB_ICONERROR);
		return 0;
	}
	SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

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
	if(quit)
	{
		weasel::Client client;
		if (client.Connect())  // try to connect to running server
		{
			client.ShutdownServer();
		}
		return 0;
	}

	bool check_updates = !wcscmp(L"/update", lpstrCmdLine);
	if (check_updates)
	{
		WeaselServerApp::check_update();
	}

	CreateDirectory(WeaselUserDataPath().c_str(), NULL);

	int nRet = 0;
	// named mutex to ensure only one instance running
    HANDLE hMutex = CreateMutex(NULL, FALSE, L"WeaselServerNamedMutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(hMutex);
		::MessageBox(NULL, L"已有算法服务實例正在運行", L"已有算法服务實例正在運行", MB_ICONINFORMATION);
		return 0;
	}
	try
	{
		WeaselServerApp app;
		RegisterApplicationRestart(NULL, 0);
		nRet = app.Run();
	}
	catch (...)
	{
		// bad luck...
		nRet = -1;
	}

	_Module.Term();
	::CoUninitialize();

	CloseHandle(hMutex);
	return nRet;
}
