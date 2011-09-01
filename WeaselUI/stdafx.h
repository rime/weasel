// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

//#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#pragma warning(disable : 4996)

#include <atlbase.h>
#include <atlwin.h>

#include <wtl/atlapp.h>
#include <wtl/atlframe.h>
#include <wtl/atlgdi.h>
#include <wtl/atlmisc.h>

#pragma warning(default: 4996)

#pragma warning(disable : 4819)

#include <boost/format.hpp>

#pragma warning(default: 4819)

#include <string>
#include <vector>

using namespace std;
