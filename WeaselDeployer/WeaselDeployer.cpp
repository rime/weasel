// WeaselDeployer.cpp : Defines the entry point for the application.
//
#include "stdafx.h"
#include "WeaselDeployer.h"
#include <string>
#include <WeaselCommon.h>
#include <WeaselIPC.h>

#pragma warning(disable: 4005)
#include <rime_api.h>
#pragma warning(default: 4005)

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
	UNREFERENCED_PARAMETER(lpCmdLine);

	HANDLE hMutex = CreateMutex(NULL, TRUE, L"WeaselDeployerMutex");
	if (!hMutex)
	{
		return 1;
	}
	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		CloseHandle(hMutex);
		return 1;
	}

	EZLOGGERPRINT("WeaselDeployer reporting.");

	weasel::Client client;
	if (client.Connect())
	{
		EZLOGGERPRINT("Turning the running server into maintenance mode.");
		client.StartMaintenance();
	}

	RimeTraits weasel_traits;
	weasel_traits.shared_data_dir = weasel_shared_data_dir();
	weasel_traits.user_data_dir = weasel_user_data_dir();
	const int len = 20;
	char utf8_str[len];
	memset(utf8_str, 0, sizeof(utf8_str));
	WideCharToMultiByte(CP_UTF8, 0, WEASEL_IME_NAME, -1, utf8_str, len - 1, NULL, NULL);
	weasel_traits.distribution_name = utf8_str;
	weasel_traits.distribution_code_name = WEASEL_CODE_NAME;
	weasel_traits.distribution_version = WEASEL_VERSION;
	// initialize default config, preset schemas
	RimeDeployInitialize(&weasel_traits);
	// initialize weasel config
	RimeDeployConfigFile("weasel.yaml", "config_version");
	
	if (hMutex)
	{
		CloseHandle(hMutex);
	}
	if (client.Connect())
	{
		EZLOGGERPRINT("Resuming service.");
		client.EndMaintenance();
	}
	return 0;
}
