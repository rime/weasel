#include "stdafx.h"
#include "HorizontalLayout.h"

using namespace weasel;

HorizontalLayout::HorizontalLayout(const UIStyle &style, const Context &context)
	: StandardLayout(style, context)
{
}

void HorizontalLayout::DoLayout(CDCHandle dc)
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
	int w = _style.margin_x + _style.hilite_padding, h = 0;
	for (int i = 0; i < candidates.size(); i++)
	{
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

			std::wstring comment = comments.at(i).str;
			dc.GetTextExtent(comment.c_str(), comment.length(), &size);
			_candidateCommentRects[i].SetRect(w, height, w + size.cx, height + size.cy);
			w += size.cx, h = max(h, size.cy);
		}
		else /* Used for highlighted candidate calculation below */
			_candidateCommentRects[i].SetRect(w, height, w, height + size.cy);
		w += _style.candidate_spacing;
	}
	w += _style.hilite_padding + _style.margin_x;

	/* Highlighted Candidate */
	int id = _context.cinfo.highlighted;
	_highlightRect.SetRect(
		_candidateLabelRects[id].left,
		height,
		_candidateCommentRects[id].right,
		height + h);

	width = max(width, w);
	height += h;

	if (candidates.size())
		height += _style.spacing;

	/* Trim the last spacing */
	if (height > 0)
		height -= _style.spacing;
	height += _style.margin_y;

	width = max(width, _style.min_width);
	height = max(height, _style.min_height);
	_contentSize.SetSize(width, height);
}