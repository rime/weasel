#include "stdafx.h"
#include "WeaselTSF.h"
#include "EditSession.h"
#include "ResponseParser.h"
#include "CandidateList.h"

/* Start Composition */
class CStartCompositionEditSession: public CEditSession
{
public:
	CStartCompositionEditSession(com_ptr<WeaselTSF> pTextService, com_ptr<ITfContext> pContext, BOOL fCUASWorkaroundEnabled)
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
	com_ptr<ITfInsertAtSelection> pInsertAtSelection;
	com_ptr<ITfRange> pRangeComposition;
	if (_pContext->QueryInterface(IID_ITfInsertAtSelection, (LPVOID *) &pInsertAtSelection) != S_OK)
		return hr;
	if (pInsertAtSelection->InsertTextAtSelection(ec, TF_IAS_QUERYONLY, NULL, 0, &pRangeComposition) != S_OK)
		return hr;
	
	com_ptr<ITfContextComposition> pContextComposition;
	com_ptr<ITfComposition> pComposition;
	if (_pContext->QueryInterface(IID_ITfContextComposition, (LPVOID *) &pContextComposition) != S_OK)
		return hr;
	if ((pContextComposition->StartComposition(ec, pRangeComposition, _pTextService, &pComposition) == S_OK)
		&& (pComposition != NULL))
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
	
	return hr;
}

void WeaselTSF::_StartComposition(com_ptr<ITfContext> pContext, BOOL fCUASWorkaroundEnabled)
{
	com_ptr<CStartCompositionEditSession> pStartCompositionEditSession;
	pStartCompositionEditSession.Attach(new CStartCompositionEditSession(this, pContext, fCUASWorkaroundEnabled));
	_cand->StartUI();
	if (pStartCompositionEditSession != nullptr)
	{
		HRESULT hr;
		pContext->RequestEditSession(_tfClientId, pStartCompositionEditSession, TF_ES_SYNC | TF_ES_READWRITE, &hr);
	}
}

/* End Composition */
class CEndCompositionEditSession: public CEditSession
{
public:
	CEndCompositionEditSession(com_ptr<WeaselTSF> pTextService, com_ptr<ITfContext> pContext, com_ptr<ITfComposition> pComposition, BOOL clear = TRUE)
		: CEditSession(pTextService, pContext), _clear(clear)
	{
		_pComposition = pComposition;
	}

