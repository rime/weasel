#include "stdafx.h"
#include "TSFRegister.h"
#include <WeaselCommon.h>
#include <VersionHelpers.hpp>

#define CLSID_STRLEN 38  // strlen("{xxxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxx}")

static const char c_szInfoKeyPrefix[] = "CLSID\\";
static const char c_szTipKeyPrefix[] = "Software\\Microsft\\CTF\\TIP\\";
static const char c_szInProcSvr32[] = "InprocServer32";
static const char c_szModelName[] = "ThreadingModel";

#ifdef WEASEL_USING_OLDER_TSF_SDK

/* For Windows 8 */
const GUID GUID_TFCAT_TIPCAP_IMMERSIVESUPPORT =
{ 0x13A016DF, 0x560B, 0x46CD,{ 0x94, 0x7A, 0x4C, 0x3A, 0xF1, 0xE0, 0xE3, 0x5D } };

const GUID GUID_TFCAT_TIPCAP_SYSTRAYSUPPORT =
{ 0x25504FB4, 0x7BAB, 0x4BC1,{ 0x9C, 0x69, 0xCF, 0x81, 0x89, 0x0F, 0x0E, 0xF5 } };

#endif

BOOL RegisterProfiles(std::wstring filename, HKL hkl)
{
	HRESULT hr;

	if (IsWindows8OrGreater()) {
		CComPtr<ITfInputProcessorProfileMgr> pInputProcessorProfileMgr;
		hr = pInputProcessorProfileMgr.CoCreateInstance(CLSID_TF_InputProcessorProfiles, NULL, CLSCTX_ALL);
		if (FAILED(hr))
			return FALSE;

		hr = pInputProcessorProfileMgr->RegisterProfile(
			c_clsidTextService,
			TEXTSERVICE_LANGID,
			c_guidProfile,
			TEXTSERVICE_DESC,
			(ULONG)wcslen(TEXTSERVICE_DESC),
			filename.c_str(),
			filename.size(),
			TEXTSERVICE_ICON_INDEX,
			hkl,
			0,
			TRUE,
			0);
	}
	else {
		CComPtr<ITfInputProcessorProfiles> pInputProcessorProfiles;
		hr = pInputProcessorProfiles.CoCreateInstance(CLSID_TF_InputProcessorProfiles, NULL, CLSCTX_INPROC_SERVER);
		if (FAILED(hr))
			return FALSE;

		hr = pInputProcessorProfiles->Register(c_clsidTextService);
		if (FAILED(hr))
			return FALSE;

		hr = pInputProcessorProfiles->AddLanguageProfile(
			c_clsidTextService,
			TEXTSERVICE_LANGID,
			c_guidProfile,
			TEXTSERVICE_DESC,
			(ULONG)wcslen(TEXTSERVICE_DESC),
			filename.c_str(),
			filename.size(),
			TEXTSERVICE_ICON_INDEX);
		if (FAILED(hr))
			return FALSE;
		if (hkl) {
			hr = pInputProcessorProfiles->SubstituteKeyboardLayout(
				c_clsidTextService, TEXTSERVICE_LANGID, c_guidProfile, hkl);
			if (FAILED(hr)) return FALSE;
		}
	}
	return TRUE;
}

void UnregisterProfiles()
{
	ITfInputProcessorProfiles *pInputProcessProfiles;
	HRESULT hr;

	hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, NULL, CLSCTX_INPROC_SERVER,
		IID_ITfInputProcessorProfiles, (void **)&pInputProcessProfiles);
	if (FAILED(hr))
		return;

	pInputProcessProfiles->SubstituteKeyboardLayout(
		c_clsidTextService, TEXTSERVICE_LANGID, c_guidProfile, NULL);
	pInputProcessProfiles->Unregister(c_clsidTextService);
	pInputProcessProfiles->Release();
}

BOOL RegisterCategories()
{
	ITfCategoryMgr *pCategoryMgr;
	HRESULT hr;

	hr = CoCreateInstance(CLSID_TF_CategoryMgr, NULL, CLSCTX_INPROC_SERVER, IID_ITfCategoryMgr, (void **)&pCategoryMgr);
	if (hr != S_OK)
		return FALSE;

	hr = pCategoryMgr->RegisterCategory(c_clsidTextService, GUID_TFCAT_TIP_KEYBOARD, c_clsidTextService);
	if (hr != S_OK)
		goto Exit;

	hr = pCategoryMgr->RegisterCategory(c_clsidTextService, GUID_TFCAT_TIPCAP_INPUTMODECOMPARTMENT, c_clsidTextService);
	if (hr != S_OK)
		goto Exit;

	InitVersion();
	if (IsWindows8OrGreater())
	{
		hr = pCategoryMgr->RegisterCategory(c_clsidTextService, GUID_TFCAT_TIPCAP_IMMERSIVESUPPORT, c_clsidTextService);
		if (hr != S_OK)
			goto Exit;

		hr = pCategoryMgr->RegisterCategory(c_clsidTextService, GUID_TFCAT_TIPCAP_SYSTRAYSUPPORT, c_clsidTextService);
	}

Exit:
	pCategoryMgr->Release();
	return (hr == S_OK);
}

void UnregisterCategories()
{
	ITfCategoryMgr *pCategoryMgr;
	HRESULT hr;

	hr = CoCreateInstance(CLSID_TF_CategoryMgr, NULL, CLSCTX_INPROC_SERVER, IID_ITfCategoryMgr, (void **)&pCategoryMgr);
	if (FAILED(hr))
		return;

	hr = pCategoryMgr->UnregisterCategory(c_clsidTextService, GUID_TFCAT_TIP_KEYBOARD, c_clsidTextService);
	pCategoryMgr->Release();
}

