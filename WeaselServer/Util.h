#pragma once
#include <winsparkle.h>

inline bool execute(const std::wstring& cmd, const std::wstring& args)
{
  return reinterpret_cast<int>(ShellExecuteW(NULL, NULL, cmd.c_str(), args.c_str(), NULL, SW_SHOWNORMAL)) > 32;
}

inline bool explore(const std::wstring& path)
{
  return reinterpret_cast<int>(ShellExecuteW(NULL, L"open", L"explorer", (L"\"" + path + L"\"").c_str(), NULL, SW_SHOWNORMAL)) > 32;
}

inline bool open(const std::wstring& path)
{
  return reinterpret_cast<int>(ShellExecuteW(NULL, L"open", path.c_str(), NULL, NULL, SW_SHOWNORMAL)) > 32;
}

inline std::string get_resource(const char *name, const char *type)
{
  HRSRC hRes = FindResourceA(NULL, name, type);
  if (!hRes) return {};
  HGLOBAL hData = LoadResource(NULL, hRes);
  if (!hData) return {};
  const char *data = static_cast<const char*>(LockResource(hData));
  size_t size = ::SizeofResource(NULL, hRes);
  if ( data && size )
  {
    if ( data[size-1] == '\0' ) // null-terminated string
      size--;
    return {data, size};
  }
  
  return {};
}

inline bool check_update()
{
  // when checked manually, show testing versions too
  std::string feed_url = get_resource("ManualUpdateFeedURL", "APPCAST");
  if (!feed_url.empty())
  {
    win_sparkle_set_appcast_url(feed_url.c_str());
  }
  win_sparkle_check_update_with_ui();
  return true;
}
