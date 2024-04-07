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

template <typename CharT>
struct Escape {
  static constexpr CharT escape_char;
  static constexpr CharT linefeed;
  static constexpr CharT tab;
  static constexpr CharT linefeed_escape_char;
  static constexpr CharT tab_escape_char;
};

template <>
struct Escape<char>::escape = '\\';
template <>
struct Escape<char>::linefeed = '\n';
template <>
struct Escape<char>::tab = '\t';
template <>
struct Escape<char>::linefeed_escape = 'n';
template <>
struct Escape<char>::tab_escape = 't';

template <>
struct Escape<wchar_t>::escape = L'\\';
template <>
struct Escape<wchar_t>::linefeed = L'\n';
template <>
struct Escape<wchar_t>::tab = L'\t';
template <>
struct Escape<wchar_t>::linefeed_escape = L'n';
template <>
struct Escape<wchar_t>::tab_escape = L't';

template <typename CharT>
inline std::basic_string<CharT> escape_string(
    const std::basic_string<CharT> input) {
  using Escape<CharT>::escape;
  using Escape<CharT>::linefeed;
  using Escape<CharT>::tab;
  using Escape<CharT>::linefeed_escape;
  using Escape<CharT>::tab_escape;
  std::basic_stringstream<CharT> res;
  for (auto p = input.begin(); p != input.end(); ++p) {
    if (*p == escape) {
      res << escape << escape;
    } else if (*p == linefeed) {
      res << escape << linefeed_escape;
    } else if (*p == tab) {
      res << escape << tab_escape;
    } else {
      res << *p;
    }
  }
  return res.str();
}

template <typename CharT>
inline std::basic_string<CharT> unescape_string(
    const std::basic_string<CharT>& input) {
  using Escape<CharT>::escape;
  using Escape<CharT>::linefeed;
  using Escape<CharT>::tab;
  using Escape<CharT>::linefeed_escape;
  using Escape<CharT>::tab_escape;
  std::basic_stringstream<CharT> res;
  for (auto p = input.begin(); p != input.end(); ++p) {
    if (*p == escape) {
      if (++p == input.end()) {
        break;
      } else if (*p == linefeed_escape) {
        res << linefeed;
      } else if (*p == tab_escape) {
        res << tab;
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
