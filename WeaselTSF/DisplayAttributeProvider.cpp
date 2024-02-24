#include "stdafx.h"

#include "WeaselTSF.h"
#include "EnumDisplayAttributeInfo.h"
#include "DisplayAttributeInfo.h"

STDAPI WeaselTSF::EnumDisplayAttributeInfo(
    __RPC__deref_out_opt IEnumTfDisplayAttributeInfo** ppEnum) {
  CEnumDisplayAttributeInfo* pAttributeEnum = nullptr;

  if (ppEnum == nullptr) {
    return E_INVALIDARG;
  }

  *ppEnum = nullptr;

  pAttributeEnum = new (std::nothrow) CEnumDisplayAttributeInfo();
  if (pAttributeEnum == nullptr) {
    return E_OUTOFMEMORY;
  }

  *ppEnum = pAttributeEnum;

  return S_OK;
}

STDAPI WeaselTSF::GetDisplayAttributeInfo(
    __RPC__in REFGUID guidInfo,
    __RPC__deref_out_opt ITfDisplayAttributeInfo** ppInfo) {
  if (ppInfo == nullptr) {
    return E_INVALIDARG;
  }

  *ppInfo = nullptr;

  if (IsEqualGUID(guidInfo, c_guidDisplayAttributeInput)) {
    *ppInfo = new (std::nothrow) CDisplayAttributeInfoInput();
    if ((*ppInfo) == nullptr) {
      return E_OUTOFMEMORY;
    }
  } else {
    return E_INVALIDARG;
  }

  return S_OK;
}
