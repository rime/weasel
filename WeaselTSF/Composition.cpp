#include "stdafx.h"
#include "WeaselTSF.h"
#include "EditSession.h"

/* Start Composition */
class CStartCompositionEditSession: public CEditSession
{
public:
	CStartCompositionEditSession(WeaselTSF *pTextService, ITfContext *pContext, BOOL fCUASWorkaroundEnabled)
		: CEditSession(pTextService, pContext)
	{
		_fCUASWorkaroundEnabled = fCUASWorkaroundEnabled;
	}

	/* ITfEditSession */
	STDMETHODIMP DoEditSession(TfEditCookie ec);

private:
	BOOL _fCUASWorkaroundEnabled;
};

STDAPI CStartCompositionEditSession::DoEditSession(TfEditCookie ec)
{
	HRESULT hr = E_FAIL;
	ITfInsertAtSelection *pInsertAtSelection = NULL;
	ITfRange *pRangeComposition = NULL;
	if (_pContext->QueryInterface(IID_ITfInsertAtSelection, (LPVOID *) &pInsertAtSelection) != S_OK)
		goto Exit;
	if (pInsertAtSelection->InsertTextAtSelection(ec, TF_IAS_QUERYONLY, NULL, 0, &pRangeComposition) != S_OK)
		goto Exit;
	
	ITfContextComposition *pContextComposition = NULL;
	ITfComposition *pComposition = NULL;
	if (_pContext->QueryInterface(IID_ITfContextComposition, (LPVOID *) &pContextComposition) != S_OK)
		goto Exit;
	if ((pContextComposition->StartComposition(ec, pRangeComposition, _pTextService, &pComposition) == S_OK) && (pComposition != NULL))
	{
		_pTextService->_SetComposition(pComposition);
		
		/* set selection */
		/* WORKAROUND:
		 *   CUAS does not provide a correct GetTextExt() position unless the composition is filled with characters.
		 *   So we insert a dummy space character here.
		 *   This is the same workaround used by Microsoft Pinyin IME (New Experience).
		 */
		if (_fCUASWorkaroundEnabled)
		{
			pRangeComposition->SetText(ec, TF_ST_CORRECTION, L" ", 1);
			pRangeComposition->Collapse(ec, TF_ANCHOR_START);
		}
		TF_SELECTION tfSelection;
		tfSelection.range = pRangeComposition;
		tfSelection.style.ase = TF_AE_NONE;
		tfSelection.style.fInterimChar = FALSE;
		_pContext->SetSelection(ec, 1, &tfSelection);
	}
	
Exit:
	if (pContextComposition != NULL)
		pContextComposition->Release();
	if (pRangeComposition != NULL)
		pRangeComposition->Release();
	if (pInsertAtSelection != NULL)
		pInsertAtSelection->Release();

	return hr;
}

void WeaselTSF::_StartComposition(ITfContext *pContext)
{
	CStartCompositionEditSession *pStartCompositionEditSession;
	if ((pStartCompositionEditSession = new CStartCompositionEditSession(this, pContext, _fCUASWorkaroundEnabled)) != NULL)
	{
		HRESULT hr;
		pContext->RequestEditSession(_tfClientId, pStartCompositionEditSession, TF_ES_SYNC | TF_ES_READWRITE, &hr);
		pStartCompositionEditSession->Release();
	}
}

/* End Composition */
class CEndCompositionEditSession: public CEditSession
{
public:
	CEndCompositionEditSession(WeaselTSF *pTextService, ITfContext *pContext, ITfComposition *pComposition)
		: CEditSession(pTextService, pContext)
	{
		_pComposition = pComposition;
	}

	/* ITfEditSession */
	STDMETHODIMP DoEditSession(TfEditCookie ec);

private:
	ITfComposition *_pComposition;
};

STDAPI CEndCompositionEditSession::DoEditSession(TfEditCookie ec)
{
	/* Clear the dummy text we set before, if any. */
	ITfRange *pCompositionRange;
	if (_pComposition->GetRange(&pCompositionRange) == S_OK)
		pCompositionRange->SetText(ec, 0, L"", 0);
	
	_pComposition->EndComposition(ec);
	_pTextService->OnCompositionTerminated(ec, _pComposition);
	return S_OK;
}

