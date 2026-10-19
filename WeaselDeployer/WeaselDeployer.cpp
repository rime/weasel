// WeaselDeployer.cpp : Defines the entry point for the application.
//
#include "stdafx.h"
#include <WeaselUtility.h>
#include <fstream>
#include <vector>
#include "WeaselDeployer.h"
#include "Configurator.h"
#include <ShellScalingApi.h>

CAppModule _Module;

static int Run(int argc, wchar_t* argv[]);

static bool IsOption(const std::wstring& value, const wchar_t* option) {
  return _wcsicmp(value.c_str(), option) == 0;
}

static int ShowHelp() {
  WCHAR msg[2048] = {0};
  if (LoadString(GetModuleHandle(NULL), IDS_STR_HELP, msg,
                 sizeof(msg) / sizeof(TCHAR))) {
    std::wstring help(msg);
    help.append(
        L"\n\n/dict list - List user dictionaries\n"
        L"/dict backup <dict> - Backup a user dictionary\n"
        L"/dict restore <file> - Restore a user dictionary snapshot\n"
        L"/dict export <dict> <file> - Export a user dictionary\n"
        L"/dict import <dict> <file> - Import a user dictionary");
    MessageBox(NULL, help.c_str(), L"Weasel Deployer",
               MB_ICONINFORMATION | MB_OK);
  } else {
    MessageBox(NULL,
               L"Usage: WeaselDeployer.exe [options]\n"
               L"/? or /help\t\t- Show this help message\n"
               L"/deploy\t\t- Update Workspace\n"
               L"/dict\t\t- Manage dictionary\n"
               L"/dict list\t\t- List user dictionaries\n"
               L"/dict backup <dict>\t- Backup a user dictionary\n"
               L"/dict restore <file>\t- Restore a user dictionary snapshot\n"
               L"/dict export <dict> <file>\t- Export a user dictionary\n"
               L"/dict import <dict> <file>\t- Import a user dictionary\n"
               L"/sync\t\t- Sync user data\n"
               L"/install\t\t- Install Weasel (Initial deployment)",
               L"Weasel Deployer", MB_ICONINFORMATION | MB_OK);
  }
  return 0;
}

static bool IsDictionaryCli(int argc, wchar_t* argv[]) {
  return argc > 2 && IsOption(argv[1], L"/dict");
}

static void DetachGuiConsole() {
  HWND console = GetConsoleWindow();
  if (console) {
    DWORD processes[64] = {0};
    const DWORD count = GetConsoleProcessList(processes, _countof(processes));
    if (count == 1 && processes[0] == GetCurrentProcessId()) {
      ShowWindow(console, SW_HIDE);
    }
  }
  FreeConsole();
}

int wmain(int argc, wchar_t* argv[]) {
  if (!IsDictionaryCli(argc, argv)) {
    // The GUI entry points are launched from the tray or Explorer. Detach
    // from the inherited or newly created console so those paths stay silent.
    DetachGuiConsole();
  }

  HINSTANCE hInstance = GetModuleHandle(NULL);
  LoadWeaselLogLevelFromRegistry();
  SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
  LANGID langId = get_language_id();
  SetThreadUILanguage(langId);
  SetThreadLocale(langId);

  HRESULT hRes = ::CoInitialize(NULL);
  // If you are running on NT 4.0 or higher you can use the following call
  // instead to make the EXE free threaded. This means that calls come in on a
  // random RPC thread.
  // HRESULT hRes = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
  ATLASSERT(SUCCEEDED(hRes));

  // this resolves ATL window thunking problem when Microsoft Layer for Unicode
  // (MSLU) is used
  ::DefWindowProc(NULL, 0, 0, 0L);

  AtlInitCommonControls(
      ICC_BAR_CLASSES);  // add flags to support other controls

  hRes = _Module.Init(NULL, hInstance);
  ATLASSERT(SUCCEEDED(hRes));

  CreateDirectory(WeaselUserDataPath().c_str(), NULL);

  int ret = 0;
  HANDLE hMutex = CreateMutex(NULL, TRUE, L"WeaselDeployerExclusiveMutex");
  if (!hMutex) {
    ret = 1;
  } else if (GetLastError() == ERROR_ALREADY_EXISTS) {
    ret = 1;
  } else {
    ret = Run(argc, argv);
  }

  if (hMutex) {
    CloseHandle(hMutex);
  }
  _Module.Term();
  ::CoUninitialize();

  return ret;
}

static int Run(int argc, wchar_t* argv[]) {
  std::vector<std::wstring> args;
  for (int i = 1; i < argc; ++i) {
    args.emplace_back(argv[i]);
  }

  if (args.size() == 1 &&
      (IsOption(args.front(), L"/?") || IsOption(args.front(), L"/help"))) {
    return ShowHelp();
  }

  Configurator configurator;
  configurator.Initialize();

  if (args.empty()) {
    return configurator.Run(false);
  }

  if (IsOption(args.front(), L"/deploy") && args.size() == 1) {
    return configurator.UpdateWorkspace();
  }

  if (IsOption(args.front(), L"/dict")) {
    return configurator.DictManagement(
        std::vector<std::wstring>(args.begin() + 1, args.end()));
  }

  if (IsOption(args.front(), L"/sync") && args.size() == 1) {
    return configurator.SyncUserData();
  }

  bool installing = IsOption(args.front(), L"/install") && args.size() == 1;
  return configurator.Run(installing);
}
