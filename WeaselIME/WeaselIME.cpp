// WeaselIME.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include <ResponseParser.h>
#include "WeaselIME.h"

const WCHAR WEASEL[] = L"小狼毫 0.9";
const WCHAR WEASEL_IME_FILE[] = L"weasel0.ime";
const WCHAR WEASEL_REG_KEY[] = L"Software\\Rime\\Weasel0";

HINSTANCE WeaselIME::s_hModule = 0;
HIMCMap WeaselIME::s_instances;

static bool launch_server()
{

	// 從註冊表取得server位置
	HKEY hKey;
	LSTATUS ret = RegOpenKey(HKEY_LOCAL_MACHINE, WeaselIME::GetRegKey(), &hKey);
	if (ret != ERROR_SUCCESS)
	{
		MessageBox(NULL, L"註冊表信息無影了", WEASEL, MB_ICONERROR | MB_OK);
		return false;
	}

	WCHAR value[MAX_PATH];
	DWORD len = sizeof(value);
	DWORD type = 0;
	ret = RegQueryValueEx(hKey, L"WeaselRoot", NULL, &type, (LPBYTE)value, &len);
	if (ret != ERROR_SUCCESS)
	{
		MessageBox(NULL, L"未設置 WeaselRoot", WEASEL, MB_ICONERROR | MB_OK);
		RegCloseKey(hKey);
		return false;
	}
	wpath weaselRoot(value);

	len = sizeof(value);
	type = 0;
	ret = RegQueryValueEx(hKey, L"ServerExecutable", NULL, &type, (LPBYTE)value, &len);
	if (ret != ERROR_SUCCESS)
	{
		MessageBox(NULL, L"未設置 ServerExecutable", WEASEL, MB_ICONERROR | MB_OK);
		RegCloseKey(hKey);
		return false;
	}
	wpath serverPath(weaselRoot / value);

	RegCloseKey(hKey);

	// 啓動服務進程
	wstring exe = serverPath.native_file_string();
	wstring dir = weaselRoot.native_file_string();
	int retCode = (int)ShellExecute(NULL, L"open", exe.c_str(), NULL, dir.c_str(), SW_HIDE);
	if (retCode <= 32)
	{
		MessageBox(NULL, L"服務進程啓動不起來 :(", WEASEL, MB_ICONERROR | MB_OK);
		return false;
	}
	return true;
}

WeaselIME::WeaselIME(HIMC hIMC) 
	: m_hIMC(hIMC), m_preferCandidatePos(false)
{
	WCHAR path[MAX_PATH];
	GetModuleFileName(NULL, path, _countof(path));
	wstring exe = wpath(path).filename();
	if (boost::iequals(L"chrome.exe", exe))
		m_preferCandidatePos = true;
}

LPCWSTR WeaselIME::GetIMEName()
{
	return WEASEL;
}

LPCWSTR WeaselIME::GetIMEFileName()
{
	return WEASEL_IME_FILE;
}

