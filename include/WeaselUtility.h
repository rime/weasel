#pragma once
#include <boost/filesystem.hpp>
#include <string>

inline int utf8towcslen(const char* utf8_str, int utf8_len) {
  return MultiByteToWideChar(CP_UTF8, 0, utf8_str, utf8_len, NULL, 0);
}

inline std::wstring getUsername() {
  DWORD len = 0;
  GetUserName(NULL, &len);

  if (len <= 0) {
    return L"";
  }

  wchar_t* username = new wchar_t[len + 1];

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
boost::filesystem::path WeaselSharedDataPath();
boost::filesystem::path WeaselUserDataPath();

inline BOOL IsUserDarkMode() {
  constexpr const LPCWSTR key =
      L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize";
  constexpr const LPCWSTR value = L"AppsUseLightTheme";

  DWORD type;
  DWORD data;
  DWORD size = sizeof(DWORD);
  LSTATUS st = RegGetValue(HKEY_CURRENT_USER, key, value, RRF_RT_REG_DWORD,
                           &type, &data, &size);

  if (st == ERROR_SUCCESS && type == REG_DWORD)
    return data == 0;
  return false;
}

inline std::wstring string_to_wstring(const std::string& str,
                                      int code_page = CP_ACP) {
  // support CP_ACP and CP_UTF8 only
  if (code_page != 0 && code_page != CP_UTF8)
    return L"";
  // calc len
  int len =
      MultiByteToWideChar(code_page, 0, str.c_str(), (int)str.size(), NULL, 0);
  if (len <= 0)
    return L"";
  std::wstring res;
  TCHAR* buffer = new TCHAR[len + 1];
  MultiByteToWideChar(code_page, 0, str.c_str(), (int)str.size(), buffer, len);
  buffer[len] = '\0';
  res.append(buffer);
  delete[] buffer;
  return res;
}

inline std::string wstring_to_string(const std::wstring& wstr,
                                     int code_page = CP_ACP) {
  // support CP_ACP and CP_UTF8 only
  if (code_page != 0 && code_page != CP_UTF8)
    return "";
  int len = WideCharToMultiByte(code_page, 0, wstr.c_str(), (int)wstr.size(),
                                NULL, 0, NULL, NULL);
  if (len <= 0)
    return "";
  std::string res;
  char* buffer = new char[len + 1];
  WideCharToMultiByte(code_page, 0, wstr.c_str(), (int)wstr.size(), buffer, len,
                      NULL, NULL);
  buffer[len] = '\0';
  res.append(buffer);
  delete[] buffer;
  return res;
}

inline std::wstring unescape_wstring(const std::wstring& input) {
  std::wstring res = input;
  std::wstring search1 = L"\\\\";
  std::wstring replace1 = L"\\";
  std::wstring search2 = L"\\n";
  std::wstring replace2 = L"\n";

  // unescape L"\\n" to L"\n", skip L"\\\\n"
  size_t pos = 0;
  while ((pos = res.find(search2, pos)) != std::wstring::npos) {
    if (pos > 0 && res[pos - 1] == L'\\') {
      pos += search2.length();
      continue;
    }
    res.replace(pos, search2.length(), replace2);
    pos += replace2.length();
  }

  // unescape L"\\\\" to L"\\"
  pos = 0;
  while ((pos = res.find(search1, pos)) != std::wstring::npos) {
    res.replace(pos, search1.length(), replace1);
    pos += replace1.length();
  }

  return res;
}

// resource
std::string GetCustomResource(const char* name, const char* type);
