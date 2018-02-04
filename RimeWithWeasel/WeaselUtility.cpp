#include "stdafx.h"
#include <string>

const char* wcstoutf8(const WCHAR* wstr)
{
	const int buffer_len = 1024;
	static char buffer[buffer_len];
	memset(buffer, 0, sizeof(buffer));
	WideCharToMultiByte(CP_UTF8, 0, wstr, -1, buffer, buffer_len - 1, NULL, NULL);
	return buffer;
}

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

const char* weasel_shared_data_dir() {
	static char path[MAX_PATH] = {0};
	GetModuleFileNameA(NULL, path, _countof(path));
	std::string str_path(path);
	size_t k = str_path.find_last_of("/\\");
	strcpy_s(path + k + 1, _countof(path) - (k + 1), "data");
	return path;
}

const char* weasel_user_data_dir() {
	static char path[MAX_PATH] = {0};
	// Windows wants multi-byte file paths in native encoding
	WideCharToMultiByte(CP_ACP, 0, WeaselUserDataPath().c_str(), -1, path, _countof(path) - 1, NULL, NULL);
	return path;
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
