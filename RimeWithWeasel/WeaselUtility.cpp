#include "stdafx.h"
#include <string>
#include <WeaselUtility.h>

std::wstring WeaselUserDataPath() {
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

std::string weasel_shared_data_dir() {
	wchar_t _path[MAX_PATH] = {0};
	GetModuleFileNameW(NULL, _path, _countof(_path));
	std::wstring _pathw(_path);
	return wstring_to_string(_pathw, CP_UTF8);
}

std::string weasel_user_data_dir() {
	char path[MAX_PATH] = {0};
	// Windows wants multi-byte file paths in native encoding
	WideCharToMultiByte(CP_UTF8, 0, WeaselUserDataPath().c_str(), -1, path, _countof(path) - 1, NULL, NULL);
	return std::string(path);
}

std::string GetCustomResource(const char *name, const char *type)
{
    const HINSTANCE module = 0; // main executable
    HRSRC hRes = FindResourceA(module, name, type);
    if ( hRes )
    {
        HGLOBAL hData = LoadResource(module, hRes);
        if ( hData )
        {
            const char *data = (const char*)::LockResource(hData);
            size_t size = ::SizeofResource(module, hRes);

            if ( data && size )
            {
                if ( data[size-1] == '\0' ) // null-terminated string
                    size--;
                return std::string(data, size);
            }
        }
    }

    return std::string();
}
