﻿// WeaselIME.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include <strsafe.h>
#include <StringAlgorithm.hpp>
#include <WeaselCommon.h>
#include <ResponseParser.h>
#include "WeaselIME.h"
#include <algorithm>

// logging disabled
#define EZDBGONLYLOGGERVAR(...)
#define EZDBGONLYLOGGERPRINT(...)
#define EZDBGONLYLOGGERFUNCTRACKER

HINSTANCE WeaselIME::s_hModule = 0;
HIMCMap WeaselIME::s_instances;

static void error_message(const WCHAR *msg)
{
	static DWORD next_tick = 0;
	DWORD now = GetTickCount();
	if (now > next_tick)
	{
		next_tick = now + 10000;  // (ms)
		MessageBox(NULL, msg, WEASEL_IME_NAME, MB_ICONERROR | MB_OK);
	}
}

/*
static bool launch_server()
{
	EZDBGONLYLOGGERPRINT("Launching weasel server.");

	// 從註冊表取得server位置
	HKEY hKey;
	LSTATUS ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, WEASEL_REG_KEY, 0, KEY_READ | KEY_WOW64_32KEY, &hKey);
	if (ret != ERROR_SUCCESS)
	{
		error_message(L"註冊表信息無影了");
		return false;
	}

	WCHAR value[MAX_PATH];
	DWORD len = sizeof(value);
	DWORD type = 0;
	ret = RegQueryValueEx(hKey, L"WeaselRoot", NULL, &type, (LPBYTE)value, &len);
	if (ret != ERROR_SUCCESS)
	{
		error_message(L"未設置 WeaselRoot");
		RegCloseKey(hKey);
		return false;
	}
	wpath weaselRoot(value);

	len = sizeof(value);
	type = 0;
	ret = RegQueryValueEx(hKey, L"ServerExecutable", NULL, &type, (LPBYTE)value, &len);
	if (ret != ERROR_SUCCESS)
	{
		error_message(L"未設置 ServerExecutable");
		RegCloseKey(hKey);
		return false;
	}
	wpath serverPath(weaselRoot / value);

	RegCloseKey(hKey);

	// 啓動服務進程
	std::wstring exe = serverPath.wstring();
	std::wstring dir = weaselRoot.wstring();

	STARTUPINFO startup_info = {0};
	PROCESS_INFORMATION process_info = {0};
	startup_info.cb = sizeof(startup_info);

	if (!CreateProcess(exe.c_str(), NULL, NULL, NULL, FALSE, 0, NULL, dir.c_str(), &startup_info, &process_info))
	{
		EZDBGONLYLOGGERPRINT("ERROR: failed to launch weasel server.");
		error_message(L"服務進程啓動不起來 :(");
		return false;
	}

	if (!WaitForInputIdle(process_info.hProcess, 1500))
	{
		EZDBGONLYLOGGERPRINT("WARNING: WaitForInputIdle() timed out; succeeding IPC messages might not be delivered.");
	}
	if (process_info.hProcess) CloseHandle(process_info.hProcess);
	if (process_info.hThread) CloseHandle(process_info.hThread);

	return true;
}
*/

WeaselIME::WeaselIME(HIMC hIMC)
: m_hIMC(hIMC)
, m_composing(false)
, m_preferCandidatePos(false)
{
	WCHAR path[MAX_PATH];
	WCHAR fname[_MAX_FNAME];
	WCHAR ext[_MAX_EXT];
	GetModuleFileNameW(NULL, path, _countof(path));
	_wsplitpath_s(path, NULL, 0, NULL, 0, fname, _countof(fname), ext, _countof(ext));
	if (iequals(L"chrome", fname) && iequals(L"exe", ext))
		m_preferCandidatePos = true;
}

HINSTANCE WeaselIME::GetModuleInstance()
{
	return s_hModule;
}

void WeaselIME::SetModuleInstance(HINSTANCE hModule)
{
	s_hModule = hModule;
}

HRESULT WeaselIME::RegisterUIClass()
{
	WNDCLASSEX wc;
	wc.cbSize         = sizeof(WNDCLASSEX);
	wc.style          = CS_IME;
	wc.lpfnWndProc    = WeaselIME::UIWndProc;
	wc.cbClsExtra     = 0;
	wc.cbWndExtra     = 2 * sizeof(LONG);
	wc.hInstance      = GetModuleInstance();
	wc.hCursor        = NULL;
	wc.hIcon          = NULL;
	wc.lpszMenuName   = NULL;
	wc.lpszClassName  = GetUIClassName();
	wc.hbrBackground  = NULL;
	wc.hIconSm        = NULL;

	if (RegisterClassExW(&wc) == 0)
	{
		DWORD dwErr = GetLastError();
		return HRESULT_FROM_WIN32(dwErr);
	}

	return S_OK;
}

