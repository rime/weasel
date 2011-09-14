#include "stdafx.h"
#include "WeaselIME.h"

#pragma warning(disable: 4996)

//
// IME export functions
//

BOOL WINAPI ImeInquire(IMEINFO* lpIMEInfo, LPWSTR lpszUIClass, DWORD dwSystemInfoFlags)
{
	if (!lpIMEInfo || !lpszUIClass)
		return FALSE;

	if (dwSystemInfoFlags & IME_SYSINFO_WINLOGON)
	{
		// TODO: disable input method in winlogon.exe (not tested)
		return FALSE;
	}

	wcscpy(lpszUIClass, WeaselIME::GetUIClassName());

	lpIMEInfo->dwPrivateDataSize = 0;
	lpIMEInfo->fdwProperty = IME_PROP_UNICODE | IME_PROP_SPECIAL_UI;
	lpIMEInfo->fdwConversionCaps = IME_CMODE_FULLSHAPE | IME_CMODE_NATIVE;
	lpIMEInfo->fdwSentenceCaps = IME_SMODE_NONE;
	lpIMEInfo->fdwUICaps = UI_CAP_2700;
	lpIMEInfo->fdwSCSCaps = 0;
	lpIMEInfo->fdwSelectCaps = SELECT_CAP_CONVERSION;

	return TRUE;
}

BOOL WINAPI ImeConfigure(HKL hKL, HWND hWnd, DWORD dwMode, LPVOID lpData)
{
	// TODO:
	MessageBox(hWnd, L"本品無設定介面 :)", L"輸入法設定", MB_OK);
	return TRUE;
}

DWORD WINAPI ImeConversionList(HIMC hIMC, LPCTSTR lpSource, LPCANDIDATELIST lpCandList, DWORD dwBufLen, UINT uFlag)
{
	//
	return TRUE;
}

BOOL WINAPI ImeDestroy(UINT uForce)
{
	//
	return TRUE;
}

LRESULT WINAPI ImeEscape(HIMC hIMC, UINT uSubFunc, LPVOID lpData)
{
	//
	return TRUE;
}

BOOL WINAPI ImeProcessKey(HIMC hIMC, UINT vKey, LPARAM lKeyData, const LPBYTE lpbKeyState)
{
	BOOL accepted = FALSE;
	boost::shared_ptr<WeaselIME> p = WeaselIME::GetInstance(hIMC);
    if (!p)
        return FALSE;
	accepted = p->ProcessKeyEvent(vKey, lKeyData, lpbKeyState);
	return accepted;
}

BOOL WINAPI ImeSelect(HIMC hIMC, BOOL fSelect)
{
	boost::shared_ptr<WeaselIME> p = WeaselIME::GetInstance(hIMC);
    if (!p)
        return FALSE;
	HRESULT hr = p->OnIMESelect(fSelect);
	if (FAILED(hr))
		return FALSE;

	return TRUE;
}

BOOL WINAPI ImeSetActiveContext(HIMC hIMC, BOOL fFocus)
{
	if (hIMC)
	{	
		boost::shared_ptr<WeaselIME> p = WeaselIME::GetInstance(hIMC);
        if (!p)
            return FALSE;
		HRESULT hr = p->OnIMEFocus(fFocus);
        if (FAILED(hr))
			return FALSE;
	}

	return TRUE;
}

UINT WINAPI ImeToAsciiEx (UINT uVKey, UINT uScanCode, CONST LPBYTE lpbKeyState, LPDWORD lpdwTransKey, UINT fuState, HIMC hIMC)
{
	//
	return 0;
}

BOOL WINAPI NotifyIME(HIMC hIMC, DWORD dwAction, DWORD dwIndex, DWORD dwValue)
{
	//
	return TRUE;
}
BOOL WINAPI ImeRegisterWord(LPCTSTR lpRead, DWORD dw, LPCTSTR lpStr)
{
	//
	return FALSE;
}

BOOL WINAPI ImeUnregisterWord(LPCTSTR lpRead, DWORD dw, LPCTSTR lpStr)
{
	//
	return FALSE;
}

UINT WINAPI ImeGetRegisterWordStyle(UINT nItem, LPSTYLEBUF lp)
{
	//
	return 0;
}

UINT WINAPI ImeEnumRegisterWord(REGISTERWORDENUMPROC lpfn, LPCTSTR lpRead, DWORD dw, LPCTSTR lpStr, LPVOID lpData)
{
	//
	return 0;
}

BOOL WINAPI ImeSetCompositionString(HIMC hIMC, DWORD dwIndex, LPCVOID lpComp, DWORD dwComp, LPCVOID lpRead, DWORD dwRead)
{
	//
	return FALSE;
}
