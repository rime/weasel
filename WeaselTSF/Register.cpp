#include "stdafx.h"
#include "Register.h"
#include <strsafe.h>

#define CLSID_STRLEN 38  // strlen("{xxxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxx}")

static const char c_szInfoKeyPrefix[] = "CLSID\\";
static const char c_szTipKeyPrefix[] = "Software\\Microsft\\CTF\\TIP\\";
static const char c_szInProcSvr32[] = "InprocServer32";
static const char c_szModelName[] = "ThreadingModel";

HKL FindIME() {
  HKL hKL = NULL;
  WCHAR key[9];
  HKEY hKey;
  LSTATUS ret =
      RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                    L"SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts", 0,
                    KEY_READ, &hKey);
  if (ret == ERROR_SUCCESS) {
    for (DWORD id = (0xE0200000 | TEXTSERVICE_LANGID);
         hKL == NULL && id <= (0xE0FF0000 | TEXTSERVICE_LANGID);
         id += 0x10000) {
      StringCchPrintfW(key, _countof(key), L"%08X", id);
      HKEY hSubKey;
      ret = RegOpenKeyExW(hKey, key, 0, KEY_READ, &hSubKey);
      if (ret == ERROR_SUCCESS) {
        WCHAR data[32];
        DWORD type;
        DWORD size = sizeof data;
        ret = RegQueryValueExW(hSubKey, L"Ime File", NULL, &type, (LPBYTE)data,
                               &size);
        if (ret == ERROR_SUCCESS && type == REG_SZ &&
            _wcsicmp(data, L"weasel.ime") == 0)
          hKL = (HKL)id;
      }
      RegCloseKey(hSubKey);
    }
  }
  RegCloseKey(hKey);
  return hKL;
}

BOOL RegisterProfiles() {
  WCHAR achIconFile[MAX_PATH];
  ULONG cchIconFile =
      GetModuleFileNameW(g_hInst, achIconFile, ARRAYSIZE(achIconFile));
  HRESULT hr;

  {
    CComPtr<ITfInputProcessorProfileMgr> pInputProcessorProfileMgr;
    hr = pInputProcessorProfileMgr.CoCreateInstance(
        CLSID_TF_InputProcessorProfiles, NULL, CLSCTX_ALL);
    if (FAILED(hr))
      return FALSE;

    hr = pInputProcessorProfileMgr->RegisterProfile(
        c_clsidTextService, TEXTSERVICE_LANGID, c_guidProfile, TEXTSERVICE_DESC,
        (ULONG)wcslen(TEXTSERVICE_DESC), achIconFile, cchIconFile,
        TEXTSERVICE_ICON_INDEX, FindIME(), 0, TRUE, 0);
    if (FAILED(hr))
      return FALSE;
  }

  return TRUE;
}

void UnregisterProfiles() {
  HRESULT hr;

  {
    CComPtr<ITfInputProcessorProfileMgr> pInputProcessorProfileMgr;
    hr = pInputProcessorProfileMgr.CoCreateInstance(
        CLSID_TF_InputProcessorProfiles, NULL, CLSCTX_ALL);
    if (FAILED(hr))
      return;

    hr = pInputProcessorProfileMgr->UnregisterProfile(
        c_clsidTextService, TEXTSERVICE_LANGID, c_guidProfile, 0);
  }
}

const GUID SupportCategories0[] = {
    GUID_TFCAT_CATEGORY_OF_TIP, GUID_TFCAT_TIP_KEYBOARD,
    // GUID_TFCAT_TIP_SPEECH,
    // GUID_TFCAT_TIP_HANDWRITING,
    GUID_TFCAT_TIPCAP_SECUREMODE, GUID_TFCAT_TIPCAP_UIELEMENTENABLED,
    GUID_TFCAT_TIPCAP_INPUTMODECOMPARTMENT, GUID_TFCAT_TIPCAP_COMLESS,
    GUID_TFCAT_TIPCAP_WOW16, GUID_TFCAT_TIPCAP_IMMERSIVESUPPORT,
    GUID_TFCAT_TIPCAP_SYSTRAYSUPPORT, GUID_TFCAT_PROP_AUDIODATA,
    GUID_TFCAT_PROP_INKDATA, GUID_TFCAT_PROPSTYLE_CUSTOM,
    GUID_TFCAT_PROPSTYLE_STATIC, GUID_TFCAT_PROPSTYLE_STATICCOMPACT,
    GUID_TFCAT_DISPLAYATTRIBUTEPROVIDER, GUID_TFCAT_DISPLAYATTRIBUTEPROPERTY};

BOOL RegisterCategories() {
  ITfCategoryMgr* pCategoryMgr;
  HRESULT hr;

  hr = CoCreateInstance(CLSID_TF_CategoryMgr, NULL, CLSCTX_INPROC_SERVER,
                        IID_ITfCategoryMgr, (void**)&pCategoryMgr);
  if (hr != S_OK)
    return FALSE;

  BOOL flag = TRUE;
  for (const auto& guid : SupportCategories0) {
    hr = pCategoryMgr->RegisterCategory(c_clsidTextService, guid,
                                        c_clsidTextService);
    if (hr != S_OK)
      flag = FALSE;
  }

  pCategoryMgr->Release();
  return flag;
}

void UnregisterCategories() {
  ITfCategoryMgr* pCategoryMgr;
  HRESULT hr;

  hr = CoCreateInstance(CLSID_TF_CategoryMgr, NULL, CLSCTX_INPROC_SERVER,
                        IID_ITfCategoryMgr, (void**)&pCategoryMgr);
  if (FAILED(hr))
    return;

  for (const auto& guid : SupportCategories0)
    pCategoryMgr->UnregisterCategory(c_clsidTextService, guid,
                                     c_clsidTextService);

  pCategoryMgr->Release();
}

