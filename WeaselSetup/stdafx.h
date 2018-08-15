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
#include <Imm.h>
#include <msctf.h>

#include <atlbase.h>
#include <atlwin.h>
#include <atlimage.h>

#include <wtl/atlapp.h>
#include <wtl/atlctrls.h>
#include <wtl/atlctrlx.h>
#include <wtl/atlmisc.h>
#include <wtl/atldlgs.h>

#include <boost/filesystem.hpp>

// {A3F4CDED-B1E9-41EE-9CA6-7B4D0DE6CB0A}
extern const GUID c_clsidTextService;

// {3D02CAB6-2B8E-4781-BA20-1C9267529467}
extern const GUID c_guidProfile;

#ifndef TF_IPP_CAPS_IMMERSIVESUPPORT

#define WEASEL_USING_OLDER_TSF_SDK

/* for Windows 8 */
#define TF_TMF_IMMERSIVEMODE			0x40000000
#define TF_IPP_CAPS_IMMERSIVESUPPORT	0x00010000
#define TF_IPP_CAPS_SYSTRAYSUPPORT		0x00020000

extern const GUID GUID_TFCAT_TIPCAP_IMMERSIVESUPPORT;
extern const GUID GUID_TFCAT_TIPCAP_SYSTRAYSUPPORT;

#endif

// TODO: reference additional headers your program requires here
