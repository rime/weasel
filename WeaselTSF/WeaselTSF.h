#pragma once

#include <WeaselCommon.h>
#include "Globals.h"
#include "WeaselIPC.h"

#include <ComPtr.h>

namespace weasel {
	class CandidateList;
}
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
	void _StartComposition(ITfContext *pContext, BOOL fCUASWorkaroundEnabled);
	void _EndComposition(ITfContext *pContext, BOOL clear);
	BOOL _ShowInlinePreedit(ITfContext *pContext, const std::shared_ptr<weasel::Context> context);
	void _UpdateComposition(ITfContext *pContext);
	BOOL _IsComposing();
	void _SetComposition(ITfComposition *pComposition);
	void _SetCompositionPosition(const RECT &rc);
	BOOL _UpdateCompositionWindow(ITfContext *pContext);
	void _FinalizeComposition();

	/* Language bar */
	HWND _GetFocusedContextWindow();
	void _HandleLangBarMenuSelect(UINT wID);

	/* IPC */
	void _EnsureServerConnected();

	/* UI */
	void _UpdateUI(const weasel::Context & ctx, const weasel::Status & status);

private:
	friend class weasel::CandidateList;

	/* TSF Related */
	BOOL _InitThreadMgrEventSink();
	void _UninitThreadMgrEventSink();

	BOOL _InitTextEditSink(ITfDocumentMgr *pDocMgr);

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

	BOOL _InsertText(ITfContext *pContext, const std::wstring& ext);
	void _AbortComposition(bool clear = true);

	void _DeleteCandidateList();

	BOOL _InitCompartment();
	void _UninitCompartment();
	HRESULT _HandleCompartment(REFGUID guidCompartment);

	bool isImmersive() const {
		return (_activateFlags & TF_TMF_IMMERSIVEMODE) != 0;
	}

	ITfThreadMgr *_pThreadMgr;
	TfClientId _tfClientId;
	DWORD _dwThreadMgrEventSinkCookie;

	ITfContext *_pTextEditSinkContext;
	DWORD _dwTextEditSinkCookie, _dwTextLayoutSinkCookie;
	BYTE _lpbKeyState[256];
	BOOL _fTestKeyDownPending, _fTestKeyUpPending;

	ITfContext *_pEditSessionContext;
	std::wstring _editSessionText;

	CCompartmentEventSink *_pKeyboardCompartmentSink;

	ITfComposition *_pComposition;

	CLangBarItemButton *_pLangBarButton;

	std::unique_ptr<weasel::CandidateList> _cand;

	LONG _cRef;	// COM ref count

	/* CUAS Candidate Window Position Workaround */
	BOOL _fCUASWorkaroundTested, _fCUASWorkaroundEnabled;

	/* Weasel Related */
	weasel::Client m_client;
	DWORD _activateFlags;

	/* IME status */
	weasel::Status _status;
};