void WeaselTSF::_EndComposition(ITfContext *pContext)
{
	CEndCompositionEditSession *pEditSession;
	HRESULT hr;

	if ((pEditSession = new CEndCompositionEditSession(this, pContext, _pComposition)) != NULL)
	{
		pContext->RequestEditSession(_tfClientId, pEditSession, TF_ES_ASYNCDONTCARE | TF_ES_READWRITE, &hr);
		pEditSession->Release();
	}
}

/* Get Text Extent */
class CGetTextExtentEditSession: public CEditSession
{
public:
	CGetTextExtentEditSession(WeaselTSF *pTextService, ITfContext *pContext, ITfContextView *pContextView, ITfComposition *pComposition)
		: CEditSession(pTextService, pContext)
	{
		_pContextView = pContextView;
		_pComposition = pComposition;
	}

	/* ITfEditSession */
	STDMETHODIMP DoEditSession(TfEditCookie ec);

private:
	ITfContextView *_pContextView;
	ITfComposition *_pComposition;
};

STDAPI CGetTextExtentEditSession::DoEditSession(TfEditCookie ec)
{
	ITfInsertAtSelection *pInsertAtSelection = NULL;
	ITfRange *pRangeComposition = NULL;
	RECT rc;
	BOOL fClipped;
	if ((_pContext->QueryInterface(IID_ITfInsertAtSelection, (LPVOID *) &pInsertAtSelection)) != S_OK)
		goto Exit;
	if ((pInsertAtSelection->InsertTextAtSelection(ec, TF_IAS_QUERYONLY, NULL, 0, &pRangeComposition)) != S_OK)
		goto Exit;

	if ((_pContextView->GetTextExt(ec, pRangeComposition, &rc, &fClipped)) == S_OK)
		_pTextService->_SetCompositionPosition(rc);

Exit:
	if (pRangeComposition != NULL)
		pRangeComposition->Release();
	if (pInsertAtSelection != NULL)
		pInsertAtSelection->Release();
	return S_OK;
}

/* Composition Window Handling */
BOOL WeaselTSF::_UpdateCompositionWindow(ITfContext *pContext)
{
	ITfContextView *pContextView = NULL;
	if (pContext->GetActiveView(&pContextView) != S_OK)
		return FALSE;
	CGetTextExtentEditSession *pEditSession;
	if ((pEditSession = new CGetTextExtentEditSession(this, pContext, pContextView, _pComposition)) != NULL)
	{
		HRESULT hr;
		pContext->RequestEditSession(_tfClientId, pEditSession, TF_ES_SYNC | TF_ES_READ, &hr);
		pEditSession->Release();
		pContextView->Release();
		return TRUE;
	}
	pContextView->Release();
	return FALSE;
}

void WeaselTSF::_SetCompositionPosition(const RECT &rc)
{
	/* Test if rect is valid.
	 * If it is invalid during CUAS test, we need to apply CUAS workaround */
	if (!_fCUASWorkaroundTested)
	{
		_fCUASWorkaroundTested = TRUE;
		if (rc.top == rc.bottom)
		{
			_fCUASWorkaroundEnabled = TRUE;
			return;
		}
	}
	RECT _rc;
	_rc.left = _rc.right = rc.left;
	_rc.top = _rc.bottom = rc.bottom;
	m_client.UpdateInputPosition(rc);
}

/* Composition State */
STDAPI WeaselTSF::OnCompositionTerminated(TfEditCookie ecWrite, ITfComposition *pComposition)
{
	if (_pComposition != NULL)
	{
		_pComposition->Release();
		_pComposition = NULL;
	}
	return S_OK;
}

void WeaselTSF::_SetComposition(ITfComposition *pComposition)
{
	_pComposition = pComposition;
}

BOOL WeaselTSF::_IsComposing()
{
	return _pComposition != NULL;
}