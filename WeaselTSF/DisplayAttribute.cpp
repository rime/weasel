#include "stdafx.h"

#include "WeaselTSF.h"

void WeaselTSF::_ClearCompositionDisplayAttributes(TfEditCookie ec,
                                                   _In_ ITfContext* pContext) {
  ITfRange* pRangeComposition = nullptr;
  ITfProperty* pDisplayAttributeProperty = nullptr;

  if (FAILED(_pComposition->GetRange(&pRangeComposition))) {
    return;
  }

  if (SUCCEEDED(pContext->GetProperty(GUID_PROP_ATTRIBUTE,
                                      &pDisplayAttributeProperty))) {
    pDisplayAttributeProperty->Clear(ec, pRangeComposition);

    pDisplayAttributeProperty->Release();
  }

  pRangeComposition->Release();
}

BOOL WeaselTSF::_SetCompositionDisplayAttributes(TfEditCookie ec,
                                                 _In_ ITfContext* pContext,
                                                 ITfRange* pRangeComposition) {
  ITfProperty* pDisplayAttributeProperty = nullptr;
  HRESULT hr = S_OK;

  if (pRangeComposition == nullptr)
    hr = _pComposition->GetRange(&pRangeComposition);
  if (FAILED(hr)) {
    return FALSE;
  }

  hr = E_FAIL;

  if (SUCCEEDED(pContext->GetProperty(GUID_PROP_ATTRIBUTE,
                                      &pDisplayAttributeProperty))) {
    VARIANT var;
    var.vt = VT_I4;  // we're going to set a TfGuidAtom
    var.lVal = _gaDisplayAttributeInput;

    hr = pDisplayAttributeProperty->SetValue(ec, pRangeComposition, &var);

    pDisplayAttributeProperty->Release();
  }

  // DO NOT release range composition here
  // it will be released in another function
  // pRangeComposition->Release();
  return (hr == S_OK);
}

BOOL WeaselTSF::_SetRangeLanguage(TfEditCookie ec,
                                  _In_ ITfContext* pContext,
                                  ITfRange* pRange) {
  if (!pRange || !_textLangId)
    return FALSE;

  ITfProperty* pLangIdProperty = nullptr;
  HRESULT hr = E_FAIL;

  if (SUCCEEDED(pContext->GetProperty(GUID_PROP_LANGID, &pLangIdProperty))) {
    VARIANT var;
    var.vt = VT_I4;
    var.lVal = _textLangId;
    hr = pLangIdProperty->SetValue(ec, pRange, &var);
    pLangIdProperty->Release();
  }

  return hr == S_OK;
}

BOOL WeaselTSF::_InitDisplayAttributeGuidAtom() {
  ITfCategoryMgr* pCategoryMgr = nullptr;
  HRESULT hr =
      CoCreateInstance(CLSID_TF_CategoryMgr, nullptr, CLSCTX_INPROC_SERVER,
                       IID_ITfCategoryMgr, (void**)&pCategoryMgr);

  if (FAILED(hr)) {
    return FALSE;
  }

  hr = pCategoryMgr->RegisterGUID(c_guidDisplayAttributeInput,
                                  &_gaDisplayAttributeInput);
  if (FAILED(hr)) {
    goto Exit;
  }

Exit:
  pCategoryMgr->Release();

  return (hr == S_OK);
}
