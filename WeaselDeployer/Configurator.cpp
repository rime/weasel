#include "stdafx.h"
#include "Configurator.h"
#include "SwitcherSettingsDialog.h"
#include "UIStyleSettings.h"
#include "UIStyleSettingsDialog.h"
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

Configurator::Configurator()
{
}

int Configurator::Run(bool installing)
{
    rime::Deployer& deployer(rime::Service::instance().deployer());
	bool reconfigured = false;

	rime::SwitcherSettings switcher_settings(&deployer);
	if (!installing || switcher_settings.IsFirstRun()) {
		if (ConfigureSwitcher(&switcher_settings))
			reconfigured = true;
	}

	UIStyleSettings ui_style_settings(&deployer);
	if (!installing || ui_style_settings.IsFirstRun()) {
		if (ConfigureUI(&ui_style_settings))
			reconfigured = true;
	}

	if (installing || reconfigured) {
		return UpdateWorkspace(reconfigured);
	}
	return 0;
}

bool Configurator::ConfigureSwitcher(rime::SwitcherSettings* settings)
{
    if (!settings->Load())
        return false;
	bool reconfigured = false;
	SwitcherSettingsDialog dialog(settings);
	if (dialog.DoModal() == IDOK) {
		reconfigured = settings->Save();
	}
	return reconfigured;
}

bool Configurator::ConfigureUI(UIStyleSettings* settings) {
    if (!settings->Load())
        return false;
	bool reconfigured = false;
	UIStyleSettingsDialog dialog(settings);
	if (dialog.DoModal() == IDOK) {
		reconfigured = settings->Save();
	}
	return reconfigured;
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