static BOOL CLSIDToStringA(REFGUID refGUID, char *pchA)
{
	static const BYTE GuidMap[] = { 3, 2, 1, 0, '-', 5, 4, '-', 7, 6, '-',
		8, 9, '-', 10, 11, 12, 13, 14, 15 };

	static const char szDigits[] = "0123456789ABCDEF";

	int i;
	char *p = pchA;

	const BYTE *pBytes = (const BYTE *)&refGUID;

	*p++ = '{';
	for (i = 0; i < sizeof(GuidMap); i++)
	{
		if (GuidMap[i] == '-')
			*p++ = '-';
		else
		{
			*p++ = szDigits[(pBytes[GuidMap[i]] & 0xF0) >> 4];
			*p++ = szDigits[(pBytes[GuidMap[i]] & 0x0F)];
		}
	}
	*p++ = '}';
	*p = '\0';
	return TRUE;
}

static LONG RecurseDeleteKeyA(HKEY hParentKey, LPCSTR lpszKey)
{
	HKEY hKey;
	LONG lRes;
	FILETIME time;
	CHAR szBuffer[256];
	DWORD dwSize = ARRAYSIZE(szBuffer);

	if (RegOpenKeyA(hParentKey, lpszKey, &hKey) != ERROR_SUCCESS)
		return ERROR_SUCCESS;

	lRes = ERROR_SUCCESS;
	while (RegEnumKeyExA(hKey, 0, szBuffer, &dwSize, NULL, NULL, NULL, &time) == ERROR_SUCCESS)
	{
		szBuffer[ARRAYSIZE(szBuffer) - 1] = '\0';
		lRes = RecurseDeleteKeyA(hKey, szBuffer);
		if (lRes != ERROR_SUCCESS)
			break;
		dwSize = ARRAYSIZE(szBuffer);
	}
	RegCloseKey(hKey);

	return lRes == ERROR_SUCCESS ? RegDeleteKeyA(hParentKey, lpszKey) : lRes;
}

BOOL RegisterServer(std::wstring filename, bool wow64)
{
	DWORD dw;
	HKEY hKey;
	HKEY hSubKey;
	BOOL fRet;
	char achIMEKey[ARRAYSIZE(c_szInfoKeyPrefix) + CLSID_STRLEN];
	DWORD flags = KEY_WRITE;
	if (wow64) {
		flags |= KEY_WOW64_64KEY;
	}
	//TCHAR achFileName[MAX_PATH];

	if (!CLSIDToStringA(c_clsidTextService, achIMEKey + ARRAYSIZE(c_szInfoKeyPrefix) - 1))
		return FALSE;
	memcpy(achIMEKey, c_szInfoKeyPrefix, sizeof(c_szInfoKeyPrefix) - 1);

	if (fRet = RegCreateKeyExA(HKEY_CLASSES_ROOT, achIMEKey, 0, NULL, REG_OPTION_NON_VOLATILE, flags, NULL, &hKey, &dw) == ERROR_SUCCESS)
	{
		fRet &= RegSetValueExA(hKey, NULL, 0, REG_SZ, (BYTE *)TEXTSERVICE_DESC_A, (strlen(TEXTSERVICE_DESC_A) + 1) * sizeof(TCHAR)) == ERROR_SUCCESS;
		if (fRet &= RegCreateKeyExA(hKey, c_szInProcSvr32, 0, NULL, REG_OPTION_NON_VOLATILE, flags, NULL, &hSubKey, &dw) == ERROR_SUCCESS)
		{
			//dw = GetModuleFileName(g_hInst, achFileName, ARRAYSIZE(achFileName));

			fRet &= RegSetValueEx(hSubKey, NULL, 0, REG_SZ, (BYTE *)filename.c_str(), (filename.size() + 1) * sizeof(TCHAR)) == ERROR_SUCCESS;
			fRet &= RegSetValueExA(hSubKey, c_szModelName, 0, REG_SZ, (BYTE *)TEXTSERVICE_MODEL, (strlen(TEXTSERVICE_MODEL) + 1)) == ERROR_SUCCESS;
			RegCloseKey(hSubKey);
		}
		RegCloseKey(hKey);
	}
	return fRet;
}

void UnregisterServer()
{
	char achIMEKey[ARRAYSIZE(c_szInfoKeyPrefix) + CLSID_STRLEN];
	if (!CLSIDToStringA(c_clsidTextService, achIMEKey + ARRAYSIZE(c_szInfoKeyPrefix) - 1))
		return;
	memcpy(achIMEKey, c_szInfoKeyPrefix, sizeof(c_szInfoKeyPrefix) - 1);
	RecurseDeleteKeyA(HKEY_CLASSES_ROOT, achIMEKey);

	// On Windows 8, we need to manually delete the registry key for our TIP
	char tipKey[ARRAYSIZE(c_szTipKeyPrefix) + CLSID_STRLEN];
	if (!CLSIDToStringA(c_clsidTextService, tipKey + ARRAYSIZE(c_szTipKeyPrefix) - 1))
		return;
	memcpy(tipKey, c_szTipKeyPrefix, sizeof(c_szTipKeyPrefix) - 1);
	RecurseDeleteKeyA(HKEY_CLASSES_ROOT, tipKey);
}