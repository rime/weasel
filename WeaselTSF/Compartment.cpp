#include "stdafx.h"
#include "WeaselTSF.h"
#include "Compartment.h"
#include <resource.h>
#include <functional>

STDAPI CCompartmentEventSink::QueryInterface(REFIID riid, _Outptr_ void **ppvObj)
{
	if (ppvObj == nullptr)
		return E_INVALIDARG;

	*ppvObj = nullptr;

	if (IsEqualIID(riid, IID_IUnknown) ||
		IsEqualIID(riid, IID_ITfCompartmentEventSink))
	{
		*ppvObj = (CCompartmentEventSink *)this;
	}

	if (*ppvObj)
	{
		AddRef();
		return S_OK;
	}

	return E_NOINTERFACE;
}

STDAPI_(ULONG) CCompartmentEventSink::AddRef()
{
	return ++_refCount;
}

STDAPI_(ULONG) CCompartmentEventSink::Release()
{
	LONG cr = --_refCount;

	assert(_refCount >= 0);

	if (_refCount == 0)
	{
		delete this;
	}

	return cr;
}

STDAPI CCompartmentEventSink::OnChange(_In_ REFGUID guidCompartment)
{
	return _callback(guidCompartment);
}

HRESULT CCompartmentEventSink::_Advise(_In_ com_ptr<IUnknown> punk, _In_ REFGUID guidCompartment)
{
	HRESULT hr = S_OK;
	ITfCompartmentMgr* pCompartmentMgr = nullptr;
	ITfSource* pSource = nullptr;

	hr = punk->QueryInterface(IID_ITfCompartmentMgr, (void **)&pCompartmentMgr);
	if (FAILED(hr))
	{
		return hr;
	}

	hr = pCompartmentMgr->GetCompartment(guidCompartment, &_compartment);
	if (SUCCEEDED(hr))
	{
		hr = _compartment->QueryInterface(IID_ITfSource, (void **)&pSource);
		if (SUCCEEDED(hr))
		{
			hr = pSource->AdviseSink(IID_ITfCompartmentEventSink, this, &_cookie);
			pSource->Release();
		}
	}

	pCompartmentMgr->Release();

	return hr;
}
HRESULT CCompartmentEventSink::_Unadvise()
{
	HRESULT hr = S_OK;
	ITfSource* pSource = nullptr;

	hr = _compartment->QueryInterface(IID_ITfSource, (void **)&pSource);
	if (SUCCEEDED(hr))
	{
		hr = pSource->UnadviseSink(_cookie);
		pSource->Release();
	}

	_compartment = nullptr;
	_cookie = 0;

	return hr;
}

HRESULT CCompartmentEventSink::_GlobalAdvise(_In_ com_ptr<ITfThreadMgr> pThreadMgr,
                                       _In_ REFGUID guidCompartment) {

	HRESULT hr = S_OK;
	ITfCompartmentMgr* pCompartmentMgr = nullptr;
	ITfSource* pSource = nullptr;

	hr = pThreadMgr->GetGlobalCompartment(&pCompartmentMgr);
	if (FAILED(hr)) {
		return hr;
	}

	hr = pCompartmentMgr->GetCompartment(guidCompartment, &_compartment);
	if (SUCCEEDED(hr)) {
		hr = _compartment->QueryInterface(IID_ITfSource, (void **)&pSource);
		if (SUCCEEDED(hr)) {
			hr = pSource->AdviseSink(IID_ITfCompartmentEventSink, this, &_cookie);
			pSource->Release();
		}
	}

	pCompartmentMgr->Release();

	return hr;
}


BOOL WeaselTSF::_IsKeyboardDisabled()
{
	ITfCompartmentMgr *pCompMgr = NULL;
	ITfDocumentMgr *pDocMgrFocus = NULL;
	ITfContext *pContext = NULL;
	BOOL fDisabled = FALSE;

	if ((_pThreadMgr->GetFocus(&pDocMgrFocus) != S_OK) || (pDocMgrFocus == NULL))
	{
		fDisabled = TRUE;
		goto Exit;
	}

	if ((pDocMgrFocus->GetTop(&pContext) != S_OK) || (pContext == NULL))
	{
		fDisabled = TRUE;
		goto Exit;
	}

	if (pContext->QueryInterface(IID_ITfCompartmentMgr, (void **) &pCompMgr) == S_OK)
	{
		ITfCompartment *pCompartmentDisabled;
		ITfCompartment *pCompartmentEmptyContext;

		/* Check GUID_COMPARTMENT_KEYBOARD_DISABLED */
		if (pCompMgr->GetCompartment(GUID_COMPARTMENT_KEYBOARD_DISABLED, &pCompartmentDisabled) == S_OK)
		{
			VARIANT var;
			if (pCompartmentDisabled->GetValue(&var) == S_OK)
			{
				if (var.vt == VT_I4) // Even VT_EMPTY, GetValue() can succeed
					fDisabled = (BOOL) var.lVal;
			}
			pCompartmentDisabled->Release();
		}

		/* Check GUID_COMPARTMENT_EMPTYCONTEXT */
		if (pCompMgr->GetCompartment(GUID_COMPARTMENT_EMPTYCONTEXT, &pCompartmentEmptyContext)  == S_OK)
		{
			VARIANT var;
			if (pCompartmentEmptyContext->GetValue(&var) == S_OK)
			{
				if (var.vt == VT_I4) // Even VT_EMPTY, GetValue() can succeed
					fDisabled = (BOOL) var.lVal;
			}
			pCompartmentEmptyContext->Release();
		}
		pCompMgr->Release();
	}

Exit:
	if (pContext)
		pContext->Release();
	if (pDocMgrFocus)
		pDocMgrFocus->Release();
	return fDisabled;
}

