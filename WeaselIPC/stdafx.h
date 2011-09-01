// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#include <windows.h>

#pragma warning(disable : 4819)
#pragma warning(disable : 4996)

#include <boost/algorithm/string.hpp>
#include <boost/function.hpp>
#include <boost/interprocess/streams/bufferstream.hpp>
#include <boost/interprocess/windows_shared_memory.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/smart_ptr.hpp>

#pragma warning(default: 4819)
#pragma warning(default: 4996)

#include <map>
#include <string>
#include <vector>

using namespace std;
using namespace boost::algorithm;
using boost::interprocess::wbufferstream;
using boost::shared_ptr;
