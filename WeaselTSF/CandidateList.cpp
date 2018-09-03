#include "stdafx.h"

#include "WeaselTSF.h"
#include "CandidateList.h"

using namespace std;
using namespace weasel;

CCandidateList::CCandidateList(com_ptr<WeaselTSF> pTextService)
	: _ui(make_unique<UI>())
	, _tsf(pTextService)
	, _curp(NULL)
{
	_cRef = 1;
}

CCandidateList::~CCandidateList()
{}

STDMETHODIMP CCandidateList::QueryInterface(REFIID riid, void ** ppvObj)
{
	if (ppvObj == nullptr)
	{
		return E_INVALIDARG;
	}

	*ppvObj = nullptr;

	if (IsEqualIID(riid, IID_ITfUIElement) ||
		IsEqualIID(riid, IID_ITfCandidateListUIElement) ||
		IsEqualIID(riid, IID_ITfCandidateListUIElementBehavior))
	{
		*ppvObj = (ITfCandidateListUIElementBehavior*)this;
	}
	else if (IsEqualIID(riid, IID_IUnknown) ||
		IsEqualIID(riid, __uuidof(ITfIntegratableCandidateListUIElement)))
	{
		*ppvObj = (ITfIntegratableCandidateListUIElement*)this;
	}

	if (*ppvObj)
	{
		AddRef();
		return S_OK;
	}

	return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CCandidateList::AddRef(void)
{
	return ++_cRef;
}

STDMETHODIMP_(ULONG) CCandidateList::Release(void)
{
	LONG cr = --_cRef;

	assert(_cRef >= 0);

	if (_cRef == 0)
	{
		delete this;
	}

	return cr;
}

STDMETHODIMP CCandidateList::GetDescription(BSTR * pbstr)
{
	static auto str = SysAllocString(L"Candidate List");
	if (pbstr)
	{
		*pbstr = str;
	}
	return S_OK;
}

STDMETHODIMP CCandidateList::GetGUID(GUID * pguid)
{
	/// 36c3c795-7159-45aa-ab12-30229a51dbd3
	*pguid = { 0x36c3c795, 0x7159, 0x45aa, { 0xab, 0x12, 0x30, 0x22, 0x9a, 0x51, 0xdb, 0xd3 } };
	return S_OK;
}

STDMETHODIMP CCandidateList::Show(BOOL showCandidateWindow)
{
	if (showCandidateWindow)
		_ui->Show();
	else
		_ui->Hide();
	return S_OK;
}

STDMETHODIMP CCandidateList::IsShown(BOOL * pIsShow)
{
	*pIsShow = _ui->IsShown();
	return S_OK;
}

STDMETHODIMP CCandidateList::GetUpdatedFlags(DWORD * pdwFlags)
{
	if (!pdwFlags)
		return E_INVALIDARG;

	*pdwFlags = TF_CLUIE_DOCUMENTMGR | TF_CLUIE_COUNT | TF_CLUIE_SELECTION | TF_CLUIE_STRING | TF_CLUIE_CURRENTPAGE;
	return S_OK;
}

STDMETHODIMP CCandidateList::GetDocumentMgr(ITfDocumentMgr ** ppdim)
{
	*ppdim = nullptr;
	if ((_tsf->_pThreadMgr->GetFocus(ppdim) == S_OK) && (*ppdim != nullptr))
	{
		return S_OK;
	}

	return E_FAIL;
}

STDMETHODIMP CCandidateList::GetCount(UINT * pCandidateCount)
{
	*pCandidateCount = _ui->ctx().cinfo.candies.size();
	return S_OK;
}

STDMETHODIMP CCandidateList::GetSelection(UINT * pSelectedCandidateIndex)
{
	*pSelectedCandidateIndex = _ui->ctx().cinfo.highlighted;
	return S_OK;
}

STDMETHODIMP CCandidateList::GetString(UINT uIndex, BSTR * pbstr)
{
	*pbstr = nullptr;
	auto &cinfo = _ui->ctx().cinfo;
	if (uIndex >= cinfo.candies.size())
		return E_INVALIDARG;

	auto &str = cinfo.candies[uIndex].str;
	*pbstr = SysAllocStringLen(str.c_str(), str.size() + 1);

	return S_OK;
}

STDMETHODIMP CCandidateList::GetPageIndex(UINT * pIndex, UINT uSize, UINT * puPageCnt)
{
	if (!puPageCnt)
		return E_INVALIDARG;
	*puPageCnt = 1;
	if (pIndex) {
		if (uSize < *puPageCnt) {
			return E_INVALIDARG;
		}
		*pIndex = 0;
	}
	return S_OK;
}

STDMETHODIMP CCandidateList::SetPageIndex(UINT * pIndex, UINT uPageCnt)
{
	if (!pIndex)
		return E_INVALIDARG;
	return S_OK;
}

STDMETHODIMP CCandidateList::GetCurrentPage(UINT * puPage)
{
	*puPage = _ui->ctx().cinfo.currentPage;
	return S_OK;
}

STDMETHODIMP CCandidateList::SetSelection(UINT nIndex)
{
	return S_OK;
}

STDMETHODIMP CCandidateList::Finalize(void)
{
	Destroy();
	return S_OK;
}

STDMETHODIMP CCandidateList::Abort(void)
{
	_tsf->_AbortComposition(true);
	Destroy();
	return S_OK;
}

STDMETHODIMP CCandidateList::SetIntegrationStyle(GUID guidIntegrationStyle)
{
	return S_OK;
}

STDMETHODIMP CCandidateList::GetSelectionStyle(TfIntegratableCandidateListSelectionStyle * ptfSelectionStyle)
{
	*ptfSelectionStyle = _selectionStyle;
	return S_OK;
}

STDMETHODIMP CCandidateList::OnKeyDown(WPARAM wParam, LPARAM lParam, BOOL * pIsEaten)
{
	*pIsEaten = TRUE;
	return S_OK;
}

STDMETHODIMP CCandidateList::ShowCandidateNumbers(BOOL * pIsShow)
{
	*pIsShow = TRUE;
	return S_OK;
}

STDMETHODIMP CCandidateList::FinalizeExactCompositionString()
{
	_tsf->_AbortComposition(false);
	return E_NOTIMPL;
}

void CCandidateList::UpdateUI(const Context & ctx, const Status & status)
{
	if (_ui->style().inline_preedit) {
		_ui->style().client_caps |= weasel::INLINE_PREEDIT_CAPABLE;
	}
	else {
		_ui->style().client_caps &= ~weasel::INLINE_PREEDIT_CAPABLE;
	}

	/// In UWP, candidate window will only be shown
	/// if it is owned by active view window
	_UpdateOwner();
	_ui->Update(ctx, status);

	if (status.composing)
		_StartUI();
	else
		_EndUI();
}

void CCandidateList::UpdateStyle(const UIStyle & sty)
{
	_ui->style() = sty;
}

void CCandidateList::UpdateInputPosition(RECT const & rc)
{
	_ui->UpdateInputPosition(rc);
}

void CCandidateList::Destroy()
{
	//_EndUI();
	Show(FALSE);
	_ui->Destroy();
	_curp = NULL;
}

UIStyle & CCandidateList::style()
{
	return _ui->style();
}

void CCandidateList::_UpdateOwner()
{
	HWND actw = _GetActiveWnd();
	if (actw != _curp) {
		UIStyle sty = _ui->style();
		_ui->Destroy();
		_ui->Create(actw);
		_curp = actw;
		_ui->style() = sty;
	}
}

HWND CCandidateList::_GetActiveWnd()
{
	com_ptr<ITfDocumentMgr> dmgr;
	com_ptr<ITfContext> ctx;
	com_ptr<ITfContextView> view;
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
	if (w == NULL) w = ::GetFocus();
	return w;
}

void CCandidateList::_StartUI()
{
	BOOL pbShow = TRUE;
	com_ptr<ITfThreadMgr> pThreadMgr;
	pThreadMgr = _tsf->_pThreadMgr;
	com_ptr<ITfUIElementMgr> emgr;
	auto hr = pThreadMgr->QueryInterface(&emgr);
	if (FAILED(hr))
		return;

	if (emgr != NULL) {
		if (!_ui->IsShown())
			emgr->BeginUIElement(this, &pbShow, &uiid);
		emgr->UpdateUIElement(uiid);
	}

	Show(pbShow);
}

void CCandidateList::_EndUI()
{
	com_ptr<ITfThreadMgr> pThreadMgr;
	pThreadMgr = _tsf->_pThreadMgr;
	com_ptr<ITfUIElementMgr> emgr;
	auto hr = pThreadMgr->QueryInterface(&emgr);
	if (FAILED(hr))
		return;
	if (emgr != NULL)
		emgr->EndUIElement(uiid);
	if (_ui->IsShown())
		Show(false);
}

void WeaselTSF::_UpdateUI(const Context & ctx, const Status & status)
{
	_cand->UpdateUI(ctx, status);
}

void WeaselTSF::_DeleteCandidateList()
{
	_cand->Destroy();
}
