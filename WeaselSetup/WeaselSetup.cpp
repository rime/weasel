// WeaselSetup.cpp : main source file for WeaselSetup.exe
//

#include "stdafx.h"

#include "resource.h"
#include "WeaselUtility.h"
#include <thread>

#include "InstallOptionsDlg.h"

#include <ShellScalingApi.h>
#pragma comment(lib, "Shcore.lib")
CAppModule _Module;

static int Run(LPTSTR lpCmdLine);
static bool IsProcAdmin();
static int RestartAsAdmin(LPTSTR lpCmdLine);

int WINAPI _tWinMain(HINSTANCE hInstance,
                     HINSTANCE /*hPrevInstance*/,
                     LPTSTR lpstrCmdLine,
                     int /*nCmdShow*/) {
  HRESULT hRes = ::CoInitialize(NULL);
  ATLASSERT(SUCCEEDED(hRes));

  AtlInitCommonControls(
      ICC_BAR_CLASSES);  // add flags to support other controls

  hRes = _Module.Init(NULL, hInstance);
  ATLASSERT(SUCCEEDED(hRes));

  LANGID langId = get_language_id();
  SetThreadUILanguage(langId);
  SetThreadLocale(langId);

  int nRet = Run(lpstrCmdLine);

  _Module.Term();
  ::CoUninitialize();

  return nRet;
}
int install(bool hant, bool silent, bool old_ime_support);
int uninstall(bool silent);
bool has_installed();

static std::wstring install_dir() {
  WCHAR exe_path[MAX_PATH] = {0};
  GetModuleFileNameW(GetModuleHandle(NULL), exe_path, _countof(exe_path));
  std::wstring dir(exe_path);
  size_t pos = dir.find_last_of(L"\\");
  dir.resize(pos);
  return dir;
}

static int CustomInstall(bool installing) {
  bool hant = false;
  bool silent = false;
  bool old_ime_support = false;
  std::wstring user_dir;

  const WCHAR KEY[] = L"Software\\Rime\\Weasel";
  HKEY hKey;
  LSTATUS ret = RegOpenKey(HKEY_CURRENT_USER, KEY, &hKey);
  if (ret == ERROR_SUCCESS) {
    WCHAR value[MAX_PATH];
    DWORD len = sizeof(value);
    DWORD type = 0;
    DWORD data = 0;
    ret =
        RegQueryValueEx(hKey, L"RimeUserDir", NULL, &type, (LPBYTE)value, &len);
    if (ret == ERROR_SUCCESS && type == REG_SZ) {
      user_dir = value;
    }
    len = sizeof(data);
    ret = RegQueryValueEx(hKey, L"Hant", NULL, &type, (LPBYTE)&data, &len);
    if (ret == ERROR_SUCCESS && type == REG_DWORD) {
      hant = (data != 0);
      if (installing)
        silent = true;
    }
    RegCloseKey(hKey);
  }
  bool _has_installed = has_installed();
  if (!silent) {
    InstallOptionsDialog dlg;
    dlg.installed = _has_installed;
    dlg.hant = hant;
    dlg.user_dir = user_dir;
    if (IDOK != dlg.DoModal()) {
      if (!installing)
        return 1;  // aborted by user
    } else {
      hant = dlg.hant;
      user_dir = dlg.user_dir;
      old_ime_support = dlg.old_ime_support;
      _has_installed = dlg.installed;
    }
  }
  if (!_has_installed)
    if (0 != install(hant, silent, old_ime_support))
      return 1;

  if (user_dir.empty()) {
    // default user dir %APPDATA%\Rime
    WCHAR _path[MAX_PATH] = {0};
    ExpandEnvironmentStringsW(L"%APPDATA%\\Rime", _path, _countof(_path));
    user_dir = std::wstring(_path);
  }
  ret = SetRegKeyValue(HKEY_CURRENT_USER, KEY, L"RimeUserDir", user_dir.c_str(),
                       REG_SZ, false);
  if (FAILED(HRESULT_FROM_WIN32(ret))) {
    MSG_BY_IDS(IDS_STR_ERR_WRITE_USER_DIR, IDS_STR_INSTALL_FAILED,
               MB_ICONERROR | MB_OK);
    return 1;
  }
  ret = SetRegKeyValue(HKEY_CURRENT_USER, KEY, L"Hant", (hant ? 1 : 0),
                       REG_DWORD, false);
  if (FAILED(HRESULT_FROM_WIN32(ret))) {
    MSG_BY_IDS(IDS_STR_ERR_WRITE_HANT, IDS_STR_INSTALL_FAILED,
               MB_ICONERROR | MB_OK);
    return 1;
  }
  if (_has_installed) {
    std::wstring dir(install_dir());
    std::thread th([dir]() {
      ShellExecuteW(NULL, NULL, (dir + L"\\WeaselServer.exe").c_str(), L"/q",
                    NULL, SW_SHOWNORMAL);
      Sleep(500);
      ShellExecuteW(NULL, NULL, (dir + L"\\WeaselServer.exe").c_str(), L"",
                    NULL, SW_SHOWNORMAL);
      Sleep(500);
      ShellExecuteW(NULL, NULL, (dir + L"\\WeaselDeployer.exe").c_str(),
                    L"/deploy", NULL, SW_SHOWNORMAL);
    });
    th.detach();
    MSG_BY_IDS(IDS_STR_MODIFY_SUCCESS_INFO, IDS_STR_MODIFY_SUCCESS_CAP,
               MB_ICONINFORMATION | MB_OK);
  }

  return 0;
}

