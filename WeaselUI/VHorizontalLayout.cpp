
#include "stdafx.h"
#include "VHorizontalLayout.h"

using namespace weasel;

void VHorizontalLayout::DoLayout(CDCHandle dc, DirectWriteResources* pDWR )
{
	if(_style.vertical_text_with_wrap)
	{
		DoLayoutWithWrap(dc, pDWR);
		return;
	}

	CSize size;
	int height = offsetY, width = offsetX + real_margin_x;
	int h = offsetY + real_margin_y;

#ifdef USE_HILITE_MARK
	if (!_style.mark_text.empty() && (_style.hilited_mark_color & 0xff000000))
	{
		CSize sg;
		GetTextSizeDW(_style.mark_text, _style.mark_text.length(), pDWR->pTextFormat, pDWR, &sg);
		MARK_WIDTH = sg.cx;
		MARK_HEIGHT = sg.cy;
		MARK_GAP = MARK_HEIGHT + 4 * dpiScale;
	}
	int base_offset =  ((_style.hilited_mark_color & 0xff000000) && !_style.mark_text.empty()) ? MARK_GAP : 0;
#else
	int base_offset =  0;
#endif

	/* Preedit */
	if (!IsInlinePreedit() && !_context.preedit.str.empty())
	{
		size = GetPreeditSize(dc, _context.preedit, pDWR->pPreeditTextFormat, pDWR);
		// icon size wider then preedit text
		int xoffset = (STATUS_ICON_SIZE * dpiScale >= size.cx && ShouldDisplayStatusIcon()) ? (STATUS_ICON_SIZE * dpiScale - size.cx) / 2 : 0;
		_preeditRect.SetRect(width + xoffset, h, width + xoffset + size.cx, h + size.cy);
		width += size.cx + xoffset * 2 + _style.spacing * dpiScale;
		height = max(height, offsetY + real_margin_y + size.cy);
	}

	/* Auxiliary */
	if (!_context.aux.str.empty())
	{
		size = GetPreeditSize(dc, _context.aux, pDWR->pPreeditTextFormat, pDWR);
		// icon size wider then preedit text
		int xoffset = (STATUS_ICON_SIZE * dpiScale >= size.cx && ShouldDisplayStatusIcon()) ? (STATUS_ICON_SIZE * dpiScale - size.cx) / 2 : 0;
		_auxiliaryRect.SetRect(width + xoffset, h, width + xoffset + size.cx, h + size.cy);
		width += size.cx + xoffset * 2 + _style.spacing * dpiScale;
		height = max(height, offsetY + real_margin_y + size.cy);
	}
	/* Candidates */
	int wids[MAX_CANDIDATES_COUNT] = {0};
	int w = width;
	int max_comment_heihgt = 0, max_content_height =  0;
	if (candidates_count)
	{
		for (auto i = 0; i < candidates_count && i < MAX_CANDIDATES_COUNT; ++i)
		{
			int wid = 0;
			h = offsetY + real_margin_y + base_offset;
			/* Label */
			std::wstring label = GetLabelText(labels, i, _style.label_text_format.c_str());
			GetTextSizeDW(label, label.length(), pDWR->pLabelTextFormat, pDWR, &size);
			_candidateLabelRects[i].SetRect(w, h, w + size.cx * labelFontValid, h + size.cy);
			h += size.cy * labelFontValid;
			max_content_height = max(max_content_height, h);
			wid = max(wid, size.cx);

			/* Text */
			h += _style.hilite_spacing * dpiScale * labelFontValid;
			const std::wstring& text = candidates.at(i).str;
			GetTextSizeDW(text, text.length(), pDWR->pTextFormat, pDWR, &size);
			_candidateTextRects[i].SetRect(w, h, w + size.cx * textFontValid, h + size.cy);
			h += size.cy * textFontValid;
			max_content_height = max(max_content_height, h);
			wid = max(wid, size.cx);

			/* Comment */
			if (!comments.at(i).str.empty() && cmtFontValid)
			{
				h += _style.hilite_spacing * dpiScale;
				const std::wstring& comment = comments.at(i).str;
				GetTextSizeDW(comment, comment.length(), pDWR->pCommentTextFormat, pDWR, &size);
				_candidateCommentRects[i].SetRect(w, 0, w + size.cx * cmtFontValid, size.cy * cmtFontValid);
				h += size.cy * cmtFontValid;
				wid = max(wid, size.cx);
			}
			else /* Used for highlighted candidate calculation below */
			{
				_candidateCommentRects[i].SetRect(w, 0, w, 0);
				wid = max(wid, size.cx);
			}
			wids[i] = wid;
			max_comment_heihgt = max(max_comment_heihgt, _candidateCommentRects[i].Height());
			w += wid + _style.candidate_spacing * dpiScale;
		}
		w -= _style.candidate_spacing * dpiScale;
	}

	width = max(width, w);
	width += real_margin_x;

	// reposition candidates
	if (candidates_count)
	{
		for (auto i = 0; i < candidates_count && i < MAX_CANDIDATES_COUNT; ++i)
		{
			int ol = 0, ot = 0, oc = 0;
			if(_style.align_type == UIStyle::ALIGN_CENTER)
			{
				ol = (wids[i] - _candidateLabelRects[i].Width()) / 2;
				ot = (wids[i] - _candidateTextRects[i].Width()) / 2;
				oc = (wids[i] - _candidateCommentRects[i].Width()) / 2;
			}
			else if((_style.align_type == UIStyle::ALIGN_BOTTOM) && _style.vertical_text_left_to_right)
			{
				ol = (wids[i] - _candidateLabelRects[i].Width());
				ot = (wids[i] - _candidateTextRects[i].Width());
				oc = (wids[i] - _candidateCommentRects[i].Width());
			}
			else if((_style.align_type == UIStyle::ALIGN_TOP) && (!_style.vertical_text_left_to_right))
			{
				ol = (wids[i] - _candidateLabelRects[i].Width());
				ot = (wids[i] - _candidateTextRects[i].Width());
				oc = (wids[i] - _candidateCommentRects[i].Width());
			}
			// offset rects
			_candidateLabelRects[i].OffsetRect(ol, 0);
			_candidateTextRects[i].OffsetRect(ot, 0);
			_candidateCommentRects[i].OffsetRect(oc, max_content_height + _style.hilite_spacing * dpiScale);
			// define  _candidateRects
			_candidateRects[i].left = min(_candidateLabelRects[i].left, _candidateTextRects[i].left);
			_candidateRects[i].left = min(_candidateRects[i].left, _candidateCommentRects[i].left);
			_candidateRects[i].right = max(_candidateLabelRects[i].right, _candidateTextRects[i].right);
			_candidateRects[i].right = max(_candidateRects[i].right, _candidateCommentRects[i].right);
			_candidateRects[i].top = _candidateLabelRects[i].top - base_offset;
			_candidateRects[i].bottom = _candidateCommentRects[i].top + max_comment_heihgt;
		}
		height = max(height, offsetY + _candidateRects[0].Height() + real_margin_y);
		if((_candidateRects[0].top - real_margin_y + height) > _candidateRects[0].bottom)
			for (auto i = 0; i < candidates_count && i < MAX_CANDIDATES_COUNT; ++i)
				_candidateRects[i].bottom = (_candidateRects[0].top - offsetY - real_margin_y + height);
		if(!_style.vertical_text_left_to_right)
		{
			// re position right to left
			int base_left;
			if ((!IsInlinePreedit() && !_context.preedit.str.empty()))
				base_left = _preeditRect.left;
			else if( !_context.aux.str.empty())
				base_left = _auxiliaryRect.left;	
			else
				base_left = _candidateRects[0].left;
			for(int i = candidates_count - 1; i>=0 ; i --)
			{
				int offset;
				if(i == candidates_count - 1)
					offset = base_left - _candidateRects[i].left;
				else
					offset = _candidateRects[i+1].right + _style.candidate_spacing * dpiScale - _candidateRects[i].left;
				_candidateRects[i].OffsetRect(offset, 0);
				_candidateLabelRects[i].OffsetRect(offset, 0);
				_candidateTextRects[i].OffsetRect(offset, 0);
				_candidateCommentRects[i].OffsetRect(offset, 0);
			}
			if (!IsInlinePreedit() && !_context.preedit.str.empty())
				_preeditRect.OffsetRect(_candidateRects[0].right + _style.spacing * dpiScale - _preeditRect.left, 0);
			if (!_context.aux.str.empty())
				_auxiliaryRect.OffsetRect(_candidateRects[0].right + _style.spacing * dpiScale - _auxiliaryRect.left, 0);
		}
	}
	else
		width -= _style.spacing * dpiScale;

	height += real_margin_y;
	_highlightRect = _candidateRects[id];
	UpdateStatusIconLayout(&width, &height);
	_contentSize.SetSize(width + offsetX, height + offsetY);

	_contentRect.SetRect(0, 0, _contentSize.cx, _contentSize.cy);
	// background rect prepare for Hemispherical calculation
	CopyRect(_bgRect, _contentRect);
	_bgRect.DeflateRect(offsetX + 1, offsetY + 1);
	// prepare round info 
	_PrepareRoundInfo(dc);

	// truely draw content size calculation
	int deflatex = offsetX - _style.border * dpiScale / 2;
	int deflatey = offsetY - _style.border * dpiScale / 2;
	_contentRect.DeflateRect(deflatex, deflatey);
	// eliminate the 1 pixel gap when border width odd and padding equal to margin
	if ((int)(_style.border * dpiScale) % 2 == 0)	_contentRect.DeflateRect(1 * dpiScale, 1 * dpiScale);
}