LPCWSTR WeaselIME::GetRegKey()
{
	return WEASEL_REG_KEY;
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
		shared_ptr<WeaselIME> p = WeaselIME::GetInstance(hIMC);
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

shared_ptr<WeaselIME> WeaselIME::GetInstance(HIMC hIMC)
{
    if (!s_instances.is_valid())
    {
        return shared_ptr<WeaselIME>();
    }
    boost::lock_guard<boost::mutex> lock(s_instances.get_mutex());
	shared_ptr<WeaselIME>& p = s_instances[hIMC];
	if (!p)
	{
		p.reset(new WeaselIME(hIMC));
	}
	return p;
}

void WeaselIME::Cleanup()
{
	for (map<HIMC, shared_ptr<WeaselIME> >::const_iterator i = s_instances.begin(); i != s_instances.end(); ++i)
	{
		shared_ptr<WeaselIME> p = i->second;
		p->OnIMESelect(FALSE);
	}
	boost::lock_guard<boost::mutex> lock(s_instances.get_mutex());
	s_instances.clear();
}

LRESULT WeaselIME::OnIMESelect(BOOL fSelect)
{
	if (fSelect)
	{
		if (!m_ui.Create(NULL))
			return 0;
		
		m_ui.SetStyle(GetUIStyleSettings());

		// initialize weasel client
		m_client.Connect(launch_server);
		m_client.StartSession();

		wstring ignored;
		m_ctx.clear();
		m_status.reset();
		weasel::ResponseParser parser(ignored, m_ctx, m_status);
		m_client.GetResponseData(boost::ref(parser));

		m_ui.UpdateContext(m_ctx);
		//m_ui.UpdateStatus(m_status);
		
		return _Initialize();
	}
	else
	{
		m_client.EndSession();
		m_ui.Destroy();

		return _Finalize();
	}
}

LRESULT WeaselIME::OnIMEFocus(BOOL fFocus)
{
	if (fFocus)
	{
		LPINPUTCONTEXT lpIMC = ImmLockIMC(m_hIMC);
		if(lpIMC)
		{
			if(!(lpIMC->fdwInit & INIT_COMPFORM))
			{
				lpIMC->cfCompForm.dwStyle = CFS_DEFAULT;
				GetCursorPos(&lpIMC->cfCompForm.ptCurrentPos);
				ScreenToClient(lpIMC->hWnd, &lpIMC->cfCompForm.ptCurrentPos);
				lpIMC->fdwInit |= INIT_COMPFORM;
			}
			ImmUnlockIMC(m_hIMC);
		}
		
		if (!m_ctx.empty())
			m_ui.Show();
	}
	else
	{
		m_ui.Hide();
	}

	return 0;
}

LRESULT WeaselIME::OnUIMessage(HWND hWnd, UINT uMsg, WPARAM wp, LPARAM lp)
{
	LPINPUTCONTEXT lpIMC = (LPINPUTCONTEXT)ImmLockIMC(m_hIMC);
	switch (uMsg)
	{
	case WM_IME_NOTIFY:
		{
			_OnIMENotify(lpIMC, wp, lp);
		}
		break;
	case WM_IME_SELECT:
		{
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
	}

	ImmUnlockIMC(m_hIMC);
	return 0;
}

LRESULT WeaselIME::_OnIMENotify(LPINPUTCONTEXT lpIMC, WPARAM wp, LPARAM lp)
{			
	switch (wp)
	{
	case IMN_SETCANDIDATEPOS:
		{
			_SetCandidatePos(lpIMC);
		}

	case IMN_SETCOMPOSITIONWINDOW:
		{
			if (m_preferCandidatePos)
				_SetCandidatePos(lpIMC);
			else
				_SetCompositionWindow(lpIMC);
		}
		break;
	}

	return 0;
}

void WeaselIME::_SetCandidatePos(LPINPUTCONTEXT lpIMC)
{
	POINT pt = lpIMC->cfCandForm[0].ptCurrentPos;
	_UpdateInputPosition(lpIMC, pt);
}

void WeaselIME::_SetCompositionWindow(LPINPUTCONTEXT lpIMC)
{
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
	bool accepted = false;

#ifdef KEYCODE_VIEWER
	{
		weasel::KeyEvent ke;
		if (!ConvertKeyEvent(vKey, kinfo, lpbKeyState, ke))
		{
			// unknown key event
			m_ctx.clear();
			m_ctx.aux.str = (boost::wformat(L"unknown key event vKey: %x, scanCode: %x, isKeyUp: %u") % vKey % kinfo.scanCode % kinfo.isKeyUp).str();
			return FALSE;
		}
		m_ctx.aux.str = (boost::wformat(L"keycode: %x, mask: %x, isKeyUp: %u") % ke.keycode % ke.mask % kinfo.isKeyUp).str();
		_UpdateContext(m_ctx);
		return FALSE;
	}
#endif

	// 要处理KEY_UP事件（宫保拼音用KEY_UP）
	//if (kinfo.isKeyUp)
	//{
	//	return FALSE;
	//}

	if (!m_client.Echo())
	{
		m_client.Connect(launch_server);
		m_client.StartSession();
	}
	
	weasel::KeyEvent ke;
	if (!ConvertKeyEvent(vKey, kinfo, lpbKeyState, ke))
	{
		// unknown key event
		return FALSE;
	}

	accepted = m_client.ProcessKeyEvent(ke);

	wstring commit;
	m_ctx.clear();
	weasel::ResponseParser parser(commit, m_ctx, m_status);
	bool ok = m_client.GetResponseData(boost::ref(parser));
	if (!ok)
	{
		// may suffer loss of data...
		m_ctx.preedit.clear();
		m_ctx.preedit.str = L"不好使了 :(";
		//return TRUE;
	}

	if (!commit.empty())
	{
		if (!m_status.composing)
		{
			_StartComposition();
		}
		_EndComposition(commit.c_str());
		m_status.composing = false;
	}

	if (!m_ctx.preedit.empty())
	{
		if (!m_status.composing)
		{
			_StartComposition();
			m_status.composing = true;
		}
	}
	else
	{
		if (m_status.composing)
		{
			_EndComposition(L"");
			m_status.composing = false;
		}
	}

	_UpdateContext(m_ctx);

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
	_AddIMEMessage(WM_IME_NOTIFY, IMN_OPENCANDIDATE, 0);

	return S_OK;
}

HRESULT WeaselIME::_EndComposition(LPCWSTR composition)
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
	_AddIMEMessage(WM_IME_ENDCOMPOSITION, 0, 0);
	_AddIMEMessage(WM_IME_NOTIFY, IMN_CLOSECANDIDATE, 0);
	
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
	if (pt.x == -1 && pt.y == -1)
	{
		// caret pos detection required
		if(!GetCaretPos(&pt))
			pt.x = pt.y = 0;
	}

	ClientToScreen(lpIMC->hWnd, &pt);
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
	m_ui.UpdateInputPosition(rc);
}

void WeaselIME::_UpdateContext(weasel::Context const& ctx)
{
	if (!ctx.empty())
	{
		m_ui.UpdateContext(m_ctx);
		m_ui.Show();
	}
	else
	{
		m_ui.Hide();
		m_ui.UpdateContext(m_ctx);
	}
}

weasel::UIStyle const WeaselIME::GetUIStyleSettings()
{
	weasel::UIStyle style;

	HKEY hKey;
	LSTATUS ret = RegOpenKey(HKEY_LOCAL_MACHINE, WeaselIME::GetRegKey(), &hKey);
	if (ret != ERROR_SUCCESS)
	{
		return style;
	}

	{
		WCHAR value[100];
		DWORD len = sizeof(value);
		DWORD type = REG_SZ;
		ret = RegQueryValueEx(hKey, L"FontFace", NULL, &type, (LPBYTE)value, &len);
		if (ret == ERROR_SUCCESS)
			style.fontFace = value;
	}

	{
		DWORD dword = 0;
		DWORD len = sizeof(dword);
		DWORD type = REG_DWORD;
		ret = RegQueryValueEx(hKey, L"FontPoint", NULL, &type, (LPBYTE)&dword, &len);
		if (ret == ERROR_SUCCESS)
			style.fontPoint = (int)dword;
	}

	RegCloseKey(hKey);
	return style;
}
