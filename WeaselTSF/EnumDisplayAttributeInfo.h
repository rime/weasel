#pragma once

class CEnumDisplayAttributeInfo : public IEnumTfDisplayAttributeInfo {
 public:
  CEnumDisplayAttributeInfo();
  ~CEnumDisplayAttributeInfo();

  // IUnknown
  STDMETHODIMP QueryInterface(REFIID riid, _Outptr_ void** ppvObj);
  STDMETHODIMP_(ULONG) AddRef(void);
  STDMETHODIMP_(ULONG) Release(void);

  // IEnumTfDisplayAttributeInfo
  STDMETHODIMP Clone(_Out_ IEnumTfDisplayAttributeInfo** ppEnum);
  STDMETHODIMP Next(ULONG ulCount,
                    __RPC__out_ecount_part(ulCount, *pcFetched)
                        ITfDisplayAttributeInfo** rgInfo,
                    __RPC__out ULONG* pcFetched);
  STDMETHODIMP Reset();
  STDMETHODIMP Skip(ULONG ulCount);

 private:
  LONG _index;     // next display attribute to enum
  LONG _refCount;  // COM ref count
};
