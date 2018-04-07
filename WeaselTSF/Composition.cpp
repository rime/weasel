#include "stdafx.h"
#include "WeaselTSF.h"
#include "EditSession.h"
#include "ResponseParser.h"
#include "CandidateList.h"

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
		//if (_fCUASWorkaroundEnabled)
		//{
		//	pRangeComposition->SetText(ec, TF_ST_CORRECTION, L" ", 1);
		//	pRangeComposition->Collapse(ec, TF_ANCHOR_START);
		//}

		// NOTE: Seems that `OnCompositionTerminated` will be triggered even when
		//       normally end a composition if not put any string in it.
		//       So just insert a blank here.
		pRangeComposition->SetText(ec, TF_ST_CORRECTION, L" ", 1);
		pRangeComposition->Collapse(ec, TF_ANCHOR_START);
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

void WeaselTSF::_StartComposition(ITfContext *pContext, BOOL fCUASWorkaroundEnabled)
{
	CStartCompositionEditSession *pStartCompositionEditSession;
	if ((pStartCompositionEditSession = new CStartCompositionEditSession(this, pContext, fCUASWorkaroundEnabled)) != NULL)
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
	CEndCompositionEditSession(WeaselTSF *pTextService, ITfContext *pContext, ITfComposition *pComposition, BOOL clear = TRUE)
		: CEditSession(pTextService, pContext), _clear(clear)
	{
		_pComposition = pComposition;
	}

	/* ITfEditSession */
	STDMETHODIMP DoEditSession(TfEditCookie ec);

private:
	ITfComposition *_pComposition;
	BOOL _clear;
};

STDAPI CEndCompositionEditSession::DoEditSession(TfEditCookie ec)
{
	/* Clear the dummy text we set before, if any. */
	if (_pComposition == nullptr) return S_OK;
	ITfRange *pCompositionRange;
	if (_clear && _pComposition->GetRange(&pCompositionRange) == S_OK)
		pCompositionRange->SetText(ec, 0, L"", 0);
	
	_pComposition->EndComposition(ec);
	_pTextService->_FinalizeComposition();
	return S_OK;
}

