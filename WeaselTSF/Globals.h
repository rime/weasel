/* Global definitions */
#pragma once

#include "stdafx.h"
#include <WeaselCommon.h>


void DllAddRef();
void DllRelease();

extern HINSTANCE g_hInst;

extern LONG g_cRefDll;

extern CRITICAL_SECTION g_cs;

extern const CLSID c_clsidTextService;

extern const GUID c_guidProfile;

extern const GUID c_guidLangBarItemButton;

#ifndef TF_IPP_CAPS_IMMERSIVESUPPORT

#define WEASEL_USING_OLDER_TSF_SDK

/* for Windows 8 */
#define TF_TMF_IMMERSIVEMODE			0x40000000
#define TF_IPP_CAPS_IMMERSIVESUPPORT	0x00010000
#define TF_IPP_CAPS_SYSTRAYSUPPORT		0x00020000

extern const GUID GUID_TFCAT_TIPCAP_IMMERSIVESUPPORT;
extern const GUID GUID_TFCAT_TIPCAP_SYSTRAYSUPPORT;

#endif

extern const GUID GUID_LBI_INPUTMODE;
extern const GUID GUID_IME_MODE_PRESERVED_KEY;