LPCTSTR GetParamByPrefix(LPCTSTR lpCmdLine, LPCTSTR prefix) {
  return (wcsncmp(lpCmdLine, prefix, wcslen(prefix)) == 0)
             ? (lpCmdLine + wcslen(prefix))
             : 0;
}

static int Run(LPTSTR lpCmdLine) {
  constexpr bool silent = true;
  constexpr bool old_ime_support = false;
  // parameter /? or /help to show commandline args
  if (!wcscmp(L"/?", lpCmdLine) || !wcscmp(L"/help", lpCmdLine)) {
    WCHAR msg[1024] = {0};
    if (LoadString(GetModuleHandle(NULL), IDS_STR_HELP, msg,
                   sizeof(msg) / sizeof(TCHAR))) {
      MessageBox(NULL, msg, L"WeaselSetup", MB_ICONINFORMATION | MB_OK);
    } else {
      MessageBox(
          NULL,
          L"Usage: WeaselSetup.exe [options]\n"
          L"/? or /help    - Show this help message\n"
          L"/u             - Uninstall Weasel\n"
          L"/i             - Install Weasel\n"
          L"/s             - Install Weasel (Simplified Chinese)\n"
          L"/t             - Install Weasel (Traditional Chinese)\n"
          L"/ls            - Set Weasel language to Simplified Chinese\n"
          L"/lt            - Set Weasel language to Traditional Chinese\n"
          L"/le            - Set Weasel language to English\n"
          L"/eu            - Enable automatic update check\n"
          L"/du            - Disable automatic update check\n"
          L"/toggleime     - Toggle IME on open/close(ctrl+space)\n"
          L"/toggleascii   - Toggle ASCII on open/close(ctrl+space)\n"
          L"/testing       - Set update channel to testing\n"
          L"/release       - Set update channel to release\n"
          L"/userdir:<dir> - Set user directory\n",
          L"WeaselSetup", MB_ICONINFORMATION | MB_OK);
    }
    return 0;
  }
  bool uninstalling = !wcscmp(L"/u", lpCmdLine);
  if (uninstalling) {
    if (IsProcAdmin())
      return uninstall(silent);
    else
      return RestartAsAdmin(lpCmdLine);
  }

  if (auto res = GetParamByPrefix(lpCmdLine, L"/userdir:")) {
    return SetRegKeyValue(HKEY_CURRENT_USER, L"Software\\Rime\\weasel",
                          L"RimeUserDir", res, REG_SZ);
  }

  if (!wcscmp(L"/ls", lpCmdLine)) {
    return SetRegKeyValue(HKEY_CURRENT_USER, L"Software\\Rime\\weasel",
                          L"Language", L"chs", REG_SZ);
  } else if (!wcscmp(L"/lt", lpCmdLine)) {
    return SetRegKeyValue(HKEY_CURRENT_USER, L"Software\\Rime\\weasel",
                          L"Language", L"cht", REG_SZ);
  } else if (!wcscmp(L"/le", lpCmdLine)) {
    return SetRegKeyValue(HKEY_CURRENT_USER, L"Software\\Rime\\weasel",
                          L"Language", L"eng", REG_SZ);
  }

  if (!wcscmp(L"/eu", lpCmdLine)) {
    return SetRegKeyValue(HKEY_CURRENT_USER, L"Software\\Rime\\weasel\\Updates",
                          L"CheckForUpdates", L"1", REG_SZ);
  }
  if (!wcscmp(L"/du", lpCmdLine)) {
    return SetRegKeyValue(HKEY_CURRENT_USER, L"Software\\Rime\\weasel\\Updates",
                          L"CheckForUpdates", L"0", REG_SZ);
  }

  if (!wcscmp(L"/toggleime", lpCmdLine)) {
    return SetRegKeyValue(HKEY_CURRENT_USER, L"Software\\Rime\\weasel",
                          L"ToggleImeOnOpenClose", L"yes", REG_SZ);
  }
  if (!wcscmp(L"/toggleascii", lpCmdLine)) {
    return SetRegKeyValue(HKEY_CURRENT_USER, L"Software\\Rime\\weasel",
                          L"ToggleImeOnOpenClose", L"no", REG_SZ);
  }
  if (!wcscmp(L"/testing", lpCmdLine)) {
    return SetRegKeyValue(HKEY_CURRENT_USER, L"Software\\Rime\\weasel",
                          L"UpdateChannel", L"testing", REG_SZ);
  }
  if (!wcscmp(L"/release", lpCmdLine)) {
    return SetRegKeyValue(HKEY_CURRENT_USER, L"Software\\Rime\\weasel",
                          L"UpdateChannel", L"release", REG_SZ);
  }

  if (!IsProcAdmin()) {
    return RestartAsAdmin(lpCmdLine);
  }

  bool hans = !wcscmp(L"/s", lpCmdLine);
  if (hans)
    return install(false, silent, old_ime_support);
  bool hant = !wcscmp(L"/t", lpCmdLine);
  if (hant)
    return install(true, silent, old_ime_support);
  bool installing = !wcscmp(L"/i", lpCmdLine);
  return CustomInstall(installing);
}

