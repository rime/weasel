#include "stdafx.h"
#include "Globals.h"

HINSTANCE g_hInst;

LONG g_cRefDll = -1;

CRITICAL_SECTION g_cs;

// Iōng Power Shell sán-seng sin ê GUID: '{'+[guid]::NewGuid().ToString()+'}'
// GUID: {d5026f36-1b08-4269-a3d7-0c04e277c327}
static const GUID c_clsidTextService = 
{ 0xd5026f36, 0x1b08, 0x4269, { 0xa3, 0xd7, 0x0c, 0x04, 0xe2, 0x77, 0xc3, 0x27 } };

// Iōng Power Shell sán-seng sin ê GUID: '{'+[guid]::NewGuid().ToString()+'}'
// GUID: {632f7393-d0a8-4626-9108-f22c195bd427}
static const GUID c_guidProfile = 
{ 0x632f7393, 0xd0a8, 0x4626, { 0x91, 0x08, 0xf2, 0x2c, 0x19, 0x5b, 0xd4, 0x27 } };

// Iōng Power Shell sán-seng sin ê GUID: '{'+[guid]::NewGuid().ToString()+'}'
// GUID: {65ecc8ea-21f4-4b0f-b2ad-a26e6c4a26f0}
static const GUID c_guidLangBarItemButton = 
{ 0x65ecc8ea, 0x21f4, 0x4b0f, { 0xb2, 0xad, 0xa2, 0x6e, 0x6c, 0x4a, 0x26, 0xf0 } };

#ifdef WEASEL_USING_OLDER_TSF_SDK

/* For Windows 8 */
// https://docs.microsoft.com/en-us/previous-versions/windows/apps/hh967425(v=win.10)?redirectedfrom=MSDN#declaring-compatibility
const GUID GUID_TFCAT_TIPCAP_IMMERSIVESUPPORT =
{ 0x13A016DF, 0x560B, 0x46CD, { 0x94, 0x7A, 0x4C, 0x3A, 0xF1, 0xE0, 0xE3, 0x5D } };

const GUID GUID_TFCAT_TIPCAP_SYSTRAYSUPPORT =
{ 0x25504FB4, 0x7BAB, 0x4BC1, { 0x9C, 0x69, 0xCF, 0x81, 0x89, 0x0F, 0x0E, 0xF5 } };

#endif

const GUID GUID_LBI_INPUTMODE =
{ 0x2C77A81E, 0x41CC, 0x4178, { 0xA3, 0xA7, 0x5F, 0x8A, 0x98, 0x75, 0x68, 0xE6 } };

const GUID GUID_IME_MODE_PRESERVED_KEY =
{ 0x0bd899fc, 0xa8f7, 0x4b42, { 0xa9, 0x6d, 0xce, 0xc7, 0xc5, 0x0e, 0x0e, 0xae } };
