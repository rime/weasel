#include "stdafx.h"

#include "EnumDisplayAttributeInfo.h"
#include "Globals.h"
#include "DisplayAttributeInfo.h"

CEnumDisplayAttributeInfo::CEnumDisplayAttributeInfo() {
  DllAddRef();

  _index = 0;
  _refCount = 1;
}

CEnumDisplayAttributeInfo::~CEnumDisplayAttributeInfo() {
  DllRelease();
}

STDAPI CEnumDisplayAttributeInfo::QueryInterface(REFIID riid,
                                                 _Outptr_ void** ppvObj) {
  if (ppvObj == nullptr)
    return E_INVALIDARG;

  *ppvObj = nullptr;

  if (IsEqualIID(riid, IID_IUnknown) ||
      IsEqualIID(riid, IID_IEnumTfDisplayAttributeInfo)) {
    *ppvObj = (IEnumTfDisplayAttributeInfo*)this;
  }

  if (*ppvObj) {
    AddRef();
    return S_OK;
  }

  return E_NOINTERFACE;
}

STDAPI_(ULONG) CEnumDisplayAttributeInfo::AddRef() {
  return ++_refCount;
}

STDAPI_(ULONG) CEnumDisplayAttributeInfo::Release() {
  LONG cr = --_refCount;

  assert(_refCount >= 0);

  if (_refCount == 0) {
    delete this;
  }

  return cr;
}

STDAPI CEnumDisplayAttributeInfo::Clone(
    _Out_ IEnumTfDisplayAttributeInfo** ppEnum) {
  CEnumDisplayAttributeInfo* pClone = nullptr;

  if (ppEnum == nullptr) {
    return E_INVALIDARG;
  }

  *ppEnum = nullptr;

  pClone = new (std::nothrow) CEnumDisplayAttributeInfo();
  if ((pClone) == nullptr) {
    return E_OUTOFMEMORY;
  }

  // the clone should match this object's state
  pClone->_index = _index;

  *ppEnum = pClone;

  return S_OK;
}

STDAPI CEnumDisplayAttributeInfo::Next(ULONG ulCount,
                                       __RPC__out_ecount_part(ulCount,
                                                              *pcFetched)
                                           ITfDisplayAttributeInfo** rgInfo,
                                       __RPC__out ULONG* pcFetched) {
  ULONG fetched;

  fetched = 0;

  if (ulCount == 0) {
    return S_OK;
  }
  if (rgInfo == nullptr) {
    return E_INVALIDARG;
  }
  *rgInfo = nullptr;

  while (fetched < ulCount) {
    ITfDisplayAttributeInfo* pDisplayAttributeInfo = nullptr;

    if (_index == 0) {
      pDisplayAttributeInfo = new (std::nothrow) CDisplayAttributeInfoInput();
      if ((pDisplayAttributeInfo) == nullptr) {
        return E_OUTOFMEMORY;
      }
    } else {
      break;
    }

    *rgInfo = pDisplayAttributeInfo;
    rgInfo++;
    fetched++;
    _index++;
  }

  if (pcFetched != nullptr) {
    *pcFetched = fetched;
  }

  return (fetched == ulCount) ? S_OK : S_FALSE;
}

STDAPI CEnumDisplayAttributeInfo::Reset() {
  _index = 0;
  return S_OK;
}

STDAPI CEnumDisplayAttributeInfo::Skip(ULONG ulCount) {
  if ((ulCount + _index) > 1 || (ulCount + _index) < ulCount) {
    _index = 1;
    return S_FALSE;
  }
  _index += ulCount;
  return S_OK;
}
