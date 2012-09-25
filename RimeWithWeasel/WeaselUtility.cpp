#include "stdafx.h"
#include <string>

const WCHAR* utf8towcs(const char* utf8_str)
{
	const int buffer_len = 4096;
	static WCHAR buffer[buffer_len];
	memset(buffer, 0, sizeof(buffer));
	MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, buffer, buffer_len - 1);
	return buffer;
}

int utf8towcslen(const char* utf8_str, int utf8_len)
{
	return MultiByteToWideChar(CP_UTF8, 0, utf8_str, utf8_len, NULL, 0);
}

const std::wstring WeaselUserDataPath() {
	WCHAR path[MAX_PATH] = {0};
	const WCHAR KEY[] = L"Software\\Rime\\Weasel";
	HKEY hKey;
	LSTATUS ret = RegOpenKey(HKEY_CURRENT_USER, KEY, &hKey);
	if (ret == ERROR_SUCCESS)
	{
		DWORD len = sizeof(path);
		DWORD type = 0;
		DWORD data = 0;
		ret = RegQueryValueEx(hKey, L"RimeUserDir", NULL, &type, (LPBYTE)path, &len);
		RegCloseKey(hKey);
		if (ret == ERROR_SUCCESS && type == REG_SZ && path[0])
		{
			return path;
		}
	}
	// default location
	ExpandEnvironmentStringsW(L"%AppData%\\Rime", path, _countof(path));
	return path;
}

const char* weasel_shared_data_dir() {
	static char path[MAX_PATH] = {0};
	GetModuleFileNameA(NULL, path, _countof(path));
	std::string str_path(path);
	size_t k = str_path.find_last_of("/\\");
	strcpy(path + k + 1, "data");
	return path;
}

const char* weasel_user_data_dir() {
	static char path[MAX_PATH] = {0};
	// Windows wants multi-byte file paths in native encoding
	WideCharToMultiByte(CP_ACP, 0, WeaselUserDataPath().c_str(), -1, path, _countof(path) - 1, NULL, NULL);
	return path;
}
