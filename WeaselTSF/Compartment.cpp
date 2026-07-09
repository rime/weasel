#include "stdafx.h"
#include "WeaselTSF.h"
#include "Compartment.h"
#include <resource.h>
#include <functional>
#include <cstdio>
#include <cstdarg>
#include "ResponseParser.h"
#include "CandidateList.h"
#include "LanguageBar.h"

// Debug logging to file
static bool s_dbgEnabled = false;
static FILE* s_dbgFile = nullptr;
static CRITICAL_SECTION s_dbgLock;
static bool s_dbgLockInit = false;
static char s_dbgPath[MAX_PATH] = {0};

static void _DbgInitPath() {
  if (s_dbgPath[0] != 0) return;
  char tempPath[MAX_PATH] = {0};
  DWORD len = GetTempPathA(MAX_PATH, tempPath);
  if (len == 0 || len >= MAX_PATH) {
    strcpy_s(s_dbgPath, MAX_PATH, "C:\\weasel-compartment-debug.log");
    return;
  }
  strcpy_s(s_dbgPath, MAX_PATH, tempPath);
  strcat_s(s_dbgPath, MAX_PATH, "weasel-compartment-debug.log");
}

void _DbgInit() {
  if (!s_dbgLockInit) {
    InitializeCriticalSection(&s_dbgLock);
    s_dbgLockInit = true;
  }
  if (!s_dbgFile) {
    _DbgInitPath();
    s_dbgFile = fopen(s_dbgPath, "a");
    if (s_dbgFile) {
      s_dbgEnabled = true;
      SYSTEMTIME st;
      GetLocalTime(&st);
      fprintf(s_dbgFile, "\n=== WeaselTSF compartment debug session @ %04d-%02d-%02d %02d:%02d:%02d.%03d pid=%lu ===\n",
              st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
              GetCurrentProcessId());
      fflush(s_dbgFile);
      OutputDebugStringW(L"[WeaselTSF] compartment debug log opened");
    }
  }
}

