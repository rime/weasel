#include "stdafx.h"

#include "DisplayAttributeInfo.h"
#include "Globals.h"

const WCHAR _daiInputName[] = L"DisplayAttributeInput";
const WCHAR _daiDescription[] = L"Weasel Display Attribute Input";

// change style only, leave color to app
const TF_DISPLAYATTRIBUTE _daiDisplayAttribute = {
    {TF_CT_NONE, 0},  // text color
    {TF_CT_NONE, 0},  // background color (TF_CT_NONE => app default)
    TF_LS_DOT,        // underline style
    FALSE,            // underline boldness
    {TF_CT_NONE, 0},  // underline color
    TF_ATTR_INPUT     // attribute info
};

CDisplayAttributeInfoInput::CDisplayAttributeInfoInput() {
  DllAddRef();
  _refCount = 1;

  _pguid = &c_guidDisplayAttributeInput;
  _pDisplayAttribute = &_daiDisplayAttribute;
  _pDescription = _daiDescription;
  _pValueName = _daiInputName;
}

CDisplayAttributeInfoInput::~CDisplayAttributeInfoInput() {
  DllRelease();
}

STDAPI CDisplayAttributeInfoInput::QueryInterface(REFIID riid,
                                                  _Outptr_ void** ppvObj) {
  if (ppvObj == nullptr)
    return E_INVALIDARG;

  *ppvObj = nullptr;

  if (IsEqualIID(riid, IID_IUnknown) ||
      IsEqualIID(riid, IID_ITfDisplayAttributeInfo)) {
    *ppvObj = (ITfDisplayAttributeInfo*)this;
  }

  if (*ppvObj) {
    AddRef();
    return S_OK;
  }

  return E_NOINTERFACE;
}

ULONG CDisplayAttributeInfoInput::AddRef(void) {
  return ++_refCount;
}

ULONG CDisplayAttributeInfoInput::Release(void) {
  LONG cr = --_refCount;

  assert(_refCount >= 0);

  if (_refCount == 0) {
    delete this;
  }

  return cr;
}

STDAPI CDisplayAttributeInfoInput::GetGUID(_Out_ GUID* pguid) {
  if (pguid == nullptr)
    return E_INVALIDARG;

  if (_pguid == nullptr)
    return E_FAIL;

  *pguid = *_pguid;

  return S_OK;
}

STDAPI CDisplayAttributeInfoInput::GetDescription(_Out_ BSTR* pbstrDesc) {
  BSTR tempDesc;

  if (pbstrDesc == nullptr) {
    return E_INVALIDARG;
  }

  *pbstrDesc = nullptr;

  if ((tempDesc = SysAllocString(_pDescription)) == nullptr) {
    return E_OUTOFMEMORY;
  }

  *pbstrDesc = tempDesc;

  return S_OK;
}

STDAPI CDisplayAttributeInfoInput::GetAttributeInfo(
    _Out_ TF_DISPLAYATTRIBUTE* ptfDisplayAttr) {
  if (ptfDisplayAttr == nullptr) {
    return E_INVALIDARG;
  }

  *ptfDisplayAttr = *_pDisplayAttribute;

  return S_OK;
}

STDAPI CDisplayAttributeInfoInput::SetAttributeInfo(
    _In_ const TF_DISPLAYATTRIBUTE* /*ptfDisplayAttr*/) {
  return E_NOTIMPL;
}

STDAPI CDisplayAttributeInfoInput::Reset() {
  return SetAttributeInfo(_pDisplayAttribute);
}
