#pragma once

#include "Layout.h"
#include <d2d1.h>
#include <dwrite.h>
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

namespace weasel
{
	const int MAX_CANDIDATES_COUNT = 10;
	const int STATUS_ICON_SIZE = GetSystemMetrics(SM_CXICON);
	template <class T> void SafeRelease(T** ppT)
	{
		if (*ppT)
		{
			(*ppT)->Release();
			*ppT = NULL;
		}
	}
	class StandardLayout: public Layout
	{
	public:
		StandardLayout(const UIStyle &style, const Context &context, const Status &status);

		/* Layout */

		virtual void DoLayout(CDCHandle dc) = 0;
		virtual void DoLayout(CDCHandle dc, IDWriteTextFormat* pTextFormat, IDWriteFactory* pDWFacroty) = 0;
		virtual CSize GetContentSize() const { return _contentSize; }
		virtual CRect GetPreeditRect() const { return _preeditRect; }
		virtual CRect GetAuxiliaryRect() const { return _auxiliaryRect; }
		virtual CRect GetHighlightRect() const { return _highlightRect; }
		virtual CRect GetCandidateLabelRect(int id) const { return _candidateLabelRects[id]; }
		virtual CRect GetCandidateTextRect(int id) const { return _candidateTextRects[id]; }
		virtual CRect GetCandidateCommentRect(int id) const { return _candidateCommentRects[id]; }
		virtual CRect GetStatusIconRect() const { return _statusIconRect; }
		virtual std::wstring GetLabelText(const std::vector<Text> &labels, int id, const wchar_t *format) const;
		virtual bool IsInlinePreedit() const;
		virtual bool ShouldDisplayStatusIcon() const;

		void GetTextExtentDCMultiline(CDCHandle dc, std::wstring wszString, int nCount, LPSIZE lpSize) const;
		std::wstring StandardLayout::ConvertCRLF(std::wstring strString, std::wstring strCRLF) const;
		void GetTextSizeDW(const std::wstring text, int nCount, IDWriteTextFormat* pTextFormat, IDWriteFactory* pDWFactory, LPSIZE lpSize) const;

	protected:
		/* Utility functions */
		CSize GetPreeditSize(CDCHandle dc) const;
		CSize GetPreeditSize(CDCHandle dc, IDWriteTextFormat* pTextFormat, IDWriteFactory* pDWFactory) const;
		void UpdateStatusIconLayout(int* width, int* height);

		CSize _contentSize;
		CRect _preeditRect, _auxiliaryRect, _highlightRect;
		CRect _candidateLabelRects[MAX_CANDIDATES_COUNT];
		CRect _candidateTextRects[MAX_CANDIDATES_COUNT];
		CRect _candidateCommentRects[MAX_CANDIDATES_COUNT];
		CRect _statusIconRect;
	};
};
