#pragma once
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

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
std::filesystem::path WeaselSharedDataPath();
std::filesystem::path WeaselUserDataPath();
inline fs::path WeaselLogPath() {
  WCHAR _path[MAX_PATH] = {0};
  // default location
  ExpandEnvironmentStringsW(L"%TEMP%\\rime.weasel", _path, _countof(_path));
  fs::path path = fs::path(_path);
  if (!fs::exists(path)) {
    fs::create_directories(path);
  }
  return path;
}

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

inline BOOL is_wow64() {
  DWORD errorCode;
  if (GetSystemWow64DirectoryW(NULL, 0) == 0)
    if ((errorCode = GetLastError()) == ERROR_CALL_NOT_IMPLEMENTED)
      return FALSE;
    else
      ExitProcess((UINT)errorCode);
  else
    return TRUE;
}

template <typename CharT>
struct EscapeChar {
  static const CharT escape;
  static const CharT linefeed;
  static const CharT tab;
  static const CharT linefeed_escape;
  static const CharT tab_escape;
};

template <>
const char EscapeChar<char>::escape = '\\';
template <>
const char EscapeChar<char>::linefeed = '\n';
template <>
const char EscapeChar<char>::tab = '\t';
template <>
const char EscapeChar<char>::linefeed_escape = 'n';
template <>
const char EscapeChar<char>::tab_escape = 't';

template <>
const wchar_t EscapeChar<wchar_t>::escape = L'\\';
template <>
const wchar_t EscapeChar<wchar_t>::linefeed = L'\n';
template <>
const wchar_t EscapeChar<wchar_t>::tab = L'\t';
template <>
const wchar_t EscapeChar<wchar_t>::linefeed_escape = L'n';
template <>
const wchar_t EscapeChar<wchar_t>::tab_escape = L't';

template <typename CharT>
inline std::basic_string<CharT> escape_string(
    const std::basic_string<CharT> input) {
  using Esc = EscapeChar<CharT>;
  std::basic_stringstream<CharT> res;
  for (auto p = input.begin(); p != input.end(); ++p) {
    if (*p == Esc::escape) {
      res << Esc::escape << Esc::escape;
    } else if (*p == Esc::linefeed) {
      res << Esc::escape << Esc::linefeed_escape;
    } else if (*p == Esc::tab) {
      res << Esc::escape << Esc::tab_escape;
    } else {
      res << *p;
    }
  }
  return res.str();
}

template <typename CharT>
inline std::basic_string<CharT> unescape_string(
    const std::basic_string<CharT>& input) {
  using Esc = EscapeChar<CharT>;
  std::basic_stringstream<CharT> res;
  for (auto p = input.begin(); p != input.end(); ++p) {
    if (*p == Esc::escape) {
      if (++p == input.end()) {
        break;
      } else if (*p == Esc::linefeed_escape) {
        res << Esc::linefeed;
      } else if (*p == Esc::tab_escape) {
        res << Esc::tab;
      } else {  // \a => a
        res << *p;
      }
    } else {
      res << *p;
    }
  }
  return res.str();
}

// resource
std::string GetCustomResource(const char* name, const char* type);

inline std::wstring get_weasel_ime_name() {
  LANGID langId = GetUserDefaultUILanguage();

  if (langId == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL) ||
      langId == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED) ||
      langId == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_HONGKONG) ||
      langId == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SINGAPORE) ||
      langId == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_MACAU)) {
    return L"小狼毫";
  } else {
    return L"Weasel";
  }
}

inline LONG RegGetStringValue(HKEY key,
                              LPCWSTR lpSubKey,
                              LPCWSTR lpValue,
                              std::wstring& value) {
  TCHAR szValue[MAX_PATH];
  DWORD dwBufLen = MAX_PATH;

  LONG lRes = RegGetValue(key, lpSubKey, lpValue, RRF_RT_REG_SZ, NULL, szValue,
                          &dwBufLen);
  if (lRes == ERROR_SUCCESS) {
    value = std::wstring(szValue);
  }
  return lRes;
}

inline LANGID get_language_id() {
  std::wstring lang{};
  if (RegGetStringValue(HKEY_CURRENT_USER, L"Software\\Rime\\Weasel",
                        L"Language", lang) == ERROR_SUCCESS) {
    if (lang == L"chs")
      return MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED);
    else if (lang == L"cht")
      return MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL);
    else if (lang == L"eng")
      return MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
  }
  LANGID langId = GetUserDefaultUILanguage();
  if (langId == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED) ||
      langId == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SINGAPORE)) {
    langId = MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED);
  } else if (langId == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL) ||
             langId == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_HONGKONG) ||
             langId == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_MACAU)) {
    langId = MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL);
  } else {
    langId = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
  }
  return langId;
}