void _DbgLog(const char* fmt, ...) {
  if (!s_dbgEnabled || !s_dbgFile) return;
  EnterCriticalSection(&s_dbgLock);
  SYSTEMTIME st;
  GetLocalTime(&st);
  fprintf(s_dbgFile, "[%04d-%02d-%02d %02d:%02d:%02d.%03d] ",
          st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
  va_list args;
  va_start(args, fmt);
  vfprintf(s_dbgFile, fmt, args);
  va_end(args);
  fputc('\n', s_dbgFile);
  fflush(s_dbgFile);
  LeaveCriticalSection(&s_dbgLock);
}

STDAPI CCompartmentEventSink::QueryInterface(REFIID riid,
                                             _Outptr_ void** ppvObj) {
  if (ppvObj == nullptr)
    return E_INVALIDARG;

  *ppvObj = nullptr;

  if (IsEqualIID(riid, IID_IUnknown) ||
      IsEqualIID(riid, IID_ITfCompartmentEventSink)) {
    *ppvObj = (CCompartmentEventSink*)this;
  }

  if (*ppvObj) {
    AddRef();
    return S_OK;
  }

  return E_NOINTERFACE;
}

STDAPI_(ULONG) CCompartmentEventSink::AddRef() {
  return ++_refCount;
}

STDAPI_(ULONG) CCompartmentEventSink::Release() {
  LONG cr = --_refCount;

  assert(_refCount >= 0);

  if (_refCount == 0) {
    delete this;
  }

  return cr;
}

STDAPI CCompartmentEventSink::OnChange(_In_ REFGUID guidCompartment) {
  return _callback(guidCompartment);
}

HRESULT CCompartmentEventSink::_Advise(_In_ com_ptr<IUnknown> punk,
                                       _In_ REFGUID guidCompartment) {
  HRESULT hr = S_OK;
  ITfCompartmentMgr* pCompartmentMgr = nullptr;
  ITfSource* pSource = nullptr;

  hr = punk->QueryInterface(IID_ITfCompartmentMgr, (void**)&pCompartmentMgr);
  if (FAILED(hr)) {
    return hr;
  }

  hr = pCompartmentMgr->GetCompartment(guidCompartment, &_compartment);
  if (SUCCEEDED(hr)) {
    hr = _compartment->QueryInterface(IID_ITfSource, (void**)&pSource);
    if (SUCCEEDED(hr)) {
      hr = pSource->AdviseSink(IID_ITfCompartmentEventSink, this, &_cookie);
      pSource->Release();
    }
  }

  pCompartmentMgr->Release();

  return hr;
}
HRESULT CCompartmentEventSink::_Unadvise() {
  HRESULT hr = S_OK;
  ITfSource* pSource = nullptr;

  hr = _compartment->QueryInterface(IID_ITfSource, (void**)&pSource);
  if (SUCCEEDED(hr)) {
    hr = pSource->UnadviseSink(_cookie);
    pSource->Release();
  }

  _compartment = nullptr;
  _cookie = 0;

  return hr;
}

BOOL WeaselTSF::_IsKeyboardDisabled() {
  ITfCompartmentMgr* pCompMgr = NULL;
  ITfDocumentMgr* pDocMgrFocus = NULL;
  ITfContext* pContext = NULL;
  BOOL fDisabled = FALSE;

  if ((_pThreadMgr->GetFocus(&pDocMgrFocus) != S_OK) ||
      (pDocMgrFocus == NULL)) {
    fDisabled = TRUE;
    goto Exit;
  }

  if ((pDocMgrFocus->GetTop(&pContext) != S_OK) || (pContext == NULL)) {
    fDisabled = TRUE;
    goto Exit;
  }

  if (pContext->QueryInterface(IID_ITfCompartmentMgr, (void**)&pCompMgr) ==
      S_OK) {
    ITfCompartment* pCompartmentDisabled;
    ITfCompartment* pCompartmentEmptyContext;

    /* Check GUID_COMPARTMENT_KEYBOARD_DISABLED */
    if (pCompMgr->GetCompartment(GUID_COMPARTMENT_KEYBOARD_DISABLED,
                                 &pCompartmentDisabled) == S_OK) {
      VARIANT var;
      if (pCompartmentDisabled->GetValue(&var) == S_OK) {
        if (var.vt == VT_I4)  // Even VT_EMPTY, GetValue() can succeed
          fDisabled = (BOOL)var.lVal;
      }
      pCompartmentDisabled->Release();
    }

    /* Check GUID_COMPARTMENT_EMPTYCONTEXT */
    if (pCompMgr->GetCompartment(GUID_COMPARTMENT_EMPTYCONTEXT,
                                 &pCompartmentEmptyContext) == S_OK) {
      VARIANT var;
      if (pCompartmentEmptyContext->GetValue(&var) == S_OK) {
        if (var.vt == VT_I4)  // Even VT_EMPTY, GetValue() can succeed
          fDisabled = (BOOL)var.lVal;
      }
      pCompartmentEmptyContext->Release();
    }
    pCompMgr->Release();
  }

Exit:
  if (pContext)
    pContext->Release();
  if (pDocMgrFocus)
    pDocMgrFocus->Release();
  return fDisabled;
}

BOOL WeaselTSF::_IsKeyboardOpen() {
  com_ptr<ITfCompartmentMgr> pCompMgr;
  BOOL fOpen = FALSE;

  if (_pThreadMgr->QueryInterface(&pCompMgr) == S_OK) {
    com_ptr<ITfCompartment> pCompartment;
    if (pCompMgr->GetCompartment(GUID_COMPARTMENT_KEYBOARD_OPENCLOSE,
                                 &pCompartment) == S_OK) {
      VARIANT var;
      if (pCompartment->GetValue(&var) == S_OK) {
        if (var.vt == VT_I4)  // Even VT_EMPTY, GetValue() can succeed
          fOpen = (BOOL)var.lVal;
      }
    }
  }
  return fOpen;
}

HRESULT WeaselTSF::_SetKeyboardOpen(BOOL fOpen) {
  HRESULT hr = E_FAIL;
  com_ptr<ITfCompartmentMgr> pCompMgr;

  if (_pThreadMgr->QueryInterface(&pCompMgr) == S_OK) {
    ITfCompartment* pCompartment;
    if (pCompMgr->GetCompartment(GUID_COMPARTMENT_KEYBOARD_OPENCLOSE,
                                 &pCompartment) == S_OK) {
      VARIANT var;
      var.vt = VT_I4;
      var.lVal = fOpen;
      hr = pCompartment->SetValue(_tfClientId, &var);
    }
  }

  return hr;
}

HRESULT WeaselTSF::_GetCompartmentDWORD(DWORD& value, const GUID guid) {
  HRESULT hr = E_FAIL;
  com_ptr<ITfCompartmentMgr> pComMgr;
  if (_pThreadMgr->QueryInterface(&pComMgr) == S_OK) {
    ITfCompartment* pCompartment;
    if (pComMgr->GetCompartment(guid, &pCompartment) == S_OK) {
      VARIANT var;
      if (pCompartment->GetValue(&var) == S_OK) {
        if (var.vt == VT_I4)
          value = var.lVal;
        else
          hr = S_FALSE;
      }
    }
    pCompartment->Release();
  }
  return hr;
}

HRESULT WeaselTSF::_SetCompartmentDWORD(const DWORD& value, const GUID guid) {
  HRESULT hr = S_OK;
  com_ptr<ITfCompartmentMgr> pComMgr;
  if (_pThreadMgr->QueryInterface(&pComMgr) == S_OK) {
    ITfCompartment* pCompartment;
    if (pComMgr->GetCompartment(guid, &pCompartment) == S_OK) {
      VARIANT var;
      var.vt = VT_I4;
      var.lVal = value;
      hr = pCompartment->SetValue(_tfClientId, &var);
    }
    pCompartment->Release();
  }
  return hr;
}

BOOL WeaselTSF::_InitCompartment() {
  using namespace std::placeholders;

  auto callback = std::bind(&WeaselTSF::_HandleCompartment, this, _1);
  _pKeyboardCompartmentSink = new CCompartmentEventSink(callback);
  if (!_pKeyboardCompartmentSink)
    return FALSE;
  DWORD hr = _pKeyboardCompartmentSink->_Advise(
      (IUnknown*)_pThreadMgr, GUID_COMPARTMENT_KEYBOARD_OPENCLOSE);

  _pConvertionCompartmentSink = new CCompartmentEventSink(callback);
  if (!_pConvertionCompartmentSink)
    return FALSE;
  hr = _pConvertionCompartmentSink->_Advise(
      (IUnknown*)_pThreadMgr, GUID_COMPARTMENT_KEYBOARD_INPUTMODE_CONVERSION);
  return SUCCEEDED(hr);
}

void WeaselTSF::_UninitCompartment() {
  if (_pKeyboardCompartmentSink) {
    _pKeyboardCompartmentSink->_Unadvise();
    _pKeyboardCompartmentSink = NULL;
  }
  if (_pConvertionCompartmentSink) {
    _pConvertionCompartmentSink->_Unadvise();
    _pConvertionCompartmentSink = NULL;
  }
}

HRESULT WeaselTSF::_HandleCompartment(REFGUID guidCompartment) {
  _DbgInit();
  if (IsEqualGUID(guidCompartment, GUID_COMPARTMENT_KEYBOARD_OPENCLOSE)) {
    BOOL isOpenDbg = _IsKeyboardOpen();
    _DbgLog("OPENCLOSE OnChange: isOpen=%d, isToOpenClose=%d, ascii_mode=%d",
            isOpenDbg, _isToOpenClose, _status.ascii_mode);
    if (_isToOpenClose) {
      BOOL isOpen = _IsKeyboardOpen();
      // clear composition when close keyboard
      if (!isOpen && _pEditSessionContext) {
        m_client.ClearComposition();
        _EndComposition(_pEditSessionContext, true);
      }
      _EnableLanguageBar(isOpen);
      _UpdateLanguageBar(_status);
    } else {
      _DbgLog("OPENCLOSE else branch: toggle ascii_mode %d -> %d",
              _status.ascii_mode, !_status.ascii_mode);
      _status.ascii_mode = !_status.ascii_mode;
      _SetKeyboardOpen(true);
      if (_pLangBarButton && _pLangBarButton->IsLangBarDisabled())
        _EnableLanguageBar(true);
      _HandleLangBarMenuSelect(_status.ascii_mode
                                   ? ID_WEASELTRAY_ENABLE_ASCII
                                   : ID_WEASELTRAY_DISABLE_ASCII);
      if (_pEditSessionContext)
        m_client.ClearComposition();
      _UpdateLanguageBar(_status);
      _DbgLog("OPENCLOSE else branch done: ascii_mode=%d", _status.ascii_mode);
    }
  } else if (IsEqualGUID(guidCompartment,
                         GUID_COMPARTMENT_KEYBOARD_INPUTMODE_CONVERSION)) {
    DWORD convMode = 0;
    _GetCompartmentDWORD(convMode,
                         GUID_COMPARTMENT_KEYBOARD_INPUTMODE_CONVERSION);
    bool desiredAsciiMode = !(convMode & TF_CONVERSIONMODE_NATIVE);
    _DbgLog("CONVERSION OnChange: convMode=0x%lX (NATIVE=%d), desiredAscii=%d, statusAscii=%d, updatingLangBar=%d",
            convMode, (convMode & TF_CONVERSIONMODE_NATIVE) ? 1 : 0,
            desiredAsciiMode ? 1 : 0, _status.ascii_mode ? 1 : 0,
            _updatingLanguageBar ? 1 : 0);
    if (_updatingLanguageBar) {
      _DbgLog("CONVERSION: skipped (updatingLanguageBar)");
      return S_OK;
    }
    if (desiredAsciiMode != _status.ascii_mode) {
      _DbgLog("CONVERSION: processing -> switching mode (ascii %d -> %d)",
              _status.ascii_mode, desiredAsciiMode);
      _status.ascii_mode = desiredAsciiMode;
      if (_pLangBarButton && _pLangBarButton->IsLangBarDisabled())
        _EnableLanguageBar(true);
      _HandleLangBarMenuSelect(_status.ascii_mode
                                   ? ID_WEASELTRAY_ENABLE_ASCII
                                   : ID_WEASELTRAY_DISABLE_ASCII);
      if (_pEditSessionContext)
        m_client.ClearComposition();
      _UpdateLanguageBar(_status);
      _DbgLog("CONVERSION done: ascii_mode=%d", _status.ascii_mode);
    } else {
      _DbgLog("CONVERSION: skipped (value matches state)");
    }
  }
  return S_OK;
}
