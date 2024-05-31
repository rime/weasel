#pragma once

#include "Globals.h"
#include <WeaselIPC.h>
#include <WeaselIPCData.h>

class CCandidateList;
class CLangBarItemButton;
class CCompartmentEventSink;

class WeaselTSF : public ITfTextInputProcessorEx,
                  public ITfThreadMgrEventSink,
                  public ITfTextEditSink,
                  public ITfTextLayoutSink,
                  public ITfKeyEventSink,
                  public ITfCompositionSink,
                  public ITfThreadFocusSink,
                  public ITfActiveLanguageProfileNotifySink,
                  public ITfEditSession,
                  public ITfDisplayAttributeProvider {
 public:
  WeaselTSF();
  ~WeaselTSF();

  /* IUnknown */
  STDMETHODIMP QueryInterface(REFIID riid, void** ppvObject);
  STDMETHODIMP_(ULONG) AddRef();
  STDMETHODIMP_(ULONG) Release();

  /* ITfTextInputProcessor */
  STDMETHODIMP Activate(ITfThreadMgr* pThreadMgr, TfClientId tfClientId);
  STDMETHODIMP Deactivate();

  /* ITfTextInputProcessorEx */
  STDMETHODIMP ActivateEx(ITfThreadMgr* pThreadMgr,
                          TfClientId tfClientId,
                          DWORD dwFlags);

  /* ITfThreadMgrEventSink */
  STDMETHODIMP OnInitDocumentMgr(ITfDocumentMgr* pDocMgr);
  STDMETHODIMP OnUninitDocumentMgr(ITfDocumentMgr* pDocMgr);
  STDMETHODIMP OnSetFocus(ITfDocumentMgr* pDocMgrFocus,
                          ITfDocumentMgr* pDocMgrPrevFocus);
  STDMETHODIMP OnPushContext(ITfContext* pContext);
  STDMETHODIMP OnPopContext(ITfContext* pContext);

  /* ITfTextEditSink */
  STDMETHODIMP OnEndEdit(ITfContext* pic,
                         TfEditCookie ecReadOnly,
                         ITfEditRecord* pEditRecord);

  /* ITfTextLayoutSink */
  STDMETHODIMP OnLayoutChange(ITfContext* pContext,
                              TfLayoutCode lcode,
                              ITfContextView* pContextView);

  /* ITfKeyEventSink */
  STDMETHODIMP OnSetFocus(BOOL fForeground);
  STDMETHODIMP OnTestKeyDown(ITfContext* pContext,
                             WPARAM wParam,
                             LPARAM lParam,
                             BOOL* pfEaten);
  STDMETHODIMP OnKeyDown(ITfContext* pContext,
                         WPARAM wParam,
                         LPARAM lParam,
                         BOOL* pfEaten);
  STDMETHODIMP OnTestKeyUp(ITfContext* pContext,
                           WPARAM wParam,
                           LPARAM lParam,
                           BOOL* pfEaten);
  STDMETHODIMP OnKeyUp(ITfContext* pContext,
                       WPARAM wParam,
                       LPARAM lParam,
                       BOOL* pfEaten);
  STDMETHODIMP OnPreservedKey(ITfContext* pContext,
                              REFGUID rguid,
                              BOOL* pfEaten);

  // ITfThreadFocusSink
  STDMETHODIMP OnSetThreadFocus();
  STDMETHODIMP OnKillThreadFocus();

  /* ITfCompositionSink */
  STDMETHODIMP OnCompositionTerminated(TfEditCookie ecWrite,
                                       ITfComposition* pComposition);

  /* ITfEditSession */
  STDMETHODIMP DoEditSession(TfEditCookie ec);

  /* ITfActiveLanguageProfileNotifySink */
  STDMETHODIMP OnActivated(REFCLSID clsid,
                           REFGUID guidProfile,
                           BOOL isActivated);

  // ITfDisplayAttributeProvider
  STDMETHODIMP EnumDisplayAttributeInfo(
      __RPC__deref_out_opt IEnumTfDisplayAttributeInfo** ppEnum);
  STDMETHODIMP GetDisplayAttributeInfo(
      __RPC__in REFGUID guidInfo,
      __RPC__deref_out_opt ITfDisplayAttributeInfo** ppInfo);

  ///* ITfCompartmentEventSink */
  // STDMETHODIMP OnChange(_In_ REFGUID guid);

  /* Compartments */
  BOOL _IsKeyboardDisabled();
  BOOL _IsKeyboardOpen();
  HRESULT _SetKeyboardOpen(BOOL fOpen);
  HRESULT _GetCompartmentDWORD(DWORD& value, const GUID guid);
  HRESULT _SetCompartmentDWORD(const DWORD& value, const GUID guid);

  /* Composition */
  void _StartComposition(com_ptr<ITfContext> pContext,
                         BOOL fCUASWorkaroundEnabled);
  void _EndComposition(com_ptr<ITfContext> pContext, BOOL clear);
  BOOL _ShowInlinePreedit(com_ptr<ITfContext> pContext,
                          const std::shared_ptr<weasel::Context> context);
  void _UpdateComposition(com_ptr<ITfContext> pContext);
  BOOL _IsComposing();
  void _SetComposition(com_ptr<ITfComposition> pComposition);
  void _SetCompositionPosition(const RECT& rc);
  BOOL _UpdateCompositionWindow(com_ptr<ITfContext> pContext);
  void _FinalizeComposition();
  void _AbortComposition(bool clear = true);

  /* Language bar */
  HWND _GetFocusedContextWindow();
  void _HandleLangBarMenuSelect(UINT wID);

  /* IPC */
  bool _EnsureServerConnected();

  /* UI */
  void _UpdateUI(const weasel::Context& ctx, const weasel::Status& status);
  void _StartUI();
  void _EndUI();
  void _ShowUI();
  void _HideUI();
  com_ptr<ITfContext> _GetUIContextDocument();

  /* Display Attribute */
  void _ClearCompositionDisplayAttributes(TfEditCookie ec,
                                          _In_ ITfContext* pContext);
  BOOL _SetCompositionDisplayAttributes(TfEditCookie ec,
                                        _In_ ITfContext* pContext,
                                        ITfRange* pRangeComposition);
  BOOL _InitDisplayAttributeGuidAtom();

  com_ptr<ITfThreadMgr> _GetThreadMgr() { return _pThreadMgr; }
  void HandleUICallback(size_t* const sel,
                        size_t* const hov,
                        bool* const next,
                        bool* const scroll_next);

 private:
  /* ui callback functions private */
  void _SelectCandidateOnCurrentPage(const size_t index);
  void _HandleMouseHoverEvent(const size_t index);
  void _HandleMousePageEvent(bool* const nextPage, bool* const scrollNextPage);
  /* TSF Related */
  BOOL _InitThreadMgrEventSink();
  void _UninitThreadMgrEventSink();
  // ITfThreadFocusSink
  BOOL _InitThreadFocusSink();
  void _UninitThreadFocusSink();
  DWORD _dwThreadFocusSinkCookie;

  BOOL _InitTextEditSink(com_ptr<ITfDocumentMgr> pDocMgr);

  BOOL _InitKeyEventSink();
  void _UninitKeyEventSink();
  void _ProcessKeyEvent(WPARAM wParam, LPARAM lParam, BOOL* pfEaten);

  BOOL _InitPreservedKey();
  void _UninitPreservedKey();

  BOOL _InitLanguageBar();
  void _UninitLanguageBar();
  void _UpdateLanguageBar(weasel::Status stat);
  void _ShowLanguageBar(BOOL show);
  void _EnableLanguageBar(BOOL enable);

  BOOL _InsertText(com_ptr<ITfContext> pContext, const std::wstring& ext);

  void _DeleteCandidateList();

  BOOL _InitCompartment();
  void _UninitCompartment();
  HRESULT _HandleCompartment(REFGUID guidCompartment);

  void _Reconnect();
  std::wstring _GetRootDir();

  bool isImmersive() const {
    return (_activateFlags & TF_TMF_IMMERSIVEMODE) != 0;
  }

  com_ptr<ITfThreadMgr> _pThreadMgr;
  TfClientId _tfClientId;
  DWORD _dwThreadMgrEventSinkCookie;

  com_ptr<ITfContext> _pTextEditSinkContext;
  DWORD _dwTextEditSinkCookie, _dwTextLayoutSinkCookie;
  BYTE _lpbKeyState[256];
  BOOL _fTestKeyDownPending, _fTestKeyUpPending;

  com_ptr<ITfContext> _pEditSessionContext;
  std::wstring _editSessionText;

  com_ptr<CCompartmentEventSink> _pKeyboardCompartmentSink;
  com_ptr<CCompartmentEventSink> _pConvertionCompartmentSink;

  com_ptr<ITfComposition> _pComposition;

  com_ptr<CLangBarItemButton> _pLangBarButton;

  com_ptr<CCandidateList> _cand;

  LONG _cRef;  // COM ref count

  /* CUAS Candidate Window Position Workaround */
  BOOL _fCUASWorkaroundTested, _fCUASWorkaroundEnabled;

  /* Weasel Related */
  weasel::Client m_client;
  DWORD _activateFlags;

  /* IME status */
  weasel::Status _status;

  // guidatom for the display attibute.
  TfGuidAtom _gaDisplayAttributeInput;
  BOOL _async_edit = false;
};
