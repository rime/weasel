#pragma once

#include "stdafx.h"
#include "WeaselTSF.h"

class CEditSession : public ITfEditSession {
 public:
  CEditSession(com_ptr<WeaselTSF> pTextService, com_ptr<ITfContext> pContext)
      : _pTextService(pTextService), _pContext(pContext) {
    _cRef = 1;
  }

  virtual ~CEditSession() {}

  /* IUnknown */
  STDMETHODIMP QueryInterface(REFIID riid, void** ppvObject) {
    if (ppvObject == NULL)
      return E_INVALIDARG;

    *ppvObject = NULL;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfEditSession))
      *ppvObject = (ITfEditSession*)this;

    if (*ppvObject) {
      AddRef();
      return S_OK;
    }
    return E_NOINTERFACE;
  }

  STDMETHODIMP_(ULONG) AddRef() { return ++_cRef; }

  STDMETHODIMP_(ULONG) Release() {
    LONG cr = --_cRef;
    assert(_cRef >= 0);
    if (_cRef == 0)
      delete this;
    return cr;
  }

  /* ITfEditSession */
  virtual STDMETHODIMP DoEditSession(TfEditCookie ec) = 0;

 protected:
  com_ptr<WeaselTSF> _pTextService;
  com_ptr<ITfContext> _pContext;

 private:
  LONG _cRef; /* COM reference count */
};