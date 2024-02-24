#include "stdafx.h"

#include "Globals.h"
#include "Register.h"
#include "WeaselTSF.h"
#include <VersionHelpers.hpp>

void DllAddRef() {
  InterlockedIncrement(&g_cRefDll);
}

void DllRelease() {
  InterlockedDecrement(&g_cRefDll);
}

class CClassFactory : public IClassFactory {
 public:
  // IUnknown methods
  STDMETHODIMP QueryInterface(REFIID riid, void** ppvObject);
  STDMETHODIMP_(ULONG) AddRef();
  STDMETHODIMP_(ULONG) Release();

  // IClassFactory methods
  STDMETHODIMP CreateInstance(IUnknown* pUnkOuter,
                              REFIID riid,
                              void** ppvObject);
  STDMETHODIMP LockServer(BOOL fLock);
};

STDAPI CClassFactory::QueryInterface(REFIID riid, void** ppvObject) {
  if (IsEqualIID(riid, IID_IClassFactory) || IsEqualIID(riid, IID_IUnknown)) {
    *ppvObject = this;
    DllAddRef();
    return NOERROR;
  }
  *ppvObject = NULL;
  return E_NOINTERFACE;
}

STDAPI_(ULONG) CClassFactory::AddRef() {
  DllAddRef();
  return g_cRefDll + 1;
}

STDAPI_(ULONG) CClassFactory::Release() {
  DllRelease();
  return g_cRefDll + 1;
}

STDAPI CClassFactory::CreateInstance(IUnknown* pUnkOuter,
                                     REFIID riid,
                                     void** ppvObject) {
  WeaselTSF* pCase;
  HRESULT hr;
  if (ppvObject == NULL)
    return E_INVALIDARG;
  *ppvObject = NULL;
  if (pUnkOuter != NULL)
    return CLASS_E_NOAGGREGATION;
  if ((pCase = new WeaselTSF()) == NULL)
    return E_OUTOFMEMORY;
  hr = pCase->QueryInterface(riid, ppvObject);
  pCase->Release();  // caller still holds ref if hr == S_OK
  return hr;
}

STDAPI CClassFactory::LockServer(BOOL fLock) {
  if (fLock)
    DllAddRef();
  else
    DllRelease();
  return S_OK;
}

static CClassFactory* g_classFactory = NULL;

static void BuildGlobalObjects() {
  g_classFactory = new CClassFactory();
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppvObject) {
  if (g_classFactory == NULL) {
    EnterCriticalSection(&g_cs);
    if (g_classFactory == NULL)
      BuildGlobalObjects();
    LeaveCriticalSection(&g_cs);
  }
  if (IsEqualIID(riid, IID_IClassFactory) || IsEqualIID(riid, IID_IUnknown)) {
    *ppvObject = g_classFactory;
    DllAddRef();
    return NOERROR;
  }
  *ppvObject = NULL;
  return CLASS_E_CLASSNOTAVAILABLE;
}

STDAPI DllCanUnloadNow() {
  if (g_cRefDll >= 0)
    return S_FALSE;
  return S_OK;
}

STDAPI DllRegisterServer() {
  if (!RegisterServer() || !RegisterProfiles() || !RegisterCategories()) {
    DllUnregisterServer();
    return E_FAIL;
  }
  return S_OK;
}

STDAPI DllUnregisterServer() {
  UnregisterProfiles();
  UnregisterCategories();
  UnregisterServer();
  return S_OK;
}