static BOOL CLSIDToStringA(REFGUID refGUID, char* pchA) {
  static const BYTE GuidMap[] = {3,   2, 1, 0,   '-', 5,  4,  '-', 7,  6,
                                 '-', 8, 9, '-', 10,  11, 12, 13,  14, 15};

  static const char szDigits[] = "0123456789ABCDEF";

  int i;
  char* p = pchA;

  const BYTE* pBytes = (const BYTE*)&refGUID;

  *p++ = '{';
  for (i = 0; i < sizeof(GuidMap); i++) {
    if (GuidMap[i] == '-')
      *p++ = '-';
    else {
      *p++ = szDigits[(pBytes[GuidMap[i]] & 0xF0) >> 4];
      *p++ = szDigits[(pBytes[GuidMap[i]] & 0x0F)];
    }
  }
  *p++ = '}';
  *p = '\0';
  return TRUE;
}

static LONG RecurseDeleteKeyA(HKEY hParentKey, LPCSTR lpszKey) {
  HKEY hKey;
  LONG lRes;
  FILETIME time;
  CHAR szBuffer[256];
  DWORD dwSize = ARRAYSIZE(szBuffer);

  if (RegOpenKeyA(hParentKey, lpszKey, &hKey) != ERROR_SUCCESS)
    return ERROR_SUCCESS;

  lRes = ERROR_SUCCESS;
  while (RegEnumKeyExA(hKey, 0, szBuffer, &dwSize, NULL, NULL, NULL, &time) ==
         ERROR_SUCCESS) {
    szBuffer[ARRAYSIZE(szBuffer) - 1] = '\0';
    lRes = RecurseDeleteKeyA(hKey, szBuffer);
    if (lRes != ERROR_SUCCESS)
      break;
    dwSize = ARRAYSIZE(szBuffer);
  }
  RegCloseKey(hKey);

  return lRes == ERROR_SUCCESS ? RegDeleteKeyA(hParentKey, lpszKey) : lRes;
}

BOOL RegisterServer() {
  DWORD dw;
  HKEY hKey;
  HKEY hSubKey;
  BOOL fRet;
  char achIMEKey[ARRAYSIZE(c_szInfoKeyPrefix) + CLSID_STRLEN];
  char achFileName[MAX_PATH];

  if (!CLSIDToStringA(c_clsidTextService,
                      achIMEKey + ARRAYSIZE(c_szInfoKeyPrefix) - 1))
    return FALSE;
  memcpy(achIMEKey, c_szInfoKeyPrefix, sizeof(c_szInfoKeyPrefix) - 1);

  if (fRet = RegCreateKeyExA(HKEY_CLASSES_ROOT, achIMEKey, 0, NULL,
                             REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey,
                             &dw) == ERROR_SUCCESS) {
    fRet &= RegSetValueExA(hKey, NULL, 0, REG_SZ, (BYTE*)TEXTSERVICE_DESC_A,
                           sizeof TEXTSERVICE_DESC_A) == ERROR_SUCCESS;
    if (fRet &=
        RegCreateKeyExA(hKey, c_szInProcSvr32, 0, NULL, REG_OPTION_NON_VOLATILE,
                        KEY_WRITE, NULL, &hSubKey, &dw) == ERROR_SUCCESS) {
      dw = GetModuleFileNameA(g_hInst, achFileName, ARRAYSIZE(achFileName));

#ifdef _M_ARM64
      {
        // On ARM64 we use ARM64X redirection DLL.
        // When loaded, weasel.dll will be redirected to weaselARM64.dll on
        // ARM64 processes, and weaselx64.dll on x64 processes.
        //
        // But GetModuleFileNameA will return the actual loaded DLL name aka
        // weaselARM64.dll Rewrite the path to point to the redirector.

        char wrapperPath[MAX_PATH];
        StringCbCatA(achFileName, MAX_PATH, "\\..\\weasel.dll");
        GetFullPathNameA(achFileName, MAX_PATH, wrapperPath, NULL);
        memcpy(achFileName, wrapperPath, MAX_PATH);
      }
#endif

      fRet &= RegSetValueExA(hSubKey, NULL, 0, REG_SZ, (BYTE*)achFileName,
                             (strlen(achFileName) + 1) * sizeof(char)) ==
              ERROR_SUCCESS;
      fRet &= RegSetValueExA(hSubKey, c_szModelName, 0, REG_SZ,
                             (BYTE*)TEXTSERVICE_MODEL,
                             sizeof TEXTSERVICE_MODEL) == ERROR_SUCCESS;
      RegCloseKey(hSubKey);
    }
    RegCloseKey(hKey);
  }
  return fRet;
}

void UnregisterServer() {
  char achIMEKey[ARRAYSIZE(c_szInfoKeyPrefix) + CLSID_STRLEN];
  if (!CLSIDToStringA(c_clsidTextService,
                      achIMEKey + ARRAYSIZE(c_szInfoKeyPrefix) - 1))
    return;
  memcpy(achIMEKey, c_szInfoKeyPrefix, sizeof(c_szInfoKeyPrefix) - 1);
  RecurseDeleteKeyA(HKEY_CLASSES_ROOT, achIMEKey);

  // On Windows 8, we need to manually delete the registry key for our TIP
  char tipKey[ARRAYSIZE(c_szTipKeyPrefix) + CLSID_STRLEN];
  if (!CLSIDToStringA(c_clsidTextService,
                      tipKey + ARRAYSIZE(c_szTipKeyPrefix) - 1))
    return;
  memcpy(tipKey, c_szTipKeyPrefix, sizeof(c_szTipKeyPrefix) - 1);
  RecurseDeleteKeyA(HKEY_CLASSES_ROOT, tipKey);
}
