// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#include <ShellAPI.h>
#include "Imm.h"

#include <atl.h>

#pragma warning(disable : 4996)

#include <atlimage.h>

#include <wtl/atlapp.h>
#include <wtl/atlctrls.h>
#include <wtl/atlctrlx.h>
#include <wtl/atlmisc.h>
#include <wtl/atldlgs.h>

#pragma warning(default: 4996)

#pragma warning(disable : 4819)

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

#pragma warning(default : 4819)

// TODO: reference additional headers your program requires here
