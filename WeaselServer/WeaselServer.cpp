// WeaselServer.cpp : main source file for WeaselServer.exe
//
//	WTL MessageLoop 封装了消息循环. 实现了 getmessage/dispatchmessage....

#include "stdafx.h"
#include <WeaselIPC.h>
#include <RimingWeasel.h>

CAppModule _Module;

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
		weasel::Server server(new RimingWeaselHandler);
		if (!server.Start())
			return -1;
		nRet = server.Run();
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
