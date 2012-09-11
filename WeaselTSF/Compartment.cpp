#include "stdafx.h"
#include "WeaselTSF.h"

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
	ITfCompartmentMgr *pCompMgr = NULL;
	BOOL fOpen = FALSE;

	if (_pThreadMgr->QueryInterface(IID_ITfCompartmentMgr, (void **) &pCompMgr) == S_OK)
	{
		ITfCompartment *pCompartment;
		if (pCompMgr->GetCompartment(GUID_COMPARTMENT_KEYBOARD_OPENCLOSE, &pCompartment) == S_OK)
		{
			VARIANT var;
			if (pCompartment->GetValue(&var) == S_OK)
			{
				if (var.vt == VT_I4) // Even VT_EMPTY, GetValue() can succeed
					fOpen = (BOOL) var.lVal;
			}
		}
		pCompMgr->Release();
	}
	return fOpen;
}

HRESULT WeaselTSF::_SetKeyboardOpen(BOOL fOpen)
{
	HRESULT hr = E_FAIL;
	ITfCompartmentMgr *pCompMgr = NULL;

	if (_pThreadMgr->QueryInterface(IID_ITfCompartmentMgr, (void **) &pCompMgr) == S_OK)
	{
		ITfCompartment *pCompartment;
		if (pCompMgr->GetCompartment(GUID_COMPARTMENT_KEYBOARD_OPENCLOSE, &pCompartment) == S_OK)
		{
			VARIANT var;
			var.vt = VT_I4;
			var.lVal = fOpen;
			hr = pCompartment->SetValue(_tfClientId, &var);
		}
		pCompMgr->Release();
	}

	return hr;
}