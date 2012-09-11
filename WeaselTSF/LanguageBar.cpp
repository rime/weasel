#include "stdafx.h"
#include "resource.h"
#include "WeaselTSF.h"

static const DWORD LANGBARITEMSINK_COOKIE = 0x42424242;

static void HMENU2ITfMenu(HMENU hMenu, ITfMenu *pTfMenu)
{
	/* NOTE: Only limited functions are supported */
	int N = GetMenuItemCount(hMenu);
	for (int i = 0; i < N; i++)
	{
		MENUITEMINFO mii;
		mii.cbSize = sizeof(MENUITEMINFO);
		mii.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STRING;
		mii.dwTypeData = NULL;
		if (GetMenuItemInfo(hMenu, i, TRUE, &mii))
		{
			UINT id = mii.wID;
			if (mii.fType == MFT_SEPARATOR)
				pTfMenu->AddMenuItem(id, TF_LBMENUF_SEPARATOR, NULL, NULL, NULL, 0, NULL);
			else if (mii.fType == MFT_STRING)
			{
				mii.dwTypeData = (LPWSTR) malloc(sizeof(WCHAR) * (mii.cch + 1));
				mii.cch++;
				if (GetMenuItemInfo(hMenu, i, TRUE, &mii))
					pTfMenu->AddMenuItem(id, 0, NULL, NULL, mii.dwTypeData, mii.cch, NULL);
				free(mii.dwTypeData);
			}
		}
	}
}

class CLangBarItemButton: public ITfLangBarItemButton, public ITfSource
{
public:
	CLangBarItemButton(WeaselTSF *pTextService, REFGUID guid);
	~CLangBarItemButton();

	/* IUnknown */
	STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	/* ITfLangBarItem */
	STDMETHODIMP GetInfo(TF_LANGBARITEMINFO *pInfo);
	STDMETHODIMP GetStatus(DWORD *pdwStatus);
	STDMETHODIMP Show(BOOL fShow);
	STDMETHODIMP GetTooltipString(BSTR *pbstrToolTip);

	/* ITfLangBarItemButton */
	STDMETHODIMP OnClick(TfLBIClick click, POINT pt, const RECT *prcArea);
	STDMETHODIMP InitMenu(ITfMenu *pMenu);
	STDMETHODIMP OnMenuSelect(UINT wID);
	STDMETHODIMP GetIcon(HICON *phIcon);
	STDMETHODIMP GetText(BSTR *pbstrText);

	/* ITfSource */
	STDMETHODIMP AdviseSink(REFIID riid, IUnknown *punk, DWORD *pdwCookie);
	STDMETHODIMP UnadviseSink(DWORD dwCookie);

private:
	GUID _guid;
	WeaselTSF *_pTextService;
	ITfLangBarItemSink *_pLangBarItemSink;
	LONG _cRef; /* COM Reference count */
};

CLangBarItemButton::CLangBarItemButton(WeaselTSF *pTextService, REFGUID guid)
{
	DllAddRef();

	_pLangBarItemSink = NULL;
	_cRef = 1;
	_pTextService = pTextService;
	_guid = guid;
}

CLangBarItemButton::~CLangBarItemButton()
{
	DllRelease();
}

STDAPI CLangBarItemButton::QueryInterface(REFIID riid, void **ppvObject)
{
	if (ppvObject == NULL)
		return E_INVALIDARG;

	*ppvObject = NULL;
	if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfLangBarItem) || IsEqualIID(riid, IID_ITfLangBarItemButton))
		*ppvObject = (ITfLangBarItemButton *) this;
	else if (IsEqualIID(riid, IID_ITfSource))
		*ppvObject = (ITfSource *) this;
	
	if (*ppvObject)
	{
		AddRef();
		return S_OK;
	}
	return E_NOINTERFACE;
}

STDAPI_(ULONG) CLangBarItemButton::AddRef()
{
	return ++_cRef;
}

STDAPI_(ULONG) CLangBarItemButton::Release()
{
	LONG cr = --_cRef;
	assert(_cRef >= 0);
	if (_cRef == 0)
		delete this;
	return cr;
}

STDAPI CLangBarItemButton::GetInfo(TF_LANGBARITEMINFO *pInfo)
{
	pInfo->clsidService = c_clsidTextService;
	pInfo->guidItem = _guid;
	pInfo->dwStyle = TF_LBI_STYLE_BTN_BUTTON | TF_LBI_STYLE_BTN_MENU | TF_LBI_STYLE_SHOWNINTRAY;
	pInfo->ulSort = 1;
	lstrcpyW(pInfo->szDescription, L"WeaselTSF Button");
	return S_OK;
}

STDAPI CLangBarItemButton::GetStatus(DWORD *pdwStatus)
{
	*pdwStatus = 0;
	return S_OK;
}

STDAPI CLangBarItemButton::Show(BOOL fShow)
{
	return E_NOTIMPL;
}

STDAPI CLangBarItemButton::GetTooltipString(BSTR *pbstrToolTip)
{
	*pbstrToolTip = SysAllocString(L"×óæIÇÐ“QÄ£Ê½£¬ÓÒæI´òé_²Ë†Î");
	return (*pbstrToolTip == NULL)? E_OUTOFMEMORY: S_OK;
}

