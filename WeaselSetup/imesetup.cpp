#include "stdafx.h"
#include <string>
#include <vector>
#include <StringAlgorithm.hpp>
#include <WeaselCommon.h>
#include <msctf.h>
#include <strsafe.h>


// Iōng Power Shell sán-seng sin ê GUID：'{'+[guid]::NewGuid().ToString()+'}'
// {d5026f36-1b08-4269-a3d7-0c04e277c327}
static const GUID c_clsidTextService = 
{ 0xd5026f36, 0x1b08, 0x4269, { 0xa3d7, 0x0c04e277c327 } };

// Iōng Power Shell sán-seng sin ê GUID：'{'+[guid]::NewGuid().ToString()+'}'
// {632f7393-d0a8-4626-9108-f22c195bd427}
static const GUID c_guidProfile = 
{ 0x632f7393, 0xd0a8, 0x4626, { 0x9108, 0xf22c195bd427 } };


BOOL copy_file(const std::wstring& src, const std::wstring& dest)
{
	BOOL ret = CopyFile(src.c_str(), dest.c_str(), FALSE);
	if (!ret)
	{
		for (int i = 0; i < 10; ++i)
		{
			std::wstring old = dest + L".old." + std::to_wstring(i);
			if (MoveFileEx(dest.c_str(), old.c_str(), MOVEFILE_REPLACE_EXISTING))
			{
				MoveFileEx(old.c_str(), NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
				break;
			}
		}
		ret = CopyFile(src.c_str(), dest.c_str(), FALSE);
	}
	return ret;
}

BOOL delete_file(const std::wstring& file)
{
	BOOL ret = DeleteFile(file.c_str());
	if (!ret)
	{
		for (int i = 0; i < 10; ++i)
		{
			std::wstring old = file + L".old." + std::to_wstring(i);
			if (MoveFileEx(file.c_str(), old.c_str(), MOVEFILE_REPLACE_EXISTING))
			{
				MoveFileEx(old.c_str(), NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
				return TRUE;
			}
		}
	}
	return ret;
}

BOOL is_wow64()
{
	DWORD errorCode;
	if (GetSystemWow64DirectoryW(NULL, 0) == 0)
		if ((errorCode = GetLastError()) == ERROR_CALL_NOT_IMPLEMENTED)
			return FALSE;
		else
			ExitProcess((UINT)errorCode);
	else
		return TRUE;
}

typedef BOOL (WINAPI *PW64DW64FR)(PVOID *);
typedef BOOL (WINAPI *PW64RW64FR)(PVOID);
typedef int (*ime_register_func)(const std::wstring& ime_path, bool register_ime, bool is_wow64, bool hant, bool silent);

int install_ime_file(std::wstring& srcPath, const std::wstring& ext, bool hant, bool silent, ime_register_func func)
{
	WCHAR path[MAX_PATH];
	GetModuleFileNameW(GetModuleHandle(NULL), path, _countof(path));

	std::wstring srcFileName = (hant ? L"ThuanTaigit" : L"ThuanTaigi");
	srcFileName += ext;
	WCHAR drive[_MAX_DRIVE];
	WCHAR dir[_MAX_DIR];
	_wsplitpath_s(path, drive, _countof(drive), dir, _countof(dir), NULL, 0, NULL, 0);
	srcPath = std::wstring(drive) + dir + srcFileName;

	GetSystemDirectoryW(path, _countof(path));
	std::wstring destPath = std::wstring(path) + L"\\ThuanTaigi" + ext;

	// if (!silent) MessageBox(NULL, destPath.c_str(), L"destPath", MB_ICONERROR | MB_OK);

	int retval = 0;
	// 复制 .dll/.ime 到系统目录
	if (!copy_file(srcPath, destPath))
	{
		if (!silent) MessageBoxW(NULL, destPath.c_str(), L"安裝失敗", MB_ICONERROR | MB_OK);
		return 1;
	}
	retval += func(destPath, true, false, hant, silent);
	if (is_wow64())
	{
		ireplace_last(srcPath, ext, L"x64" + ext);
		PVOID OldValue = NULL;
		PW64DW64FR fnWow64DisableWow64FsRedirection = (PW64DW64FR)GetProcAddress(GetModuleHandle(_T("kernel32.dll")), "Wow64DisableWow64FsRedirection");
		PW64RW64FR fnWow64RevertWow64FsRedirection = (PW64RW64FR)GetProcAddress(GetModuleHandle(_T("kernel32.dll")), "Wow64RevertWow64FsRedirection");
		if (fnWow64DisableWow64FsRedirection == NULL || fnWow64DisableWow64FsRedirection(&OldValue) == FALSE)
		{
			if (!silent) MessageBoxW(NULL, L"無法取消文件系統重定向", L"安裝失敗", MB_ICONERROR | MB_OK);
			return 1;
		}
		if (!copy_file(srcPath, destPath))
		{
			if (!silent) MessageBoxW(NULL, destPath.c_str(), L"安裝失敗", MB_ICONERROR | MB_OK);
			return 1;
		}
		retval += func(destPath, true, true, hant, silent);
		if (fnWow64RevertWow64FsRedirection == NULL || fnWow64RevertWow64FsRedirection(OldValue) == FALSE)
		{
			if (!silent) MessageBoxW(NULL, L"無法恢復文件系統重定向", L"安裝失敗", MB_ICONERROR | MB_OK);
			return 1;
		}
	}
	return retval;
}

int uninstall_ime_file(const std::wstring& ext, bool silent, ime_register_func func)
{
	int retval = 0;
	WCHAR path[MAX_PATH];
	GetSystemDirectoryW(path, _countof(path));
	std::wstring imePath(path);
	imePath += L"\\ThuanTaigi" + ext;
	retval += func(imePath, false, false, false, silent);
	if (!delete_file(imePath))
	{
		if (!silent) MessageBox(NULL, imePath.c_str(), L"移除失敗", MB_ICONERROR | MB_OK);
		retval += 1;
	}
	if (is_wow64())
	{
		retval += func(imePath, false, true, false, silent);
		PVOID OldValue = NULL;
		PW64DW64FR fnWow64DisableWow64FsRedirection = (PW64DW64FR)GetProcAddress(GetModuleHandle(_T("kernel32.dll")), "Wow64DisableWow64FsRedirection");
		PW64RW64FR fnWow64RevertWow64FsRedirection = (PW64RW64FR)GetProcAddress(GetModuleHandle(_T("kernel32.dll")), "Wow64RevertWow64FsRedirection");
		if (fnWow64DisableWow64FsRedirection == NULL || fnWow64DisableWow64FsRedirection(&OldValue) == FALSE)
		{
			if (!silent) MessageBoxW(NULL, L"無法取消文件系統重定向", L"移除失敗", MB_ICONERROR | MB_OK);
			return 1;
		}
		if (!delete_file(imePath))
		{
			if (!silent) MessageBoxW(NULL, imePath.c_str(), L"移除失敗", MB_ICONERROR | MB_OK);
			retval += 1;
		}
		if (fnWow64RevertWow64FsRedirection == NULL || fnWow64RevertWow64FsRedirection(OldValue) == FALSE)
		{
			if (!silent) MessageBoxW(NULL, L"無法恢復文件系統重定向", L"移除失敗", MB_ICONERROR | MB_OK);
			return 1;
		}
	}
	return retval;
}

// 注册IME输入法
int register_ime(const std::wstring& ime_path, bool register_ime, bool is_wow64, bool hant, bool silent)
{
	if (is_wow64)
	{
		return 0;  // only once
	}

	const WCHAR KEYBOARD_LAYOUTS_KEY[] = L"SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts";
	const WCHAR PRELOAD_KEY[] = L"Keyboard Layout\\Preload";

	if (register_ime)
	{
		HKL hKL = ImmInstallIME(ime_path.c_str(), WEASEL_IME_NAME);
		if (!hKL)
		{
			// manually register ime
			// https://docs.microsoft.com/zh-tw/windows-hardware/manufacture/desktop/windows-language-pack-default-values
			// https://scripts.sil.org/cms/scripts/page.php?site_id=nrsi&id=Kbd_Registry
			WCHAR hkl_str[16] = {0};
			HKEY hKey;
			LSTATUS ret = RegOpenKey(HKEY_LOCAL_MACHINE, KEYBOARD_LAYOUTS_KEY, &hKey);
			if (ret == ERROR_SUCCESS)
			{
				for (DWORD k = 0xE0200000 + (hant ? 0x0404 : 0x0804); k <= 0xE0FF0804; k += 0x10000)
				{
					StringCchPrintfW(hkl_str, _countof(hkl_str), L"%08X", k);
					HKEY hSubKey;
					ret = RegOpenKey(hKey, hkl_str, &hSubKey);
					if (ret == ERROR_SUCCESS)
					{
						WCHAR imeFile[32] = {0};
						DWORD len = sizeof(imeFile);
						DWORD type = 0;
						ret = RegQueryValueEx(hSubKey, L"Ime File", NULL, &type, (LPBYTE)imeFile, &len);
						if (ret = ERROR_SUCCESS)
						{
							if (_wcsicmp(imeFile, L"ThuanTaigi.ime") == 0)
							{
								hKL = (HKL)k;  // already there
							}
						}
						RegCloseKey(hSubKey);
					}
					else
					{
						// found a spare number to register
						ret = RegCreateKey(hKey, hkl_str, &hSubKey);
						if (ret == ERROR_SUCCESS)
						{
							const WCHAR ime_file[] = L"ThuanTaigi.ime";
							RegSetValueEx(hSubKey, L"Ime File", 0, REG_SZ, (LPBYTE)ime_file, sizeof(ime_file));
							const WCHAR layout_file[] = L"kbdus.dll";
							RegSetValueEx(hSubKey, L"Layout File", 0, REG_SZ, (LPBYTE)layout_file, sizeof(layout_file));
							const WCHAR layout_text[] = WEASEL_IME_NAME;
							RegSetValueEx(hSubKey, L"Layout Text", 0, REG_SZ, (LPBYTE)layout_text, sizeof(layout_text));
							RegCloseKey(hSubKey);
							hKL = (HKL)k;
						}
						break;
					}
				}
				RegCloseKey(hKey);
			}
			if (hKL)
			{
				HKEY hPreloadKey;
				ret = RegOpenKey(HKEY_CURRENT_USER, PRELOAD_KEY, &hPreloadKey);
				if (ret == ERROR_SUCCESS)
				{
					for (size_t i = 1; true; ++i)
					{
						std::wstring number = std::to_wstring(i);
						DWORD type = 0;
						WCHAR value[32];
						DWORD len = sizeof(value);
						ret = RegQueryValueEx(hPreloadKey, number.c_str(), 0, &type, (LPBYTE)value, &len);
						if (ret != ERROR_SUCCESS)
						{
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
		if (!hKL)
		{
			DWORD dwErr = GetLastError();
			WCHAR msg[100];
			StringCchPrintfW(msg, _countof(msg), L"註冊輸入法錯誤 ImmInstallIME: HKL=%x Err=%x", hKL, dwErr);
			if (!silent) MessageBox(NULL, msg, L"安裝失敗", MB_ICONERROR | MB_OK);
			return 1;
		}
		return 0;
	}

	// unregister ime

	HKEY hKey;
	LSTATUS ret = RegOpenKey(HKEY_LOCAL_MACHINE, KEYBOARD_LAYOUTS_KEY, &hKey);
	if (ret != ERROR_SUCCESS)
	{
		if (!silent) MessageBox(NULL, KEYBOARD_LAYOUTS_KEY, L"移除失敗", MB_ICONERROR | MB_OK);
		return 1;
	}

	for (int i = 0; true; ++i)
	{
		WCHAR subKey[16];
		ret = RegEnumKey(hKey, i, subKey, _countof(subKey));
		if (ret != ERROR_SUCCESS)
			break;

		// 中文键盘布局?
		if (wcscmp(subKey + 4, L"0804") == 0 || wcscmp(subKey + 4, L"0404") == 0)
		{
			HKEY hSubKey;
			ret = RegOpenKey(hKey, subKey, &hSubKey);
			if (ret != ERROR_SUCCESS)
				continue;

			WCHAR imeFile[32];
			DWORD len = sizeof(imeFile);
			DWORD type = 0;
			ret = RegQueryValueEx(hSubKey, L"Ime File", NULL, &type, (LPBYTE)imeFile, &len);
			RegCloseKey(hSubKey);
			if (ret != ERROR_SUCCESS)
				continue;

			// 小狼毫?
			if (_wcsicmp(imeFile, L"ThuanTaigi.ime") == 0)
			{
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
				for (size_t i = 1; true; ++i)
				{
					number = std::to_wstring(i);
					DWORD type = 0;
					WCHAR value[32];
					DWORD len = sizeof(value);
					ret = RegQueryValueEx(hPreloadKey, number.c_str(), 0, &type, (LPBYTE)value, &len);
					if (ret != ERROR_SUCCESS)
					{
						if (i > preloads.size())
						{
							// 删除最大一号注册表值
							number = std::to_wstring(i - 1);
							RegDeleteValue(hPreloadKey, number.c_str());
						}
						break;
					}
					if (_wcsicmp(subKey, value) != 0)
					{
						preloads.push_back(value);
					}
				}
				// 重写preloads
				for (size_t i = 0; i < preloads.size(); ++i)
				{
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
	ITfInputProcessorProfiles *pProfiles = NULL;

	hr = CoCreateInstance(  CLSID_TF_InputProcessorProfiles, 
							NULL, 
							CLSCTX_INPROC_SERVER, 
							IID_ITfInputProcessorProfiles, 
							(LPVOID*)&pProfiles);

	if(SUCCEEDED(hr))
	{
		LANGID lang_id = hant ? 0x0404 : 0x0804;
		if (fEnable) {
			pProfiles->EnableLanguageProfile(c_clsidTextService, lang_id, c_guidProfile, fEnable);
			pProfiles->EnableLanguageProfileByDefault(c_clsidTextService, lang_id, c_guidProfile, fEnable);
		}
		else {
			pProfiles->RemoveLanguageProfile(c_clsidTextService, lang_id, c_guidProfile);
		}

		pProfiles->Release();
	}
}

// 注册TSF输入法
int register_text_service(const std::wstring& tsf_path, bool register_ime, bool is_wow64, bool hant, bool silent)
{
	using RegisterServerFunction = HRESULT (STDAPICALLTYPE *)();

	if (!register_ime)
		enable_profile(FALSE, hant);

	std::wstring params = L" \"" + tsf_path + L"\"";
	if (!register_ime)
	{
		params = L" /u " + params;  // unregister
	}
	//if (silent)  // always silent
	{
		params = L" /s " + params;
	}
	SHELLEXECUTEINFOW shExInfo = { 0 };
	shExInfo.cbSize = sizeof(shExInfo);
	shExInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	shExInfo.hwnd = 0;
	shExInfo.lpVerb = L"open";                 // Operation to perform
	shExInfo.lpFile = L"regsvr32.exe";         // Application to start    
	shExInfo.lpParameters = params.c_str();    // Additional parameters
	shExInfo.lpDirectory = 0;
	shExInfo.nShow = SW_SHOW;
	shExInfo.hInstApp = 0;
	if (ShellExecuteExW(&shExInfo))
	{
		WaitForSingleObject(shExInfo.hProcess, INFINITE);
		CloseHandle(shExInfo.hProcess);
	}
	else
	{
		WCHAR msg[100];
		StringCchPrintfW(msg, _countof(msg), L"註冊輸入法錯誤 regsvr32.exe %s", params.c_str());
		if (!silent) MessageBoxW(NULL, msg, L"安装/移除失败", MB_ICONERROR | MB_OK);
		return 1;
	}

	if (register_ime)
		enable_profile(TRUE, hant);

	return 0;
}

int install(bool hant, bool silent)
{
	std::wstring ime_src_path;
	int retval = 0;
	retval += install_ime_file(ime_src_path, L".ime", hant, silent, &register_ime);
	retval += install_ime_file(ime_src_path, L".dll", hant, silent, &register_text_service);

	// 写注册表
	HKEY hKey;
	LSTATUS ret = RegCreateKeyEx(HKEY_LOCAL_MACHINE, WEASEL_REG_KEY,
		                         0, NULL, 0, KEY_ALL_ACCESS, 0, &hKey, NULL);
	if (FAILED(HRESULT_FROM_WIN32(ret)))
	{
		if (!silent) MessageBox(NULL, WEASEL_REG_KEY, L"安裝失敗", MB_ICONERROR | MB_OK);
		return 1;
	}

	WCHAR drive[_MAX_DRIVE];
	WCHAR dir[_MAX_DIR];
	_wsplitpath_s(ime_src_path.c_str(), drive, _countof(drive), dir, _countof(dir), NULL, 0, NULL, 0);
	std::wstring rootDir = std::wstring(drive) + dir;
	rootDir.pop_back();
	ret = RegSetValueEx(hKey, L"ThuanTaigiRoot", 0, REG_SZ,
		                (const BYTE*)rootDir.c_str(),
						(rootDir.length() + 1) * sizeof(WCHAR));
	if (FAILED(HRESULT_FROM_WIN32(ret)))
	{
		if (!silent) MessageBox(NULL, L"無法寫入 ThuanTaigiRoot", L"安裝失敗", MB_ICONERROR | MB_OK);
		return 1;
	}

	const std::wstring executable = L"WeaselServer.exe";
	ret = RegSetValueEx(hKey, L"ServerExecutable", 0, REG_SZ,
		                (const BYTE*)executable.c_str(),
						(executable.length() + 1) * sizeof(WCHAR));
	if (FAILED(HRESULT_FROM_WIN32(ret)))
	{
		if (!silent) MessageBox(NULL, L"無法寫入註冊表鍵值 ServerExecutable", L"安裝失敗", MB_ICONERROR | MB_OK);
		return 1;
	}

	RegCloseKey(hKey);

	if (retval)
		return 1;

	if (!silent) MessageBox(NULL, L"可以用【意傳台語輸入法】打字了 :)", L"安裝完成", MB_ICONINFORMATION | MB_OK);
	return 0;
}

int uninstall(bool silent)
{
	// 注销输入法
	int retval = 0;
	retval += uninstall_ime_file(L".ime", silent, &register_ime);
	retval += uninstall_ime_file(L".dll", silent, &register_text_service);

	// 清除注册信息
	RegDeleteKey(HKEY_LOCAL_MACHINE, WEASEL_REG_KEY);
	RegDeleteKey(HKEY_LOCAL_MACHINE, RIME_REG_KEY);

	if (retval)
		return 1;

	if (!silent) MessageBox(NULL, L"意傳台語輸入法 :)", L"移除完成", MB_ICONINFORMATION | MB_OK);
	return 0;
}

bool has_installed() {
	WCHAR path[MAX_PATH];
	GetSystemDirectory(path, _countof(path));
	std::wstring sysPath(path);
	DWORD attr = GetFileAttributesW((sysPath + L"\\ThuanTaigi.ime").c_str());
	return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
}
