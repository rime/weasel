#pragma once

#include "Layout.h"

namespace weasel
{
	const int MAX_CANDIDATES_COUNT = 10;
	const int STATUS_ICON_SIZE = GetSystemMetrics(SM_CXICON);

	class StandardLayout: public Layout
	{
	public:
		StandardLayout(const UIStyle &style, const Context &context, const Status &status);

		/* Layout */
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

	protected:
		/* Utility functions */
		CSize GetPreeditSize(CDCHandle dc) const;
		void UpdateStatusIconLayout(int* width, int* height);

		CSize _contentSize;
		CRect _preeditRect, _auxiliaryRect, _highlightRect;
		CRect _candidateLabelRects[MAX_CANDIDATES_COUNT];
		CRect _candidateTextRects[MAX_CANDIDATES_COUNT];
		CRect _candidateCommentRects[MAX_CANDIDATES_COUNT];
		CRect _statusIconRect;
	};
};
