#include "stdafx.h"
#include "WeaselTSF.h"

STDAPI WeaselTSF::OnInitDocumentMgr(ITfDocumentMgr *pDocMgr)
{
	return S_OK;
}

STDAPI WeaselTSF::OnUninitDocumentMgr(ITfDocumentMgr *pDocMgr)
{
	return S_OK;
}

STDAPI WeaselTSF::OnSetFocus(ITfDocumentMgr *pDocMgrFocus, ITfDocumentMgr *pDocMgrPrevFocus)
{
	return S_OK;
}

STDAPI WeaselTSF::OnPushContext(ITfContext *pContext)
{
	return S_OK;
}

STDAPI WeaselTSF::OnPopContext(ITfContext *pContext)
{
	return S_OK;
}

BOOL WeaselTSF::_InitThreadMgrEventSink()
{
	ITfSource *pSource;
	if (_pThreadMgr->QueryInterface(IID_ITfSource, (void **) &pSource) != S_OK)
		return FALSE;
	if (pSource->AdviseSink(IID_ITfThreadMgrEventSink, (ITfThreadMgrEventSink *) this, &_dwThreadMgrEventSinkCookie) != S_OK)
	{
		_dwThreadMgrEventSinkCookie = TF_INVALID_COOKIE;
		pSource->Release();
		return FALSE;
	}
	return TRUE;
}

void WeaselTSF::_UninitThreadMgrEventSink()
{
	ITfSource *pSource;
	if (_dwThreadMgrEventSinkCookie == TF_INVALID_COOKIE)
		return;
	if (SUCCEEDED(_pThreadMgr->QueryInterface(IID_ITfSource, (void **) &pSource)))
	{
		pSource->UnadviseSink(_dwThreadMgrEventSinkCookie);
		pSource->Release();
	}
	_dwThreadMgrEventSinkCookie = TF_INVALID_COOKIE;
}