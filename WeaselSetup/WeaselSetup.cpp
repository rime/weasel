// WeaselSetup.cpp : main source file for WeaselSetup.exe
//

#include "stdafx.h"

#include "resource.h"

#include "InstallOptionsDlg.h"

#include <ShellScalingApi.h>
#pragma comment(lib, "Shcore.lib")
CAppModule _Module;

static int Run(LPTSTR lpCmdLine);

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int /*nCmdShow*/)
{
	HRESULT hRes = ::CoInitialize(NULL);
	ATLASSERT(SUCCEEDED(hRes));

	AtlInitCommonControls(ICC_BAR_CLASSES);	// add flags to support other controls

	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	int nRet = Run(lpstrCmdLine);

	_Module.Term();
	::CoUninitialize();

	return nRet;
}
int install(bool hant, bool silent, bool old_ime_support);
int uninstall(bool silent);
bool has_installed();

static int CustomInstall(bool installing)
{
	bool hant = false;
	bool silent = false;
	bool old_ime_support = false;
	std::wstring user_dir;

	const WCHAR KEY[] = L"Software\\Rime\\Weasel";
	HKEY hKey;
	LSTATUS ret = RegOpenKey(HKEY_CURRENT_USER, KEY, &hKey);
	if (ret == ERROR_SUCCESS)
	{
		WCHAR value[MAX_PATH];
		DWORD len = sizeof(value);
		DWORD type = 0;
		DWORD data = 0;
		ret = RegQueryValueEx(hKey, L"RimeUserDir", NULL, &type, (LPBYTE)value, &len);
		if (ret == ERROR_SUCCESS && type == REG_SZ)
		{
			user_dir = value;
		}
		len = sizeof(data);
		ret = RegQueryValueEx(hKey, L"Hant", NULL, &type, (LPBYTE)&data, &len);
		if (ret == ERROR_SUCCESS && type == REG_DWORD)
		{
			hant = (data != 0);
			if (installing)
				silent = true;
		}
		RegCloseKey(hKey);
	}

	if (!silent)
	{
		InstallOptionsDialog dlg;
		dlg.installed = has_installed();
		dlg.hant = hant;
		dlg.user_dir = user_dir;
		if (IDOK != dlg.DoModal()) {
			if (!installing)
				return 1;  // aborted by user
		}
		else {
			hant = dlg.hant;
			user_dir = dlg.user_dir;
			old_ime_support = dlg.old_ime_support;
		}
	}
	if (0 != install(hant, silent, old_ime_support))
		return 1;

	ret = RegCreateKeyEx(HKEY_CURRENT_USER, KEY,
		0, NULL, 0, KEY_ALL_ACCESS, 0, &hKey, NULL);
	if (FAILED(HRESULT_FROM_WIN32(ret)))
	{
		MessageBox(NULL, KEY, L"安裝失敗", MB_ICONERROR | MB_OK);
		return 1;
	}

	ret = RegSetValueEx(hKey, L"RimeUserDir", 0, REG_SZ,
		(const BYTE*)user_dir.c_str(),
		(user_dir.length() + 1) * sizeof(WCHAR));
	if (FAILED(HRESULT_FROM_WIN32(ret)))
	{
		MessageBox(NULL, L"無法寫入 RimeUserDir", L"安裝失敗", MB_ICONERROR | MB_OK);
		return 1;
	}

	DWORD data = hant ? 1 : 0;
	ret = RegSetValueEx(hKey, L"Hant", 0, REG_DWORD, (const BYTE*)&data, sizeof(DWORD));
	if (FAILED(HRESULT_FROM_WIN32(ret)))
	{
		MessageBox(NULL, L"無法寫入 Hant", L"安裝失敗", MB_ICONERROR | MB_OK);
		return 1;
	}

	return 0;
}

static int Run(LPTSTR lpCmdLine)
{
	constexpr bool silent = true;
	constexpr bool old_ime_support = false;
	bool uninstalling = !wcscmp(L"/u", lpCmdLine);
	if (uninstalling)
		return uninstall(silent);

	bool hans = !wcscmp(L"/s", lpCmdLine);
	if (hans)
		return install(false, silent, old_ime_support);
	bool hant = !wcscmp(L"/t", lpCmdLine);
	if (hant)
		return install(true, silent, old_ime_support);
	bool installing = !wcscmp(L"/i", lpCmdLine);
	return CustomInstall(installing);
}

