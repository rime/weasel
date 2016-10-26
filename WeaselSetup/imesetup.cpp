#include "stdafx.h"
#include <string>
#include <vector>
#include <StringAlgorithm.hpp>
#include <WeaselCommon.h>
#include <msctf.h>


// {A3F4CDED-B1E9-41EE-9CA6-7B4D0DE6CB0A}
static const GUID c_clsidTextService = 
{ 0xa3f4cded, 0xb1e9, 0x41ee, { 0x9c, 0xa6, 0x7b, 0x4d, 0xd, 0xe6, 0xcb, 0xa } };

// {3D02CAB6-2B8E-4781-BA20-1C9267529467}
static const GUID c_guidProfile = 
{ 0x3d02cab6, 0x2b8e, 0x4781, { 0xba, 0x20, 0x1c, 0x92, 0x67, 0x52, 0x94, 0x67 } };


using namespace std;
using boost::filesystem::wpath;

BOOL copy_file(const wstring& src, const wstring& dest)
{
	BOOL ret = CopyFile(src.c_str(), dest.c_str(), FALSE);
	if (!ret)
	{
		for (int i = 0; i < 10; ++i)
		{
			wstring old = (boost::wformat(L"%1%.old.%2%") % dest % i).str();
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

BOOL delete_file(const wstring& file)
{
	BOOL ret = DeleteFile(file.c_str());
	if (!ret)
	{
		for (int i = 0; i < 10; ++i)
		{
			wstring old = (boost::wformat(L"%1%.old.%2%") % file % i).str();
			if (MoveFileEx(file.c_str(), old.c_str(), MOVEFILE_REPLACE_EXISTING))
			{
				MoveFileEx(old.c_str(), NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
				return TRUE;
			}
		}
	}
	return ret;
}

typedef int (*ime_register_func)(const wpath& ime_path, bool register_ime, bool is_wow64, bool hant, bool silent);

int install_ime_file(wpath& srcPath, const wstring& ext, bool hant, bool silent, ime_register_func func)
{
	wpath destPath;
	wpath wow64Path;

	WCHAR path[MAX_PATH];
	GetModuleFileName(GetModuleHandle(NULL), path, _countof(path));
	srcPath = path;

	bool is_x64 = (sizeof(HANDLE) == 8);
	wstring srcFileName = (hant ? L"weaselt" : L"weasel");
	srcFileName += (is_x64 ? L"x64" + ext : ext);
	srcPath = srcPath.remove_leaf() / srcFileName;

	GetSystemDirectory(path, _countof(path));
	destPath = path;
	destPath /= L"weasel" + ext;

	if (GetSystemWow64Directory(path, _countof(path)))
	{
		wow64Path = path;
		wow64Path /= L"weasel" + ext;
	}

	int retval = 0;
	// 复制 .dll/.ime 到系统目录
	if (!copy_file(srcPath.wstring(), destPath.wstring()))
	{
		if (!silent) MessageBox(NULL, destPath.wstring().c_str(), L"安裝失敗", MB_ICONERROR | MB_OK);
		return 1;
	}
	retval += func(destPath, true, false, hant, silent);
	if (!wow64Path.empty())
	{
		wstring x86 = srcPath.wstring();
		ireplace_last(x86, L"x64" + ext, ext);
		if (!copy_file(x86, wow64Path.wstring()))
		{
			if (!silent) MessageBox(NULL, wow64Path.wstring().c_str(), L"安裝失敗", MB_ICONERROR | MB_OK);
			return 1;
		}
		retval += func(wow64Path, true, true, hant, silent);
	}
	return retval;
}

int uninstall_ime_file(const wstring& ext, bool silent, ime_register_func func)
{
	int retval = 0;
	WCHAR path[MAX_PATH];
	GetSystemDirectory(path, _countof(path));
	wpath imePath = path;
	imePath /= L"weasel" + ext;
	retval += func(imePath, false, false, false, silent);
	if (!delete_file(imePath.wstring()))
	{
		if (!silent) MessageBox(NULL, imePath.wstring().c_str(), L"卸載失敗", MB_ICONERROR | MB_OK);
		retval += 1;
	}
	if (GetSystemWow64Directory(path, _countof(path)))
	{
		wpath wow64Path = path;
		wow64Path /= L"weasel" + ext;
		retval += func(wow64Path, false, true, false, silent);
		if (!delete_file(wow64Path.wstring()))
		{
			if (!silent) MessageBox(NULL, wow64Path.wstring().c_str(), L"卸載失敗", MB_ICONERROR | MB_OK);
			retval += 1;
		}
	}
	return retval;
}

// 注册IME输入法
int register_ime(const wpath& ime_path, bool register_ime, bool is_wow64, bool hant, bool silent)
{
	if (is_wow64)
	{
		return 0;  // only once
	}

	const WCHAR KEYBOARD_LAYOUTS_KEY[] = L"SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts";
	const WCHAR PRELOAD_KEY[] = L"Keyboard Layout\\Preload";

	if (register_ime)
	{
		HKL hKL = ImmInstallIME(ime_path.wstring().c_str(), WEASEL_IME_NAME);
		if (!hKL)
		{
			// manually register ime
			WCHAR hkl_str[16] = {0};
			HKEY hKey;
			LSTATUS ret = RegOpenKey(HKEY_LOCAL_MACHINE, KEYBOARD_LAYOUTS_KEY, &hKey);
			if (ret == ERROR_SUCCESS)
			{
				for (DWORD k = 0xE0200000 + (hant ? 0x0404 : 0x0804); true; k += 0x10000)
				{
					wsprintf(hkl_str, L"%08X", k);
					HKEY hSubKey;
					ret = RegOpenKey(hKey, hkl_str, &hSubKey);
					if (ret == ERROR_SUCCESS)
					{
						WCHAR imeFile[32] = {0};
						DWORD len = sizeof(imeFile);
						DWORD type = 0;
						ret = RegQueryValueEx(hSubKey, L"Ime File", NULL, &type, (LPBYTE)imeFile, &len);
						RegCloseKey(hSubKey);
						if (_wcsicmp(imeFile, L"weasel.ime") == 0)
						{
							hKL = (HKL)k;  // already there
						}
					}
					else
					{
						// found a spare number to register
						ret = RegCreateKey(hKey, hkl_str, &hSubKey);
						if (ret == ERROR_SUCCESS)
						{
							const WCHAR ime_file[] = L"weasel.ime";
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
						std::wstring number = (boost::wformat(L"%1%") % i).str();
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
			wsprintf(msg, L"註冊輸入法錯誤 ImmInstallIME: HKL=%x Err=%x", hKL, dwErr);
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
		if (!silent) MessageBox(NULL, KEYBOARD_LAYOUTS_KEY, L"卸載失敗", MB_ICONERROR | MB_OK);
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
			if (_wcsicmp(imeFile, L"weasel.ime") == 0)
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
				vector<wstring> preloads;
				wstring number;
				for (size_t i = 1; true; ++i)
				{
					number = (boost::wformat(L"%1%") % i).str();
					DWORD type = 0;
					WCHAR value[32];
					DWORD len = sizeof(value);
					ret = RegQueryValueEx(hPreloadKey, number.c_str(), 0, &type, (LPBYTE)value, &len);
					if (ret != ERROR_SUCCESS)
					{
						if (i > preloads.size())
						{
							// 删除最大一号注册表值
							number = (boost::wformat(L"%1%") % (i - 1)).str();
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
					number = (boost::wformat(L"%1%") % (i + 1)).str();
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

	//Create the object. 
	hr = CoCreateInstance(  CLSID_TF_InputProcessorProfiles, 
							NULL, 
							CLSCTX_INPROC_SERVER, 
							IID_ITfInputProcessorProfiles, 
							(LPVOID*)&pProfiles);

	if(SUCCEEDED(hr))
	{
		LANGID lang_id = hant ? 0x0404 : 0x0804;
		//Use the interface. 
		pProfiles->EnableLanguageProfile(c_clsidTextService, lang_id, c_guidProfile, fEnable);
		pProfiles->EnableLanguageProfileByDefault(c_clsidTextService, lang_id, c_guidProfile, fEnable);

		//Release the interface. 
		pProfiles->Release();
	}
}

// 注册TSF输入法
int register_text_service(const wpath& tsf_path, bool register_ime, bool is_wow64, bool hant, bool silent)
{
	if (!register_ime)
		enable_profile(FALSE, hant);

	wstring params = L" \"" + tsf_path.wstring() + L"\"";
	if (!register_ime)
	{
		params = L" /u " + params;  // unregister
	}
	//if (silent)  // always silent
	{
		params = L" /s " + params;
	}

	SHELLEXECUTEINFO shExInfo = {0};
	shExInfo.cbSize = sizeof(shExInfo);
	shExInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	shExInfo.hwnd = 0;
	shExInfo.lpVerb = L"open";                 // Operation to perform
	shExInfo.lpFile = L"regsvr32.exe";         // Application to start    
	shExInfo.lpParameters = params.c_str();    // Additional parameters
	shExInfo.lpDirectory = 0;
	shExInfo.nShow = SW_SHOW;
	shExInfo.hInstApp = 0;  

	if (ShellExecuteEx(&shExInfo))
	{
		WaitForSingleObject(shExInfo.hProcess, INFINITE);
		CloseHandle(shExInfo.hProcess);
	}
	else
	{
		WCHAR msg[100];
		wsprintf(msg, L"註冊輸入法錯誤 regsvr32.exe %s", params.c_str());
		if (!silent) MessageBox(NULL, msg, L"安装/卸載失败", MB_ICONERROR | MB_OK);
		return 1;
	}

	if (register_ime)
		enable_profile(TRUE, hant);

	return 0;
}

int install(bool hant, bool silent)
{
	wpath ime_src_path;
	int retval = 0;
	retval += install_ime_file(ime_src_path, L".ime", hant, silent, &register_ime);
	retval += install_ime_file(ime_src_path, L".dll", hant, silent, &register_text_service);

	// 写注册表
	HKEY hKey;
	LSTATUS ret = RegCreateKeyEx(HKEY_LOCAL_MACHINE, WEASEL_REG_KEY,
		                         0, NULL, 0, KEY_ALL_ACCESS | KEY_WOW64_32KEY, 0, &hKey, NULL);
	if (FAILED(HRESULT_FROM_WIN32(ret)))
	{
		if (!silent) MessageBox(NULL, WEASEL_REG_KEY, L"安裝失敗", MB_ICONERROR | MB_OK);
		return 1;
	}

	wstring rootDir = ime_src_path.parent_path().wstring();
	ret = RegSetValueEx(hKey, L"WeaselRoot", 0, REG_SZ,
		                (const BYTE*)rootDir.c_str(),
						(rootDir.length() + 1) * sizeof(WCHAR));
	if (FAILED(HRESULT_FROM_WIN32(ret)))
	{
		if (!silent) MessageBox(NULL, L"無法寫入 WeaselRoot", L"安裝失敗", MB_ICONERROR | MB_OK);
		return 1;
	}

	const wstring executable = L"WeaselServer.exe";
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

	if (!silent) MessageBox(NULL, L"可以使【小狼毫】寫字了 :)", L"安裝完成", MB_ICONINFORMATION | MB_OK);
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

	if (!silent) MessageBox(NULL, L"小狼毫 :)", L"卸載完成", MB_ICONINFORMATION | MB_OK);
	return 0;
}

bool has_installed() {
	WCHAR path[MAX_PATH];
	GetSystemDirectory(path, _countof(path));
	wpath imePath = path;
	imePath /= L"weasel.ime";
	return boost::filesystem::exists(imePath);
}
