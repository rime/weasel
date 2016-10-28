#include "stdafx.h"

#include "WeaselTSF.h"
#include "WeaselCommon.h"

static void error_message(const WCHAR *msg)
{
	static DWORD next_tick = 0;
	DWORD now = GetTickCount();
	if (now > next_tick)
	{
		next_tick = now + 10000;  // (ms)
		MessageBox(NULL, msg, TEXTSERVICE_DESC, MB_ICONERROR | MB_OK);
	}
}

static bool launch_server()
{
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
	//	EZDBGONLYLOGGERPRINT("ERROR: failed to launch weasel server.");
		error_message(L"服務進程啓動不起來 :(");
		return false;
	}

	if (!WaitForInputIdle(process_info.hProcess, 1500))
	{
//		EZDBGONLYLOGGERPRINT("WARNING: WaitForInputIdle() timed out; succeeding IPC messages might not be delivered.");
	}
	if (process_info.hProcess) CloseHandle(process_info.hProcess);
	if (process_info.hThread) CloseHandle(process_info.hThread);

	return true;
}

WeaselTSF::WeaselTSF()
{
	_cRef = 1;

	_pThreadMgr = NULL;

	_dwThreadMgrEventSinkCookie = TF_INVALID_COOKIE;

	_pTextEditSinkContext = NULL;
	_dwTextEditSinkCookie = TF_INVALID_COOKIE;
	_dwTextLayoutSinkCookie = TF_INVALID_COOKIE;
	_fTestKeyDownPending = FALSE;
	_fTestKeyUpPending = FALSE;

	_pComposition = NULL;

	_fCUASWorkaroundTested = _fCUASWorkaroundEnabled = FALSE;

	DllAddRef();
}

WeaselTSF::~WeaselTSF()
{
	DllRelease();
}

STDAPI WeaselTSF::QueryInterface(REFIID riid, void **ppvObject)
{
	if (ppvObject == NULL)
		return E_INVALIDARG;

	*ppvObject = NULL;

	if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfTextInputProcessor))
		*ppvObject = (ITfTextInputProcessor *) this;
	else if (IsEqualIID(riid, IID_ITfThreadMgrEventSink))
		*ppvObject = (ITfThreadMgrEventSink *) this;
	else if (IsEqualIID(riid, IID_ITfTextEditSink))
		*ppvObject = (ITfTextEditSink *) this;
	else if (IsEqualIID(riid, IID_ITfTextLayoutSink))
		*ppvObject = (ITfTextLayoutSink *) this;
	else if (IsEqualIID(riid, IID_ITfKeyEventSink))
		*ppvObject = (ITfKeyEventSink *) this;
	else if (IsEqualIID(riid, IID_ITfCompositionSink))
		*ppvObject = (ITfCompositionSink *) this;
	else if (IsEqualIID(riid, IID_ITfEditSession))
		*ppvObject = (ITfEditSession *) this;

	if (*ppvObject)
	{
		AddRef();
		return S_OK;
	}
	return E_NOINTERFACE;
}

STDAPI_(ULONG) WeaselTSF::AddRef()
{
	return ++_cRef;
}

STDAPI_(ULONG) WeaselTSF::Release()
{
	LONG cr = --_cRef;

	assert(_cRef >= 0);

	if (_cRef == 0)
		delete this;

	return cr;
}

STDAPI WeaselTSF::Activate(ITfThreadMgr *pThreadMgr, TfClientId tfClientId)
{
	_EnsureServerConnected();

	_pThreadMgr = pThreadMgr;
	_pThreadMgr->AddRef();
	_tfClientId = tfClientId;

	if (!_InitThreadMgrEventSink())
		goto ExitError;

	ITfDocumentMgr *pDocMgrFocus;
	if ((_pThreadMgr->GetFocus(&pDocMgrFocus) == S_OK) && (pDocMgrFocus != NULL))
	{
		_InitTextEditSink(pDocMgrFocus);
		pDocMgrFocus->Release();
	}

	if (!_InitKeyEventSink())
		goto ExitError;

	if (!_InitPreservedKey())
		goto ExitError;

	// TODO not yet complete
	_pLangBarButton = NULL;
	//if (!_InitLanguageBar())
	//	goto ExitError;

	if (!_IsKeyboardOpen())
		_SetKeyboardOpen(TRUE);

	return S_OK;

ExitError:
	Deactivate();
	return E_FAIL;
}

STDAPI WeaselTSF::Deactivate()
{
	m_client.EndSession();

	_InitTextEditSink(NULL);

	_UninitThreadMgrEventSink();

	_UninitKeyEventSink();
	_UninitPreservedKey();

	_UninitLanguageBar();

	if (_pThreadMgr != NULL)
	{
		_pThreadMgr->Release();
		_pThreadMgr = NULL;
	}
	
	_tfClientId = TF_CLIENTID_NULL;

	return S_OK;
}

void WeaselTSF::_EnsureServerConnected()
{
	if (!m_client.Echo())
	{
		m_client.Connect(launch_server);
		m_client.StartSession();
	}
}