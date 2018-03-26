#pragma once
#include <WeaselUI.h>

class WeaselTSF;

namespace weasel {
	class CandidateList :
		public ITfCandidateListUIElementBehavior,
		public ITfCandidateListUIElement,
		public ITfUIElement
	{
	public:
		CandidateList(WeaselTSF *pTextService);
		~CandidateList();

		// IUnknown
		STDMETHODIMP QueryInterface(REFIID riid, _Outptr_ void **ppvObj);
		STDMETHODIMP_(ULONG) AddRef(void);
		STDMETHODIMP_(ULONG) Release(void);

		// ITfUIElement
		STDMETHODIMP GetDescription(BSTR *pbstr);
		STDMETHODIMP GetGUID(GUID *pguid);
		STDMETHODIMP Show(BOOL showCandidateWindow);
		STDMETHODIMP IsShown(BOOL *pIsShow);

		// ITfCandidateListUIElement
		STDMETHODIMP GetUpdatedFlags(DWORD *pdwFlags);
		STDMETHODIMP GetDocumentMgr(ITfDocumentMgr **ppdim);
		STDMETHODIMP GetCount(UINT *pCandidateCount);
		STDMETHODIMP GetSelection(UINT *pSelectedCandidateIndex);
		STDMETHODIMP GetString(UINT uIndex, BSTR *pbstr);
		STDMETHODIMP GetPageIndex(UINT *pIndex, UINT uSize, UINT *puPageCnt);
		STDMETHODIMP SetPageIndex(UINT *pIndex, UINT uPageCnt);
		STDMETHODIMP GetCurrentPage(UINT *puPage);

		// ITfCandidateListUIElementBehavior methods
		STDMETHODIMP SetSelection(UINT nIndex);
		STDMETHODIMP Finalize(void);
		STDMETHODIMP Abort(void);

		/* Update */
		void UpdateUI(const Context &ctx, const Status &status);
		void UpdateStyle(const UIStyle &sty);
		void UpdateInputPosition(RECT const& rc);
		UIStyle &style();

	private:
		HWND _GetActiveWnd();

		std::unique_ptr<UI> _ui;
		DWORD _cRef;
		WeaselTSF *_tsf;
		DWORD uiid;

		/* The parent of current candidate window */
		HWND _curp;
	};
}
