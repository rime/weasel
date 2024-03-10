// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <Windows.h>
#include <Ole2.h>
#include <OleCtl.h>
#include <msctf.h>
#include <assert.h>

#include <atlcomcli.h> 

#include <map>
#include <memory>
#include <string>
#include <bitset>

template<typename I>
using com_ptr = CComPtr<I>;