HRESULT WeaselIME::UnregisterUIClass()
{
	if (!UnregisterClassW(GetUIClassName(), GetModuleInstance()))
	{
		DWORD dwErr = GetLastError();
		return HRESULT_FROM_WIN32(dwErr);
	}
	return S_OK;
}

LPCWSTR WeaselIME::GetUIClassName()
{
	return L"WeaselUIClass";
}

LRESULT WINAPI WeaselIME::UIWndProc(HWND hWnd, UINT uMsg, WPARAM wp, LPARAM lp)
{
	HIMC hIMC = (HIMC)GetWindowLongPtr(hWnd, 0);
	if (hIMC)
	{
		std::shared_ptr<WeaselIME> p = WeaselIME::GetInstance(hIMC);
		if (!p)
			return 0;
		return p->OnUIMessage(hWnd, uMsg, wp, lp);
	}
	else
	{
		if (!IsIMEMessage(uMsg))
		{
			return DefWindowProcW(hWnd, uMsg, wp, lp);
		}
	}

	return 0;
}

BOOL WeaselIME::IsIMEMessage(UINT uMsg)
{
	switch(uMsg)
	{
	case WM_IME_STARTCOMPOSITION:
	case WM_IME_ENDCOMPOSITION:
	case WM_IME_COMPOSITION:
	case WM_IME_NOTIFY:
	case WM_IME_SETCONTEXT:
	case WM_IME_CONTROL:
	case WM_IME_COMPOSITIONFULL:
	case WM_IME_SELECT:
	case WM_IME_CHAR:
		return TRUE;
	default:
		return FALSE;
	}

	return FALSE;
}

std::shared_ptr<WeaselIME> WeaselIME::GetInstance(HIMC hIMC)
{
	if (!s_instances.is_valid())
	{
		return std::shared_ptr<WeaselIME>();
	}
	std::lock_guard<std::mutex> lock(s_instances.get_mutex());
	std::shared_ptr<WeaselIME>& p = s_instances[hIMC];
	if (!p)
	{
		p.reset(new WeaselIME(hIMC));
	}
	return p;
}

void WeaselIME::Cleanup()
{
	std::for_each(s_instances.begin(), s_instances.end(), [](std::pair<const HIMC, std::shared_ptr<WeaselIME>> &pair)
	{
		pair.second->OnIMESelect(FALSE);
	});
	std::lock_guard<std::mutex> lock(s_instances.get_mutex());
	s_instances.clear();
}

LRESULT WeaselIME::OnIMESelect(BOOL fSelect)
{
	EZDBGONLYLOGGERPRINT("On IME select: %d, HIMC = 0x%x", fSelect, m_hIMC);
	ImmSetOpenStatus(m_hIMC, fSelect);
	if (fSelect)
	{
		// initialize weasel client
		m_client.Connect(NULL);
		m_client.StartSession();

		return _Initialize();
	}
	else
	{
		m_client.EndSession();

		return _Finalize();
	}
}

LRESULT WeaselIME::OnIMEFocus(BOOL fFocus)
{
	EZDBGONLYLOGGERPRINT("On IME focus: %d, HIMC = 0x%x", fFocus, m_hIMC);
	LPINPUTCONTEXT lpIMC = ImmLockIMC(m_hIMC);
	if(!lpIMC)
	{
		return 0;
	}
	if (fFocus)
	{
		if(!(lpIMC->fdwInit & INIT_COMPFORM))
		{
			lpIMC->cfCompForm.dwStyle = CFS_DEFAULT;
			GetCursorPos(&lpIMC->cfCompForm.ptCurrentPos);
			ScreenToClient(lpIMC->hWnd, &lpIMC->cfCompForm.ptCurrentPos);
			lpIMC->fdwInit |= INIT_COMPFORM;
		}
		m_client.FocusIn();
	}
	else
	{
		m_client.FocusOut();
	}
	ImmUnlockIMC(m_hIMC);

	return 0;
}

