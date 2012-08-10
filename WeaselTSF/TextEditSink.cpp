#include "stdafx.h"
#include "WeaselTSF.h"

STDAPI WeaselTSF::OnEndEdit(ITfContext *pContext, TfEditCookie ecReadOnly, ITfEditRecord *pEditRecord)
{
	BOOL fSelectionChanged;
	IEnumTfRanges *pEnumTextChanges;
	ITfRange *pRange;

	/* did the selection change? */
	if (pEditRecord->GetSelectionStatus(&fSelectionChanged) == S_OK && fSelectionChanged)
	{
	}

	/* text modification? */
	if (pEditRecord->GetTextAndPropertyUpdates(TF_GTP_INCL_TEXT, NULL, 0, &pEnumTextChanges) == S_OK)
	{
		if (pEnumTextChanges->Next(1, &pRange, NULL) == S_OK)
		{
			pRange->Release();
		}
		pEnumTextChanges->Release();
	}
	return S_OK;
}

STDAPI WeaselTSF::OnLayoutChange(ITfContext *pContext, TfLayoutCode lcode, ITfContextView *pContextView)
{
	if (pContext != _pTextEditSinkContext)
		return S_OK;

	if (lcode == TF_LC_CHANGE)
		_UpdateCompositionWindow(pContext);
	return S_OK;
}

BOOL WeaselTSF::_InitTextEditSink(ITfDocumentMgr *pDocMgr)
{
	ITfSource *pSource;
	BOOL fRet;

	/* clear out any previous sink first */
	if (_dwTextEditSinkCookie != TF_INVALID_COOKIE)
	{
		if (_pTextEditSinkContext->QueryInterface(IID_ITfSource, (void **) &pSource) == S_OK)
		{
			pSource->UnadviseSink(_dwTextEditSinkCookie);
			pSource->UnadviseSink(_dwTextLayoutSinkCookie);
			pSource->Release();
		}
		_pTextEditSinkContext->Release();
		_pTextEditSinkContext = NULL;
		_dwTextEditSinkCookie = TF_INVALID_COOKIE;
	}
	if (pDocMgr == NULL)
		return TRUE;

	if (pDocMgr->GetTop(&_pTextEditSinkContext) != S_OK)
		return FALSE;

	if (_pTextEditSinkContext == NULL)
		return TRUE;

	fRet = FALSE;
	if (_pTextEditSinkContext->QueryInterface(IID_ITfSource, (void **) &pSource) == S_OK)
	{
		if (pSource->AdviseSink(IID_ITfTextEditSink, (ITfTextEditSink *) this, &_dwTextEditSinkCookie) == S_OK)
			fRet = TRUE;
		else
			_dwTextEditSinkCookie = TF_INVALID_COOKIE;
		if (pSource->AdviseSink(IID_ITfTextLayoutSink, (ITfTextLayoutSink *) this, &_dwTextLayoutSinkCookie) == S_OK)
		{
			fRet = TRUE;
		}
		else
			_dwTextLayoutSinkCookie = TF_INVALID_COOKIE;
		pSource->Release();
	}
	if (fRet == FALSE)
	{
		_pTextEditSinkContext->Release();
		_pTextEditSinkContext = NULL;
	}

	return fRet;
}
