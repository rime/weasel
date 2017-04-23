// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>


#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // some CString constructors will be explicit

#include <atl.h>

#pragma warning(disable : 4996)

#include <wtl/atlapp.h>

#pragma warning(default : 4996)

extern CAppModule _Module;
