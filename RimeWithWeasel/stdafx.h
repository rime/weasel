// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <windows.h>

#pragma warning(disable : 4819)

#include <boost/interprocess/streams/bufferstream.hpp>
#include <boost/archive/text_woarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/thread.hpp>

#pragma warning(default : 4819)
#include <algorithm>
#include <list>
#include <set>
#include <string>
#include <sstream>

using boost::interprocess::wbufferstream;
