#include "stdafx.h"
#include "WeaselTSF.h"
#include "EditSession.h"
#include "ResponseParser.h"
#include "CandidateList.h"

/* Start Composition */
class CStartCompositionEditSession : public CEditSession {
 public:
  CStartCompositionEditSession(com_ptr<WeaselTSF> pTextService,
                               com_ptr<ITfContext> pContext,
                               BOOL fCUASWorkaroundEnabled)
      : CEditSession(pTextService, pContext) {
    _fCUASWorkaroundEnabled = fCUASWorkaroundEnabled;
  }

  /* ITfEditSession */
  STDMETHODIMP DoEditSession(TfEditCookie ec);

 private:
  BOOL _fCUASWorkaroundEnabled;
};

STDMETHODIMP CStartCompositionEditSession::DoEditSession(TfEditCookie ec) {
  HRESULT hr = E_FAIL;
  com_ptr<ITfInsertAtSelection> pInsertAtSelection;
  com_ptr<ITfRange> pRangeComposition;
  if (_pContext->QueryInterface(IID_ITfInsertAtSelection,
                                (LPVOID*)&pInsertAtSelection) != S_OK)
    return hr;
  if (pInsertAtSelection->InsertTextAtSelection(ec, TF_IAS_QUERYONLY, NULL, 0,
                                                &pRangeComposition) != S_OK)
    return hr;

  com_ptr<ITfContextComposition> pContextComposition;
  com_ptr<ITfComposition> pComposition;
  if (_pContext->QueryInterface(IID_ITfContextComposition,
                                (LPVOID*)&pContextComposition) != S_OK)
    return hr;
  if ((pContextComposition->StartComposition(
           ec, pRangeComposition, _pTextService, &pComposition) == S_OK) &&
      (pComposition != NULL)) {
    _pTextService->_SetComposition(pComposition);

    /* set selection */
    TF_SELECTION tfSelection;
    pRangeComposition->Collapse(ec, TF_ANCHOR_END);
    tfSelection.range = pRangeComposition;
    tfSelection.style.ase = TF_AE_NONE;
    tfSelection.style.fInterimChar = FALSE;
    _pContext->SetSelection(ec, 1, &tfSelection);

    // The old composition's range is still visible while its asynchronous
    // end session is pending. Position only after the new composition has
    // actually been created, not from the response handler's stale range.
    _pTextService->_UpdateCompositionWindow(_pContext);
  }

  return hr;
}

void WeaselTSF::_StartComposition(com_ptr<ITfContext> pContext,
                                  BOOL fCUASWorkaroundEnabled) {
  com_ptr<CStartCompositionEditSession> pStartCompositionEditSession;
  pStartCompositionEditSession.Attach(
      new CStartCompositionEditSession(this, pContext, fCUASWorkaroundEnabled));
  _cand->StartUI();
  if (pStartCompositionEditSession != nullptr) {
    HRESULT hr;
    pContext->RequestEditSession(_tfClientId, pStartCompositionEditSession,
                                 TF_ES_ASYNCDONTCARE | TF_ES_READWRITE, &hr);
  }
}

/* End Composition */
class CEndCompositionEditSession : public CEditSession {
 public:
  CEndCompositionEditSession(com_ptr<WeaselTSF> pTextService,
                             com_ptr<ITfContext> pContext,
                             com_ptr<ITfComposition> pComposition,
                             BOOL clear = TRUE)
      : CEditSession(pTextService, pContext), _clear(clear) {
    _pComposition = pComposition;
  }

  /* ITfEditSession */
  STDMETHODIMP DoEditSession(TfEditCookie ec);

 private:
  com_ptr<ITfComposition> _pComposition;
  BOOL _clear;
};

