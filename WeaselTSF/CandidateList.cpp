#include "stdafx.h"

#include "WeaselTSF.h"
#include "CandidateList.h"

using namespace std;
using namespace weasel;

weasel::CandidateList::CandidateList(WeaselTSF * pTextService)
	: _ui(make_unique<UI>())
{
	//_ui->Create(NULL);
	_tsf = pTextService;
	_tsf->AddRef();
	_cRef = 1;
}

weasel::CandidateList::~CandidateList()
{
	_tsf->Release();
}

STDMETHODIMP weasel::CandidateList::QueryInterface(REFIID riid, void ** ppvObj)
{
	if (ppvObj == nullptr)
	{
		return E_INVALIDARG;
	}

	*ppvObj = nullptr;

	if (IsEqualIID(riid, IID_ITfUIElement) ||
		IsEqualIID(riid, IID_ITfCandidateListUIElement))
	{
		*ppvObj = (ITfCandidateListUIElement*)this;
	}
	else if (IsEqualIID(riid, IID_IUnknown) ||
		IsEqualIID(riid, IID_ITfCandidateListUIElementBehavior))
	{
		*ppvObj = (ITfCandidateListUIElementBehavior*)this;
	}

	if (*ppvObj)
	{
		AddRef();
		return S_OK;
	}

	return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) weasel::CandidateList::AddRef(void)
{
	return ++_cRef;
}

STDMETHODIMP_(ULONG) weasel::CandidateList::Release(void)
{
	LONG cr = --_cRef;

	assert(_cRef >= 0);

	if (_cRef == 0)
	{
		delete this;
	}

	return cr;
}

STDMETHODIMP weasel::CandidateList::GetDescription(BSTR * pbstr)
{
	static auto str = SysAllocString(L"Candidate List");
	if (pbstr)
	{
		*pbstr = str;
	}
	return S_OK;
}

STDMETHODIMP weasel::CandidateList::GetGUID(GUID * pguid)
{
	return E_NOTIMPL;
}

STDMETHODIMP weasel::CandidateList::Show(BOOL showCandidateWindow)
{
	if (showCandidateWindow)
		_ui->Show();
	else
		_ui->Hide();
	return S_OK;
}

STDMETHODIMP weasel::CandidateList::IsShown(BOOL * pIsShow)
{
	*pIsShow = _ui->IsShown();
	return S_OK;
}

STDMETHODIMP weasel::CandidateList::GetUpdatedFlags(DWORD * pdwFlags)
{
	if (!pdwFlags)
		return E_INVALIDARG;

	*pdwFlags = TF_CLUIE_DOCUMENTMGR | TF_CLUIE_COUNT | TF_CLUIE_SELECTION | TF_CLUIE_STRING | TF_CLUIE_CURRENTPAGE;
	return S_OK;
}

STDMETHODIMP weasel::CandidateList::GetDocumentMgr(ITfDocumentMgr ** ppdim)
{
	*ppdim = nullptr;
	if ((_tsf->_pThreadMgr->GetFocus(ppdim) == S_OK) && (*ppdim != nullptr))
	{
		return S_OK;
	}

	return E_FAIL;
}

STDMETHODIMP weasel::CandidateList::GetCount(UINT * pCandidateCount)
{
	*pCandidateCount = _ui->ctx().cinfo.candies.size();
	return S_OK;
}

STDMETHODIMP weasel::CandidateList::GetSelection(UINT * pSelectedCandidateIndex)
{
	*pSelectedCandidateIndex = _ui->ctx().cinfo.highlighted;
	return S_OK;
}

STDMETHODIMP weasel::CandidateList::GetString(UINT uIndex, BSTR * pbstr)
{
	*pbstr = nullptr;
	if (!_ui->ctx().cinfo.empty()) {
		auto &str = _ui->ctx().cinfo.candies[_ui->ctx().cinfo.highlighted].str;
		*pbstr = SysAllocStringLen(str.c_str(), str.size() + 1);
	}

	return S_OK;
}

STDMETHODIMP weasel::CandidateList::GetPageIndex(UINT * pIndex, UINT uSize, UINT * puPageCnt)
{
	return E_NOTIMPL;
}

STDMETHODIMP weasel::CandidateList::SetPageIndex(UINT * pIndex, UINT uPageCnt)
{
	return E_NOTIMPL;
}

STDMETHODIMP weasel::CandidateList::GetCurrentPage(UINT * puPage)
{
	*puPage = _ui->ctx().cinfo.currentPage;
	return S_OK;
}

STDMETHODIMP weasel::CandidateList::SetSelection(UINT nIndex)
{
	return E_NOTIMPL;
}

STDMETHODIMP weasel::CandidateList::Finalize(void)
{
	return E_NOTIMPL;
}

STDMETHODIMP weasel::CandidateList::Abort(void)
{
	return E_NOTIMPL;
}

void weasel::CandidateList::UpdateUI(const Context & ctx, const Status & status)
{
	ITfUIElementMgr *emgr = nullptr;
	BOOL pbShow = true;

	if (_ui->style().inline_preedit) {
		_ui->style().client_caps |= weasel::INLINE_PREEDIT_CAPABLE;
	}
	else {
		_ui->style().client_caps &= ~weasel::INLINE_PREEDIT_CAPABLE;
	}

	_ui->Update(ctx, status);

	if (!FAILED(_tsf->_pThreadMgr->QueryInterface(IID_ITfUIElementMgr, (void **)&emgr))) {
		return;
	}
	if (emgr) {
		emgr->BeginUIElement(this, &pbShow, &uiid);
		if (!pbShow) {
			emgr->UpdateUIElement(uiid);
		}
	}

	if (pbShow && status.composing) {
		_ui->Show();
	}
	else {
		_ui->Hide();
		if (pbShow) {
			emgr->EndUIElement(uiid);
		}
	}
	emgr->Release();
}

void weasel::CandidateList::UpdateInputPosition(RECT const & rc)
{
	_ui->UpdateInputPosition(rc);
}

void weasel::CandidateList::Create()
{
	ITfDocumentMgr *dmgr = nullptr;
	ITfContext *ctx = nullptr;
	ITfContextView *view = nullptr;

	if (FAILED(_tsf->_pThreadMgr->GetFocus(&dmgr))) {
		goto Exit;
	}
	if (FAILED(dmgr->GetTop(&ctx))) {
		goto Exit;
	}
	if (FAILED(ctx->GetActiveView(&view))) {
		goto Exit;
	}

	HWND w = NULL;

	view->GetWnd(&w);
	_ui->Create(w);

Exit:
	if (dmgr)
		dmgr->Release();
	if (ctx)
		ctx->Release();
	if (view)
		view->Release();
}
