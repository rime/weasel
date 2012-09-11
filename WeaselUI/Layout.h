#pragma once

#include <WeaselCommon.h>
#include <WeaselUI.h>

namespace weasel
{
	class Layout
	{
	public:
		Layout(const UIStyle &style, const Context &context);

		virtual void DoLayout(CDCHandle dc) = 0;
		/* All points in this class is based on the content area */
		/* The top-left corner of the content area is always (0, 0) */
		virtual CSize GetContentSize() const = 0;
		virtual CRect GetPreeditRect() const = 0;
		virtual CRect GetAuxiliaryRect() const = 0;
		virtual CRect GetHighlightRect() const = 0;
		virtual CRect GetCandidateLabelRect(int id) const = 0;
		virtual CRect GetCandidateTextRect(int id) const = 0;
		virtual CRect GetCandidateCommentRect(int id) const = 0;

		virtual std::wstring GetLabelText(const std::string &label, int id) const = 0;

	protected:
		const UIStyle &_style;
		const Context &_context;
	};
};