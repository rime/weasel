#pragma once
#include <string>

// UTF-8 conversion
const WCHAR* utf8towcs(const char* utf8_str);
int utf8towcslen(const char* utf8_str, int utf8_len);

// data directories
const std::wstring WeaselUserDataPath();

const char* weasel_shared_data_dir();
const char* weasel_user_data_dir();
