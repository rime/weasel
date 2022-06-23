#include "stdafx.h"
#include "HorizontalLayout.h"

using namespace weasel;

HorizontalLayout::HorizontalLayout(const UIStyle &style, const Context &context, const Status &status)
	: StandardLayout(style, context, status)
{
}

void HorizontalLayout::DoLayout(CDCHandle dc, GDIFonts* pFonts )
{
	const std::vector<Text> &candidates(_context.cinfo.candies);
	const std::vector<Text> &comments(_context.cinfo.comments);
	const std::vector<Text> &labels(_context.cinfo.labels);
	CFont oldFont;
	CSize size;
	//dc.GetTextExtent(L"\x4e2d", 1, &size);
	//const int space = size.cx / 4;
	const int space = _style.hilite_spacing;
	int width = 0, height = _style.margin_y;

	oldFont = dc.SelectFont(pFonts->_TextFont);
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
		GetTextExtentDCMultiline(dc, _context.aux.str, _context.aux.str.length(), &size);
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
		oldFont = dc.SelectFont(pFonts->_LabelFont);
		std::wstring label = GetLabelText(labels, i, _style.label_text_format.c_str());
		GetTextExtentDCMultiline(dc, label, label.length(), &size);
		_candidateLabelRects[i].SetRect(w, height, w + size.cx, height + size.cy);
		w += size.cx, h = max(h, size.cy);
		w += space;

		/* Text */
		oldFont = dc.SelectFont(pFonts->_TextFont);
		const std::wstring& text = candidates.at(i).str;
		GetTextExtentDCMultiline(dc, text, text.length(), &size);
		_candidateTextRects[i].SetRect(w, height, w + size.cx, height + size.cy);
		w += size.cx + space, h = max(h, size.cy);

		/* Comment */
		oldFont = dc.SelectFont(pFonts->_CommentFont);
		if (!comments.at(i).str.empty())
		{
			const std::wstring& comment = comments.at(i).str;
			GetTextExtentDCMultiline(dc, comment, comment.length(), &size);
			_candidateCommentRects[i].SetRect(w, height, w + size.cx + space, height + size.cy);
			w += size.cx + space, h = max(h, size.cy);
		}
		else /* Used for highlighted candidate calculation below */
		{
			_candidateCommentRects[i].SetRect(w, height, w, height + size.cy);
		}
	}
	dc.SelectFont(oldFont);
	for (size_t i = 0; i < candidates.size() && i < MAX_CANDIDATES_COUNT; ++i)
	{
		int ol = 0, ot = 0, oc = 0;
		if (_style.align_type == UIStyle::ALIGN_CENTER)
		{
			ol = (h - _candidateLabelRects[i].Height()) / 2;
			ot = (h - _candidateTextRects[i].Height()) / 2;
			oc = (h - _candidateCommentRects[i].Height()) / 2;
		}
		else if (_style.align_type == UIStyle::ALIGN_BOTTOM)
		{
			ol = (h - _candidateLabelRects[i].Height()) ;
			ot = (h - _candidateTextRects[i].Height()) ;
			oc = (h - _candidateCommentRects[i].Height()) ;

		}
		_candidateLabelRects[i].OffsetRect(0, ol);
		_candidateTextRects[i].OffsetRect(0, ot);
		_candidateCommentRects[i].OffsetRect(0, oc);
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

void weasel::HorizontalLayout::DoLayout(CDCHandle dc, DirectWriteResources* pDWR)
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
		size = GetPreeditSize(dc, pDWR->pTextFormat, pDWR->pDWFactory);
		_preeditRect.SetRect(_style.margin_x, height, _style.margin_x + size.cx, height + size.cy);
		width = max(width, _style.margin_x + size.cx + _style.margin_x);
		height += size.cy + _style.spacing;
	}

	/* Auxiliary */
	if (!_context.aux.str.empty())
	{
		GetTextExtentDCMultiline(dc, _context.aux.str, _context.aux.str.length(), &size);
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
		GetTextSizeDW(label, label.length(), pDWR->pLabelTextFormat, pDWR->pDWFactory, &size);
		_candidateLabelRects[i].SetRect(w, height, w + size.cx, height + size.cy);
		w += size.cx, h = max(h, size.cy);
		w += space;

		/* Text */
		const std::wstring& text = candidates.at(i).str;
		GetTextSizeDW(text, text.length(), pDWR->pTextFormat, pDWR->pDWFactory, &size);
		_candidateTextRects[i].SetRect(w, height, w + size.cx, height + size.cy);
		w += size.cx + space, h = max(h, size.cy);

		/* Comment */
		if (!comments.at(i).str.empty())
		{
			const std::wstring& comment = comments.at(i).str;
			GetTextSizeDW(comment, comment.length(), pDWR->pCommentTextFormat, pDWR->pDWFactory, &size);
			_candidateCommentRects[i].SetRect(w, height, w + size.cx + space, height + size.cy);
			w += size.cx + space, h = max(h, size.cy);
		}
		else /* Used for highlighted candidate calculation below */
		{
			_candidateCommentRects[i].SetRect(w, height, w, height + size.cy);
		}
	}
	for (size_t i = 0; i < candidates.size() && i < MAX_CANDIDATES_COUNT; ++i)
	{
		int ol = 0, ot = 0, oc = 0;
		if (_style.align_type == UIStyle::ALIGN_CENTER)
		{
			ol = (h - _candidateLabelRects[i].Height()) / 2;
			ot = (h - _candidateTextRects[i].Height()) / 2;
			oc = (h - _candidateCommentRects[i].Height()) / 2;
		}
		else if (_style.align_type == UIStyle::ALIGN_BOTTOM)
		{
			ol = (h - _candidateLabelRects[i].Height()) ;
			ot = (h - _candidateTextRects[i].Height()) ;
			oc = (h - _candidateCommentRects[i].Height()) ;

		}
		_candidateLabelRects[i].OffsetRect(0, ol);
		_candidateTextRects[i].OffsetRect(0, ot);
		_candidateCommentRects[i].OffsetRect(0, oc);
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
