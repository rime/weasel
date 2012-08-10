#pragma once

#include "Globals.h"
#include "WeaselIPC.h"

class WeaselTSF:
	public ITfTextInputProcessor,
	public ITfThreadMgrEventSink,
	public ITfTextEditSink,
	public ITfKeyEventSink,
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

	/* ITfKeyEventSink */
	STDMETHODIMP OnSetFocus(BOOL fForeground);
	STDMETHODIMP OnTestKeyDown(ITfContext *pContext, WPARAM wParam, LPARAM lParam, BOOL *pfEaten);
	STDMETHODIMP OnKeyDown(ITfContext *pContext, WPARAM wParam, LPARAM lParam, BOOL *pfEaten);
	STDMETHODIMP OnTestKeyUp(ITfContext *pContext, WPARAM wParam, LPARAM lParam, BOOL *pfEaten);
	STDMETHODIMP OnKeyUp(ITfContext *pContext, WPARAM wParm, LPARAM lParam, BOOL *pfEaten);
	STDMETHODIMP OnPreservedKey(ITfContext *pContext, REFGUID rguid, BOOL *pfEaten);

	/* ITfEditSession */
	STDMETHODIMP DoEditSession(TfEditCookie ec);
	
	// Compartment
    BOOL _IsKeyboardDisabled();
    BOOL _IsKeyboardOpen();
    HRESULT _SetKeyboardOpen(BOOL fOpen);

private:
	/* TSF Related */
	BOOL _InitThreadMgrEventSink();
	void _UninitThreadMgrEventSink();

	BOOL _InitTextEditSink(ITfDocumentMgr *pDocMgr);

	BOOL _InitKeyEventSink();
	void _UninitKeyEventSink();

	BOOL _InitPreservedKey();
	void _UninitPreservedKey();
	
	BOOL _InsertText(ITfContext *pContext, const WCHAR *pchText, ULONG cchText);
	BOOL _IsKeyEaten(WPARAM wParam);

	ITfThreadMgr *_pThreadMgr;
	TfClientId _tfClientId;
	DWORD _dwThreadMgrEventSinkCookie;

	ITfContext *_pTextEditSinkContext;
	DWORD _dwTextEditSinkCookie;

	ITfContext *_pEditSessionContext;
	const WCHAR *_pEditSessionText;
	ULONG _cEditSessionText;

	LONG _cRef;	// COM ref count

	/* Weasel Related */
	weasel::Client m_client;
};
