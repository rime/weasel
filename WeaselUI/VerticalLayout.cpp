#include "stdafx.h"
#include "VerticalLayout.h"

using namespace weasel;

VerticalLayout::VerticalLayout(const UIStyle &style, const Context &context, const Status &status)
	: StandardLayout(style, context, status)
{
}

void VerticalLayout::DoLayout(CDCHandle dc)
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
	int comment_shift_width = 0;  /* distance to the left of the candidate text */
	int max_candidate_width = 0;  /* label + text */
	int max_comment_width = 0;    /* comment, or none */
	for (size_t i = 0; i < candidates.size() && i < MAX_CANDIDATES_COUNT; ++i)
	{
		if (i > 0 )
			height += _style.candidate_spacing;

		int w = _style.margin_x, h = 0;
		int candidate_width = 0, comment_width = 0;
		/* Label */
		std::wstring label = GetLabelText(labels, i, _style.label_text_format.c_str());
		dc.GetTextExtent(label.c_str(), label.length(), &size);
		_candidateLabelRects[i].SetRect(w, height, w + size.cx, height + size.cy);
		w += size.cx + space, h = max(h, size.cy);
		candidate_width += size.cx + space;

		/* Text */
		const std::wstring& text = candidates.at(i).str;
		dc.GetTextExtent(text.c_str(), text.length(), &size);
		_candidateTextRects[i].SetRect(w, height, w + size.cx, height + size.cy);
		w += size.cx, h = max(h, size.cy);
		candidate_width += size.cx;
		max_candidate_width = max(max_candidate_width, candidate_width);

		/* Comment */
		if (!comments.at(i).str.empty())
		{
			w += space;
			comment_shift_width = max(comment_shift_width, w - _style.margin_x);

			const std::wstring& comment = comments.at(i).str;
			dc.GetTextExtent(comment.c_str(), comment.length(), &size);
			_candidateCommentRects[i].SetRect(0, height, size.cx, height + size.cy);
			w += size.cx, h = max(h, size.cy);
			comment_width += size.cx;
			max_comment_width = max(max_comment_width, comment_width);
		}
		//w += margin;
		//width = max(width, w);
		height += h;
	}
	/* comments are left-aligned to the right of the longest candidate who has a comment */
	int max_content_width = max(max_candidate_width, comment_shift_width + max_comment_width);
	width = max(width, max_content_width + 2 * _style.margin_x);

	/* Align comments */
	for (size_t i = 0; i < candidates.size() && i < MAX_CANDIDATES_COUNT; ++i)
		_candidateCommentRects[i].OffsetRect(_style.margin_x + comment_shift_width, 0);

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

	/* Highlighted Candidate */
	int id = _context.cinfo.highlighted;
	_highlightRect.SetRect(
		_style.margin_x,
		_candidateTextRects[id].top,
		width - _style.margin_x,
		_candidateTextRects[id].bottom);
}