STDMETHODIMP CEndCompositionEditSession::DoEditSession(TfEditCookie ec) {
  /* Clear the dummy text we set before, if any. */
  if (_pComposition == nullptr)
    return S_OK;
  // Avoid null pointer dereference
  if (!_pTextService || !_pContext)
    return S_OK;

  _pTextService->_ClearCompositionDisplayAttributes(ec, _pContext);

  com_ptr<ITfRange> pCompositionRange;
  if (_clear && _pComposition->GetRange(&pCompositionRange) == S_OK)
    pCompositionRange->SetText(ec, 0, L"", 0);

  // Drop ownership before EndComposition(). Some applications notify
  // OnCompositionTerminated synchronously while the old composition ends.
  // Keeping it as the current composition makes that normal notification
  // look like an external abort and can clear a new Rime composition during
  // auto-commit.
  if (_pTextService && _pTextService->_IsCurrentComposition(_pComposition))
    _pTextService->_FinalizeComposition();
  _pComposition->EndComposition(ec);
  return S_OK;
}

void WeaselTSF::_EndComposition(com_ptr<ITfContext> pContext,
                                BOOL clear,
                                BOOL endUI) {
  CEndCompositionEditSession* pEditSession;
  HRESULT hr;
  com_ptr<ITfComposition> pComposition = _pComposition;

  if (endUI)
    _cand->EndUI();
  if ((pEditSession = new CEndCompositionEditSession(
           this, pContext, pComposition, clear)) != NULL) {
    pContext->RequestEditSession(_tfClientId, pEditSession,
                                 TF_ES_ASYNCDONTCARE | TF_ES_READWRITE, &hr);
    pEditSession->Release();
  }
}

