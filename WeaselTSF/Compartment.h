#pragma once
#include <functional>
#include <msctf.h>

class CCompartmentEventSink : public ITfCompartmentEventSink {
 public:
  using Callback = std::function<HRESULT(REFGUID guidCompartment)>;
  CCompartmentEventSink(Callback callback)
      : _callback(callback), _refCount(1) {};
  ~CCompartmentEventSink() = default;

  // IUnknown
  STDMETHODIMP QueryInterface(REFIID riid, _Outptr_ void** ppvObj);
  STDMETHODIMP_(ULONG) AddRef(void);
  STDMETHODIMP_(ULONG) Release(void);

  // ITfCompartmentEventSink
  STDMETHODIMP OnChange(_In_ REFGUID guid);

  // function
  HRESULT _Advise(_In_ com_ptr<IUnknown> punk, _In_ REFGUID guidCompartment);
  HRESULT _Unadvise();

 private:
  com_ptr<ITfCompartment> _compartment;
  DWORD _cookie;
  Callback _callback;

  LONG _refCount;
};
