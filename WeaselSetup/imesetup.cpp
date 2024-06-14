#include "stdafx.h"
#include <string>
#include <vector>
#include <msctf.h>
#include <strsafe.h>
#include <StringAlgorithm.hpp>
#include <WeaselConstants.h>
#include <WeaselUtility.h>
#include "InstallOptionsDlg.h"

// {A3F4CDED-B1E9-41EE-9CA6-7B4D0DE6CB0A}
static const GUID c_clsidTextService = {
    0xa3f4cded,
    0xb1e9,
    0x41ee,
    {0x9c, 0xa6, 0x7b, 0x4d, 0xd, 0xe6, 0xcb, 0xa}};

// {3D02CAB6-2B8E-4781-BA20-1C9267529467}
static const GUID c_guidProfile = {
    0x3d02cab6,
    0x2b8e,
    0x4781,
    {0xba, 0x20, 0x1c, 0x92, 0x67, 0x52, 0x94, 0x67}};

// if in the future, option hant is extended, maybe a function to generate this
// info is required
#define PSZTITLE_HANS                                                     \
  L"0804:{A3F4CDED-B1E9-41EE-9CA6-7B4D0DE6CB0A}{3D02CAB6-2B8E-4781-BA20-" \
  L"1C9267529467}"
#define PSZTITLE_HANT                                                     \
  L"0404:{A3F4CDED-B1E9-41EE-9CA6-7B4D0DE6CB0A}{3D02CAB6-2B8E-4781-BA20-" \
  L"1C9267529467}"
#define ILOT_UNINSTALL 0x00000001
typedef HRESULT(WINAPI* PTF_INSTALLLAYOUTORTIP)(LPCWSTR psz, DWORD dwFlags);

#define WEASEL_WER_KEY                            \
  L"SOFTWARE\\Microsoft\\Windows\\Windows Error " \
  L"Reporting\\LocalDumps\\WeaselServer.exe"

