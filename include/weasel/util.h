#pragma once
#include <wil/resource.h>
#include <xstring>

namespace weasel::utils
{

#pragma region win32
inline wil::unique_event_nothrow make_manual_reset_event()
{
  wil::unique_event_nothrow event;
  event.create(wil::EventOptions::ManualReset, nullptr);
  return event;
}

inline std::string ws_to_u8s(const std::wstring& input)
{
  if (input.empty()) return {};
  const auto char_count = WideCharToMultiByte(
    CP_UTF8,
    0,
    input.data(),
    static_cast<int>(input.size()),
    nullptr,
    0,
    nullptr,
    nullptr
  );
  if (char_count < 0) return {};
  std::string result;
  result.resize(char_count);
  WideCharToMultiByte(
    CP_UTF8,
    0,
    input.data(),
    static_cast<int>(input.size()),
    result.data(),
    result.size(),
    nullptr,
    nullptr
  );
  return result;
}

inline std::wstring u8s_to_ws(const std::string& input)
{
  if (input.empty()) return {};

  auto wchar_count = MultiByteToWideChar(
    CP_UTF8,
    0,
    input.data(),
    input.size(),
    nullptr,
    0
  );
  std::wstring result;
  result.resize(wchar_count);
  MultiByteToWideChar(
    CP_UTF8,
    0,
    input.data(),
    input.size(),
    result.data(),
    result.size()
  );
}

SECURITY_ATTRIBUTES make_security_attributes();

std::wstring get_wusername();
std::wstring install_dir();
std::wstring shared_data_dir();
std::wstring user_data_dir();

}

