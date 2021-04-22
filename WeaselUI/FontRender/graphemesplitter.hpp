#pragma once

#include <string>
#include <vector>

#if defined(_WIN32)

#ifndef NOMINMAX
#define NOMINMAX
#undef max
#undef min
#endif // !NOMINMAX

#include <BaseTsd.h>
#include <intrin.h>
#include <windows.h>

//#undef ERROR
typedef SSIZE_T ssize_t;
#endif

namespace graphemesplitter {

int32_t utf8_codepoint(const char *str, size_t length, size_t pos);
std::string codepoint_to_utf8(int32_t codepoint);

size_t next_grapheme(const char *str, size_t length, size_t pos);

}