BOOL WeaselTSF::_IsKeyboardOpen()
{
	com_ptr<ITfCompartmentMgr> pCompMgr;
	BOOL fOpen = FALSE;

	if (_pThreadMgr->QueryInterface(&pCompMgr) == S_OK)
	{
		com_ptr<ITfCompartment> pCompartment;
		if (pCompMgr->GetCompartment(GUID_COMPARTMENT_KEYBOARD_OPENCLOSE, &pCompartment) == S_OK)
		{
			VARIANT var;
			if (pCompartment->GetValue(&var) == S_OK)
			{
				if (var.vt == VT_I4) // Even VT_EMPTY, GetValue() can succeed
					fOpen = (BOOL) var.lVal;
			}
		}
	}
	return fOpen;
}

HRESULT WeaselTSF::_SetKeyboardOpen(BOOL fOpen)
{
	HRESULT hr = E_FAIL;
	com_ptr<ITfCompartmentMgr> pCompMgr;

	if (_pThreadMgr->QueryInterface(&pCompMgr) == S_OK)
	{
		ITfCompartment *pCompartment;
		if (pCompMgr->GetCompartment(GUID_COMPARTMENT_KEYBOARD_OPENCLOSE, &pCompartment) == S_OK)
		{
			VARIANT var;
			var.vt = VT_I4;
			var.lVal = fOpen;
			hr = pCompartment->SetValue(_tfClientId, &var);
		}
	}

	return hr;
}

bool WeaselTSF::_GetAsciiMode() {
	com_ptr<ITfCompartmentMgr> pCompMgr;
	bool is_ascii_mode = false;

	if (_pThreadMgr->GetGlobalCompartment(&pCompMgr) == S_OK) {
		com_ptr<ITfCompartment> pCompartment;
		if (pCompMgr->GetCompartment(c_guidStatus, &pCompartment) == S_OK) {
			VARIANT var;
			if (pCompartment->GetValue(&var) == S_OK) {
				if (var.vt == VT_I4) {
					is_ascii_mode = (bool)var.intVal;
					VariantClear(&var);
				}
			}
		}
	}
	return is_ascii_mode;
}

HRESULT WeaselTSF::_SyncAsciiMode(bool is_ascii_mode) {
	if (!_config.global_ascii_mode) {
		weasel::Status stat = {};
		stat.ascii_mode = is_ascii_mode;
		_UpdateLanguageBar(stat);
		return S_OK;
	}
	HRESULT hr = E_FAIL;
	com_ptr<ITfCompartmentMgr> pCompMgr;

	if (_pThreadMgr->GetGlobalCompartment(&pCompMgr) == S_OK) {
		com_ptr<ITfCompartment> pCompartment;
		if (pCompMgr->GetCompartment(c_guidStatus, &pCompartment) == S_OK) {
			VARIANT var;
			VariantInit(&var);
			var.vt = VT_I4;
			var.intVal = (int)is_ascii_mode;
			hr = pCompartment->SetValue(_tfClientId, &var);
			VariantClear(&var);
		}
	}

	return hr;
}

BOOL WeaselTSF::_InitCompartment()
{
	using namespace std::placeholders;

	auto callback = std::bind(&WeaselTSF::_HandleCompartment, this, _1);
	_pKeyboardCompartmentSink = new CCompartmentEventSink(callback);
	if (!_pKeyboardCompartmentSink)
		return FALSE;
	DWORD hr = _pKeyboardCompartmentSink->_Advise(
		(IUnknown *)_pThreadMgr,
		GUID_COMPARTMENT_KEYBOARD_OPENCLOSE
	);

	if (FAILED(hr)) {
		return SUCCEEDED(hr);
	}

	_pStatusCompartmentSink = new CCompartmentEventSink(callback);
	if (!_pStatusCompartmentSink)
		return FALSE;
	hr = _pStatusCompartmentSink->_GlobalAdvise(
		_pThreadMgr,
		c_guidStatus
	);

	return SUCCEEDED(hr);
}

void WeaselTSF::_UninitCompartment()
{
	if (_pKeyboardCompartmentSink) {
		_pKeyboardCompartmentSink->_Unadvise();
		_pKeyboardCompartmentSink = NULL;
	}
	if (_pStatusCompartmentSink) {
		_pStatusCompartmentSink->_Unadvise();
		_pStatusCompartmentSink = NULL;
	}

}

HRESULT WeaselTSF::_HandleCompartment(REFGUID guidCompartment)
{
	if (IsEqualGUID(guidCompartment, GUID_COMPARTMENT_KEYBOARD_OPENCLOSE)) {
		BOOL isOpen = _IsKeyboardOpen();
		_EnableLanguageBar(isOpen);
	} else if (IsEqualGUID(guidCompartment, c_guidStatus)) {
		weasel::Status stat = {};
		stat.ascii_mode = _GetAsciiMode();
		_UpdateLanguageBar(stat);
	}
	return S_OK;
}
