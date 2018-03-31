#include "stdafx.h"

#include "WeaselTSF.h"
#include "CandidateList.h"

using namespace std;
using namespace weasel;

CandidateList::CandidateList(WeaselTSF * pTextService)
	: _ui(make_unique<UI>())
{
	//_ui->Create(NULL);
	_cRef = 1;
	_tsf = pTextService;
	_tsf->AddRef();
}

CandidateList::~CandidateList()
{
	_tsf->Release();
}

STDMETHODIMP CandidateList::QueryInterface(REFIID riid, void ** ppvObj)
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

STDMETHODIMP_(ULONG) CandidateList::AddRef(void)
{
	return ++_cRef;
}

STDMETHODIMP_(ULONG) CandidateList::Release(void)
{
	LONG cr = --_cRef;

	assert(_cRef >= 0);

	if (_cRef == 0)
	{
		delete this;
	}

	return cr;
}

STDMETHODIMP CandidateList::GetDescription(BSTR * pbstr)
{
	static auto str = SysAllocString(L"Candidate List");
	if (pbstr)
	{
		*pbstr = str;
	}
	return S_OK;
}

STDMETHODIMP CandidateList::GetGUID(GUID * pguid)
{
	/// 36c3c795-7159-45aa-ab12-30229a51dbd3
	*pguid = { 0x36c3c795, 0x7159, 0x45aa, { 0xab, 0x12, 0x30, 0x22, 0x9a, 0x51, 0xdb, 0xd3 } };
	return S_OK;
}

STDMETHODIMP CandidateList::Show(BOOL showCandidateWindow)
{
	BOOL pbShow = true;
	ITfUIElementMgr *emgr = nullptr;

	if (FAILED(_tsf->_pThreadMgr->QueryInterface(IID_ITfUIElementMgr, (void **)&emgr)) || emgr == nullptr) {
		return E_FAIL;
	}

	emgr->BeginUIElement(this, &pbShow, &uiid);
	if (!pbShow) {
		emgr->UpdateUIElement(uiid);
	}

	if (pbShow && showCandidateWindow)
		_ui->Show();
	else
		_ui->Hide();

	if (!showCandidateWindow) {
		emgr->EndUIElement(uiid);
	}

	emgr->Release();

	return S_OK;
}

STDMETHODIMP CandidateList::IsShown(BOOL * pIsShow)
{
	*pIsShow = _ui->IsShown();
	return S_OK;
}

STDMETHODIMP CandidateList::GetUpdatedFlags(DWORD * pdwFlags)
{
	if (!pdwFlags)
		return E_INVALIDARG;

	*pdwFlags = TF_CLUIE_DOCUMENTMGR | TF_CLUIE_COUNT | TF_CLUIE_SELECTION | TF_CLUIE_STRING | TF_CLUIE_CURRENTPAGE;
	return S_OK;
}

STDMETHODIMP CandidateList::GetDocumentMgr(ITfDocumentMgr ** ppdim)
{
	*ppdim = nullptr;
	if ((_tsf->_pThreadMgr->GetFocus(ppdim) == S_OK) && (*ppdim != nullptr))
	{
		return S_OK;
	}

	return E_FAIL;
}

STDMETHODIMP CandidateList::GetCount(UINT * pCandidateCount)
{
	*pCandidateCount = _ui->ctx().cinfo.candies.size();
	return S_OK;
}

STDMETHODIMP CandidateList::GetSelection(UINT * pSelectedCandidateIndex)
{
	*pSelectedCandidateIndex = _ui->ctx().cinfo.highlighted;
	return S_OK;
}

STDMETHODIMP CandidateList::GetString(UINT uIndex, BSTR * pbstr)
{
	*pbstr = nullptr;
	if (!_ui->ctx().cinfo.empty()) {
		auto &str = _ui->ctx().cinfo.candies[_ui->ctx().cinfo.highlighted].str;
		*pbstr = SysAllocStringLen(str.c_str(), str.size() + 1);
	}

	return S_OK;
}