// https://learn.microsoft.com/zh-cn/windows/win32/api/securitybaseapi/nf-securitybaseapi-checktokenmembership
bool IsProcAdmin() {
  BOOL b = FALSE;
  SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
  PSID AdministratorsGroup;
  b = AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                               DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0,
                               &AdministratorsGroup);

  if (b) {
    if (!CheckTokenMembership(NULL, AdministratorsGroup, &b)) {
      b = FALSE;
    }
    FreeSid(AdministratorsGroup);
  }

  return (b);
}

int RestartAsAdmin(LPTSTR lpCmdLine) {
  SHELLEXECUTEINFO execInfo{0};
  TCHAR path[MAX_PATH];
  GetModuleFileName(GetModuleHandle(NULL), path, _countof(path));
  execInfo.lpFile = path;
  execInfo.lpParameters = lpCmdLine;
  execInfo.lpVerb = _T("runas");
  execInfo.cbSize = sizeof(execInfo);
  execInfo.nShow = SW_SHOWNORMAL;
  execInfo.fMask = SEE_MASK_NOASYNC | SEE_MASK_NOCLOSEPROCESS;
  execInfo.hwnd = NULL;
  execInfo.hProcess = NULL;
  if (::ShellExecuteEx(&execInfo) && execInfo.hProcess != NULL) {
    ::WaitForSingleObject(execInfo.hProcess, INFINITE);
    DWORD dwExitCode = 0;
    ::GetExitCodeProcess(execInfo.hProcess, &dwExitCode);
    ::CloseHandle(execInfo.hProcess);
    return dwExitCode;
  }
  return -1;
}
