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

inline bool check_update()
{
  // when checked manually, show testing versions too
  std::string feed_url = GetCustomResource("ManualUpdateFeedURL", "APPCAST");
  if (!feed_url.empty())
  {
    win_sparkle_set_appcast_url(feed_url.c_str());
  }
  win_sparkle_check_update_with_ui();
  return true;
}