void VHorizontalLayout::DoLayoutWithWrap(CDCHandle dc, DirectWriteResources* pDWR)
{
	CSize size;
	int height = offsetY, width = offsetX + real_margin_x;
	int h = offsetY + real_margin_y;

#ifdef USE_HILITE_MARK
	if (!_style.mark_text.empty() && (_style.hilited_mark_color & 0xff000000))
	{
		CSize sg;
		GetTextSizeDW(_style.mark_text, _style.mark_text.length(), pDWR->pTextFormat, pDWR, &sg);
		MARK_WIDTH = sg.cx;
		MARK_HEIGHT = sg.cy;
		MARK_GAP = MARK_HEIGHT + 4 * dpiScale;
	}
	int base_offset =  ((_style.hilited_mark_color & 0xff000000) && !_style.mark_text.empty()) ? MARK_GAP : 0;
#else
	int base_offset =  0;
#endif

	/* Preedit */
	if (!IsInlinePreedit() && !_context.preedit.str.empty())
	{
		size = GetPreeditSize(dc, _context.preedit, pDWR->pPreeditTextFormat, pDWR);
		// icon size wider then preedit text
		int xoffset = (STATUS_ICON_SIZE * dpiScale >= size.cx && ShouldDisplayStatusIcon()) ? (STATUS_ICON_SIZE * dpiScale - size.cx) / 2 : 0;
		_preeditRect.SetRect(width + xoffset, h, width + xoffset + size.cx, h + size.cy);
		width += size.cx + xoffset * 2 + _style.spacing * dpiScale;
		height = max(height, offsetY + real_margin_y + size.cy);
	}
	/* Auxiliary */
	if (!_context.aux.str.empty())
	{
		size = GetPreeditSize(dc, _context.aux, pDWR->pPreeditTextFormat, pDWR);
		// icon size wider then auxiliary text
		int xoffset = (STATUS_ICON_SIZE * dpiScale >= size.cx && ShouldDisplayStatusIcon()) ? (STATUS_ICON_SIZE * dpiScale - size.cx) / 2 : 0;
		_auxiliaryRect.SetRect(width + xoffset, h, width + xoffset + size.cx, h + size.cy);
		width += size.cx + xoffset * 2 + _style.spacing * dpiScale;
		height = max(height, offsetY + real_margin_y + size.cy);
	}
	// candidates
	int col_cnt = 0;
	int max_height_of_cols = 0;
	int width_of_cols[MAX_CANDIDATES_COUNT] = {0};
	int col_of_candidate[MAX_CANDIDATES_COUNT] = {0};
	int minleft_of_cols[MAX_CANDIDATES_COUNT] = {0};
	if( candidates_count )
	{
		h = offsetY + real_margin_y;
		for(auto i = 0; i < candidates_count && i < MAX_CANDIDATES_COUNT; i++)
		{
			int current_cand_height = 0;
			if( i > 0 )		h += _style.candidate_spacing * dpiScale;
			if( id == i )	h += base_offset;
			/* Label */
			std::wstring label = GetLabelText(labels, i, _style.label_text_format.c_str());
			GetTextSizeDW(label, label.length(), pDWR->pLabelTextFormat, pDWR, &size);
			_candidateLabelRects[i].SetRect(width, h, width + size.cx, h + size.cy * labelFontValid);
			h += size.cy * labelFontValid;
			current_cand_height += size.cy * labelFontValid;

			/* Text */
			h += _style.hilite_spacing * dpiScale;
			const std::wstring& text =candidates.at(i).str;
			GetTextSizeDW(text, text.length(), pDWR->pTextFormat, pDWR, &size);
			_candidateTextRects[i].SetRect(width, h, width + size.cx, h + size.cy * textFontValid);
			h += size.cy * textFontValid;
			current_cand_height += (size.cy + _style.hilite_spacing * dpiScale) * textFontValid;

			/* Comment */
			if (!comments.at(i).str.empty() && cmtFontValid )
			{
				const std::wstring& comment = comments.at(i).str;
				GetTextSizeDW(comment, comment.length(), pDWR->pCommentTextFormat, pDWR, &size);
				h += _style.hilite_spacing * dpiScale;
				_candidateCommentRects[i].SetRect(width, h, width + size.cx, h + size.cy * cmtFontValid);
				h += size.cy * cmtFontValid;
				current_cand_height += (size.cy + _style.hilite_spacing * dpiScale) * cmtFontValid;
			}	
			else
				_candidateCommentRects[i].SetRect(width, h, width + size.cx, h);
			int base_top = (i == id) ? _candidateLabelRects[i].top - base_offset : _candidateLabelRects[i].top;
			if( _style.max_height * dpiScale > 0 && (base_top > real_margin_y + offsetY) && (_candidateCommentRects[i].bottom - offsetY + real_margin_y > _style.max_height * dpiScale) )
			{
				max_height_of_cols = max(max_height_of_cols, _candidateCommentRects[i-1].bottom);
				h = offsetY + real_margin_y + (i==id? base_offset : 0);
				int ofy = h - _candidateLabelRects[i].top;
				int ofx = width_of_cols[col_cnt] + _style.candidate_spacing * dpiScale;
				_candidateLabelRects[i].OffsetRect(ofx, ofy);
				_candidateTextRects[i].OffsetRect(ofx, ofy);
				_candidateCommentRects[i].OffsetRect(ofx, ofy);
				minleft_of_cols[col_cnt] = width;
				width += ofx;
				h += current_cand_height;
				col_cnt ++;
			}
			else
				max_height_of_cols = max(max_height_of_cols, h);
			minleft_of_cols[col_cnt] = width;
			width_of_cols[col_cnt] = max(width_of_cols[col_cnt], _candidateLabelRects[i].Width());
			width_of_cols[col_cnt] = max(width_of_cols[col_cnt], _candidateTextRects[i].Width());
			width_of_cols[col_cnt] = max(width_of_cols[col_cnt], _candidateCommentRects[i].Width());
			col_of_candidate[i] = col_cnt;
		}

		
		for (auto i = 0; i < candidates_count && i < MAX_CANDIDATES_COUNT; ++i)
		{
			int base_top = (i == id) ? _candidateLabelRects[i].top - base_offset : _candidateLabelRects[i].top;
			_candidateRects[i].SetRect(minleft_of_cols[col_of_candidate[i]], base_top,
					minleft_of_cols[col_of_candidate[i]] + width_of_cols[col_of_candidate[i]], _candidateCommentRects[i].bottom);
			int ol = 0, ot = 0, oc = 0;
			if(_style.align_type == UIStyle::ALIGN_CENTER)
			{
				ol = (width_of_cols[col_of_candidate[i]] - _candidateLabelRects[i].Width()) / 2;
				ot = (width_of_cols[col_of_candidate[i]] - _candidateTextRects[i].Width()) / 2;
				oc = (width_of_cols[col_of_candidate[i]] - _candidateCommentRects[i].Width()) / 2;
			}
			else if((_style.align_type == UIStyle::ALIGN_BOTTOM) && _style.vertical_text_left_to_right)
			{
				ol = (width_of_cols[col_of_candidate[i]] - _candidateLabelRects[i].Width());
				ot = (width_of_cols[col_of_candidate[i]] - _candidateTextRects[i].Width());
				oc = (width_of_cols[col_of_candidate[i]] - _candidateCommentRects[i].Width());
			}
			else if((_style.align_type == UIStyle::ALIGN_TOP) && (!_style.vertical_text_left_to_right))
			{
				ol = (width_of_cols[col_of_candidate[i]] - _candidateLabelRects[i].Width());
				ot = (width_of_cols[col_of_candidate[i]] - _candidateTextRects[i].Width());
				oc = (width_of_cols[col_of_candidate[i]] - _candidateCommentRects[i].Width());
			}
			_candidateLabelRects[i].OffsetRect(ol, 0);
			_candidateTextRects[i].OffsetRect(ot, 0);
			_candidateCommentRects[i].OffsetRect(oc, 0);
			if (( i < candidates_count - 1 && col_of_candidate[i] < col_of_candidate[i+1] ) || ( i == candidates_count - 1 ))
				_candidateRects[i].bottom = max(height, max_height_of_cols);
		}
		width = minleft_of_cols[col_cnt] + width_of_cols[col_cnt] - offsetX;
		height = max(height, max_height_of_cols);
		_highlightRect = _candidateRects[id];
	}
	else
		width -= _style.spacing * dpiScale + offsetX;
	// reposition if not left to right
	int first_cand_of_cols[MAX_CANDIDATES_COUNT] = {0};
	int offset_of_cols[MAX_CANDIDATES_COUNT] = {0};
	if(!_style.vertical_text_left_to_right)
	{
		// re position right to left
		int base_left;
		if ((!IsInlinePreedit() && !_context.preedit.str.empty()))
			base_left = _preeditRect.left;
		else if( !_context.aux.str.empty())
			base_left = _auxiliaryRect.left;	
		else if(candidates_count)
			base_left = _candidateRects[0].left;
		if(candidates_count)
		{
			// calc offset for each col
			for(auto col_t = 0; col_t <= col_cnt; col_t++)
			{
				for(auto i = 0; i < candidates_count; i++ )
				{
					if(col_of_candidate[i] == col_t)
					{
						first_cand_of_cols[col_t] = i;
						break;
					}
				}		
			}
			for(auto i = col_cnt; i >= 0; i-- )
			{
				int offset ;
				if(i == col_cnt)
					offset = base_left - _candidateRects[first_cand_of_cols[i]].left;
				else
					offset = _candidateRects[first_cand_of_cols[i+1]].right + _style.candidate_spacing * dpiScale - _candidateRects[first_cand_of_cols[i]].left;
				offset_of_cols[i] = offset;
				_candidateRects[first_cand_of_cols[i]].OffsetRect(offset_of_cols[i], 0);
			}
			for(auto i = 0; i < candidates_count; i++ )
			{
				if( i != first_cand_of_cols[col_of_candidate[i]] )
					_candidateRects[i].OffsetRect(offset_of_cols[col_of_candidate[i]], 0);
				_candidateLabelRects[i].OffsetRect(offset_of_cols[col_of_candidate[i]], 0);
				_candidateTextRects[i].OffsetRect(offset_of_cols[col_of_candidate[i]], 0);
				_candidateCommentRects[i].OffsetRect(offset_of_cols[col_of_candidate[i]], 0);
			}
			_highlightRect = _candidateRects[id];
			if (!IsInlinePreedit() && !_context.preedit.str.empty())
				_preeditRect.OffsetRect(_candidateRects[0].right + _style.spacing * dpiScale - _preeditRect.left, 0);
			if (!_context.aux.str.empty())
				_auxiliaryRect.OffsetRect(_candidateRects[0].right + _style.spacing * dpiScale - _auxiliaryRect.left, 0);
		}
	}

	width += real_margin_x;
	height += real_margin_y;
	if (!_context.preedit.str.empty() && !candidates_count)
	{
		width = max(width, _style.min_width * dpiScale);
		height = max(height, _style.min_height * dpiScale);
	}
	UpdateStatusIconLayout(&width, &height);
	_contentSize.SetSize(width + 2 * offsetX, height + offsetY);

	_contentRect.SetRect(0, 0, _contentSize.cx, _contentSize.cy);
	// prepare temp rect _bgRect for roundinfo calculation
	//CopyRect(_bgRect, _contentRect);
	//_bgRect.DeflateRect(offsetX + 1, offsetY + 1);
	// truely draw content size calculation
	int deflatex = offsetX - _style.border * dpiScale / 2;
	int deflatey = offsetY - _style.border * dpiScale / 2;
	_contentRect.DeflateRect(deflatex, deflatey);
	// eliminate the 1 pixel gap when border width odd and padding equal to margin
	if ((int)(_style.border * dpiScale) % 2 == 0)	_contentRect.DeflateRect(1 * dpiScale, 1 * dpiScale);
}