namespace {
using weasel::GetUser32DpiApi;
using weasel::ThreadDpiAwarenessScope;
using weasel::User32DpiApi;

bool IsCurrentThreadPerMonitorDpiAware(const User32DpiApi& api) {
  if (!api.GetThreadDpiAwarenessContext ||
      !api.GetAwarenessFromDpiAwarenessContext) {
    return false;
  }
  const DPI_AWARENESS_CONTEXT ctx = api.GetThreadDpiAwarenessContext();
  return ctx && api.GetAwarenessFromDpiAwarenessContext(ctx) ==
                    DPI_AWARENESS_PER_MONITOR_AWARE;
}

HWND ResolveHostRootWindow(ITfContextView* contextView) {
  HWND hwnd = nullptr;
  if (contextView) {
    contextView->GetWnd(&hwnd);
  }
  if (!hwnd || !::IsWindow(hwnd)) {
    hwnd = ::GetFocus();
  }
  if (!hwnd || !::IsWindow(hwnd)) {
    HWND fg = ::GetForegroundWindow();
    if (fg && ::IsWindow(fg) &&
        ::GetWindowThreadProcessId(fg, nullptr) == ::GetCurrentThreadId()) {
      hwnd = fg;
    }
  }
  if (!hwnd || !::IsWindow(hwnd))
    return nullptr;

  HWND root = ::GetAncestor(hwnd, GA_ROOT);
  if (!root || !::IsWindow(root))
    root = hwnd;
  return ::IsIconic(root) ? nullptr : root;
}

bool DoesWindowMatchCurrentThreadDpiContext(const User32DpiApi& api,
                                            HWND hwnd) {
  if (!api.GetThreadDpiAwarenessContext || !api.GetWindowDpiAwarenessContext ||
      !api.AreDpiAwarenessContextsEqual) {
    return true;
  }
  return !!api.AreDpiAwarenessContextsEqual(
      api.GetThreadDpiAwarenessContext(),
      api.GetWindowDpiAwarenessContext(hwnd));
}

bool IsRectWithinBounds(const RECT& inner, const RECT& outer) {
  return inner.left >= outer.left && inner.top >= outer.top &&
         inner.right <= outer.right && inner.bottom <= outer.bottom;
}

bool IsRectOnSingleMonitor(const RECT& rc) {
  const POINT lt{rc.left, rc.top};
  const POINT rb{rc.right > rc.left ? rc.right - 1 : rc.right,
                 rc.bottom > rc.top ? rc.bottom - 1 : rc.bottom};
  const POINT rt{rb.x, lt.y};
  const POINT lb{lt.x, rb.y};
  const HMONITOR mon = ::MonitorFromPoint(lt, MONITOR_DEFAULTTONULL);
  if (!mon)
    return false;
  return ::MonitorFromPoint(rb, MONITOR_DEFAULTTONULL) == mon &&
         ::MonitorFromPoint(rt, MONITOR_DEFAULTTONULL) == mon &&
         ::MonitorFromPoint(lb, MONITOR_DEFAULTTONULL) == mon;
}

bool TryGetPhysicalWindowRect(const User32DpiApi& api,
                              HWND hwnd,
                              RECT* outRect,
                              bool* outSingleMonitor) {
  if (!api.SetThreadDpiAwarenessContext || !hwnd || !outRect)
    return false;
  ThreadDpiAwarenessScope dpiScope;
  if (!dpiScope.IsActive())
    return false;
  if (!::GetWindowRect(hwnd, outRect))
    return false;
  if (outSingleMonitor)
    *outSingleMonitor = IsRectOnSingleMonitor(*outRect);
  return true;
}

RECT ScaleRectBetweenWindowBounds(const RECT& rc,
                                  const RECT& srcBounds,
                                  const RECT& dstBounds) {
  const int srcW = srcBounds.right - srcBounds.left;
  const int srcH = srcBounds.bottom - srcBounds.top;
  const int dstW = dstBounds.right - dstBounds.left;
  const int dstH = dstBounds.bottom - dstBounds.top;
  RECT out = rc;
  out.left = dstBounds.left + ::MulDiv(rc.left - srcBounds.left, dstW, srcW);
  out.right = dstBounds.left + ::MulDiv(rc.right - srcBounds.left, dstW, srcW);
  out.top = dstBounds.top + ::MulDiv(rc.top - srcBounds.top, dstH, srcH);
  out.bottom = dstBounds.top + ::MulDiv(rc.bottom - srcBounds.top, dstH, srcH);
  return out;
}

bool TryConvertRectByPerMonitorDpiApi(const User32DpiApi& api,
                                      HWND root,
                                      const RECT& logicalRect,
                                      RECT* outRect) {
  if (!api.LogicalToPhysicalPointForPerMonitorDPI || !root || !outRect)
    return false;
  POINT lt{logicalRect.left, logicalRect.top};
  POINT rb{logicalRect.right, logicalRect.bottom};
  if (!api.LogicalToPhysicalPointForPerMonitorDPI(root, &lt) ||
      !api.LogicalToPhysicalPointForPerMonitorDPI(root, &rb)) {
    return false;
  }
  *outRect = RECT{lt.x, lt.y, rb.x, rb.y};
  return true;
}

// 非 Per-Monitor DPI 宿主在副屏会被 DWM 虚拟化缩放：
// 单显示器整窗内使用顶层窗口逻辑矩形到物理矩形的仿射映射；跨显示器骑跨或超出窗口边界时优先走逐点
// LogicalToPhysicalPointForPerMonitorDPI。
RECT ConvertRectToPhysicalScreen(const RECT& logicalRect,
                                 ITfContextView* contextView) {
  const User32DpiApi& api = GetUser32DpiApi();
  if (IsCurrentThreadPerMonitorDpiAware(api))
    return logicalRect;

  HWND root = ResolveHostRootWindow(contextView);
  if (!root)
    return logicalRect;

  RECT rcLogical{};
  RECT rcPhysical{};
  bool hasValidWindowBounds = false;
  bool isSingleMonitorWindow = false;
  if (DoesWindowMatchCurrentThreadDpiContext(api, root) &&
      ::GetWindowRect(root, &rcLogical) &&
      TryGetPhysicalWindowRect(api, root, &rcPhysical,
                               &isSingleMonitorWindow)) {
    const int logW = rcLogical.right - rcLogical.left;
    const int logH = rcLogical.bottom - rcLogical.top;
    const int physW = rcPhysical.right - rcPhysical.left;
    const int physH = rcPhysical.bottom - rcPhysical.top;
    hasValidWindowBounds = (logW > 0 && logH > 0 && physW > 0 && physH > 0);
    if (hasValidWindowBounds && isSingleMonitorWindow &&
        IsRectWithinBounds(logicalRect, rcLogical) &&
        IsRectOnSingleMonitor(rcLogical)) {
      return ScaleRectBetweenWindowBounds(logicalRect, rcLogical, rcPhysical);
    }
  }

  RECT converted{};
  if (TryConvertRectByPerMonitorDpiApi(api, root, logicalRect, &converted))
    return converted;

  if (hasValidWindowBounds)
    return ScaleRectBetweenWindowBounds(logicalRect, rcLogical, rcPhysical);

  return logicalRect;
}
}  // namespace