STDMETHODIMP CandidateList::GetPageIndex(UINT * pIndex, UINT uSize, UINT * puPageCnt)
{
	return E_NOTIMPL;
}

STDMETHODIMP CandidateList::SetPageIndex(UINT * pIndex, UINT uPageCnt)
{
	return E_NOTIMPL;
}

STDMETHODIMP CandidateList::GetCurrentPage(UINT * puPage)
{
	*puPage = _ui->ctx().cinfo.currentPage;
	return S_OK;
}

STDMETHODIMP CandidateList::SetSelection(UINT nIndex)
{
	return E_NOTIMPL;
}

STDMETHODIMP CandidateList::Finalize(void)
{
	Destroy();
	return S_OK;
}

STDMETHODIMP CandidateList::Abort(void)
{
	return E_NOTIMPL;
}

STDMETHODIMP CandidateList::SetIntegrationStyle(GUID guidIntegrationStyle)
{
	return E_NOTIMPL;
}

STDMETHODIMP CandidateList::GetSelectionStyle(TfIntegratableCandidateListSelectionStyle * ptfSelectionStyle)
{
	*ptfSelectionStyle = _selectionStyle;
	return S_OK;
}

STDMETHODIMP CandidateList::OnKeyDown(WPARAM wParam, LPARAM lParam, BOOL * pIsEaten)
{
	*pIsEaten = TRUE;
	return S_OK;
}

STDMETHODIMP CandidateList::ShowCandidateNumbers(BOOL * pIsShow)
{
	*pIsShow = TRUE;
	return S_OK;
}

STDMETHODIMP CandidateList::FinalizeExactCompositionString()
{
	_tsf->_AbortComposition(false);
	return E_NOTIMPL;
}

void CandidateList::UpdateUI(const Context & ctx, const Status & status)
{
	if (_ui->style().inline_preedit) {
		_ui->style().client_caps |= weasel::INLINE_PREEDIT_CAPABLE;
	}
	else {
		_ui->style().client_caps &= ~weasel::INLINE_PREEDIT_CAPABLE;
	}

	/// In UWP, candidate window will only be shown
	/// if it is owned by active view window
	HWND actw = _GetActiveWnd();
	if (actw != _curp) {
		UIStyle sty = _ui->style();
		_ui->Destroy();
		_ui->Create(actw);
		_curp = actw;
		_ui->style() = sty;
	}
	_ui->Update(ctx, status);

	Show(status.composing);
}

void CandidateList::UpdateStyle(const UIStyle & sty)
{
	_ui->style() = sty;
}

void CandidateList::UpdateInputPosition(RECT const & rc)
{
	_ui->UpdateInputPosition(rc);
}

void CandidateList::Destroy()
{
	Show(false);
	_ui->Destroy();
	_curp = NULL;
}

UIStyle & CandidateList::style()
{
	return _ui->style();
}

HWND CandidateList::_GetActiveWnd()
{
	ITfDocumentMgr *dmgr = nullptr;
	ITfContext *ctx = nullptr;
	ITfContextView *view = nullptr;
	HWND w = NULL;

	if (FAILED(_tsf->_pThreadMgr->GetFocus(&dmgr))) {
		goto Exit;
	}
	if (FAILED(dmgr->GetTop(&ctx))) {
		goto Exit;
	}
	if (FAILED(ctx->GetActiveView(&view))) {
		goto Exit;
	}

	view->GetWnd(&w);

Exit:
	if (dmgr)
		dmgr->Release();
	if (ctx)
		ctx->Release();
	if (view)
		view->Release();

	if (w == NULL) w = ::GetFocus();
	return w;
}

void WeaselTSF::_UpdateUI(const Context & ctx, const Status & status)
{
	_cand->UpdateUI(ctx, status);
}
