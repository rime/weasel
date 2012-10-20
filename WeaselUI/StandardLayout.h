#pragma once

#include "Layout.h"

namespace weasel
{
	const int MAX_CANDIDATES_COUNT = 10;

	class StandardLayout: public Layout
	{
	public:
		StandardLayout(const UIStyle &style, const Context &context);

		/* Layout */
		virtual CSize GetContentSize() const { return _contentSize; }
		virtual CRect GetPreeditRect() const { return _preeditRect; }
		virtual CRect GetAuxiliaryRect() const { return _auxiliaryRect; }
		virtual CRect GetHighlightRect() const { return _highlightRect; }
		virtual CRect GetCandidateLabelRect(int id) const { return _candidateLabelRects[id]; }
		virtual CRect GetCandidateTextRect(int id) const { return _candidateTextRects[id]; }
		virtual CRect GetCandidateCommentRect(int id) const { return _candidateCommentRects[id]; }

		virtual std::wstring GetLabelText(const std::string &label, int id) const;
		virtual bool IsInlinePreedit() const;

	protected:
		/* Utility functions */
		CSize GetPreeditSize(CDCHandle dc) const;

		CSize _contentSize;
		CRect _preeditRect, _auxiliaryRect, _highlightRect;
		CRect _candidateLabelRects[MAX_CANDIDATES_COUNT];
		CRect _candidateTextRects[MAX_CANDIDATES_COUNT];
		CRect _candidateCommentRects[MAX_CANDIDATES_COUNT];
	};
};
