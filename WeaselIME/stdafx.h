// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define NOIME

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <shellapi.h>

#include "immdev.h"

#pragma warning(disable : 4819)

#include <boost/filesystem.hpp>

#pragma warning(default : 4819)

#include <map>
#include <memory>
#include <mutex>
#include <string>

using boost::filesystem::wpath;
