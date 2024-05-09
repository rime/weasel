#include "stdafx.h"

#include "WeaselTSF.h"
#include "CandidateList.h"
#include "KeyEvent.h"
#include <math.h>

using namespace std;
using namespace weasel;

CCandidateList::CCandidateList(com_ptr<WeaselTSF> pTextService)
    : _ui(make_unique<UI>()), _tsf(pTextService), _pbShow(TRUE) {
  _cRef = 1;
}

CCandidateList::~CCandidateList() {}

STDMETHODIMP CCandidateList::QueryInterface(REFIID riid, void** ppvObj) {
  if (ppvObj == nullptr) {
    return E_INVALIDARG;
  }

  *ppvObj = nullptr;

  if (IsEqualIID(riid, IID_ITfUIElement) ||
      IsEqualIID(riid, IID_ITfCandidateListUIElement) ||
      IsEqualIID(riid, IID_ITfCandidateListUIElementBehavior)) {
    *ppvObj = (ITfCandidateListUIElementBehavior*)this;
  } else if (IsEqualIID(riid, IID_IUnknown) ||
             IsEqualIID(riid,
                        __uuidof(ITfIntegratableCandidateListUIElement))) {
    *ppvObj = (ITfIntegratableCandidateListUIElement*)this;
  }

  if (*ppvObj) {
    AddRef();
    return S_OK;
  }

  return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CCandidateList::AddRef(void) {
  return ++_cRef;
}

STDMETHODIMP_(ULONG) CCandidateList::Release(void) {
  LONG cr = --_cRef;

  assert(_cRef >= 0);

  if (_cRef == 0) {
    delete this;
  }

  return cr;
}

STDMETHODIMP CCandidateList::GetDescription(BSTR* pbstr) {
  static auto str = SysAllocString(L"Candidate List");
  if (pbstr) {
    *pbstr = str;
  }
  return S_OK;
}

STDMETHODIMP CCandidateList::GetGUID(GUID* pguid) {
  /// 36c3c795-7159-45aa-ab12-30229a51dbd3
  *pguid = {0x36c3c795,
            0x7159,
            0x45aa,
            {0xab, 0x12, 0x30, 0x22, 0x9a, 0x51, 0xdb, 0xd3}};
  return S_OK;
}

STDMETHODIMP CCandidateList::Show(BOOL showCandidateWindow) {
  if (showCandidateWindow)
    _ui->Show();
  else
    _ui->Hide();
  return S_OK;
}

STDMETHODIMP CCandidateList::IsShown(BOOL* pIsShow) {
  *pIsShow = _ui->IsShown();
  return S_OK;
}

STDMETHODIMP CCandidateList::GetUpdatedFlags(DWORD* pdwFlags) {
  if (!pdwFlags)
    return E_INVALIDARG;

  *pdwFlags = TF_CLUIE_DOCUMENTMGR | TF_CLUIE_COUNT | TF_CLUIE_SELECTION |
              TF_CLUIE_STRING | TF_CLUIE_CURRENTPAGE;
  return S_OK;
}

STDMETHODIMP CCandidateList::GetDocumentMgr(ITfDocumentMgr** ppdim) {
  *ppdim = nullptr;
  auto pThreadMgr = _tsf->_GetThreadMgr();
  if (pThreadMgr == nullptr) {
    return E_FAIL;
  }
  if (FAILED(pThreadMgr->GetFocus(ppdim)) || (*ppdim == nullptr)) {
    return E_FAIL;
  }
  return S_OK;
}

STDMETHODIMP CCandidateList::GetCount(UINT* pCandidateCount) {
  *pCandidateCount = static_cast<UINT>(_ui->ctx().cinfo.candies.size());
  return S_OK;
}

STDMETHODIMP CCandidateList::GetSelection(UINT* pSelectedCandidateIndex) {
  *pSelectedCandidateIndex = _ui->ctx().cinfo.highlighted;
  return S_OK;
}

STDMETHODIMP CCandidateList::GetString(UINT uIndex, BSTR* pbstr) {
  *pbstr = nullptr;
  auto& cinfo = _ui->ctx().cinfo;
  if (uIndex >= cinfo.candies.size())
    return E_INVALIDARG;

  auto& str = cinfo.candies[uIndex].str;
  *pbstr = SysAllocStringLen(str.c_str(), static_cast<UINT>(str.size()) + 1);

  return S_OK;
}

STDMETHODIMP CCandidateList::GetPageIndex(UINT* pIndex,
                                          UINT uSize,
                                          UINT* puPageCnt) {
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

STDMETHODIMP CCandidateList::SetPageIndex(UINT* pIndex, UINT uPageCnt) {
  if (!pIndex)
    return E_INVALIDARG;
  return S_OK;
}

STDMETHODIMP CCandidateList::GetCurrentPage(UINT* puPage) {
  *puPage = 0;
  return S_OK;
}

STDMETHODIMP CCandidateList::SetSelection(UINT nIndex) {
  _ui->ctx().cinfo.highlighted = nIndex;
  return S_OK;
}

STDMETHODIMP CCandidateList::Finalize(void) {
  Destroy();
  return S_OK;
}

STDMETHODIMP CCandidateList::Abort(void) {
  _tsf->_AbortComposition(true);
  Destroy();
  return S_OK;
}

STDMETHODIMP CCandidateList::SetIntegrationStyle(GUID guidIntegrationStyle) {
  return S_OK;
}

STDMETHODIMP CCandidateList::GetSelectionStyle(
    TfIntegratableCandidateListSelectionStyle* ptfSelectionStyle) {
  *ptfSelectionStyle = _selectionStyle;
  return S_OK;
}

STDMETHODIMP CCandidateList::OnKeyDown(WPARAM wParam,
                                       LPARAM lParam,
                                       BOOL* pIsEaten) {
  *pIsEaten = TRUE;
  return S_OK;
}

STDMETHODIMP CCandidateList::ShowCandidateNumbers(BOOL* pIsShow) {
  *pIsShow = TRUE;
  return S_OK;
}

STDMETHODIMP CCandidateList::FinalizeExactCompositionString() {
  _tsf->_AbortComposition(false);
  return E_NOTIMPL;
}

void CCandidateList::UpdateUI(const Context& ctx, const Status& status) {
  if (_ui->style().inline_preedit) {
    _ui->style().client_caps |= weasel::INLINE_PREEDIT_CAPABLE;
  } else {
    _ui->style().client_caps &= ~weasel::INLINE_PREEDIT_CAPABLE;
  }

  /// In UWP, candidate window will only be shown
  /// if it is owned by active view window
  //_UpdateOwner();
  _ui->Update(ctx, status);
  if (_pbShow == FALSE)
    _UpdateUIElement();

  if (status.composing)
    Show(_pbShow);
  else
    Show(FALSE);
}

void CCandidateList::UpdateStyle(const UIStyle& sty) {
  _ui->style() = sty;
}

void CCandidateList::UpdateInputPosition(RECT const& rc) {
  _ui->UpdateInputPosition(rc);
}

void CCandidateList::Destroy() {
  // EndUI();
  Show(FALSE);
  _DisposeUIWindow();
}

void CCandidateList::DestroyAll() {
  // EndUI();
  Show(FALSE);
  _DisposeUIWindowAll();
}
UIStyle& CCandidateList::style() {
  // return _ui->style();
  return _style;
}

HWND CCandidateList::_GetActiveWnd() {
  com_ptr<ITfDocumentMgr> pDocumentMgr;
  com_ptr<ITfContext> pContext;
  com_ptr<ITfContextView> pContextView;
  com_ptr<ITfThreadMgr> pThreadMgr = _tsf->_GetThreadMgr();

  HWND w = NULL;

  // Reset current context
  _pContextDocument = nullptr;

  if (pThreadMgr != nullptr && SUCCEEDED(pThreadMgr->GetFocus(&pDocumentMgr)) &&
      SUCCEEDED(pDocumentMgr->GetTop(&pContext)) &&
      SUCCEEDED(pContext->GetActiveView(&pContextView))) {
    // Set current context
    _pContextDocument = pContext;
    pContextView->GetWnd(&w);
  }

  if (w == NULL)
    w = ::GetFocus();
  return w;
}

HRESULT CCandidateList::_UpdateUIElement() {
  HRESULT hr = S_OK;

  com_ptr<ITfUIElementMgr> pUIElementMgr;
  com_ptr<ITfThreadMgr> pThreadMgr = _tsf->_GetThreadMgr();
  if (nullptr == pThreadMgr) {
    return S_OK;
  }
  hr = pThreadMgr->QueryInterface(IID_ITfUIElementMgr, (void**)&pUIElementMgr);

  if (hr == S_OK) {
    pUIElementMgr->UpdateUIElement(uiid);
  }

  return S_OK;
}

void CCandidateList::StartUI() {
  com_ptr<ITfThreadMgr> pThreadMgr = _tsf->_GetThreadMgr();
  com_ptr<ITfUIElementMgr> pUIElementMgr;
  auto hr = pThreadMgr->QueryInterface(&pUIElementMgr);
  if (FAILED(hr))
    return;

  if (pUIElementMgr == NULL) {
    return;
  }

  if (!_ui->uiCallback())
    _ui->SetUICallBack([this](size_t* const sel, size_t* const hov,
                              bool* const next, bool* const scroll_next) {
      _tsf->HandleUICallback(sel, hov, next, scroll_next);
    });
  pUIElementMgr->BeginUIElement(this, &_pbShow, &uiid);
  // pUIElementMgr->UpdateUIElement(uiid);
  if (_pbShow) {
    _ui->style() = _style;
    _MakeUIWindow();
  }
}

void CCandidateList::EndUI() {
  com_ptr<ITfThreadMgr> pThreadMgr = _tsf->_GetThreadMgr();
  com_ptr<ITfUIElementMgr> emgr;
  auto hr = pThreadMgr->QueryInterface(&emgr);
  if (FAILED(hr))
    return;
  if (emgr != NULL)
    emgr->EndUIElement(uiid);
  _DisposeUIWindow();
}

com_ptr<ITfContext> CCandidateList::GetContextDocument() {
  return _pContextDocument;
}

void CCandidateList::_DisposeUIWindow() {
  if (_ui == nullptr) {
    return;
  }

  _ui->Destroy();
}

void CCandidateList::_DisposeUIWindowAll() {
  if (_ui == nullptr) {
    return;
  }

  // call _ui->Destroy(true) to clean resources
  _ui->Destroy(true);
}

void CCandidateList::_MakeUIWindow() {
  HWND p = _GetActiveWnd();
  _ui->Create(p);
}

void WeaselTSF::_UpdateUI(const Context& ctx, const Status& status) {
  _cand->UpdateUI(ctx, status);
}

void WeaselTSF::_StartUI() {
  _cand->StartUI();
}

void WeaselTSF::_EndUI() {
  _cand->EndUI();
}

void WeaselTSF::_ShowUI() {
  _cand->Show(TRUE);
}

void WeaselTSF::_HideUI() {
  _cand->Show(FALSE);
}

com_ptr<ITfContext> WeaselTSF::_GetUIContextDocument() {
  return _cand->GetContextDocument();
}

void WeaselTSF::_DeleteCandidateList() {
  _cand->Destroy();
}

void WeaselTSF::_SelectCandidateOnCurrentPage(size_t index) {
  m_client.SelectCandidateOnCurrentPage(index);
  // simulate a VK_SELECT presskey to get data back and DoEditSession
  // the simulated keycode must be the one make TranslateKeycode Non-Zero return
  // fix me: are there any better ways?
  INPUT inputs[2];
  inputs[0].type = INPUT_KEYBOARD;
  inputs[0].ki = {VK_SELECT, 0, 0, 0, 0};
  inputs[1].type = INPUT_KEYBOARD;
  inputs[1].ki = {VK_SELECT, 0, KEYEVENTF_KEYUP, 0, 0};
  ::SendInput(sizeof(inputs) / sizeof(INPUT), inputs, sizeof(INPUT));
}

void WeaselTSF::_HandleMousePageEvent(bool* const nextPage,
                                      bool* const scrollNextPage) {
  // from scrolling event
  if (scrollNextPage) {
    if (_cand->style().paging_on_scroll)
      m_client.ChangePage(!(*scrollNextPage));
    else {
      UINT current_select = 0, cand_count = 0;
      _cand->GetSelection(&current_select);
      _cand->GetCount(&cand_count);
      bool is_reposition = _cand->GetIsReposition();
      int offset = *scrollNextPage ? 1 : -1;
      offset = offset * (is_reposition ? -1 : 1);
      int index = (int)current_select + offset;
      if (index >= 0 && index < (int)cand_count)
        m_client.HighlightCandidateOnCurrentPage((size_t)index);
      else {
        KeyEvent ke{0, 0};
        ke.keycode = (index < 0) ? ibus::Up : ibus::Down;
        m_client.ProcessKeyEvent(ke);
      }
    }
  } else {  // from click event
    m_client.ChangePage(!(*nextPage));
  }
  _UpdateComposition(_pEditSessionContext);
}

void WeaselTSF::_HandleMouseHoverEvent(const size_t index) {
  UINT current_select = 0;
  _cand->GetSelection(&current_select);

  if (index != current_select) {
    m_client.HighlightCandidateOnCurrentPage(index);
    _UpdateComposition(_pEditSessionContext);
  }
}

void WeaselTSF::HandleUICallback(size_t* const sel,
                                 size_t* const hov,
                                 bool* const next,
                                 bool* const scroll_next) {
  if (sel)
    _SelectCandidateOnCurrentPage(*sel);
  else if (hov)
    _HandleMouseHoverEvent(*hov);
  else if (next || scroll_next)
    _HandleMousePageEvent(next, scroll_next);
}