BOOL copy_file(const std::wstring& src, const std::wstring& dest) {
  BOOL ret = CopyFile(src.c_str(), dest.c_str(), FALSE);
  if (!ret) {
    for (int i = 0; i < 10; ++i) {
      std::wstring old = dest + L".old." + std::to_wstring(i);
      if (MoveFileEx(dest.c_str(), old.c_str(), MOVEFILE_REPLACE_EXISTING)) {
        MoveFileEx(old.c_str(), NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
        break;
      }
    }
    ret = CopyFile(src.c_str(), dest.c_str(), FALSE);
  }
  return ret;
}

BOOL delete_file(const std::wstring& file) {
  BOOL ret = DeleteFile(file.c_str());
  if (!ret) {
    for (int i = 0; i < 10; ++i) {
      std::wstring old = file + L".old." + std::to_wstring(i);
      if (MoveFileEx(file.c_str(), old.c_str(), MOVEFILE_REPLACE_EXISTING)) {
        MoveFileEx(old.c_str(), NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
        return TRUE;
      }
    }
  }
  return ret;
}

typedef BOOL(WINAPI* PISWOW64P2)(HANDLE, USHORT*, USHORT*);
BOOL is_arm64_machine() {
  PISWOW64P2 fnIsWow64Process2 = (PISWOW64P2)GetProcAddress(
      GetModuleHandle(_T("kernel32.dll")), "IsWow64Process2");

  if (fnIsWow64Process2 == NULL) {
    return FALSE;
  }

  USHORT processMachine;
  USHORT nativeMachine;

  if (!fnIsWow64Process2(GetCurrentProcess(), &processMachine,
                         &nativeMachine)) {
    return FALSE;
  }
  return nativeMachine == IMAGE_FILE_MACHINE_ARM64;
}

typedef HRESULT(WINAPI* PISWOWGMS)(USHORT, BOOL*);
typedef UINT(WINAPI* PGSW64DIR2)(LPWSTR, UINT, WORD);
INT get_wow_arm32_system_dir(LPWSTR lpBuffer, UINT uSize) {
  PISWOWGMS fnIsWow64GuestMachineSupported = (PISWOWGMS)GetProcAddress(
      GetModuleHandle(_T("kernel32.dll")), "IsWow64GuestMachineSupported");
  PGSW64DIR2 fnGetSystemWow64Directory2W = (PGSW64DIR2)GetProcAddress(
      GetModuleHandle(_T("kernelbase.dll")), "GetSystemWow64Directory2W");

  if (fnIsWow64GuestMachineSupported == NULL ||
      fnGetSystemWow64Directory2W == NULL) {
    return 0;
  }

  BOOL supported;
  if (fnIsWow64GuestMachineSupported(IMAGE_FILE_MACHINE_ARMNT, &supported) !=
      S_OK) {
    return 0;
  }

  if (!supported) {
    return 0;
  }

  return fnGetSystemWow64Directory2W(lpBuffer, uSize, IMAGE_FILE_MACHINE_ARMNT);
}

typedef int (*ime_register_func)(const std::wstring& ime_path,
                                 bool register_ime,
                                 bool is_wow64,
                                 bool is_wowarm,
                                 bool hant,
                                 bool silent);

int install_ime_file(std::wstring& srcPath,
                     const std::wstring& ext,
                     bool hant,
                     bool silent,
                     ime_register_func func) {
  WCHAR path[MAX_PATH];
  GetModuleFileNameW(GetModuleHandle(NULL), path, _countof(path));

  std::wstring srcFileName = L"weasel";

  srcFileName += ext;
  WCHAR drive[_MAX_DRIVE];
  WCHAR dir[_MAX_DIR];
  _wsplitpath_s(path, drive, _countof(drive), dir, _countof(dir), NULL, 0, NULL,
                0);
  srcPath = std::wstring(drive) + dir + srcFileName;

  GetSystemDirectoryW(path, _countof(path));
  std::wstring destPath = std::wstring(path) + L"\\weasel" + ext;

  int retval = 0;
  // 复制 .dll/.ime 到系统目录
  if (!copy_file(srcPath, destPath)) {
    MSG_NOT_SILENT_ID_CAP(silent, destPath.c_str(), IDS_STR_INSTALL_FAILED,
                          MB_ICONERROR | MB_OK);
    return 1;
  }
  retval += func(destPath, true, false, false, hant, silent);
  if (is_wow64()) {
    PVOID OldValue = NULL;
    // PW64DW64FR fnWow64DisableWow64FsRedirection =
    // (PW64DW64FR)GetProcAddress(GetModuleHandle(_T("kernel32.dll")),
    // "Wow64DisableWow64FsRedirection"); PW64RW64FR
    // fnWow64RevertWow64FsRedirection =
    // (PW64RW64FR)GetProcAddress(GetModuleHandle(_T("kernel32.dll")),
    // "Wow64RevertWow64FsRedirection");
    if (Wow64DisableWow64FsRedirection(&OldValue) == FALSE) {
      MSG_NOT_SILENT_BY_IDS(silent, IDS_STR_ERRCANCELFSREDIRECT,
                            IDS_STR_INSTALL_FAILED, MB_ICONERROR | MB_OK);
      return 1;
    }

    if (is_arm64_machine()) {
      WCHAR sysarm32[MAX_PATH];
      if (get_wow_arm32_system_dir(sysarm32, _countof(sysarm32)) > 0) {
        // Install the ARM32 version if ARM32 WOW is supported （lower than
        // Windows 11 24H2).
        std::wstring srcPathARM32 = srcPath;
        ireplace_last(srcPathARM32, ext, L"ARM" + ext);

        std::wstring destPathARM32 = std::wstring(sysarm32) + L"\\weasel" + ext;
        if (!copy_file(srcPathARM32, destPathARM32)) {
          MSG_NOT_SILENT_ID_CAP(silent, destPathARM32.c_str(),
                                IDS_STR_INSTALL_FAILED, MB_ICONERROR | MB_OK);
          return 1;
        }
        retval += func(destPathARM32, true, true, true, hant, silent);
      }

      // Then install the ARM64 (and x64) version.
      // On ARM64 weasel.dll(ime) is an ARM64X redirection DLL (weaselARM64X).
      // When loaded, it will be redirected to weaselARM64.dll(ime) on ARM64
      // processes, and weaselx64.dll(ime) on x64 processes. So we need a total
      // of three files.

      std::wstring srcPathX64 = srcPath;
      std::wstring destPathX64 = destPath;
      ireplace_last(srcPathX64, ext, L"x64" + ext);
      ireplace_last(destPathX64, ext, L"x64" + ext);
      if (!copy_file(srcPathX64, destPathX64)) {
        MSG_NOT_SILENT_ID_CAP(silent, destPathX64.c_str(),
                              IDS_STR_INSTALL_FAILED, MB_ICONERROR | MB_OK);
        return 1;
      }

      std::wstring srcPathARM64 = srcPath;
      std::wstring destPathARM64 = destPath;
      ireplace_last(srcPathARM64, ext, L"ARM64" + ext);
      ireplace_last(destPathARM64, ext, L"ARM64" + ext);
      if (!copy_file(srcPathARM64, destPathARM64)) {
        MSG_NOT_SILENT_ID_CAP(silent, destPathARM64.c_str(),
                              IDS_STR_INSTALL_FAILED, MB_ICONERROR | MB_OK);
        return 1;
      }

      // Since weaselARM64X is just a redirector we don't have separate
      // HANS and HANT variants.
      srcPath = std::wstring(drive) + dir + L"weaselARM64X" + ext;
    } else {
      ireplace_last(srcPath, ext, L"x64" + ext);
    }

    if (!copy_file(srcPath, destPath)) {
      MSG_NOT_SILENT_ID_CAP(silent, destPath.c_str(), IDS_STR_INSTALL_FAILED,
                            MB_ICONERROR | MB_OK);
      return 1;
    }
    retval += func(destPath, true, true, false, hant, silent);
    if (Wow64RevertWow64FsRedirection(OldValue) == FALSE) {
      MSG_NOT_SILENT_BY_IDS(silent, IDS_STR_ERRRECOVERFSREDIRECT,
                            IDS_STR_INSTALL_FAILED, MB_ICONERROR | MB_OK);
      return 1;
    }
  }
  return retval;
}

int uninstall_ime_file(const std::wstring& ext,
                       bool silent,
                       ime_register_func func) {
  int retval = 0;
  WCHAR path[MAX_PATH];
  GetSystemDirectoryW(path, _countof(path));
  std::wstring imePath(path);
  imePath += L"\\weasel" + ext;
  retval += func(imePath, false, false, false, false, silent);
  delete_file(imePath);
  if (is_wow64()) {
    retval += func(imePath, false, true, false, false, silent);
    PVOID OldValue = NULL;
    if (Wow64DisableWow64FsRedirection(&OldValue) == FALSE) {
      MSG_NOT_SILENT_BY_IDS(silent, IDS_STR_ERRCANCELFSREDIRECT,
                            IDS_STR_UNINSTALL_FAILED, MB_ICONERROR | MB_OK);
      return 1;
    }

    if (is_arm64_machine()) {
      WCHAR sysarm32[MAX_PATH];
      if (get_wow_arm32_system_dir(sysarm32, _countof(sysarm32)) > 0) {
        std::wstring imePathARM32 = std::wstring(sysarm32) + L"\\weasel" + ext;
        retval += func(imePathARM32, false, true, true, false, silent);
        delete_file(imePathARM32);
      }

      std::wstring imePathX64 = imePath;
      ireplace_last(imePathX64, ext, L"x64" + ext);
      delete_file(imePathX64);

      std::wstring imePathARM64 = imePath;
      ireplace_last(imePathARM64, ext, L"ARM64" + ext);
      delete_file(imePathARM64);
    }

    delete_file(imePath);
    if (Wow64RevertWow64FsRedirection(OldValue) == FALSE) {
      MSG_NOT_SILENT_BY_IDS(silent, IDS_STR_ERRRECOVERFSREDIRECT,
                            IDS_STR_UNINSTALL_FAILED, MB_ICONERROR | MB_OK);
      return 1;
    }
  }
  return retval;
}

// 注册IME输入法
int register_ime(const std::wstring& ime_path,
                 bool register_ime,
                 bool is_wow64,
                 bool is_wowarm,
                 bool hant,
                 bool silent) {
  if (is_wow64) {
    return 0;  // only once
  }

  const WCHAR KEYBOARD_LAYOUTS_KEY[] =
      L"SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts";
  const WCHAR PRELOAD_KEY[] = L"Keyboard Layout\\Preload";

  if (register_ime) {
    HKL hKL = ImmInstallIME(ime_path.c_str(), get_weasel_ime_name().c_str());
    if (!hKL) {
      // manually register ime
      WCHAR hkl_str[16] = {0};
      HKEY hKey;
      LSTATUS ret = RegOpenKey(HKEY_LOCAL_MACHINE, KEYBOARD_LAYOUTS_KEY, &hKey);
      if (ret == ERROR_SUCCESS) {
        for (DWORD k = 0xE0200000 + (hant ? 0x0404 : 0x0804); k <= 0xE0FF0804;
             k += 0x10000) {
          StringCchPrintfW(hkl_str, _countof(hkl_str), L"%08X", k);
          HKEY hSubKey;
          ret = RegOpenKey(hKey, hkl_str, &hSubKey);
          if (ret == ERROR_SUCCESS) {
            WCHAR imeFile[32] = {0};
            DWORD len = sizeof(imeFile);
            DWORD type = 0;
            ret = RegQueryValueEx(hSubKey, L"Ime File", NULL, &type,
                                  (LPBYTE)imeFile, &len);
            if (ret = ERROR_SUCCESS) {
              if (_wcsicmp(imeFile, L"weasel.ime") == 0) {
                hKL = (HKL)k;  // already there
              }
            }
            RegCloseKey(hSubKey);
          } else {
            // found a spare number to register
            ret = RegCreateKey(hKey, hkl_str, &hSubKey);
            if (ret == ERROR_SUCCESS) {
              const WCHAR ime_file[] = L"weasel.ime";
              RegSetValueEx(hSubKey, L"Ime File", 0, REG_SZ, (LPBYTE)ime_file,
                            sizeof(ime_file));
              const WCHAR layout_file[] = L"kbdus.dll";
              RegSetValueEx(hSubKey, L"Layout File", 0, REG_SZ,
                            (LPBYTE)layout_file, sizeof(layout_file));
              const std::wstring layout_text = get_weasel_ime_name();
              RegSetValueEx(hSubKey, L"Layout Text", 0, REG_SZ,
                            (LPBYTE)layout_text.c_str(),
                            layout_text.size() * sizeof(wchar_t));
              RegCloseKey(hSubKey);
              hKL = (HKL)k;
            }
            break;
          }
        }
        RegCloseKey(hKey);
      }
      if (hKL) {
        HKEY hPreloadKey;
        ret = RegOpenKey(HKEY_CURRENT_USER, PRELOAD_KEY, &hPreloadKey);
        if (ret == ERROR_SUCCESS) {
          for (size_t i = 1; true; ++i) {
            std::wstring number = std::to_wstring(i);
            DWORD type = 0;
            WCHAR value[32];
            DWORD len = sizeof(value);
            ret = RegQueryValueEx(hPreloadKey, number.c_str(), 0, &type,
                                  (LPBYTE)value, &len);
            if (ret != ERROR_SUCCESS) {
              RegSetValueEx(hPreloadKey, number.c_str(), 0, REG_SZ,
                            (const BYTE*)hkl_str,
                            (wcslen(hkl_str) + 1) * sizeof(WCHAR));
              break;
            }
          }
          RegCloseKey(hPreloadKey);
        }
      }
    }
    if (!hKL) {
      DWORD dwErr = GetLastError();
      WCHAR msg[100];
      CString str;
      str.LoadStringW(IDS_STR_ERRREGIME);
      StringCchPrintfW(msg, _countof(msg), str, hKL, dwErr);
      MSG_NOT_SILENT_ID_CAP(silent, msg, IDS_STR_INSTALL_FAILED,
                            MB_ICONERROR | MB_OK);
      return 1;
    }
    return 0;
  }

  // unregister ime

  HKEY hKey;
  LSTATUS ret = RegOpenKey(HKEY_LOCAL_MACHINE, KEYBOARD_LAYOUTS_KEY, &hKey);
  if (ret != ERROR_SUCCESS) {
    MSG_NOT_SILENT_ID_CAP(silent, KEYBOARD_LAYOUTS_KEY,
                          IDS_STR_UNINSTALL_FAILED, MB_ICONERROR | MB_OK);
    return 1;
  }

  for (int i = 0; true; ++i) {
    WCHAR subKey[16];
    ret = RegEnumKey(hKey, i, subKey, _countof(subKey));
    if (ret != ERROR_SUCCESS)
      break;

    // 中文键盘布局?
    if (wcscmp(subKey + 4, L"0804") == 0 || wcscmp(subKey + 4, L"0404") == 0) {
      HKEY hSubKey;
      ret = RegOpenKey(hKey, subKey, &hSubKey);
      if (ret != ERROR_SUCCESS)
        continue;

      WCHAR imeFile[32];
      DWORD len = sizeof(imeFile);
      DWORD type = 0;
      ret = RegQueryValueEx(hSubKey, L"Ime File", NULL, &type, (LPBYTE)imeFile,
                            &len);
      RegCloseKey(hSubKey);
      if (ret != ERROR_SUCCESS)
        continue;

      // 小狼毫?
      if (_wcsicmp(imeFile, L"weasel.ime") == 0) {
        DWORD value;
        swscanf_s(subKey, L"%x", &value);
        UnloadKeyboardLayout((HKL)value);

        RegDeleteKey(hKey, subKey);

        // 移除preload
        HKEY hPreloadKey;
        ret = RegOpenKey(HKEY_CURRENT_USER, PRELOAD_KEY, &hPreloadKey);
        if (ret != ERROR_SUCCESS)
          continue;
        std::vector<std::wstring> preloads;
        std::wstring number;
        for (size_t i = 1; true; ++i) {
          number = std::to_wstring(i);
          DWORD type = 0;
          WCHAR value[32];
          DWORD len = sizeof(value);
          ret = RegQueryValueEx(hPreloadKey, number.c_str(), 0, &type,
                                (LPBYTE)value, &len);
          if (ret != ERROR_SUCCESS) {
            if (i > preloads.size()) {
              // 删除最大一号注册表值
              number = std::to_wstring(i - 1);
              RegDeleteValue(hPreloadKey, number.c_str());
            }
            break;
          }
          if (_wcsicmp(subKey, value) != 0) {
            preloads.push_back(value);
          }
        }
        // 重写preloads
        for (size_t i = 0; i < preloads.size(); ++i) {
          number = std::to_wstring(i + 1);
          RegSetValueEx(hPreloadKey, number.c_str(), 0, REG_SZ,
                        (const BYTE*)preloads[i].c_str(),
                        (preloads[i].length() + 1) * sizeof(WCHAR));
        }
        RegCloseKey(hPreloadKey);
      }
    }
  }

  RegCloseKey(hKey);
  return 0;
}

void enable_profile(BOOL fEnable, bool hant) {
  HRESULT hr;
  ITfInputProcessorProfiles* pProfiles = NULL;

  hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, NULL,
                        CLSCTX_INPROC_SERVER, IID_ITfInputProcessorProfiles,
                        (LPVOID*)&pProfiles);

  if (SUCCEEDED(hr)) {
    LANGID lang_id = hant ? 0x0404 : 0x0804;
    if (fEnable) {
      pProfiles->EnableLanguageProfile(c_clsidTextService, lang_id,
                                       c_guidProfile, fEnable);
      pProfiles->EnableLanguageProfileByDefault(c_clsidTextService, lang_id,
                                                c_guidProfile, fEnable);
    } else {
      pProfiles->RemoveLanguageProfile(c_clsidTextService, lang_id,
                                       c_guidProfile);
    }

    pProfiles->Release();
  }
}

// 注册TSF输入法
int register_text_service(const std::wstring& tsf_path,
                          bool register_ime,
                          bool is_wow64,
                          bool is_wowarm32,
                          bool hant,
                          bool silent) {
  using RegisterServerFunction = HRESULT(STDAPICALLTYPE*)();

  if (!register_ime)
    enable_profile(FALSE, hant);

  std::wstring params = L" \"" + tsf_path + L"\"";
  if (!register_ime) {
    params = L" /u " + params;  // unregister
  }
  // if (silent)  // always silent
  { params = L" /s " + params; }

  if (hant) {
    if (!SetEnvironmentVariable(L"TEXTSERVICE_PROFILE", L"hant")) {
      // bad luck
    }
  } else {
    if (!SetEnvironmentVariable(L"TEXTSERVICE_PROFILE", L"hans")) {
      // bad luck
    }
  }

  std::wstring app = L"regsvr32.exe";
  if (is_wowarm32) {
    WCHAR sysarm32[MAX_PATH];
    get_wow_arm32_system_dir(sysarm32, _countof(sysarm32));

    app = std::wstring(sysarm32) + L"\\" + app;
  }

  SHELLEXECUTEINFOW shExInfo = {0};
  shExInfo.cbSize = sizeof(shExInfo);
  shExInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
  shExInfo.hwnd = 0;
  shExInfo.lpVerb = L"open";               // Operation to perform
  shExInfo.lpFile = app.c_str();           // Application to start
  shExInfo.lpParameters = params.c_str();  // Additional parameters
  shExInfo.lpDirectory = 0;
  shExInfo.nShow = SW_SHOW;
  shExInfo.hInstApp = 0;
  if (ShellExecuteExW(&shExInfo)) {
    WaitForSingleObject(shExInfo.hProcess, INFINITE);
    CloseHandle(shExInfo.hProcess);
  } else {
    WCHAR msg[100];
    CString str;
    str.LoadStringW(IDS_STR_ERRREGTSF);
    StringCchPrintfW(msg, _countof(msg), str, params.c_str());
    // StringCchPrintfW(msg, _countof(msg), L"註冊輸入法錯誤 regsvr32.exe %s",
    // params.c_str()); if (!silent) MessageBoxW(NULL, msg, L"安装/卸載失败",
    // MB_ICONERROR | MB_OK);
    MSG_NOT_SILENT_ID_CAP(silent, msg, IDS_STR_INORUN_FAILED,
                          MB_ICONERROR | MB_OK);
    return 1;
  }

  if (register_ime)
    enable_profile(TRUE, hant);

  return 0;
}

int install(bool hant, bool silent, bool old_ime_support) {
  std::wstring ime_src_path;
  int retval = 0;
  if (old_ime_support) {
    retval +=
        install_ime_file(ime_src_path, L".ime", hant, silent, &register_ime);
  }
  retval += install_ime_file(ime_src_path, L".dll", hant, silent,
                             &register_text_service);

  // 写注册表
  WCHAR drive[_MAX_DRIVE];
  WCHAR dir[_MAX_DIR];
  _wsplitpath_s(ime_src_path.c_str(), drive, _countof(drive), dir,
                _countof(dir), NULL, 0, NULL, 0);
  std::wstring rootDir = std::wstring(drive) + dir;
  rootDir.pop_back();
  auto ret = SetRegKeyValue(HKEY_LOCAL_MACHINE, WEASEL_REG_KEY, L"WeaselRoot",
                            rootDir.c_str(), REG_SZ);
  if (FAILED(HRESULT_FROM_WIN32(ret))) {
    MSG_NOT_SILENT_BY_IDS(silent, IDS_STR_ERRWRITEWEASELROOT,
                          IDS_STR_INSTALL_FAILED, MB_ICONERROR | MB_OK);
    return 1;
  }

  const std::wstring executable = L"WeaselServer.exe";
  ret = SetRegKeyValue(HKEY_LOCAL_MACHINE, WEASEL_REG_KEY, L"ServerExecutable",
                       executable.c_str(), REG_SZ);
  if (FAILED(HRESULT_FROM_WIN32(ret))) {
    MSG_NOT_SILENT_BY_IDS(silent, IDS_STR_ERRREGIMEWRITESVREXE,
                          IDS_STR_INSTALL_FAILED, MB_ICONERROR | MB_OK);
    return 1;
  }

  // InstallLayoutOrTip
  // https://learn.microsoft.com/zh-cn/windows/win32/tsf/installlayoutortip
  // example in ref page not right with "*PTF_ INSTALLLAYOUTORTIP"
  // space inside should be removed
  HMODULE hInputDLL = LoadLibrary(TEXT("input.dll"));
  if (hInputDLL) {
    PTF_INSTALLLAYOUTORTIP pfnInstallLayoutOrTip;
    pfnInstallLayoutOrTip =
        (PTF_INSTALLLAYOUTORTIP)GetProcAddress(hInputDLL, "InstallLayoutOrTip");
    if (pfnInstallLayoutOrTip) {
      if (hant)
        (*pfnInstallLayoutOrTip)(PSZTITLE_HANT, 0);
      else
        (*pfnInstallLayoutOrTip)(PSZTITLE_HANS, 0);
    }
    FreeLibrary(hInputDLL);
  }

  // https://learn.microsoft.com/zh-cn/windows/win32/wer/collecting-user-mode-dumps
  const std::wstring dmpPathW = WeaselLogPath().wstring();
  // DumpFolder
  SetRegKeyValue(HKEY_LOCAL_MACHINE, WEASEL_WER_KEY, L"DumpFolder",
                 dmpPathW.c_str(), REG_SZ, true);
  // dump type 0
  SetRegKeyValue(HKEY_LOCAL_MACHINE, WEASEL_WER_KEY, L"DumpType", 0, REG_DWORD,
                 true);
  // CustomDumpFlags, MiniDumpNormal
  SetRegKeyValue(HKEY_LOCAL_MACHINE, WEASEL_WER_KEY, L"CustomDumpFlags", 0,
                 REG_DWORD, true);
  // maximium dump count 10
  SetRegKeyValue(HKEY_LOCAL_MACHINE, WEASEL_WER_KEY, L"DumpCount", 10,
                 REG_DWORD, true);

  if (retval)
    return 1;

  MSG_NOT_SILENT_BY_IDS(silent, IDS_STR_INSTALL_SUCCESS_INFO,
                        IDS_STR_INSTALL_SUCCESS_CAP,
                        MB_ICONINFORMATION | MB_OK);
  return 0;
}

int uninstall(bool silent) {
  // 注销输入法
  int retval = 0;

  const WCHAR KEY[] = L"Software\\Rime\\Weasel";
  HKEY hKey;
  LSTATUS ret = RegOpenKey(HKEY_CURRENT_USER, KEY, &hKey);
  if (ret == ERROR_SUCCESS) {
    DWORD type = 0;
    DWORD data = 0;
    DWORD len = sizeof(data);
    ret = RegQueryValueEx(hKey, L"Hant", NULL, &type, (LPBYTE)&data, &len);
    if (ret == ERROR_SUCCESS && type == REG_DWORD) {
      HMODULE hInputDLL = LoadLibrary(TEXT("input.dll"));
      if (hInputDLL) {
        PTF_INSTALLLAYOUTORTIP pfnInstallLayoutOrTip;
        pfnInstallLayoutOrTip = (PTF_INSTALLLAYOUTORTIP)GetProcAddress(
            hInputDLL, "InstallLayoutOrTip");
        if (pfnInstallLayoutOrTip) {
          if (data != 0)
            (*pfnInstallLayoutOrTip)(PSZTITLE_HANT, ILOT_UNINSTALL);
          else
            (*pfnInstallLayoutOrTip)(PSZTITLE_HANS, ILOT_UNINSTALL);
        }
        FreeLibrary(hInputDLL);
      }
    }
    RegCloseKey(hKey);
  }

  uninstall_ime_file(L".ime", silent, &register_ime);
  retval += uninstall_ime_file(L".dll", silent, &register_text_service);

  // 清除注册信息
  RegDeleteKey(HKEY_LOCAL_MACHINE, WEASEL_REG_KEY);
  RegDeleteKey(HKEY_LOCAL_MACHINE, RIME_REG_KEY);

  // delete WER register,
  // "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\Windows Error
  // Reporting\\LocalDumps\\WeaselServer.exe" no WOW64 redirect

  auto flag_wow64 = is_wow64() ? KEY_WOW64_64KEY : 0;
  RegDeleteKeyEx(HKEY_LOCAL_MACHINE, WEASEL_WER_KEY, flag_wow64, 0);
  if (retval)
    return 1;

  MSG_NOT_SILENT_BY_IDS(silent, IDS_STR_UNINSTALL_SUCCESS_INFO,
                        IDS_STR_UNINSTALL_SUCCESS_CAP,
                        MB_ICONINFORMATION | MB_OK);
  return 0;
}

bool has_installed() {
  WCHAR path[MAX_PATH];
  GetSystemDirectory(path, _countof(path));
  std::wstring sysPath(path);
  DWORD attr = GetFileAttributesW((sysPath + L"\\weasel.dll").c_str());
  return (attr != INVALID_FILE_ATTRIBUTES &&
          !(attr & FILE_ATTRIBUTE_DIRECTORY));
}