/* Get Text Extent */
class CGetTextExtentEditSession : public CEditSession {
 public:
  CGetTextExtentEditSession(com_ptr<WeaselTSF> pTextService,
                            com_ptr<ITfContext> pContext,
                            com_ptr<ITfContextView> pContextView,
                            com_ptr<ITfComposition> pComposition,
                            bool enhancedPosition)
      : CEditSession(pTextService, pContext) {
    _pContextView = pContextView;
    _pComposition = pComposition;
    _enhancedPosition = enhancedPosition;
  }

  /* ITfEditSession */
  STDMETHODIMP DoEditSession(TfEditCookie ec);

 private:
  com_ptr<ITfContextView> _pContextView;
  com_ptr<ITfComposition> _pComposition;
  bool _enhancedPosition;
};

STDMETHODIMP CGetTextExtentEditSession::DoEditSession(TfEditCookie ec) {
  com_ptr<ITfInsertAtSelection> pInsertAtSelection;
  com_ptr<ITfRange> pRangeComposition;
  ITfRange* pRange;
  RECT rc;
  BOOL fClipped;
  TF_SELECTION selection;
  ULONG nSelection;

  if (FAILED(_pContext->QueryInterface(IID_ITfInsertAtSelection,
                                       (LPVOID*)&pInsertAtSelection)))
    return E_FAIL;
  if (FAILED(_pContext->GetSelection(ec, TF_DEFAULT_SELECTION, 1, &selection,
                                     &nSelection)))
    return E_FAIL;

  if (_pComposition != nullptr && _pComposition->GetRange(&pRange) == S_OK) {
    pRange->Collapse(ec, TF_ANCHOR_START);
  } else {
    // composition end
    // note: selection.range is always an empty range
    pRange = selection.range;
  }

  if ((_pContextView->GetTextExt(ec, pRange, &rc, &fClipped)) == S_OK &&
      (rc.left != 0 || rc.top != 0)) {
    // get the foreground window pos and check if rc from GetTextExt is out of
    // window
    if (_enhancedPosition) {
      HWND hwnd = GetForegroundWindow();
      RECT rcForegroundWindow{};
      if (hwnd && ::GetWindowRect(hwnd, &rcForegroundWindow)) {
        if (rc.left < rcForegroundWindow.left ||
            rc.left > rcForegroundWindow.right ||
            rc.top < rcForegroundWindow.top ||
            rc.top > rcForegroundWindow.bottom) {
          POINT pt{};
          bool hasCaret = ::GetCaretPos(&pt);
          int offsetx =
              rcForegroundWindow.left - rc.left + (hasCaret ? pt.x : 0);
          int offsety = rcForegroundWindow.top - rc.top + (hasCaret ? pt.y : 0);
          rc.left += offsetx;
          rc.right += offsetx;
          rc.top += offsety;
          rc.bottom += offsety;
        }
      }
    }
    rc = ConvertRectToPhysicalScreen(rc, _pContextView);
    _pTextService->_SetCompositionPosition(rc);
  }
  return S_OK;
}

/* Composition Window Handling */
BOOL WeaselTSF::_UpdateCompositionWindow(com_ptr<ITfContext> pContext) {
  com_ptr<ITfContextView> pContextView;
  if (pContext->GetActiveView(&pContextView) != S_OK)
    return FALSE;
  com_ptr<CGetTextExtentEditSession> pEditSession;
  pEditSession.Attach(
      new CGetTextExtentEditSession(this, pContext, pContextView, _pComposition,
                                    _cand->style().enhanced_position));
  if (pEditSession == NULL) {
    return FALSE;
  }
  HRESULT hr;
  pContext->RequestEditSession(_tfClientId, pEditSession,
                               TF_ES_ASYNCDONTCARE | TF_ES_READ, &hr);
  return SUCCEEDED(hr);
}

