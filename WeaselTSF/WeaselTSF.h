#pragma once

#include "Globals.h"
#include "WeaselIPC.h"

class WeaselTSF:
	public ITfTextInputProcessor,
	public ITfThreadMgrEventSink,
	public ITfTextEditSink,
	public ITfTextLayoutSink,
	public ITfKeyEventSink,
	public ITfCompositionSink,
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
	STDMETHODIMP OnKeyUp(ITfContext *pContext, WPARAM wParm, LPARAM lParam, BOOL *pfEaten);
	STDMETHODIMP OnPreservedKey(ITfContext *pContext, REFGUID rguid, BOOL *pfEaten);

	/* ITfCompositionSink */
	STDMETHODIMP OnCompositionTerminated(TfEditCookie ecWrite, ITfComposition *pComposition);

	/* ITfEditSession */
	STDMETHODIMP DoEditSession(TfEditCookie ec);
	
	/* Compartments */
    BOOL _IsKeyboardDisabled();
    BOOL _IsKeyboardOpen();
    HRESULT _SetKeyboardOpen(BOOL fOpen);

	/* Composition */
	void _StartComposition(ITfContext *pContext, BOOL fCUASWorkaroundEnabled);
	void _EndComposition(ITfContext *pContext);
	BOOL _ShowInlinePreedit(ITfContext *pContext, const weasel::Context &context);
	void _UpdateComposition(ITfContext *pContext);
	BOOL _IsComposing();
	void _SetComposition(ITfComposition *pComposition);
	void _SetCompositionPosition(const RECT &rc);
	BOOL _UpdateCompositionWindow(ITfContext *pContext);

	/* IPC */
	void _EnsureServerConnected();

private:
	/* TSF Related */
	BOOL _InitThreadMgrEventSink();
	void _UninitThreadMgrEventSink();

	BOOL _InitTextEditSink(ITfDocumentMgr *pDocMgr);

	BOOL _InitKeyEventSink();
	void _UninitKeyEventSink();
	void _ProcessKeyEvent(WPARAM wParam, LPARAM lParam, BOOL *pfEaten);

	BOOL _InitPreservedKey();
	void _UninitPreservedKey();
	
	BOOL _InsertText(ITfContext *pContext, const WCHAR *pchText, ULONG cchText);

	ITfThreadMgr *_pThreadMgr;
	TfClientId _tfClientId;
	DWORD _dwThreadMgrEventSinkCookie;

	ITfContext *_pTextEditSinkContext;
	DWORD _dwTextEditSinkCookie, _dwTextLayoutSinkCookie;
	BYTE _lpbKeyState[256];
	BOOL _fTestKeyDownPending, _fTestKeyUpPending;

	ITfContext *_pEditSessionContext;
	const WCHAR *_pEditSessionText;
	ULONG _cEditSessionText;

	ITfComposition *_pComposition;

	LONG _cRef;	// COM ref count

	/* CUAS Candidate Window Position Workaround */
	BOOL _fCUASWorkaroundTested, _fCUASWorkaroundEnabled;

	/* Weasel Related */
	weasel::Client m_client;
};
