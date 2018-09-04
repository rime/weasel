#pragma once

#include <WeaselCommon.h>
#include "Globals.h"
#include "WeaselIPC.h"

class CCandidateList;
class CLangBarItemButton;
class CCompartmentEventSink;

class WeaselTSF:
	public ITfTextInputProcessorEx,
	public ITfThreadMgrEventSink,
	public ITfTextEditSink,
	public ITfTextLayoutSink,
	public ITfKeyEventSink,
	public ITfCompositionSink,
	public ITfThreadFocusSink,
	public ITfActiveLanguageProfileNotifySink,
	public ITfEditSession
{
public:
	WeaselTSF();
	~WeaselTSF();

	/* IUnknown */
	STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	/* ITfTextInputProcessor */
	STDMETHODIMP Activate(ITfThreadMgr *pThreadMgr, TfClientId tfClientId);
	STDMETHODIMP Deactivate();

	/* ITfTextInputProcessorEx */
	STDMETHODIMP ActivateEx(ITfThreadMgr *pThreadMgr, TfClientId tfClientId, DWORD dwFlags);

	/* ITfThreadMgrEventSink */
	STDMETHODIMP OnInitDocumentMgr(ITfDocumentMgr *pDocMgr);
	STDMETHODIMP OnUninitDocumentMgr(ITfDocumentMgr *pDocMgr);
	STDMETHODIMP OnSetFocus(ITfDocumentMgr *pDocMgrFocus, ITfDocumentMgr *pDocMgrPrevFocus);
	STDMETHODIMP OnPushContext(ITfContext *pContext);
	STDMETHODIMP OnPopContext(ITfContext *pContext);

	/* ITfTextEditSink */
	STDMETHODIMP OnEndEdit(ITfContext *pic, TfEditCookie ecReadOnly, ITfEditRecord *pEditRecord);

	/* ITfTextLayoutSink */
	STDMETHODIMP OnLayoutChange(ITfContext *pContext, TfLayoutCode lcode, ITfContextView *pContextView);

	/* ITfKeyEventSink */
	STDMETHODIMP OnSetFocus(BOOL fForeground);
	STDMETHODIMP OnTestKeyDown(ITfContext *pContext, WPARAM wParam, LPARAM lParam, BOOL *pfEaten);
	STDMETHODIMP OnKeyDown(ITfContext *pContext, WPARAM wParam, LPARAM lParam, BOOL *pfEaten);
	STDMETHODIMP OnTestKeyUp(ITfContext *pContext, WPARAM wParam, LPARAM lParam, BOOL *pfEaten);
	STDMETHODIMP OnKeyUp(ITfContext *pContext, WPARAM wParam, LPARAM lParam, BOOL *pfEaten);
	STDMETHODIMP OnPreservedKey(ITfContext *pContext, REFGUID rguid, BOOL *pfEaten);

	// ITfThreadFocusSink
	STDMETHODIMP OnSetThreadFocus();
	STDMETHODIMP OnKillThreadFocus();

	/* ITfCompositionSink */
	STDMETHODIMP OnCompositionTerminated(TfEditCookie ecWrite, ITfComposition *pComposition);

	/* ITfEditSession */
	STDMETHODIMP DoEditSession(TfEditCookie ec);

	/* ITfActiveLanguageProfileNotifySink */
	STDMETHODIMP OnActivated(REFCLSID clsid, REFGUID guidProfile, BOOL isActivated);

	///* ITfCompartmentEventSink */
	//STDMETHODIMP OnChange(_In_ REFGUID guid);
	
	/* Compartments */
    BOOL _IsKeyboardDisabled();
    BOOL _IsKeyboardOpen();
    HRESULT _SetKeyboardOpen(BOOL fOpen);

	/* Composition */
	void _StartComposition(com_ptr<ITfContext> pContext, BOOL fCUASWorkaroundEnabled);
	void _EndComposition(com_ptr<ITfContext> pContext, BOOL clear);
	BOOL _ShowInlinePreedit(com_ptr<ITfContext> pContext, const std::shared_ptr<weasel::Context> context);
	void _UpdateComposition(com_ptr<ITfContext> pContext);
	BOOL _IsComposing();
	void _SetComposition(com_ptr<ITfComposition> pComposition);
	void _SetCompositionPosition(const RECT &rc);
	BOOL _UpdateCompositionWindow(com_ptr<ITfContext> pContext);
	void _FinalizeComposition();

	/* Language bar */
	HWND _GetFocusedContextWindow();
	void _HandleLangBarMenuSelect(UINT wID);

	/* IPC */
	void _EnsureServerConnected();

	/* UI */
	void _UpdateUI(const weasel::Context & ctx, const weasel::Status & status);

private:
	friend class CCandidateList;

	/* TSF Related */
	BOOL _InitThreadMgrEventSink();
	void _UninitThreadMgrEventSink();

	BOOL _InitTextEditSink(com_ptr<ITfDocumentMgr> pDocMgr);

	BOOL _InitKeyEventSink();
	void _UninitKeyEventSink();
	void _ProcessKeyEvent(WPARAM wParam, LPARAM lParam, BOOL *pfEaten);

	BOOL _InitPreservedKey();
	void _UninitPreservedKey();

	BOOL _InitLanguageBar();
	void _UninitLanguageBar();
	void _UpdateLanguageBar(weasel::Status stat);
	void _ShowLanguageBar(BOOL show);
	void _EnableLanguageBar(BOOL enable);

	BOOL _InsertText(com_ptr<ITfContext> pContext, const std::wstring& ext);
	void _AbortComposition(bool clear = true);

	void _DeleteCandidateList();

	BOOL _InitCompartment();
	void _UninitCompartment();
	HRESULT _HandleCompartment(REFGUID guidCompartment);

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

	com_ptr<ITfComposition> _pComposition;

	com_ptr<CLangBarItemButton> _pLangBarButton;

	com_ptr<CCandidateList> _cand;

	LONG _cRef;	// COM ref count

	/* CUAS Candidate Window Position Workaround */
	BOOL _fCUASWorkaroundTested, _fCUASWorkaroundEnabled;

	/* Weasel Related */
	weasel::Client m_client;
	DWORD _activateFlags;

	/* IME status */
	weasel::Status _status;
};