LRESULT WeaselIME::OnUIMessage(HWND hWnd, UINT uMsg, WPARAM wp, LPARAM lp)
{
	LPINPUTCONTEXT lpIMC = (LPINPUTCONTEXT)ImmLockIMC(m_hIMC);
	switch (uMsg)
	{
	case WM_IME_NOTIFY:
		{
			EZDBGONLYLOGGERPRINT("WM_IME_NOTIFY: wp = 0x%x, lp = 0x%x, HIMC = 0x%x", wp, lp, m_hIMC);
			_OnIMENotify(lpIMC, wp, lp);
		}
		break;
	case WM_IME_SELECT:
		{
			EZDBGONLYLOGGERPRINT("WM_IME_SELECT: wp = 0x%x, lp = 0x%x, HIMC = 0x%x", wp, lp, m_hIMC);
			if (m_preferCandidatePos)
				_SetCandidatePos(lpIMC);
			else
				_SetCompositionWindow(lpIMC);
		}
		break;
	case WM_IME_STARTCOMPOSITION:
		{
			EZDBGONLYLOGGERPRINT("WM_IME_STARTCOMPOSITION: wp = 0x%x, lp = 0x%x, HIMC = 0x%x", wp, lp, m_hIMC);
			if (m_preferCandidatePos)
				_SetCandidatePos(lpIMC);
			else
				_SetCompositionWindow(lpIMC);
		}
		break;
	default:
		if (!IsIMEMessage(uMsg))
		{
			ImmUnlockIMC(m_hIMC);
			return DefWindowProcW(hWnd, uMsg, wp, lp);
		}
		EZDBGONLYLOGGERPRINT("WM_IME_(0x%x): wp = 0x%x, lp = 0x%x, HIMC = 0x%x", uMsg, wp, lp, m_hIMC);
	}

	ImmUnlockIMC(m_hIMC);
	return 0;
}

LRESULT WeaselIME::_OnIMENotify(LPINPUTCONTEXT lpIMC, WPARAM wp, LPARAM lp)
{
	switch (wp)
	{
	case IMN_OPENCANDIDATE:
		{
			EZDBGONLYLOGGERPRINT("IMN_OPENCANDIDATE: HIMC = 0x%x", m_hIMC);
			if (m_preferCandidatePos)
				_SetCandidatePos(lpIMC);
			else
				_SetCompositionWindow(lpIMC);
		}
		break;
	case IMN_SETCANDIDATEPOS:
		{
			EZDBGONLYLOGGERPRINT("IMN_SETCANDIDATEPOS: HIMC = 0x%x", m_hIMC);
			_SetCandidatePos(lpIMC);
		}
		break;
	case IMN_SETCOMPOSITIONWINDOW:
		{
			EZDBGONLYLOGGERPRINT("IMN_SETCOMPOSITIONWINDOW: HIMC = 0x%x", m_hIMC);
			if (m_preferCandidatePos)
				_SetCandidatePos(lpIMC);
			else
				_SetCompositionWindow(lpIMC);
		}
		break;
	case IMN_SETOPENSTATUS:
		{
			if (!ImmGetOpenStatus(m_hIMC))  // gvim command mode
			{
				m_client.ClearComposition();  // cancel unfinished input (eg. quitting insert mode with Ctrl+[ )
			}
		}
		break;
	default:
		EZDBGONLYLOGGERPRINT("IMN_(0x%x): HIMC = 0x%x", wp, m_hIMC);
	}

	return 0;
}

void WeaselIME::_SetCandidatePos(LPINPUTCONTEXT lpIMC)
{
	EZDBGONLYLOGGERFUNCTRACKER;
	POINT pt = lpIMC->cfCandForm[0].ptCurrentPos;
	_UpdateInputPosition(lpIMC, pt);
}

void WeaselIME::_SetCompositionWindow(LPINPUTCONTEXT lpIMC)
{
	EZDBGONLYLOGGERVAR(lpIMC->cfCompForm.dwStyle);
	POINT pt = {-1, -1};
	switch (lpIMC->cfCompForm.dwStyle)
	{
	case CFS_DEFAULT:
		// require caret pos detection
		break;
	case CFS_RECT:
		//pt.x = lpIMC->cfCompForm.rcArea.left;
		//pt.y = lpIMC->cfCompForm.rcArea.top;
		break;
	case CFS_POINT:
		if (lpIMC->cfCompForm.rcArea.left == 0 && lpIMC->cfCompForm.rcArea.top == 0)
		{
			return;  // may be invalid position
		}
		pt = lpIMC->cfCompForm.ptCurrentPos;
		break;
	case CFS_FORCE_POSITION:
		pt = lpIMC->cfCompForm.ptCurrentPos;
		break;
	case CFS_CANDIDATEPOS:
		pt = lpIMC->cfCandForm[0].ptCurrentPos;
		break;
	default:
		// require caret pos detection
		break;
	}
	_UpdateInputPosition(lpIMC, pt);
}