void WeaselTSF::_SetCompositionPosition(const RECT& rc) {
  /* Test if rect is valid.
   * If it is invalid during CUAS test, we need to apply CUAS workaround */
  if (!_fCUASWorkaroundTested) {
    _fCUASWorkaroundTested = TRUE;
    if (rc.top == rc.bottom) {
      _fCUASWorkaroundEnabled = TRUE;
      return;
    }
  }
  RECT _rc;
  _rc.left = _rc.right = rc.left;
  _rc.top = _rc.bottom = rc.bottom;
  m_client.UpdateInputPosition(rc);
  _cand->UpdateInputPosition(rc);
  _FlushPendingUI();
}

void WeaselTSF::_FlushPendingUI() {
  if (!_hasPendingUI || !_pendingUIContext)
    return;
  auto context = std::move(_pendingUIContext);
  auto status = _pendingUIStatus;
  _hasPendingUI = false;
  _UpdateUI(*context, status);
}

/* Inline Preedit */
class CInlinePreeditEditSession : public CEditSession {
 public:
  CInlinePreeditEditSession(com_ptr<WeaselTSF> pTextService,
                            com_ptr<ITfContext> pContext,
                            com_ptr<ITfComposition> pComposition,
                            const std::shared_ptr<weasel::Context> context)
      : CEditSession(pTextService, pContext),
        _pComposition(pComposition),
        _context(context) {}

  /* ITfEditSession */
  STDMETHODIMP DoEditSession(TfEditCookie ec);

 private:
  com_ptr<ITfComposition> _pComposition;
  const std::shared_ptr<weasel::Context> _context;
};

STDMETHODIMP CInlinePreeditEditSession::DoEditSession(TfEditCookie ec) {
  std::wstring preedit = _context->preedit.str;

  com_ptr<ITfRange> pRangeComposition;
  if (_pComposition == nullptr)
    return E_FAIL;
  if ((_pComposition->GetRange(&pRangeComposition)) != S_OK)
    return E_FAIL;

  if ((pRangeComposition->SetText(ec, 0, preedit.c_str(),
                                  static_cast<LONG>(preedit.length()))) != S_OK)
    return E_FAIL;

  /* TODO: Check the availability and correctness of these values */
  int sel_cursor = -1;
  for (size_t i = 0; i < _context->preedit.attributes.size(); i++) {
    if (_context->preedit.attributes.at(i).type == weasel::HIGHLIGHTED) {
      sel_cursor = _context->preedit.attributes.at(i).range.cursor;
      break;
    }
  }

  _pTextService->_SetCompositionDisplayAttributes(ec, _pContext,
                                                  pRangeComposition);

  /* Set caret */
  LONG cch;
  TF_SELECTION tfSelection;
  if (sel_cursor < 0) {
    pRangeComposition->Collapse(ec, TF_ANCHOR_END);
  } else {
    pRangeComposition->Collapse(ec, TF_ANCHOR_START);
    pRangeComposition->ShiftStart(ec, sel_cursor, &cch, NULL);
  }
  tfSelection.range = pRangeComposition;
  tfSelection.style.ase = TF_AE_NONE;
  tfSelection.style.fInterimChar = FALSE;
  _pContext->SetSelection(ec, 1, &tfSelection);

  return S_OK;
}

BOOL WeaselTSF::_ShowInlinePreedit(
    com_ptr<ITfContext> pContext,
    const std::shared_ptr<weasel::Context> context) {
  com_ptr<CInlinePreeditEditSession> pEditSession;
  pEditSession.Attach(
      new CInlinePreeditEditSession(this, pContext, _pComposition, context));
  if (pEditSession != NULL) {
    HRESULT hr;
    pContext->RequestEditSession(_tfClientId, pEditSession,
                                 TF_ES_ASYNCDONTCARE | TF_ES_READWRITE, &hr);
  }
  return TRUE;
}

