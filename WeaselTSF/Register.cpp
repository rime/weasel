#include "stdafx.h"
#include "Register.h"

#define CLSID_STRLEN 38  // strlen("{xxxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxx}")

static const char c_szInfoKeyPrefix[] = "CLSID\\";
static const char c_szInProcSvr32[] = "InprocServer32";
static const char c_szModelName[] = "ThreadingModel";

BOOL RegisterProfiles()
{
	ITfInputProcessorProfiles *pInputProcessProfiles;
	WCHAR achIconFile[MAX_PATH];
	char achFileNameA[MAX_PATH];
	DWORD cchA;
	int cchIconFile;
	HRESULT hr;

	hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, NULL, CLSCTX_INPROC_SERVER,
		IID_ITfInputProcessorProfiles, (void **) &pInputProcessProfiles);
	if (FAILED(hr))
		return E_FAIL;

	hr = pInputProcessProfiles->Register(c_clsidTextService);
	if (FAILED(hr))
		goto Exit;

	cchA = GetModuleFileNameA(g_hInst, achFileNameA, ARRAYSIZE(achFileNameA));
	cchIconFile = MultiByteToWideChar(CP_ACP, 0, achFileNameA, cchA, achIconFile, ARRAYSIZE(achIconFile) - 1);
	achIconFile[cchIconFile] = '\0';

	hr = pInputProcessProfiles->AddLanguageProfile(
		c_clsidTextService,
		TEXTSERVICE_LANGID,
		c_guidProfile,
		TEXTSERVICE_DESC,
		(ULONG) wcslen(TEXTSERVICE_DESC),
		achIconFile,
		cchIconFile,
		TEXTSERVICE_ICON_INDEX);

Exit:
	pInputProcessProfiles->Release();
	return (hr == S_OK);
}

void UnregisterProfiles()
{
	ITfInputProcessorProfiles *pInputProcessProfiles;
	HRESULT hr;

	hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, NULL, CLSCTX_INPROC_SERVER,
		IID_ITfInputProcessorProfiles, (void **) &pInputProcessProfiles);
	if (FAILED(hr))
		return;

	pInputProcessProfiles->Unregister(c_clsidTextService);
	pInputProcessProfiles->Release();
}

BOOL RegisterCategories()
{
	ITfCategoryMgr *pCategoryMgr;
	HRESULT hr;

	hr = CoCreateInstance(CLSID_TF_CategoryMgr, NULL, CLSCTX_INPROC_SERVER, IID_ITfCategoryMgr, (void **) &pCategoryMgr);
	if (FAILED(hr))
		return FALSE;
	
	hr = pCategoryMgr->RegisterCategory(c_clsidTextService, GUID_TFCAT_TIP_KEYBOARD, c_clsidTextService);
	pCategoryMgr->Release();
	return (hr == S_OK);
}

void UnregisterCategories()
{
	ITfCategoryMgr *pCategoryMgr;
	HRESULT hr;

	hr = CoCreateInstance(CLSID_TF_CategoryMgr, NULL, CLSCTX_INPROC_SERVER, IID_ITfCategoryMgr, (void **) &pCategoryMgr);
	if (FAILED(hr))
		return;

	hr = pCategoryMgr->UnregisterCategory(c_clsidTextService, GUID_TFCAT_TIP_KEYBOARD, c_clsidTextService);
	pCategoryMgr->Release();
}

static BOOL CLSIDToStringA(REFGUID refGUID, char *pchA)
{
	static const BYTE GuidMap[] = {3, 2, 1, 0, '-', 5, 4, '-', 7, 6, '-',
		8, 9, '-', 10, 11, 12, 13, 14, 15};

	static const char szDigits[] = "0123456789ABCDEF";

	int i;
	char *p = pchA;

	const BYTE *pBytes = (const BYTE *) &refGUID;

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

BOOL RegisterServer()
{
	DWORD dw;
	HKEY hKey;
	HKEY hSubKey;
	BOOL fRet;
	char achIMEKey[ARRAYSIZE(c_szInfoKeyPrefix) + CLSID_STRLEN];
	TCHAR achFileName[MAX_PATH];

	if (!CLSIDToStringA(c_clsidTextService, achIMEKey + ARRAYSIZE(c_szInfoKeyPrefix) - 1))
		return FALSE;
	memcpy(achIMEKey, c_szInfoKeyPrefix, sizeof(c_szInfoKeyPrefix) - 1);

	if (fRet = RegCreateKeyExA(HKEY_CLASSES_ROOT, achIMEKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &dw) == ERROR_SUCCESS)
	{
		fRet &= RegSetValueExA(hKey, NULL, 0, REG_SZ, (BYTE *) TEXTSERVICE_DESC_A, (strlen(TEXTSERVICE_DESC_A) + 1) * sizeof(TCHAR)) == ERROR_SUCCESS;
		if (fRet &= RegCreateKeyExA(hKey, c_szInProcSvr32, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hSubKey, &dw) == ERROR_SUCCESS)
		{
			dw = GetModuleFileName(g_hInst, achFileName, ARRAYSIZE(achFileName));

			fRet &= RegSetValueEx(hSubKey, NULL, 0, REG_SZ, (BYTE *) achFileName, (lstrlen(achFileName) + 1) * sizeof(TCHAR)) == ERROR_SUCCESS;
			fRet &= RegSetValueExA(hSubKey, c_szModelName, 0, REG_SZ, (BYTE *) TEXTSERVICE_MODEL, (strlen(TEXTSERVICE_MODEL) + 1)) == ERROR_SUCCESS;
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
}