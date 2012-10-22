#include "stdafx.h"
#include "WeaselIPC.h"
#include "WeaselTSF.h"
#include "KeyEvent.h"

void WeaselTSF::_ProcessKeyEvent(WPARAM wParam, LPARAM lParam, BOOL *pfEaten)
{
	if (!_IsKeyboardOpen())
		return;

	_EnsureServerConnected();
	weasel::KeyEvent ke;
	GetKeyboardState(_lpbKeyState);
	if (!ConvertKeyEvent(wParam, lParam, _lpbKeyState, ke))
	{
		/* Unknown key event */
		*pfEaten = FALSE;
	}
	else
    {
		*pfEaten = (BOOL) m_client.ProcessKeyEvent(ke);
    }
}

STDAPI WeaselTSF::OnSetFocus(BOOL fForeground)
{
	if (fForeground)
		m_client.FocusIn();
	else
		m_client.FocusOut();
	return S_OK;
}

/* Some apps sends strange OnTestKeyDown/OnKeyDown combinations:
 *  Some sends OnKeyDown() only. (QQ2012)
 *  Some sends multiple OnTestKeyDown() for a single key event. (MS WORD 2010 x64)
 *
 * We assume every key event will eventually cause a OnKeyDown() call.
 * We use _fTestKeyDownPending to omit multiple OnTestKeyDown() calls,
 *  and for OnKeyDown() to check if the key has already been sent to the server.
 */

STDAPI WeaselTSF::OnTestKeyDown(ITfContext *pContext, WPARAM wParam, LPARAM lParam, BOOL *pfEaten)
{
	if (_fTestKeyDownPending)
	{
		*pfEaten = TRUE;
		return S_OK;
	}
	_ProcessKeyEvent(wParam, lParam, pfEaten);
	_UpdateComposition(pContext);
	if (*pfEaten)
		_fTestKeyDownPending = TRUE;
	return S_OK;
}

STDAPI WeaselTSF::OnKeyDown(ITfContext *pContext, WPARAM wParam, LPARAM lParam, BOOL *pfEaten)
{
	if (_fTestKeyDownPending)
    {
		_fTestKeyDownPending = FALSE;
		*pfEaten = TRUE;
    }
	else
    {
		_ProcessKeyEvent(wParam, lParam, pfEaten);
	    _UpdateComposition(pContext);
    }
	return S_OK;
} 

STDAPI WeaselTSF::OnTestKeyUp(ITfContext *pContext, WPARAM wParam, LPARAM lParam, BOOL *pfEaten)
{
	if (_fTestKeyUpPending)
	{
		*pfEaten = TRUE;
		return S_OK;
	}
	_ProcessKeyEvent(wParam, lParam, pfEaten);
	_UpdateComposition(pContext);
	if (*pfEaten)
		_fTestKeyUpPending = TRUE;
	return S_OK;
}

STDAPI WeaselTSF::OnKeyUp(ITfContext *pContext, WPARAM wParam, LPARAM lParam, BOOL *pfEaten)
{
	if (_fTestKeyUpPending)
    {
		_fTestKeyUpPending = FALSE;
		*pfEaten = TRUE;
    }
	else
    {
		_ProcessKeyEvent(wParam, lParam, pfEaten);
        _UpdateComposition(pContext);
    }
	return S_OK;
}

STDAPI WeaselTSF::OnPreservedKey(ITfContext *pContext, REFGUID rguid, BOOL *pfEaten)
{
	*pfEaten = FALSE;
	return S_OK;
}

BOOL WeaselTSF::_InitKeyEventSink()
{
	ITfKeystrokeMgr *pKeystrokeMgr;
	HRESULT hr;

	if (_pThreadMgr->QueryInterface(IID_ITfKeystrokeMgr, (void **) &pKeystrokeMgr) != S_OK)
		return FALSE;

	hr = pKeystrokeMgr->AdviseKeyEventSink(_tfClientId, (ITfKeyEventSink *) this, TRUE);
	pKeystrokeMgr->Release();
	
	return (hr == S_OK);
}

void WeaselTSF::_UninitKeyEventSink()
{
	ITfKeystrokeMgr *pKeystrokeMgr;
	
	if (_pThreadMgr->QueryInterface(IID_ITfKeystrokeMgr, (void **) &pKeystrokeMgr) != S_OK)
		return;

	pKeystrokeMgr->UnadviseKeyEventSink(_tfClientId);
	pKeystrokeMgr->Release();
}

BOOL WeaselTSF::_InitPreservedKey()
{
	return TRUE;
}

void WeaselTSF::_UninitPreservedKey()
{
}
