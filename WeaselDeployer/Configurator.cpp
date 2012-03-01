#include "stdafx.h"
#include "WeaselDeployer.h"
#include "Configurator.h"
#include "SwitcherSettingsDialog.h"
#include "UIStyleSettings.h"
#include "UIStyleSettingsDialog.h"
#include <WeaselCommon.h>
#include <WeaselIPC.h>
#pragma warning(disable: 4005)
#pragma warning(disable: 4995)
#pragma warning(disable: 4996)
#include <rime_api.h>
#include <rime/deployer.h>
#include <rime/service.h>
#include <rime/expl/switcher_settings.h>
#pragma warning(default: 4996)
#pragma warning(default: 4995)
#pragma warning(default: 4005)


const WCHAR* utf8towcs(const char* utf8_str)
{
	const int buffer_len = 1024;
	static WCHAR buffer[buffer_len];
	memset(buffer, 0, sizeof(buffer));
	MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, buffer, buffer_len - 1);
	return buffer;
}

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


Configurator::Configurator()
{
}

void Configurator::Initialize()
{
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
	RimeDeployerInitialize(&weasel_traits);
}

int Configurator::Run(bool installing)
{
    rime::Deployer& deployer(rime::Service::instance().deployer());
	bool reconfigured = false;

	rime::SwitcherSettings switcher_settings(&deployer);
	UIStyleSettings ui_style_settings(&deployer);

	bool skip_switcher_settings = installing && !switcher_settings.IsFirstRun();
	bool skip_ui_style_settings = installing && !ui_style_settings.IsFirstRun();
	
	(skip_switcher_settings || ConfigureSwitcher(&switcher_settings, &reconfigured)) &&
		(skip_ui_style_settings || ConfigureUI(&ui_style_settings, &reconfigured));

	if (installing || reconfigured) {
		return UpdateWorkspace(reconfigured);
	}
	return 0;
}

bool Configurator::ConfigureSwitcher(rime::SwitcherSettings* settings, bool* reconfigured)
{
    if (!settings->Load())
        return false;
	SwitcherSettingsDialog dialog(settings);
	if (dialog.DoModal() == IDOK) {
		*reconfigured = settings->Save();
		return true;
	}
	return false;
}

bool Configurator::ConfigureUI(UIStyleSettings* settings, bool* reconfigured) {
    if (!settings->Load())
        return false;
	UIStyleSettingsDialog dialog(settings);
	if (dialog.DoModal() == IDOK) {
		*reconfigured = settings->Save();
		return true;
	}
	return false;
}

int Configurator::UpdateWorkspace(bool report_errors) {
	HANDLE hMutex = CreateMutex(NULL, TRUE, L"WeaselDeployerMutex");
	if (!hMutex)
	{
		EZLOGGERPRINT("Error creating WeaselDeployerMutex.");
		return 1;
	}
	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		EZLOGGERPRINT("Warning: another deployer process is running; aborting operation.");
		CloseHandle(hMutex);
		if (report_errors)
		{
			MessageBox(NULL, L"正在執行另一項部署任務，方纔所做的修改將在輸入法再次啓動後生效。", L"【小狼毫】", MB_OK | MB_ICONINFORMATION);
		}
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

	CloseHandle(hMutex);  // should be closed before resuming service.

	if (client.Connect())
	{
		EZLOGGERPRINT("Resuming service.");
		client.EndMaintenance();
	}
	return 0;
}