/* Update Composition */
class CInsertTextEditSession : public CEditSession {
 public:
  CInsertTextEditSession(com_ptr<WeaselTSF> pTextService,
                         com_ptr<ITfContext> pContext,
                         com_ptr<ITfComposition> pComposition,
                         const std::wstring& text)
      : CEditSession(pTextService, pContext),
        _text(text),
        _pComposition(pComposition) {}

  /* ITfEditSession */
  STDMETHODIMP DoEditSession(TfEditCookie ec);

 private:
  std::wstring _text;
  com_ptr<ITfComposition> _pComposition;
};

STDMETHODIMP CInsertTextEditSession::DoEditSession(TfEditCookie ec) {
  com_ptr<ITfRange> pRange;
  TF_SELECTION tfSelection;
  HRESULT hRet = S_OK;

  if (_pComposition == nullptr)
    return E_FAIL;
  if (FAILED(_pComposition->GetRange(&pRange)))
    return E_FAIL;

  if (FAILED(pRange->SetText(ec, 0, _text.c_str(),
                             static_cast<LONG>(_text.length()))))
    return E_FAIL;

  /* update the selection to an insertion point just past the inserted text. */
  pRange->Collapse(ec, TF_ANCHOR_END);

  tfSelection.range = pRange;
  tfSelection.style.ase = TF_AE_NONE;
  tfSelection.style.fInterimChar = FALSE;

  _pContext->SetSelection(ec, 1, &tfSelection);

  return hRet;
}

BOOL WeaselTSF::_InsertText(com_ptr<ITfContext> pContext,
                            const std::wstring& text) {
  CInsertTextEditSession* pEditSession;
  HRESULT hr;

  if ((pEditSession = new CInsertTextEditSession(this, pContext, _pComposition,
                                                 text)) != NULL) {
    pContext->RequestEditSession(_tfClientId, pEditSession,
                                 TF_ES_ASYNCDONTCARE | TF_ES_READWRITE, &hr);
    pEditSession->Release();
  }

  return TRUE;
}

void WeaselTSF::_UpdateComposition(com_ptr<ITfContext> pContext) {
  HRESULT hr;

  _pEditSessionContext = pContext;

  _pEditSessionContext->RequestEditSession(
      _tfClientId, this, TF_ES_ASYNCDONTCARE | TF_ES_READWRITE, &hr);
  _async_edit = !!(hr == TF_S_ASYNC);
}

/* Composition State */
STDMETHODIMP WeaselTSF::OnCompositionTerminated(TfEditCookie ecWrite,
                                                ITfComposition* pComposition) {
  // NOTE:
  // This will be called when an edit session ended up with an empty composition
  // string, Even if it is closed normally. Silly M$.

  // EndComposition() may generate this callback for the composition we just
  // closed. Only an active, matching composition is an external termination.
  if (!_IsCurrentComposition(pComposition))
    return S_OK;

  // A host may terminate the empty TSF composition used for a non-inline
  // preedit. Keep Rime's composing state; the next key will create a fresh
  // TSF composition. Only an inactive Rime session should be aborted here.
  if (_status.composing) {
    _FinalizeComposition();
    return S_OK;
  }

  _AbortComposition();
  return S_OK;
}

void WeaselTSF::_AbortComposition(bool clear) {
  m_client.ClearComposition();
  if (_IsComposing()) {
    _EndComposition(_pEditSessionContext, clear);
  } else {
    // The host may terminate the TSF composition before focus-loss cleanup.
    // End the candidate UI even when the composition pointer is already gone.
    _cand->EndUI();
  }
  _committed = TRUE;
  _cand->Destroy();
}

void WeaselTSF::_FinalizeComposition() {
  _pComposition = nullptr;
}

void WeaselTSF::_SetComposition(com_ptr<ITfComposition> pComposition) {
  _pComposition = pComposition;
}

BOOL WeaselTSF::_IsComposing() {
  return _pComposition != NULL;
}

BOOL WeaselTSF::_IsCurrentComposition(ITfComposition* pComposition) {
  return _pComposition != nullptr && _pComposition == pComposition;
}
