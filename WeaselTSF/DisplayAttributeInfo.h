#pragma once

#include <msctf.h>

class CDisplayAttributeInfoInput : public ITfDisplayAttributeInfo {
 public:
  CDisplayAttributeInfoInput();
  ~CDisplayAttributeInfoInput();

  // IUnknown
  STDMETHODIMP QueryInterface(REFIID riid, _Outptr_ void** ppvObj);
  STDMETHODIMP_(ULONG) AddRef(void);
  STDMETHODIMP_(ULONG) Release(void);

  // ITfDisplayAttributeInfo
  STDMETHODIMP GetGUID(_Out_ GUID* pguid);
  STDMETHODIMP GetDescription(_Out_ BSTR* pbstrDesc);
  STDMETHODIMP GetAttributeInfo(_Out_ TF_DISPLAYATTRIBUTE* pTSFDisplayAttr);
  STDMETHODIMP SetAttributeInfo(_In_ const TF_DISPLAYATTRIBUTE* ptfDisplayAttr);
  STDMETHODIMP Reset();

 private:
  const GUID* _pguid;
  const TF_DISPLAYATTRIBUTE* _pDisplayAttribute;
  const WCHAR* _pDescription;
  const WCHAR* _pValueName;
  LONG _refCount;  // COM ref count
};