void WeaselTSF::_EndComposition(ITfContext *pContext, BOOL clear)
{
	CEndCompositionEditSession *pEditSession;
	HRESULT hr;

	if ((pEditSession = new CEndCompositionEditSession(this, pContext, _pComposition, clear)) != NULL)
	{
		pContext->RequestEditSession(_tfClientId, pEditSession, TF_ES_SYNC | TF_ES_READWRITE, &hr);
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

	if ((_pContextView->GetTextExt(ec, pRangeComposition, &rc, &fClipped)) == S_OK && (rc.left != 0 || rc.top != 0))
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
	_cand->UpdateInputPosition(rc);
}

/* Inline Preedit */
class CInlinePreeditEditSession: public CEditSession
{
public:
	CInlinePreeditEditSession(WeaselTSF *pTextService, ITfContext *pContext, ITfComposition *pComposition, const std::shared_ptr<weasel::Context> context)
		: CEditSession(pTextService, pContext), _pComposition(pComposition), _context(context)
	{
	}

	/* ITfEditSession */
	STDMETHODIMP DoEditSession(TfEditCookie ec);

private:
	ITfComposition *_pComposition;
	const std::shared_ptr<weasel::Context> _context;
};

STDAPI CInlinePreeditEditSession::DoEditSession(TfEditCookie ec)
{
	std::wstring preedit = _context->preedit.str;

	ITfRange *pRangeComposition = NULL;
	if ((_pComposition->GetRange(&pRangeComposition)) != S_OK)
		goto Exit;

	if ((pRangeComposition->SetText(ec, 0, preedit.c_str(), preedit.length())) != S_OK)
		goto Exit;

	int sel_start = 0, sel_end = 0; /* TODO: Check the availability and correctness of these values */
	for (size_t i = 0; i < _context->preedit.attributes.size(); i++)
		if (_context->preedit.attributes.at(i).type == weasel::HIGHLIGHTED)
		{
			sel_start = _context->preedit.attributes.at(i).range.start;
			sel_end = _context->preedit.attributes.at(i).range.end;
			break;
		}

	/* Set caret */
	LONG cch;
	TF_SELECTION tfSelection;
	pRangeComposition->Collapse(ec, TF_ANCHOR_START);
	pRangeComposition->ShiftEnd(ec, sel_end, &cch, NULL);
	pRangeComposition->ShiftStart(ec, sel_start, &cch, NULL);
	tfSelection.range = pRangeComposition;
	tfSelection.style.ase = TF_AE_NONE;
	tfSelection.style.fInterimChar = FALSE;
	_pContext->SetSelection(ec, 1, &tfSelection);

Exit:
	if (pRangeComposition != NULL)
		pRangeComposition->Release();
	return S_OK;
}

BOOL WeaselTSF::_ShowInlinePreedit(ITfContext *pContext, const std::shared_ptr<weasel::Context> context)
{
	CInlinePreeditEditSession *pEditSession;
	if ((pEditSession = new CInlinePreeditEditSession(this, pContext, _pComposition, context)) != NULL)
	{
		HRESULT hr;
		pContext->RequestEditSession(_tfClientId, pEditSession, TF_ES_ASYNCDONTCARE | TF_ES_READWRITE, &hr);
		pEditSession->Release();
	}
	return TRUE;
}

/* Update Composition */
class CInsertTextEditSession : public CEditSession
{
public:
	CInsertTextEditSession(WeaselTSF *pTextService, ITfContext *pContext, ITfComposition *pComposition, const std::wstring &text)
		: CEditSession(pTextService, pContext), _text(text), _pComposition(pComposition)
	{
	}

	/* ITfEditSession */
	STDMETHODIMP DoEditSession(TfEditCookie ec);

private:
	std::wstring _text;
	ITfComposition *_pComposition;
};

STDMETHODIMP CInsertTextEditSession::DoEditSession(TfEditCookie ec)
{
	ITfRange *pRange;
	TF_SELECTION tfSelection;
	HRESULT hRet = S_OK;

	if ((hRet = _pComposition->GetRange(&pRange)) != S_OK)
		goto Exit;

	if ((hRet = pRange->SetText(ec, 0, _text.c_str(), _text.length())) != S_OK)
		goto Exit;

	/* update the selection to an insertion point just past the inserted text. */
	pRange->Collapse(ec, TF_ANCHOR_END);

	tfSelection.range = pRange;
	tfSelection.style.ase = TF_AE_NONE;
	tfSelection.style.fInterimChar = FALSE;

	_pContext->SetSelection(ec, 1, &tfSelection);

Exit:
	if (pRange != NULL)
		pRange->Release();

	return hRet;
}

BOOL WeaselTSF::_InsertText(ITfContext *pContext, const std::wstring& text)
{
	CInsertTextEditSession *pEditSession;
	HRESULT hr;

	if ((pEditSession = new CInsertTextEditSession(this, pContext, _pComposition, text)) != NULL)
	{
		pContext->RequestEditSession(_tfClientId, pEditSession, TF_ES_ASYNCDONTCARE | TF_ES_READWRITE, &hr);
		pEditSession->Release();
	}

	return TRUE;
}

void WeaselTSF::_UpdateComposition(ITfContext *pContext)
{
	HRESULT hr;

	_pEditSessionContext = pContext;
	
	_pEditSessionContext->RequestEditSession(_tfClientId, this, TF_ES_ASYNCDONTCARE | TF_ES_READWRITE, &hr);

}

/* Composition State */
STDAPI WeaselTSF::OnCompositionTerminated(TfEditCookie ecWrite, ITfComposition *pComposition)
{
	// NOTE:
	// This will be called when an edit session ended up with an empty composition string,
	// Even if it is closed normally.
	// Silly M$.

	_AbortComposition();
	return S_OK;
}

void WeaselTSF::_AbortComposition(bool clear)
{
	m_client.ClearComposition();
	if (_IsComposing()) {
		_EndComposition(_pEditSessionContext, clear);
	}
	_cand->Destroy();
}

void WeaselTSF::_FinalizeComposition()
{
	if (_pComposition != NULL)
	{
		_pComposition->Release();
		_pComposition = NULL;
	}
}

void WeaselTSF::_SetComposition(ITfComposition *pComposition)
{
	_pComposition = pComposition;
}

BOOL WeaselTSF::_IsComposing()
{
	return _pComposition != NULL;
}