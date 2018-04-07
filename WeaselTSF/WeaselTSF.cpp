#include "stdafx.h"

#include "WeaselTSF.h"
#include "WeaselCommon.h"
#include "CandidateList.h"
#include "ResponseParser.h"

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

	_cand = std::make_unique<weasel::CandidateList>(this);

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
		*ppvObject = (ITfTextInputProcessor *)this;
	else if (IsEqualIID(riid, IID_ITfTextInputProcessorEx))
		*ppvObject = (ITfTextInputProcessorEx *)this;
	else if (IsEqualIID(riid, IID_ITfThreadMgrEventSink))
		*ppvObject = (ITfThreadMgrEventSink *)this;
	else if (IsEqualIID(riid, IID_ITfTextEditSink))
		*ppvObject = (ITfTextEditSink *)this;
	else if (IsEqualIID(riid, IID_ITfTextLayoutSink))
		*ppvObject = (ITfTextLayoutSink *)this;
	else if (IsEqualIID(riid, IID_ITfKeyEventSink))
		*ppvObject = (ITfKeyEventSink *)this;
	else if (IsEqualIID(riid, IID_ITfCompositionSink))
		*ppvObject = (ITfCompositionSink *)this;
	else if (IsEqualIID(riid, IID_ITfEditSession))
		*ppvObject = (ITfEditSession *)this;
	else if (IsEqualIID(riid, IID_ITfThreadFocusSink))
		*ppvObject = (ITfThreadFocusSink *)this;

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
	return ActivateEx(pThreadMgr, tfClientId, 0U);
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

	_cand->Destroy();

	return S_OK;
}

STDAPI WeaselTSF::ActivateEx(ITfThreadMgr *pThreadMgr, TfClientId tfClientId, DWORD dwFlags)
{
	_activateFlags = dwFlags;

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
	if (!_InitLanguageBar())
		goto ExitError;

	if (!_IsKeyboardOpen())
		_SetKeyboardOpen(TRUE);

	_EnsureServerConnected();

	return S_OK;

ExitError:
	Deactivate();
	return E_FAIL;
}

STDMETHODIMP WeaselTSF::OnSetThreadFocus()
{
	return S_OK;
}
STDMETHODIMP WeaselTSF::OnKillThreadFocus()
{
	_AbortComposition();
	return S_OK;
}


void WeaselTSF::_EnsureServerConnected()
{
	if (!m_client.Echo())
	{
		m_client.Disconnect();
		m_client.Connect(NULL);
		m_client.StartSession();
		weasel::Status status;
		weasel::ResponseParser parser(NULL, NULL, &status, NULL, &_cand->style());
		bool ok = m_client.GetResponseData(std::ref(parser));
		if (ok) {
			_UpdateLanguageBar(status);
		}
	}
}