	/* ITfEditSession */
	STDMETHODIMP DoEditSession(TfEditCookie ec);

private:
	com_ptr<ITfComposition> _pComposition;
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

void WeaselTSF::_EndComposition(com_ptr<ITfContext> pContext, BOOL clear)
{
	CEndCompositionEditSession *pEditSession;
	HRESULT hr;
	_cand->EndUI();

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
	CGetTextExtentEditSession(com_ptr<WeaselTSF> pTextService, com_ptr<ITfContext> pContext, com_ptr<ITfContextView> pContextView, com_ptr<ITfComposition> pComposition)
		: CEditSession(pTextService, pContext)
	{
		_pContextView = pContextView;
		_pComposition = pComposition;
	}

	/* ITfEditSession */
	STDMETHODIMP DoEditSession(TfEditCookie ec);

private:
	com_ptr<ITfContextView> _pContextView;
	com_ptr<ITfComposition> _pComposition;
};

STDAPI CGetTextExtentEditSession::DoEditSession(TfEditCookie ec)
{
	com_ptr<ITfInsertAtSelection> pInsertAtSelection;
	com_ptr<ITfRange> pRangeComposition;
	RECT rc;
	BOOL fClipped;
	TF_SELECTION selection;
	ULONG nSelection;
	if (FAILED(_pContext->QueryInterface(IID_ITfInsertAtSelection, (LPVOID *) &pInsertAtSelection)))
		return E_FAIL;
	if (FAILED(_pContext->GetSelection(ec, TF_DEFAULT_SELECTION, 1, &selection, &nSelection)))
		return E_FAIL;
	if ((_pContextView->GetTextExt(ec, selection.range, &rc, &fClipped)) == S_OK && (rc.left != 0 || rc.top != 0))
		_pTextService->_SetCompositionPosition(rc);
	return S_OK;
}

/* Composition Window Handling */
BOOL WeaselTSF::_UpdateCompositionWindow(com_ptr<ITfContext> pContext)
{
	com_ptr<ITfContextView> pContextView;
	if (pContext->GetActiveView(&pContextView) != S_OK)
		return FALSE;
	com_ptr<CGetTextExtentEditSession> pEditSession;
	pEditSession.Attach(new CGetTextExtentEditSession(this, pContext, pContextView, _pComposition));
	if (pEditSession == NULL)
	{
		return FALSE;
	}
	HRESULT hr;
	pContext->RequestEditSession(_tfClientId, pEditSession, TF_ES_SYNC | TF_ES_READ, &hr);
	return SUCCEEDED(hr);
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
	CInlinePreeditEditSession(com_ptr<WeaselTSF> pTextService, com_ptr<ITfContext> pContext, com_ptr<ITfComposition> pComposition, const std::shared_ptr<weasel::Context> context)
		: CEditSession(pTextService, pContext), _pComposition(pComposition), _context(context)
	{
	}

	/* ITfEditSession */
	STDMETHODIMP DoEditSession(TfEditCookie ec);

private:
	com_ptr<ITfComposition> _pComposition;
	const std::shared_ptr<weasel::Context> _context;
};

STDAPI CInlinePreeditEditSession::DoEditSession(TfEditCookie ec)
{
	std::wstring preedit = _context->preedit.str;

	com_ptr<ITfRange> pRangeComposition;
	if ((_pComposition->GetRange(&pRangeComposition)) != S_OK)
		return E_FAIL;

	if ((pRangeComposition->SetText(ec, 0, preedit.c_str(), preedit.length())) != S_OK)
		return E_FAIL;

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

	return S_OK;
}

BOOL WeaselTSF::_ShowInlinePreedit(com_ptr<ITfContext> pContext, const std::shared_ptr<weasel::Context> context)
{
	com_ptr<CInlinePreeditEditSession> pEditSession;
	pEditSession.Attach(new CInlinePreeditEditSession(this, pContext, _pComposition, context));
	if (pEditSession != NULL)
	{
		HRESULT hr;
		pContext->RequestEditSession(_tfClientId, pEditSession, TF_ES_ASYNCDONTCARE | TF_ES_READWRITE, &hr);
	}
	return TRUE;
}

/* Update Composition */
class CInsertTextEditSession : public CEditSession
{
public:
	CInsertTextEditSession(com_ptr<WeaselTSF> pTextService, com_ptr<ITfContext> pContext, com_ptr<ITfComposition> pComposition, const std::wstring &text)
		: CEditSession(pTextService, pContext), _text(text), _pComposition(pComposition)
	{
	}

	/* ITfEditSession */
	STDMETHODIMP DoEditSession(TfEditCookie ec);

private:
	std::wstring _text;
	com_ptr<ITfComposition> _pComposition;
};

STDMETHODIMP CInsertTextEditSession::DoEditSession(TfEditCookie ec)
{
	com_ptr<ITfRange> pRange;
	TF_SELECTION tfSelection;
	HRESULT hRet = S_OK;

	if (FAILED(_pComposition->GetRange(&pRange)))
		return E_FAIL;

	if (FAILED(pRange->SetText(ec, 0, _text.c_str(), _text.length())))
		return E_FAIL;

	/* update the selection to an insertion point just past the inserted text. */
	pRange->Collapse(ec, TF_ANCHOR_END);

	tfSelection.range = pRange;
	tfSelection.style.ase = TF_AE_NONE;
	tfSelection.style.fInterimChar = FALSE;

	_pContext->SetSelection(ec, 1, &tfSelection);

	return hRet;
}

BOOL WeaselTSF::_InsertText(com_ptr<ITfContext> pContext, const std::wstring& text)
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

void WeaselTSF::_UpdateComposition(com_ptr<ITfContext> pContext)
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
	_pComposition = nullptr;
}

void WeaselTSF::_SetComposition(com_ptr<ITfComposition> pComposition)
{
	_pComposition = pComposition;
}

BOOL WeaselTSF::_IsComposing()
{
	return _pComposition != NULL;
}