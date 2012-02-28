// WeaselDeployer.cpp : Defines the entry point for the application.
//
#include "stdafx.h"
#include "WeaselDeployer.h"
#include "Configurator.h"

const std::string WeaselDeployerLogFilePath()
{
	char path[MAX_PATH] = {0};
	ExpandEnvironmentStringsA("%AppData%\\Rime\\deployer.log", path, _countof(path));
	return path;
}

CAppModule _Module;

static int Run(LPTSTR lpCmdLine);

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);

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

	// this should be done before any logging
	{
		WCHAR path[MAX_PATH] = {0};
		ExpandEnvironmentStrings(L"%AppData%\\Rime", path, _countof(path));
		CreateDirectory(path, NULL);
	}

	int ret = 0;
	HANDLE hMutex = CreateMutex(NULL, TRUE, L"WeaselDeployerExclusiveMutex");
	if (!hMutex)
	{
		ret = 1;
	}
	else if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		ret = 1;
	}
	else
	{
		ret = Run(lpCmdLine);
	}
	
	if (hMutex)
	{
		CloseHandle(hMutex);
	}
	_Module.Term();
	::CoUninitialize();

	return ret;
}

static int Run(LPTSTR lpCmdLine)
{
	EZLOGGERPRINT("WeaselDeployer reporting.");

	Configurator configurator;
	configurator.Initialize();

	bool deployment_scheduled = !wcscmp(L"/deploy", lpCmdLine);
	if (deployment_scheduled)
	{
		return configurator.UpdateWorkspace();
	}

	bool installing = !wcscmp(L"/install", lpCmdLine);
	return configurator.Run(installing);
}
