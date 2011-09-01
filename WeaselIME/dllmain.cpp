// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include "WeaselIME.h"


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	WeaselIME::SetModuleInstance(hModule);

	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		{
			HRESULT hr = WeaselIME::RegisterUIClass();
			if (FAILED(hr))
				return FALSE;
		}
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		WeaselIME::Cleanup();
		WeaselIME::UnregisterUIClass();
		break;
	}
	return TRUE;
}

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

//
// rundll32
//

extern "C" __declspec(dllexport)
void install(HWND hWnd, HINSTANCE hInstance, LPWSTR lpszCmdLine, int nCmdShow)
{
	WCHAR path[MAX_PATH];

	GetModuleFileName(WeaselIME::GetModuleInstance(), path, _countof(path));
	wpath srcPath(path);
	
	ExpandEnvironmentStrings(L"%SystemRoot%\\system32\\", path, _countof(path));
	wpath destPath(path);
	destPath /= WeaselIME::GetIMEFileName();

	// 复制 weasel.ime 到系统目录
	if (!copy_file(srcPath.native_file_string(), destPath.native_file_string()))
	{
		MessageBox(hWnd, destPath.native_file_string().c_str(), L"安裝失敗", MB_ICONERROR | MB_OK);
		return;
	}

	// 写注册表
	HKEY hKey;
	LSTATUS ret = RegCreateKeyEx(HKEY_LOCAL_MACHINE, WeaselIME::GetRegKey(), 
		                         0, NULL, 0, KEY_ALL_ACCESS, 0, &hKey, NULL);
	if (FAILED(HRESULT_FROM_WIN32(ret)))
	{
		MessageBox(hWnd, WeaselIME::GetRegKey(), L"安裝失敗", MB_ICONERROR | MB_OK);
		return;
	}

	wstring rootDir = srcPath.parent_path().native_directory_string();
	ret = RegSetValueEx(hKey, L"WeaselRoot", 0, REG_SZ, 
		                (const BYTE*)rootDir.c_str(),  
						(rootDir.length() + 1) * sizeof(WCHAR));
	if (FAILED(HRESULT_FROM_WIN32(ret)))
	{
		MessageBox(hWnd, L"無法寫入 WeaselRoot", L"安裝失敗", MB_ICONERROR | MB_OK);
		return;
	}

	const wstring executable = L"WeaselServer.exe";
	ret = RegSetValueEx(hKey, L"ServerExecutable", 0, REG_SZ, 
		                (const BYTE*)executable.c_str(),  
						(executable.length() + 1) * sizeof(WCHAR));
	if (FAILED(HRESULT_FROM_WIN32(ret)))
	{
		MessageBox(hWnd, L"無法寫入 ServerExecutable", L"安裝失敗", MB_ICONERROR | MB_OK);
		return;
	}

	RegCloseKey(hKey);

	// 注册输入法
	HKL hKL = ImmInstallIME(destPath.native_file_string().c_str(), WeaselIME::GetIMEName());
	if (!hKL)
	{
		DWORD dwErr = GetLastError();
		WCHAR msg[100];
		wsprintf(msg, L"ImmInstallIME: HKL=%x Err=%x", hKL, dwErr);
		MessageBox(hWnd, msg, L"安裝失敗", MB_ICONERROR | MB_OK);
		return;
	}

	MessageBox(hWnd, L"小狼毫 :)", L"安裝完成", MB_OK);
}

extern "C" __declspec(dllexport)
void uninstall(HWND hWnd, HINSTANCE hInstance, LPWSTR lpszCmdLine, int nCmdShow)
{
	const WCHAR KL_KEY[] = L"SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts";
	const WCHAR PRELOAD_KEY[] = L"Keyboard Layout\\Preload";

	HKEY hKey;
	LSTATUS ret = RegOpenKey(HKEY_LOCAL_MACHINE, KL_KEY, &hKey);
	if (ret != ERROR_SUCCESS)
	{
		MessageBox(hWnd, KL_KEY, L"卸載失敗", MB_ICONERROR | MB_OK);
		return;
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
			if (_wcsicmp(imeFile, WeaselIME::GetIMEFileName()) == 0)
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

	// 清除注册信息
	RegDeleteKey(HKEY_LOCAL_MACHINE, WeaselIME::GetRegKey());

	// 删除文件
	WCHAR path[MAX_PATH];
	ExpandEnvironmentStrings(L"%SystemRoot%\\system32\\", path, _countof(path));
	wstring file = path;
	file += WeaselIME::GetIMEFileName();
	if (!delete_file(file))
	{
		MessageBox(hWnd, file.c_str(), L"卸載失敗", MB_ICONERROR | MB_OK);
		return;
	}

	MessageBox(hWnd, L"小狼毫 :)", L"卸載完成", MB_OK);
}
