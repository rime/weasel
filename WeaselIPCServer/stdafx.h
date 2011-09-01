// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#include <windows.h>

#include <atlbase.h>
#include <atlwin.h>

#pragma warning(disable : 4996)

#include <wtl/atlapp.h>
#include <wtl/atlframe.h>

#pragma warning(default: 4996)

#pragma warning(disable : 4819)

#include <boost/interprocess/windows_shared_memory.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/smart_ptr.hpp>

#pragma warning(default: 4819)

using namespace boost::interprocess;
using boost::shared_ptr;