BOOL WeaselIME::ProcessKeyEvent(UINT vKey, KeyInfo kinfo, const LPBYTE lpbKeyState)
{
	EZDBGONLYLOGGERPRINT("Process key event: vKey = 0x%x, kinfo = 0x%x, HIMC = 0x%x", vKey, UINT32(kinfo), m_hIMC);

	if (!ImmGetOpenStatus(m_hIMC))  // gvim command mode
	{
		return FALSE;
	}

	if (!m_client.Echo())
	{
		m_client.Connect(NULL);
		m_client.StartSession();
	}

	weasel::KeyEvent ke;
	if (!ConvertKeyEvent(vKey, kinfo, lpbKeyState, ke))
	{
		// unknown key event
		return FALSE;
	}

	bool accepted = m_client.ProcessKeyEvent(ke);

	// get commit string from server
	std::wstring commit;
	weasel::Status status;
	weasel::ResponseParser parser(&commit, NULL, &status);
	bool ok = m_client.GetResponseData(std::ref(parser));

	if (ok)
	{
		if (!commit.empty())
		{
			_EndComposition(commit.c_str());
		}
		else if (status.composing != m_composing)
		{
			if (m_composing)
				_EndComposition(NULL);
			else
				_StartComposition();
		}
	}

	return (BOOL)accepted;
}

HRESULT WeaselIME::_Initialize()
{
	LPINPUTCONTEXT lpIMC = ImmLockIMC(m_hIMC);
	if(!lpIMC)
		return E_FAIL;

	lpIMC->fOpen = TRUE;

	HIMCC& hIMCC = lpIMC->hCompStr;
	if (!hIMCC)
		hIMCC = ImmCreateIMCC(sizeof(CompositionInfo));
	else
		hIMCC = ImmReSizeIMCC(hIMCC, sizeof(CompositionInfo));
	if(!hIMCC)
	{
		ImmUnlockIMC(m_hIMC);
		return E_FAIL;
	}

	CompositionInfo* pInfo = (CompositionInfo*)ImmLockIMCC(hIMCC);
	if (!pInfo)
	{
		ImmUnlockIMC(m_hIMC);
		return E_FAIL;
	}

	pInfo->Reset();
	ImmUnlockIMCC(hIMCC);
	ImmUnlockIMC(m_hIMC);

	return S_OK;
}

HRESULT WeaselIME::_Finalize()
{
	LPINPUTCONTEXT lpIMC = ImmLockIMC(m_hIMC);
	if (lpIMC)
	{
		lpIMC->fOpen = FALSE;
		if (lpIMC->hCompStr)
		{
			ImmDestroyIMCC(lpIMC->hCompStr);
			lpIMC->hCompStr = NULL;
		}
	}
	ImmUnlockIMC(m_hIMC);

	return S_OK;
}

HRESULT WeaselIME::_StartComposition()
{
	_AddIMEMessage(WM_IME_STARTCOMPOSITION, 0, 0);
	_AddIMEMessage(WM_IME_NOTIFY, IMN_CHANGECANDIDATE, 0);
	_AddIMEMessage(WM_IME_NOTIFY, IMN_OPENCANDIDATE, 0);

	m_composing = true;
	return S_OK;
}

HRESULT WeaselIME::_EndComposition(LPCWSTR composition)
{
	if (composition)
	{
		LPINPUTCONTEXT lpIMC;
		LPCOMPOSITIONSTRING lpCompStr;

		lpIMC = ImmLockIMC(m_hIMC);
		if (!lpIMC)
			return E_FAIL;

		lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);
		if (!lpCompStr)
		{
			ImmUnlockIMC(m_hIMC);
			return E_FAIL;
		}

		CompositionInfo* pInfo = (CompositionInfo*)lpCompStr;
		wcscpy_s(pInfo->szResultStr, composition);
		lpCompStr->dwResultStrLen = wcslen(pInfo->szResultStr);

		ImmUnlockIMCC(lpIMC->hCompStr);
		ImmUnlockIMC(m_hIMC);

		_AddIMEMessage(WM_IME_COMPOSITION, 0, GCS_COMP|GCS_RESULTSTR);
	}
	_AddIMEMessage(WM_IME_ENDCOMPOSITION, 0, 0);
	_AddIMEMessage(WM_IME_NOTIFY, IMN_CLOSECANDIDATE, 0);

	m_composing = false;
	return S_OK;
}

