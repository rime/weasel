#include "stdafx.h"
#include "WeaselDeployer.h"
#include "Configurator.h"
#include "SwitcherSettingsDialog.h"
#include "UIStyleSettings.h"
#include "UIStyleSettingsDialog.h"
#include "DictManagementDialog.h"
#include <WeaselConstants.h>
#include <WeaselIPC.h>
#include <WeaselIPCData.h>
#include <WeaselUtility.h>
#pragma warning(disable : 4005)
#include <rime_api.h>
#include <rime_levers_api.h>
#pragma warning(default : 4005)
#include <filesystem>
#include <fstream>
#include <iostream>
#include "WeaselDeployer.h"

namespace {

constexpr int kDictCliUsageError = 2;

bool IsDictCommand(const std::wstring& value, const wchar_t* command) {
  return _wcsicmp(value.c_str(), command) == 0;
}

RimeLeversApi* GetLeversApi() {
  RimeModule* levers = rime_get_api()->find_module("levers");
  if (!levers)
    return nullptr;
  return (RimeLeversApi*)levers->get_api();
}

void PrintDictCliUsage() {
  std::wcerr << L"Usage: WeaselDeployer.exe /dict <command> [arguments]\n"
             << L"Commands:\n"
             << L"  list\n"
             << L"  backup <dict-name>\n"
             << L"  restore <snapshot-file>\n"
             << L"  export <dict-name> <text-file>\n"
             << L"  import <dict-name> <text-file>\n";
}

std::filesystem::path UserDataSyncDir(RimeApi* rime) {
  char dir[MAX_PATH] = {0};
  rime->get_user_data_sync_dir(dir, _countof(dir));
  return std::filesystem::path(u8tow(dir));
}

}  // namespace

static void CreateFileIfNotExist(std::string filename) {
  std::filesystem::path file_path = WeaselUserDataPath() / u8tow(filename);
  DWORD dwAttrib = GetFileAttributes(file_path.c_str());
  if (!(INVALID_FILE_ATTRIBUTES != dwAttrib &&
        0 == (dwAttrib & FILE_ATTRIBUTE_DIRECTORY))) {
    std::wofstream o(file_path.c_str(), std::ios::app);
    o.close();
  }
}
Configurator::Configurator() {
  CreateFileIfNotExist("default.custom.yaml");
  CreateFileIfNotExist("weasel.custom.yaml");
}

void Configurator::Initialize() {
  RIME_STRUCT(RimeTraits, weasel_traits);
  std::string shared_dir = wtou8(WeaselSharedDataPath().wstring());
  std::string user_dir = wtou8(WeaselUserDataPath().wstring());
  weasel_traits.shared_data_dir = shared_dir.c_str();
  weasel_traits.user_data_dir = user_dir.c_str();
  weasel_traits.prebuilt_data_dir = weasel_traits.shared_data_dir;
  std::string distribution_name = wtou8(get_weasel_ime_name());
  weasel_traits.distribution_name = distribution_name.c_str();
  weasel_traits.distribution_code_name = WEASEL_CODE_NAME;
  weasel_traits.distribution_version = WEASEL_PRODUCT_VERSION;
  weasel_traits.app_name = "rime.weasel";
  std::string log_dir = WeaselLogPath().u8string();
  weasel_traits.log_dir = log_dir.c_str();
  RimeApi* rime_api = rime_get_api();
  assert(rime_api);
  rime_api->setup(&weasel_traits);
  LOG(INFO) << "WeaselDeployer reporting.";
  rime_api->deployer_initialize(NULL);
}

static bool configure_switcher(RimeLeversApi* api,
                               RimeSwitcherSettings* switchcer_settings,
                               bool* reconfigured) {
  RimeCustomSettings* settings = (RimeCustomSettings*)switchcer_settings;
  if (!api->load_settings(settings))
    return false;
  SwitcherSettingsDialog dialog(switchcer_settings);
  if (dialog.DoModal() == IDOK) {
    if (api->save_settings(settings))
      *reconfigured = true;
    return true;
  }
  return false;
}

static bool configure_ui(RimeLeversApi* api,
                         UIStyleSettings* ui_style_settings,
                         bool* reconfigured) {
  RimeCustomSettings* settings = ui_style_settings->settings();
  if (!api->load_settings(settings))
    return false;
  UIStyleSettingsDialog dialog(ui_style_settings);
  if (dialog.DoModal() == IDOK) {
    if (api->save_settings(settings))
      *reconfigured = true;
    return true;
  }
  return false;
}

