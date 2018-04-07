#pragma once
#include <WeaselUI.h>
#include "ctffunc.h"

class WeaselTSF;

namespace weasel {
	class CandidateList :
		public ITfIntegratableCandidateListUIElement,
		public ITfCandidateListUIElementBehavior
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

		// ITfIntegratableCandidateListUIElement methods
		STDMETHODIMP SetIntegrationStyle(GUID guidIntegrationStyle);
		STDMETHODIMP GetSelectionStyle(_Out_ TfIntegratableCandidateListSelectionStyle *ptfSelectionStyle);
		STDMETHODIMP OnKeyDown(_In_ WPARAM wParam, _In_ LPARAM lParam, _Out_ BOOL *pIsEaten);
		STDMETHODIMP ShowCandidateNumbers(_Out_ BOOL *pIsShow);
		STDMETHODIMP FinalizeExactCompositionString();

		/* Update */
		void UpdateUI(const Context &ctx, const Status &status);
		void UpdateStyle(const UIStyle &sty);
		void UpdateInputPosition(RECT const& rc);
		void Destroy();
		UIStyle &style();

	private:
		void _UpdateOwner();
		HWND _GetActiveWnd();

		void _StartUI();
		void _EndUI();

		std::unique_ptr<UI> _ui;
		DWORD _cRef;
		WeaselTSF *_tsf;
		DWORD uiid;
		TfIntegratableCandidateListSelectionStyle _selectionStyle = STYLE_ACTIVE_SELECTION;

		/* The parent of current candidate window */
		HWND _curp;
	};
}
