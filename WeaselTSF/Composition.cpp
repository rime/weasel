#include "stdafx.h"
#include "WeaselTSF.h"
#include "EditSession.h"

class CGetTextExtentEditSession: public CEditSession
{
public:
	CGetTextExtentEditSession(WeaselTSF *pTextService, ITfContext *pContext, ITfContextView *pContextView)
		: CEditSession(pTextService, pContext)
	{
		_pContextView = pContextView;
	}

	/* ITfEditSession */
	STDMETHODIMP DoEditSession(TfEditCookie ec);

private:
	ITfContextView *_pContextView;
};

STDAPI CGetTextExtentEditSession::DoEditSession(TfEditCookie ec)
{
    ITfInsertAtSelection *pInsertAtSelection = NULL;
    ITfRange *pRangeComposition = NULL;
	RECT rc;
	BOOL fClipped;

	if (_pContext->QueryInterface(IID_ITfInsertAtSelection, (LPVOID *) &pInsertAtSelection) != S_OK)
		goto Exit;
	if (pInsertAtSelection->InsertTextAtSelection(ec, TF_IAS_QUERYONLY, NULL, 0, &pRangeComposition) != S_OK)
		goto Exit;

	if (SUCCEEDED(_pContextView->GetTextExt(ec, pRangeComposition, &rc, &fClipped)))
		_pTextService->_SetCompositionPosition(rc.left, rc.bottom);

Exit:
	if (pRangeComposition != NULL)
		pRangeComposition->Release();
	if (pInsertAtSelection != NULL)
		pInsertAtSelection->Release();
	return S_OK;
}

BOOL WeaselTSF::_UpdateCompositionWindow(ITfContext *pContext)
{
	if (!_bCompositing)
		return TRUE;

	ITfContextView *pContextView = NULL;
	if (pContext->GetActiveView(&pContextView) != S_OK)
		return FALSE;
	CGetTextExtentEditSession *pEditSession;
	if ((pEditSession = new CGetTextExtentEditSession(this, pContext, pContextView)) != NULL)
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

void WeaselTSF::_SetCompositionPosition(LONG lLeft, LONG lTop)
{
	RECT rc;
	rc.left = rc.right = lLeft;
	rc.top = rc.bottom = lTop;
	m_client.UpdateInputPosition(rc);
}