int Configurator::Run(bool installing) {
  RimeModule* levers = rime_get_api()->find_module("levers");
  if (!levers)
    return 1;
  RimeLeversApi* api = (RimeLeversApi*)levers->get_api();
  if (!api)
    return 1;

  bool reconfigured = false;

  RimeSwitcherSettings* switcher_settings = api->switcher_settings_init();
  UIStyleSettings ui_style_settings;

  bool skip_switcher_settings =
      installing && !api->is_first_run((RimeCustomSettings*)switcher_settings);
  bool skip_ui_style_settings =
      installing && !api->is_first_run(ui_style_settings.settings());

  (skip_switcher_settings ||
   configure_switcher(api, switcher_settings, &reconfigured)) &&
      (skip_ui_style_settings ||
       configure_ui(api, &ui_style_settings, &reconfigured));

  api->custom_settings_destroy((RimeCustomSettings*)switcher_settings);

  if (installing || reconfigured) {
    return UpdateWorkspace(reconfigured);
  }
  return 0;
}

int Configurator::UpdateWorkspace(bool report_errors) {
  HANDLE hMutex = CreateMutex(NULL, TRUE, L"WeaselDeployerMutex");
  if (!hMutex) {
    LOG(ERROR) << "Error creating WeaselDeployerMutex.";
    return 1;
  }
  if (GetLastError() == ERROR_ALREADY_EXISTS) {
    LOG(WARNING) << "another deployer process is running; aborting operation.";
    CloseHandle(hMutex);
    if (report_errors) {
      // MessageBox(NULL,
      // L"正在執行另一項部署任務，方纔所做的修改將在輸入法再次啓動後生效。",
      // L"【小狼毫】", MB_OK | MB_ICONINFORMATION);
      MSG_BY_IDS(IDS_STR_DEPLOYING_RESTARTREQ, IDS_STR_WEASEL,
                 MB_OK | MB_ICONINFORMATION);
    }
    return 1;
  }

  weasel::Client client;
  if (client.Connect()) {
    LOG(INFO) << "Turning WeaselServer into maintenance mode.";
    client.StartMaintenance();
  }

  {
    RimeApi* rime = rime_get_api();
    // initialize default config, preset schemas
    rime->deploy();
    // initialize weasel config
    rime->deploy_config_file("weasel.yaml", "config_version");
  }

  CloseHandle(hMutex);  // should be closed before resuming service.

  if (client.Connect()) {
    LOG(INFO) << "Resuming service.";
    client.EndMaintenance();
  }
  return 0;
}

int Configurator::DictManagement() {
  HANDLE hMutex = CreateMutex(NULL, TRUE, L"WeaselDeployerMutex");
  if (!hMutex) {
    LOG(ERROR) << "Error creating WeaselDeployerMutex.";
    return 1;
  }
  if (GetLastError() == ERROR_ALREADY_EXISTS) {
    LOG(WARNING) << "another deployer process is running; aborting operation.";
    CloseHandle(hMutex);
    // MessageBox(NULL, L"正在執行另一項部署任務，請稍候再試。", L"【小狼毫】",
    // MB_OK | MB_ICONINFORMATION);
    MSG_BY_IDS(IDS_STR_DEPLOYING_WAIT, IDS_STR_WEASEL,
               MB_OK | MB_ICONINFORMATION);
    return 1;
  }

  weasel::Client client;
  if (client.Connect()) {
    LOG(INFO) << "Turning WeaselServer into maintenance mode.";
    client.StartMaintenance();
  }

  {
    RimeApi* rime = rime_get_api();
    if (RIME_API_AVAILABLE(rime, run_task)) {
      rime->run_task("installation_update");  // setup user data sync dir
    }
    DictManagementDialog dlg;
    dlg.DoModal();
  }

  CloseHandle(hMutex);  // should be closed before resuming service.

  if (client.Connect()) {
    LOG(INFO) << "Resuming service.";
    client.EndMaintenance();
  }
  return 0;
}

