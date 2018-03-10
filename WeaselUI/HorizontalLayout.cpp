#include "stdafx.h"
#include "HorizontalLayout.h"

using namespace weasel;

HorizontalLayout::HorizontalLayout(const UIStyle &style, const Context &context, const Status &status)
	: StandardLayout(style, context, status)
{
}

void HorizontalLayout::DoLayout(CDCHandle dc)
{
	const std::vector<Text> &candidates(_context.cinfo.candies);
	const std::vector<Text> &comments(_context.cinfo.comments);
	const std::vector<Text> &labels(_context.cinfo.labels);

	CSize size;
	//dc.GetTextExtent(L"\x4e2d", 1, &size);
	//const int space = size.cx / 4;
	const int space = _style.hilite_spacing;
	int width = 0, height = _style.margin_y;

	/* Preedit */
	if (!IsInlinePreedit() && !_context.preedit.str.empty())
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
	int w = _style.margin_x, h = 0;
	for (size_t i = 0; i < candidates.size() && i < MAX_CANDIDATES_COUNT; ++i)
	{
		if (i > 0)
			w += _style.candidate_spacing;

		/* Label */
		std::wstring label = GetLabelText(labels, i, _style.label_text_format.c_str());
		dc.GetTextExtent(label.c_str(), label.length(), &size);
		_candidateLabelRects[i].SetRect(w, height, w + size.cx, height + size.cy);
		w += size.cx, h = max(h, size.cy);
		w += space;

		/* Text */
		const std::wstring& text = candidates.at(i).str;
		dc.GetTextExtent(text.c_str(), text.length(), &size);
		_candidateTextRects[i].SetRect(w, height, w + size.cx, height + size.cy);
		w += size.cx + space, h = max(h, size.cy);

		/* Comment */
		if (!comments.at(i).str.empty())
		{
			const std::wstring& comment = comments.at(i).str;
			dc.GetTextExtent(comment.c_str(), comment.length(), &size);
			_candidateCommentRects[i].SetRect(w, height, w + size.cx + space, height + size.cy);
			w += size.cx + space, h = max(h, size.cy);
		}
		else /* Used for highlighted candidate calculation below */
		{
			_candidateCommentRects[i].SetRect(w, height, w, height + size.cy);
		}
	}
	w += _style.margin_x;

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

	if (!_context.preedit.str.empty() && !candidates.empty())
	{
		width = max(width, _style.min_width);
		height = max(height, _style.min_height);
	}
	UpdateStatusIconLayout(&width, &height);
	_contentSize.SetSize(width, height);
}