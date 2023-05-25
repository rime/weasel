#pragma once
#include <string>

// UTF-8 conversion
inline const char* wcstoutf8(const WCHAR* wstr)
{
	int buffer_len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
	static char* buffer;
	if(buffer)
		delete []buffer;
	buffer = new char[buffer_len+1];
	memset(buffer, 0, buffer_len+1);
	WideCharToMultiByte(CP_UTF8, 0, wstr, -1, buffer, buffer_len, NULL, NULL);
	return buffer;
}

inline const WCHAR* utf8towcs(const char* utf8_str)
{
	int nLen = MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, NULL, 0);
	static WCHAR* wbuffer;
	if(wbuffer)
		delete []wbuffer;
	wbuffer = new WCHAR[nLen + 1];
	memset(wbuffer, 0, sizeof(WCHAR)*(nLen + 1));
	MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, wbuffer, nLen);
	return wbuffer;
}

inline int utf8towcslen(const char* utf8_str, int utf8_len)
{
	return MultiByteToWideChar(CP_UTF8, 0, utf8_str, utf8_len, NULL, 0);
}

inline std::wstring getUsername() {
	DWORD len = 0;
	GetUserName(NULL, &len);

	if (len <= 0) {
		return L"";
	}

	wchar_t *username = new wchar_t[len + 1];

	GetUserName(username, &len);
	if (len <= 0) {
		delete[] username;
		return L"";
	}
	auto res = std::wstring(username);
	delete[] username;
	return res;
}

// data directories
std::wstring WeaselUserDataPath();

const char* weasel_shared_data_dir();
const char* weasel_user_data_dir();

inline std::wstring string_to_wstring(const std::string& str, int code_page = CP_ACP)
{
	// support CP_ACP and CP_UTF8 only
	if (code_page != 0 && code_page != CP_UTF8)	return L"";
	// calc len
	int len = MultiByteToWideChar(code_page, 0, str.c_str(), str.size(), NULL, 0);
    if(len <= 0)    return L"";
	std::wstring res;
	TCHAR* buffer = new TCHAR[len + 1];
	MultiByteToWideChar(code_page, 0, str.c_str(), str.size(), buffer, len);
	buffer[len] = '\0';
	res.append(buffer);
	delete[] buffer;
	return res;
}

inline std::string wstring_to_string(const std::wstring& wstr, int code_page = CP_ACP)
{
	// support CP_ACP and CP_UTF8 only
	if (code_page != 0 && code_page != CP_UTF8)	return "";
	int len = WideCharToMultiByte(code_page, 0, wstr.c_str(), wstr.size(), NULL, 0, NULL, NULL);
    if(len <= 0)    return "";
	std::string res;
	char* buffer = new char[len + 1];
	WideCharToMultiByte(code_page, 0, wstr.c_str(), wstr.size(), buffer, len, NULL, NULL);
	buffer[len] = '\0';
	res.append(buffer);
	delete[] buffer;
	return res;
}

// resource
std::string GetCustomResource(const char *name, const char *type);