STDAPI CLangBarItemButton::OnClick(TfLBIClick click, POINT pt, const RECT *prcArea)
{
	if (click == TF_LBI_CLK_LEFT)
	{
		/* TODO : Switch mode */
	}
	else if (click == TF_LBI_CLK_RIGHT)
	{
		/* Open menu */
		HWND hwnd = _pTextService->_GetFocusedContextWindow();
		if (hwnd != NULL)
		{
			HMENU menu = LoadMenuW(g_hInst, MAKEINTRESOURCE(IDR_MENU_POPUP));
			HMENU popupMenu = GetSubMenu(menu, 0);
			UINT wID = TrackPopupMenuEx(popupMenu, TPM_NONOTIFY | TPM_RETURNCMD | TPM_HORPOSANIMATION, pt.x, pt.y, hwnd, NULL);
			DestroyMenu(menu);
			_pTextService->_HandleLangBarMenuSelect(wID);
		}
	}
	return S_OK;
}

STDAPI CLangBarItemButton::InitMenu(ITfMenu *pMenu)
{
	HMENU menu = LoadMenuW(g_hInst, MAKEINTRESOURCE(IDR_MENU_POPUP));
	HMENU popupMenu = GetSubMenu(menu, 0);
	HMENU2ITfMenu(popupMenu, pMenu);
	DestroyMenu(menu);
	return S_OK;
}

STDAPI CLangBarItemButton::OnMenuSelect(UINT wID)
{
	_pTextService->_HandleLangBarMenuSelect(wID);
	return S_OK;
}

STDAPI CLangBarItemButton::GetIcon(HICON *phIcon)
{
	*phIcon = (HICON) LoadImageW(g_hInst, MAKEINTRESOURCEW(IDI_ENABLED), IMAGE_ICON, 16, 16, LR_SHARED);
	return (*phIcon == NULL)? E_FAIL: S_OK;
}

STDAPI CLangBarItemButton::GetText(BSTR *pbstrText)
{
	*pbstrText = SysAllocString(L"WeaselTSF Button");
	return (*pbstrText == NULL)? E_OUTOFMEMORY: S_OK;
}

STDAPI CLangBarItemButton::AdviseSink(REFIID riid, IUnknown *punk, DWORD *pdwCookie)
{
	if (!IsEqualIID(riid, IID_ITfLangBarItemSink))
		return CONNECT_E_CANNOTCONNECT;
	if (_pLangBarItemSink != NULL)
		return CONNECT_E_ADVISELIMIT;

	if (punk->QueryInterface(IID_ITfLangBarItemSink, (LPVOID *) &_pLangBarItemSink) != S_OK)
	{
		_pLangBarItemSink = NULL;
		return E_NOINTERFACE;
	}
	*pdwCookie = LANGBARITEMSINK_COOKIE;
	return S_OK;
}

STDAPI CLangBarItemButton::UnadviseSink(DWORD dwCookie)
{
	if (dwCookie != LANGBARITEMSINK_COOKIE || _pLangBarItemSink == NULL)
		return CONNECT_E_NOCONNECTION;
	_pLangBarItemSink->Release();
	_pLangBarItemSink = NULL;
	return S_OK;
}

void WeaselTSF::_HandleLangBarMenuSelect(UINT wID)
{
	switch (wID)
	{
	case ID_WEASELTRAY_QUIT:
	case ID_WEASELTRAY_DEPLOY:
	case ID_WEASELTRAY_SETTINGS:
	case ID_WEASELTRAY_DICT_MANAGEMENT:
	case ID_WEASELTRAY_WIKI:
	case ID_WEASELTRAY_FORUM:
	case ID_WEASELTRAY_CHECKUPDATE:
	case ID_WEASELTRAY_INSTALLDIR:
	case ID_WEASELTRAY_USERCONFIG:
		/* TODO */
	}
}

HWND WeaselTSF::_GetFocusedContextWindow()
{
	HWND hwnd = NULL;
	ITfDocumentMgr *pDocMgr;
	if (_pThreadMgr->GetFocus(&pDocMgr) == S_OK && pDocMgr != NULL)
	{
		ITfContext *pContext;
		if (pDocMgr->GetTop(&pContext) == S_OK && pContext != NULL)
		{
			ITfContextView *pContextView;
			if (pContext->GetActiveView(&pContextView) == S_OK && pContextView != NULL)
			{
				pContextView->GetWnd(&hwnd);
				pContextView->Release();
			}
			pContext->Release();
		}
		pDocMgr->Release();
	}

	if (hwnd == NULL)
	{
		HWND hwndForeground = GetForegroundWindow();
		if (GetWindowThreadProcessId(hwndForeground, NULL) == GetCurrentThreadId())
			hwnd = hwndForeground;
	}

	return hwnd;
}

BOOL WeaselTSF::_InitLanguageBar()
{
	ITfLangBarItemMgr *pLangBarItemMgr;
	BOOL fRet = FALSE;

	if (_pThreadMgr->QueryInterface(IID_ITfLangBarItemMgr, (LPVOID *) &pLangBarItemMgr) != S_OK)
		return FALSE;

	if ((_pLangBarButton = new CLangBarItemButton(this, GUID_LBI_INPUTMODE)) == NULL)
		goto Exit;
	
	if (pLangBarItemMgr->AddItem(_pLangBarButton) != S_OK)
	{
		_pLangBarButton->Release();
		_pLangBarButton = NULL;
		goto Exit;
	}

	fRet = TRUE;

Exit:
	pLangBarItemMgr->Release();
	return fRet;
}

void WeaselTSF::_UninitLanguageBar()
{
	ITfLangBarItemMgr *pLangBarItemMgr;

	if (_pLangBarButton == NULL)
		return;

	if (_pThreadMgr->QueryInterface(IID_ITfLangBarItemMgr, (LPVOID *) &pLangBarItemMgr) == S_OK)
	{
		pLangBarItemMgr->RemoveItem(_pLangBarButton);
		pLangBarItemMgr->Release();
	}

	_pLangBarButton->Release();
	_pLangBarButton = NULL;
}
