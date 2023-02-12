#pragma once
#include <string>
#include <codecvt>

// UTF-8 conversion
inline const char* wcstoutf8(const WCHAR* wstr)
{
	int buffer_len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
	static char* buffer;
	if(buffer)
		delete []buffer;
	buffer = new char[buffer_len];
	memset(buffer, 0, buffer_len);
	WideCharToMultiByte(CP_UTF8, 0, wstr, -1, buffer, buffer_len - 1, NULL, NULL);
	return buffer;
}

inline const WCHAR* utf8towcs(const char* utf8_str)
{
	const int buffer_len = 4096;
	static WCHAR buffer[buffer_len];
	memset(buffer, 0, sizeof(buffer));
	MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, buffer, buffer_len - 1);
	return buffer;
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

inline std::string to_byte_string(const std::wstring& input)
{
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.to_bytes(input);
}

inline std::wstring to_wide_string(const std::string& input)
{
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.from_bytes(input);
}
// resource
std::string GetCustomResource(const char *name, const char *type);