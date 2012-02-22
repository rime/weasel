// WeaselDeployer.cpp : Defines the entry point for the application.
//
#include "stdafx.h"
#include "WeaselDeployer.h"
#include "Configurator.h"
#include <string>
#include <WeaselCommon.h>
#include <WeaselIPC.h>

#pragma warning(disable: 4005)
#include <rime_api.h>
#pragma warning(default: 4005)


CAppModule _Module;

static const std::string WeaselDeployerLogFilePath()
{
	char path[MAX_PATH] = {0};
	ExpandEnvironmentStringsA("%AppData%\\Rime\\deployer.log", path, _countof(path));
	return path;
}

#define EZLOGGER_OUTPUT_FILENAME WeaselDeployerLogFilePath()
#define EZLOGGER_REPLACE_EXISTING_LOGFILE_

#pragma warning(disable: 4995)
#pragma warning(disable: 4996)
#include <ezlogger/ezlogger_headers.hpp>
#pragma warning(default: 4996)
#pragma warning(default: 4995)


static const char* weasel_shared_data_dir() {
	static char path[MAX_PATH] = {0};
	GetModuleFileNameA(NULL, path, _countof(path));
	std::string str_path(path);
	size_t k = str_path.find_last_of("/\\");
	strcpy(path + k + 1, "data");
	return path;
}

static const char* weasel_user_data_dir() {
	static char path[MAX_PATH] = {0};
	ExpandEnvironmentStringsA("%AppData%\\Rime", path, _countof(path));
	return path;
}

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
        
	RimeTraits weasel_traits;
	{
		weasel_traits.shared_data_dir = weasel_shared_data_dir();
		weasel_traits.user_data_dir = weasel_user_data_dir();
		const int len = 20;
		char utf8_str[len];
		memset(utf8_str, 0, sizeof(utf8_str));
		WideCharToMultiByte(CP_UTF8, 0, WEASEL_IME_NAME, -1, utf8_str, len - 1, NULL, NULL);
		weasel_traits.distribution_name = utf8_str;
		weasel_traits.distribution_code_name = WEASEL_CODE_NAME;
		weasel_traits.distribution_version = WEASEL_VERSION;
	}
	RimeDeployerInitialize(&weasel_traits);

	HANDLE hMutex = CreateMutex(NULL, TRUE, L"WeaselDeployerProcessMutex");
	if (!hMutex)
	{
		EZLOGGERPRINT("Error creating mutex.");
		return 1;
	}
	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		EZLOGGERPRINT("Warning: another deployer process is running; quitting.");
		CloseHandle(hMutex);
		_Module.Term();
		::CoUninitialize();
		return 1;
	}

	EZLOGGERPRINT("WeaselDeployer reporting.");
        
	bool deployment_scheduled = !wcscmp(L"/deploy", lpCmdLine);
	bool reconfigured = false;
	if (!deployment_scheduled)
	{
		Configurator configurator;
		reconfigured = configurator.Run();
		if (!reconfigured)
		{
			EZLOGGERPRINT("User cancelled the operation.");
			CloseHandle(hMutex);
			_Module.Term();
			::CoUninitialize();
			return 0;
		}
	}

	HANDLE hDeployerMutex = CreateMutex(NULL, TRUE, L"WeaselDeployerMutex");
	if (!hDeployerMutex)
	{
		EZLOGGERPRINT("Error creating WeaselDeployerMutex.");
		return 1;
	}
	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		EZLOGGERPRINT("Warning: another deployer process is running; aborting operation.");
		CloseHandle(hDeployerMutex);
		CloseHandle(hMutex);
		if (reconfigured)
		{
			MessageBox(NULL, L"正在執行另一項部署任務，方纔所做的修改將在輸入法再次啓動後生效。", L"【小狼毫】", MB_OK | MB_ICONINFORMATION);
		}
		_Module.Term();
		::CoUninitialize();
		return 1;
	}

	weasel::Client client;
	if (client.Connect())
	{
		EZLOGGERPRINT("Turning WeaselServer into maintenance mode.");
		client.StartMaintenance();
	}

	{
		// initialize default config, preset schemas
		RimeDeployWorkspace();
		// initialize weasel config
		RimeDeployConfigFile("weasel.yaml", "config_version");
	}

	CloseHandle(hDeployerMutex);  // should be closed before resuming service.
	CloseHandle(hMutex);

	if (client.Connect())
	{
		EZLOGGERPRINT("Resuming service.");
		client.EndMaintenance();
	}

	_Module.Term();
	::CoUninitialize();

	return 0;
}
