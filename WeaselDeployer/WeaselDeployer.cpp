// WeaselDeployer.cpp : Defines the entry point for the application.
//
#include "stdafx.h"
#include <WeaselUtility.h>
#include <fstream>
#include "WeaselDeployer.h"
#include "Configurator.h"

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

	CreateDirectory(WeaselUserDataPath().c_str(), NULL);

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

static void CreateFileIfNotExist(std::string filename)
{
	std::string user_data_dir = weasel_user_data_dir();
	std::wstring filepathw = to_wide_string(user_data_dir) + L"\\" + to_wide_string(filename);
	DWORD dwAttrib = GetFileAttributes(filepathw.c_str());
	if (!(INVALID_FILE_ATTRIBUTES != dwAttrib && 0 == (dwAttrib & FILE_ATTRIBUTE_DIRECTORY)))
	{
		std::wofstream o(filepathw, std::ios::app);
		o.close();
	}
}
static int Run(LPTSTR lpCmdLine)
{
	Configurator configurator;
	configurator.Initialize();
	CreateFileIfNotExist("default.custom.yaml");
	CreateFileIfNotExist("weasel.custom.yaml");

	bool deployment_scheduled = !wcscmp(L"/deploy", lpCmdLine);
	if (deployment_scheduled)
	{
		return configurator.UpdateWorkspace();
	}

	bool dict_management = !wcscmp(L"/dict", lpCmdLine);
	if (dict_management)
	{
		return configurator.DictManagement();
	}

	bool sync_user_dict = !wcscmp(L"/sync", lpCmdLine);
	if (sync_user_dict)
	{
		return configurator.SyncUserData();
	}

	bool installing = !wcscmp(L"/install", lpCmdLine);
	return configurator.Run(installing);
}