HRESULT WeaselIME::_AddIMEMessage(UINT msg, WPARAM wp, LPARAM lp)
{
	if(!m_hIMC)
		return S_FALSE;

	LPINPUTCONTEXT lpIMC = (LPINPUTCONTEXT)ImmLockIMC(m_hIMC);
	if(!lpIMC)
		return E_FAIL;

	HIMCC hBuf = ImmReSizeIMCC(lpIMC->hMsgBuf,
		sizeof(TRANSMSG) * (lpIMC->dwNumMsgBuf + 1));
	if(!hBuf)
	{
		ImmUnlockIMC(m_hIMC);
		return E_FAIL;
	}
	lpIMC->hMsgBuf = hBuf;

	LPTRANSMSG pBuf = (LPTRANSMSG)ImmLockIMCC(hBuf);
	if(!pBuf)
	{
		ImmUnlockIMC(m_hIMC);
		return E_FAIL;
	}

	DWORD last = lpIMC->dwNumMsgBuf;
	pBuf[last].message = msg;
	pBuf[last].wParam = wp;
	pBuf[last].lParam = lp;
	lpIMC->dwNumMsgBuf++;
	ImmUnlockIMCC(hBuf);

	ImmUnlockIMC(m_hIMC);

	if (!ImmGenerateMessage(m_hIMC))
	{
		return E_FAIL;
	}

	return S_OK;
}

void WeaselIME::_UpdateInputPosition(LPINPUTCONTEXT lpIMC, POINT pt)
{
	EZDBGONLYLOGGERPRINT("_UpdateInputPosition: (%d, %d)", pt.x, pt.y);

	//EZDBGONLYLOGGERPRINT("cfCompForm: ptCurrentPos = (%d, %d), rcArea = (%d, %d)",
	//	lpIMC->cfCompForm.ptCurrentPos.x, lpIMC->cfCompForm.ptCurrentPos.y,
	//	lpIMC->cfCompForm.rcArea.left, lpIMC->cfCompForm.rcArea.top);
	//EZDBGONLYLOGGERPRINT("cfCandForm[0]: ptCurrentPos = (%d, %d), rcArea = (%d, %d)",
	//	lpIMC->cfCandForm[0].ptCurrentPos.x, lpIMC->cfCandForm[0].ptCurrentPos.y,
	//	lpIMC->cfCandForm[0].rcArea.left, lpIMC->cfCandForm[0].rcArea.top);
	//EZDBGONLYLOGGERPRINT("cfCandForm[1]: ptCurrentPos = (%d, %d), rcArea = (%d, %d)",
	//	lpIMC->cfCandForm[1].ptCurrentPos.x, lpIMC->cfCandForm[1].ptCurrentPos.y,
	//	lpIMC->cfCandForm[1].rcArea.left, lpIMC->cfCandForm[1].rcArea.top);

	if (pt.x == -1 && pt.y == -1)
	{
		EZDBGONLYLOGGERPRINT("Caret pos detection required.");
		if(!GetCaretPos(&pt))
		{
			EZDBGONLYLOGGERPRINT("Failed to determine caret pos.");
			pt.x = pt.y = 0;
		}
	}

	ClientToScreen(lpIMC->hWnd, &pt);
	if (pt.x < -4096 || pt.x >= 4096 || pt.y < -4096 || pt.y >= 4096)
	{
		EZDBGONLYLOGGERPRINT("Input position out of range, possibly invalid.");
		return;
	}

	int height = abs(lpIMC->lfFont.W.lfHeight);
	if (height == 0)
	{
		HDC hDC = GetDC(lpIMC->hWnd);
		SIZE sz = {0};
		GetTextExtentPoint(hDC, L"A", 1, &sz);
		height = sz.cy;
		ReleaseDC(lpIMC->hWnd, hDC);
	}
	const int width = 6;
	RECT rc;
	SetRect(&rc, pt.x, pt.y, pt.x + width, pt.y + height);
	EZDBGONLYLOGGERPRINT("Updating input position: (%d, %d)", pt.x, pt.y);
	m_client.UpdateInputPosition(rc);
}
