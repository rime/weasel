#include "stdafx.h"
#include "WeaselIPC.h"
#include "WeaselTSF.h"
#include "KeyEvent.h"
#include "ResponseParser.h"

/* TODO */
static BYTE lpbKeyState[256];

BOOL WeaselTSF::_IsKeyEaten(WPARAM wParam)
{
	if (_IsKeyboardDisabled())
		return FALSE;

	if (!_IsKeyboardOpen())
		return FALSE;
	
	return (wParam >= 'A') && (wParam <= 'Z');
}

STDAPI WeaselTSF::OnSetFocus(BOOL fForeground)
{
	if (fForeground)
		m_client.FocusIn();
	else
		m_client.FocusOut();
	return S_OK;
}

STDAPI WeaselTSF::OnTestKeyDown(ITfContext *pContext, WPARAM wParam, LPARAM lParam, BOOL *pfEaten)
{
	bool accepted = false;

	/*if (!m_client.Echo())
	{
		m_client.Connect(launch_server);
		m_client.StartSession();
	}*/

	weasel::KeyEvent ke;
	/* TODO : Optimize this */
	GetKeyboardState(lpbKeyState);
	if (!ConvertKeyEvent(wParam, lParam, lpbKeyState, ke))
	{
		// unknown key event
		*pfEaten = FALSE;
		return S_OK;
	}

	accepted = m_client.ProcessKeyEvent(ke);

	// get commit string from server
	wstring commit;
	weasel::Status status;
	weasel::ResponseParser parser(&commit, NULL, &status);
	bool ok = m_client.GetResponseData(boost::ref(parser));

	if (ok)
	{
		if (status.composing && !_IsComposing())
		{
			if (!_fCUASWorkaroundTested)
			{
				/* Test if we need to apply the workaround */
				_UpdateCompositionWindow(pContext);
			}
			else if (!_fCUASWorkaroundEnabled)
			{
				/* Workaround not applied, update candidate window position at this point. */
				_UpdateCompositionWindow(pContext);
			}
			_StartComposition(pContext);
		}
		else if (!status.composing && _IsComposing())
			_EndComposition(pContext);
		if (!commit.empty())
			_InsertText(pContext, commit.c_str(), commit.length());
	}
	
	*pfEaten = (BOOL) accepted;

	return S_OK;
}

STDAPI WeaselTSF::OnKeyDown(ITfContext *pContext, WPARAM wParam, LPARAM lParam, BOOL *pfEaten)
{
	*pfEaten = TRUE;
	return S_OK;
} 

STDAPI WeaselTSF::OnTestKeyUp(ITfContext *pContext, WPARAM wParam, LPARAM lParam, BOOL *pfEaten)
{
	*pfEaten = FALSE;
	return S_OK;
}

STDAPI WeaselTSF::OnKeyUp(ITfContext *pContext, WPARAM wParam, LPARAM lParam, BOOL *pfEaten)
{
	*pfEaten = FALSE;
	return S_OK;
}

STDAPI WeaselTSF::OnPreservedKey(ITfContext *pContext, REFGUID rguid, BOOL *pfEaten)
{
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