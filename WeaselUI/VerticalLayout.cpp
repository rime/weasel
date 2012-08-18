#include "stdafx.h"
#include "VerticalLayout.h"

using namespace weasel;

VerticalLayout::VerticalLayout(const UIStyle &style, const Context &context)
	: StandardLayout(style, context)
{
}

void VerticalLayout::DoLayout(CDCHandle dc)
{
	const std::vector<Text> &candidates(_context.cinfo.candies);
	const std::vector<Text> &comments(_context.cinfo.comments);
	const std::string &labels(_context.cinfo.labels);

	int width = 0, height = _style.margin_y;
	CSize size;

	/* Preedit */
	if (!_style.inline_preedit)
	{
		size = GetPreeditSize(dc);
		_preeditRect.SetRect(_style.margin_x, height, _style.margin_x + size.cx, height + size.cy);
		width = max(width, _style.margin_x + size.cx + _style.margin_x);
		height += size.cy + _style.spacing;
	}

	/* Auxiliary */
	if (!_context.aux.str.empty())
	{
		dc.GetTextExtent(_context.aux.str.c_str(), _context.aux.str.length(), &size);
		_auxiliaryRect.SetRect(_style.margin_x, height, _style.margin_x + size.cx, height + size.cy);
		width = max(width, _style.margin_x + size.cx + _style.margin_x);
		height += size.cy + _style.spacing;
	}

	/* Candidates */
	int comment_shift_width = 0;
	for (int i = 0; i < candidates.size(); i++)
	{
		int w = _style.margin_x + _style.hilite_padding, h = 0;
		/* Label */
		std::wstring label = GetLabelText(labels, i);
		dc.GetTextExtent(label.c_str(), label.length(), &size);
		_candidateLabelRects[i].SetRect(w, height, w + size.cx, height + size.cy);
		w += size.cx, h = max(h, size.cy);

		/* Text */
		std::wstring text = candidates.at(i).str;
		dc.GetTextExtent(text.c_str(), text.length(), &size);
		_candidateTextRects[i].SetRect(w, height, w + size.cx, height + size.cy);
		w += size.cx, h = max(h, size.cy);

		/* Comment */
		if (!comments.at(i).str.empty())
		{
			/* Add a space */
			dc.GetTextExtent(L" ", 1, &size);
			w += size.cx, h = max(h, size.cy);
			comment_shift_width = max(comment_shift_width, w);

			std::wstring comment = comments.at(i).str;
			dc.GetTextExtent(comment.c_str(), comment.length(), &size);
			_candidateCommentRects[i].SetRect(0, height, size.cx, height + size.cy);
			w += size.cx, h = max(h, size.cy);
		}
		w += _style.hilite_padding + _style.margin_x;

		width = max(width, w);
		height += h + _style.candidate_spacing;
	}
	/* Align comments */
	for (int i = 0; i < candidates.size(); i++)
		_candidateCommentRects[i].OffsetRect(comment_shift_width, 0);

	if (candidates.size())
		height += _style.spacing;

	/* Trim the last spacing */
	if (height > 0)
		height -= _style.spacing;
	height += _style.margin_y;

	width = max(width, _style.min_width);
	height = max(height, _style.min_height);
	_contentSize.SetSize(width, height);

	/* Highlighted Candidate */
	int id = _context.cinfo.highlighted;
	_highlightRect.SetRect(
		_style.margin_x + _style.hilite_padding,
		_candidateTextRects[id].top,
		width - _style.margin_x  - _style.hilite_padding,
		_candidateTextRects[id].bottom);
}