int Configurator::DictManagement(const std::vector<std::wstring>& args) {
  if (args.empty()) {
    return DictManagement();
  }

  const std::wstring& command = args.front();
  if (IsDictCommand(command, L"help")) {
    PrintDictCliUsage();
    return args.size() == 1 ? 0 : kDictCliUsageError;
  }

  const bool list = IsDictCommand(command, L"list");
  const bool backup = IsDictCommand(command, L"backup");
  const bool restore = IsDictCommand(command, L"restore");
  const bool export_dict = IsDictCommand(command, L"export");
  const bool import_dict = IsDictCommand(command, L"import");

  const size_t expected_args =
      list ? 1 : (backup || restore ? 2 : (export_dict || import_dict ? 3 : 0));
  if (!expected_args || args.size() != expected_args) {
    PrintDictCliUsage();
    return kDictCliUsageError;
  }

  HANDLE hMutex = CreateMutex(NULL, TRUE, L"WeaselDeployerMutex");
  if (!hMutex) {
    LOG(ERROR) << "Error creating WeaselDeployerMutex.";
    return 1;
  }
  if (GetLastError() == ERROR_ALREADY_EXISTS) {
    LOG(WARNING) << "another deployer process is running; aborting operation.";
    CloseHandle(hMutex);
    return 1;
  }

  weasel::Client client;
  if (client.Connect()) {
    LOG(INFO) << "Turning WeaselServer into maintenance mode.";
    client.StartMaintenance();
  }

  RimeApi* rime = rime_get_api();
  RimeLeversApi* api = GetLeversApi();
  int result = 0;

  if (!api) {
    LOG(ERROR) << "Rime levers module is unavailable.";
    result = 1;
  } else if (RIME_API_AVAILABLE(rime, run_task)) {
    rime->run_task("installation_update");
  }

  if (!result && list) {
    RimeUserDictIterator iter = {0};
    if (!api->user_dict_iterator_init(&iter)) {
      LOG(ERROR) << "Failed to enumerate user dictionaries.";
      result = 1;
    } else {
      while (const char* dict = api->next_user_dict(&iter)) {
        std::wcout << u8tow(dict) << std::endl;
      }
      api->user_dict_iterator_destroy(&iter);
    }
  } else if (!result && backup) {
    const std::string dict_name = wtou8(args[1]);
    if (!api->backup_user_dict(dict_name.c_str())) {
      LOG(ERROR) << "Failed to backup user dictionary: " << dict_name;
      result = 1;
    } else {
      const std::filesystem::path snapshot =
          UserDataSyncDir(rime) / (args[1] + L".userdb.txt");
      std::wcout << snapshot.wstring() << std::endl;
    }
  } else if (!result && restore) {
    const std::string snapshot = wtou8(args[1]);
    if (!api->restore_user_dict(snapshot.c_str())) {
      LOG(ERROR) << "Failed to restore user dictionary: " << snapshot;
      result = 1;
    }
  } else if (!result && export_dict) {
    const std::string dict_name = wtou8(args[1]);
    const std::string text_file = wtou8(args[2]);
    const int count =
        api->export_user_dict(dict_name.c_str(), text_file.c_str());
    if (count < 0) {
      LOG(ERROR) << "Failed to export user dictionary: " << dict_name;
      result = 1;
    } else {
      std::wcout << count << std::endl;
    }
  } else if (!result && import_dict) {
    const std::string dict_name = wtou8(args[1]);
    const std::string text_file = wtou8(args[2]);
    const int count =
        api->import_user_dict(dict_name.c_str(), text_file.c_str());
    if (count < 0) {
      LOG(ERROR) << "Failed to import user dictionary: " << dict_name;
      result = 1;
    } else {
      std::wcout << count << std::endl;
    }
  }

  CloseHandle(hMutex);  // should be closed before resuming service.

  if (client.Connect()) {
    LOG(INFO) << "Resuming service.";
    client.EndMaintenance();
  }
  return result;
}

int Configurator::SyncUserData() {
  HANDLE hMutex = CreateMutex(NULL, TRUE, L"WeaselDeployerMutex");
  if (!hMutex) {
    LOG(ERROR) << "Error creating WeaselDeployerMutex.";
    return 1;
  }
  if (GetLastError() == ERROR_ALREADY_EXISTS) {
    LOG(WARNING) << "another deployer process is running; aborting operation.";
    CloseHandle(hMutex);
    // MessageBox(NULL, L"正在執行另一項部署任務，請稍候再試。", L"【小狼毫】",
    // MB_OK | MB_ICONINFORMATION);
    MSG_BY_IDS(IDS_STR_DEPLOYING_WAIT, IDS_STR_WEASEL,
               MB_OK | MB_ICONINFORMATION);
    return 1;
  }

  weasel::Client client;
  if (client.Connect()) {
    LOG(INFO) << "Turning WeaselServer into maintenance mode.";
    client.StartMaintenance();
  }

  {
    RimeApi* rime = rime_get_api();
    if (!rime->sync_user_data()) {
      LOG(ERROR) << "Error synching user data.";
      CloseHandle(hMutex);
      return 1;
    }
    rime->join_maintenance_thread();
  }

  CloseHandle(hMutex);  // should be closed before resuming service.

  if (client.Connect()) {
    LOG(INFO) << "Resuming service.";
    client.EndMaintenance();
  }
  return 0;
}
