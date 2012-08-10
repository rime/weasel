#include "stdafx.h"
#include "WeaselTSF.h"

STDAPI WeaselTSF::DoEditSession(TfEditCookie ec)
{
	ITfInsertAtSelection *pInsertAtSelection;
	ITfRange *pRange;
	TF_SELECTION tfSelection;

	if (_pEditSessionContext->QueryInterface(IID_ITfInsertAtSelection, (LPVOID *) &pInsertAtSelection) != S_OK)
		return E_FAIL;

	/* insert the text */
	if (pInsertAtSelection->InsertTextAtSelection(ec, 0, _pEditSessionText, _cEditSessionText, &pRange) != S_OK)
	{
		pInsertAtSelection->Release();
		return E_FAIL;
	}

	/* update the selection to an insertion point just past the inserted text. */
	pRange->Collapse(ec, TF_ANCHOR_END);

	tfSelection.range = pRange;
	tfSelection.style.ase = TF_AE_NONE;
	tfSelection.style.fInterimChar = FALSE;

	_pEditSessionContext->SetSelection(ec, 1, &tfSelection);

	pRange->Release();
	pInsertAtSelection->Release();

	return S_OK;
}

BOOL WeaselTSF::_InsertText(ITfContext *pContext, const WCHAR *pchText, ULONG cchText)
{
	HRESULT hr;

	_pEditSessionContext = pContext;
	_pEditSessionText = pchText;
	_cEditSessionText = cchText;
	
	if (_pEditSessionContext->RequestEditSession(_tfClientId, this, TF_ES_READWRITE | TF_ES_ASYNCDONTCARE, &hr) != S_OK || hr != S_OK)
		return FALSE;

	return TRUE;
}