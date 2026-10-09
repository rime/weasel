#include "VHorizontalLayout.h"
#include <vector>

using namespace weasel;

void VHorizontalLayout::DoLayout() {
  if (_style.vertical_text_with_wrap) {
    DoLayoutWithWrap();
    return;
  }

  int height = offsetY, width = offsetX + real_margin_x;
  int h = offsetY + real_margin_y;

  int base_offset = _CalcMarkMetrics(true);

  // calc page indicator
  int pgw = 0, pgh = 0;
  _CalcPageIndicator(true, pgw, pgh);

  /* Preedit */
  if (!IsInlinePreedit() && !_context.preedit.str.empty()) {
    _LayoutInlineRect(_preeditSize, true, true, pgw, pgh, h, width, height,
                      _preeditRect);
  }

  /* Auxiliary */
  if (!_context.aux.str.empty()) {
    _LayoutInlineRect(_auxSize, true, false, pgw, pgh, h, width, height,
                      _auxiliaryRect);
  }
  /* Candidates */
  std::vector<int> wids(candidates_count);
  int w = width;
  int max_comment_heihgt = 0, max_content_height = 0;
  if (candidates_count) {
    for (auto i = 0; i < candidates_count && i < MAX_CANDIDATES_COUNT; ++i) {
      int wid = 0;
      h = offsetY + real_margin_y + base_offset;
      /* Label */
      const auto& labelSize = _candidateLabelSizes[i];
      _candidateLabelRects[i].SetRect(w, h, w + labelSize.cx, h + labelSize.cy);
      h += labelSize.cy * labelFontValid;
      max_content_height = MAX(max_content_height, h);
      wid = MAX(wid, labelSize.cx);

      /* Text */
      h += _style.hilite_spacing * labelFontValid;
      const auto& textSize = _candidateTextSizes[i];
      _candidateTextRects[i].SetRect(w, h, w + textSize.cx, h + textSize.cy);
      h += textSize.cy * textFontValid;
      max_content_height = MAX(max_content_height, h);
      wid = MAX(wid, textSize.cx);

      /* Comment */
      bool cmtFontNotTrans =
          (i == id && (_style.hilited_comment_text_color & 0xff000000)) ||
          (i != id && (_style.comment_text_color & 0xff000000));
      if (!comments.at(i).str.empty() && cmtFontValid && cmtFontNotTrans) {
        h += _style.hilite_spacing;
        const auto& commentSize = _candidateCommentSizes[i];
        _candidateCommentRects[i].SetRect(w, 0, w + commentSize.cx,
                                          commentSize.cy);
        h += commentSize.cy * cmtFontValid;
        wid = MAX(wid, commentSize.cx);
      } else /* Used for highlighted candidate calculation below */
      {
        _candidateCommentRects[i].SetRect(w, 0, w, 0);
        wid = MAX(wid, textSize.cx);
      }
      wids[i] = wid;
      max_comment_heihgt =
          MAX(max_comment_heihgt, _candidateCommentRects[i].Height());
      w += wid + _style.candidate_spacing;
    }
    w -= _style.candidate_spacing;
  }

  width = MAX(width, w);
  width += real_margin_x;

  // reposition candidates
  if (candidates_count) {
    for (auto i = 0; i < candidates_count && i < MAX_CANDIDATES_COUNT; ++i) {
      int ol = 0, ot = 0, oc = 0;
      if (_style.align_type == UIStyle::ALIGN_CENTER) {
        ol = (wids[i] - _candidateLabelRects[i].Width()) / 2;
        ot = (wids[i] - _candidateTextRects[i].Width()) / 2;
        oc = (wids[i] - _candidateCommentRects[i].Width()) / 2;
      } else if ((_style.align_type == UIStyle::ALIGN_BOTTOM) &&
                 _style.vertical_text_left_to_right) {
        ol = (wids[i] - _candidateLabelRects[i].Width());
        ot = (wids[i] - _candidateTextRects[i].Width());
        oc = (wids[i] - _candidateCommentRects[i].Width());
      } else if ((_style.align_type == UIStyle::ALIGN_TOP) &&
                 (!_style.vertical_text_left_to_right)) {
        ol = (wids[i] - _candidateLabelRects[i].Width());
        ot = (wids[i] - _candidateTextRects[i].Width());
        oc = (wids[i] - _candidateCommentRects[i].Width());
      }
      // offset rects
      _candidateLabelRects[i].OffsetRect(ol, 0);
      _candidateTextRects[i].OffsetRect(ot, 0);
      _candidateCommentRects[i].OffsetRect(
          oc, max_content_height + _style.hilite_spacing);
      // define  _candidateRects
      _candidateRects[i].left =
          MIN(_candidateLabelRects[i].left, _candidateTextRects[i].left,
              _candidateCommentRects[i].left);
      _candidateRects[i].right =
          MAX(_candidateLabelRects[i].right, _candidateTextRects[i].right,
              _candidateCommentRects[i].right);
      _candidateRects[i].top = _candidateLabelRects[i].top - base_offset;
      _candidateRects[i].bottom =
          _candidateCommentRects[i].top + max_comment_heihgt;
    }
    height = MAX(height, offsetY + _candidateRects[0].Height() + real_margin_y);
    if ((_candidateRects[0].top - real_margin_y + height) >
        _candidateRects[0].bottom)
      for (auto i = 0; i < candidates_count && i < MAX_CANDIDATES_COUNT; ++i)
        _candidateRects[i].bottom =
            (_candidateRects[0].top - offsetY - real_margin_y + height);
    if (!_style.vertical_text_left_to_right) {
      // re position right to left
      int base_left;
      if ((!IsInlinePreedit() && !_context.preedit.str.empty()))
        base_left = _preeditRect.left;
      else if (!_context.aux.str.empty())
        base_left = _auxiliaryRect.left;
      else
        base_left = _candidateRects[0].left;
      for (int i = candidates_count - 1; i >= 0; i--) {
        int offset;
        if (i == candidates_count - 1)
          offset = base_left - _candidateRects[i].left;
        else
          offset = _candidateRects[i + 1].right + _style.candidate_spacing -
                   _candidateRects[i].left;
        _candidateRects[i].OffsetRect(offset, 0);
        _candidateLabelRects[i].OffsetRect(offset, 0);
        _candidateTextRects[i].OffsetRect(offset, 0);
        _candidateCommentRects[i].OffsetRect(offset, 0);
      }
      if (!IsInlinePreedit() && !_context.preedit.str.empty())
        _preeditRect.OffsetRect(
            _candidateRects[0].right + _style.spacing - _preeditRect.left, 0);
      if (!_context.aux.str.empty())
        _auxiliaryRect.OffsetRect(
            _candidateRects[0].right + _style.spacing - _auxiliaryRect.left, 0);
    }
  } else
    width -= _style.spacing;

  height += real_margin_y;

  if (candidates_count) {
    width = MAX(width, _style.min_width);
    height = MAX(height, _style.min_height);
  }
  _UpdateStatusIconLayout(&width, &height);
  // candidate rectangle always align to bottom side, margin_y to the bottom
  // edge
  for (auto i = 0; i < candidates_count && i < MAX_CANDIDATES_COUNT; ++i) {
    int bottom = MAX(_candidateRects[i].bottom, height - real_margin_y);
    _candidateCommentRects[i].OffsetRect(
        0, bottom - _candidateCommentRects[i].bottom);
    _candidateRects[i].bottom = bottom;
  }

  _highlightRect = _candidateRects[id];
  _contentSize.SetSize(width + offsetX, height + offsetY);

  // calc page indicator
  if (_pageEnabled && candidates_count && !_style.inline_preedit) {
    int _prey = _contentSize.cy - offsetY - real_margin_y +
                _style.hilite_padding_y - pgh;
    int _prex =
        (_preeditRect.left + _preeditRect.right) / 2 - _pagePrevSize.cx / 2;
    _prePageRect.SetRect(_prex, _prey, _prex + _pagePrevSize.cx,
                         _prey + _pagePrevSize.cy);
    _nextPageRect.SetRect(
        _prex, _prePageRect.bottom + _style.hilite_spacing,
        _prex + _pageNextSize.cx,
        _prePageRect.bottom + _style.hilite_spacing + _pageNextSize.cy);
    if (ShouldDisplayStatusIcon()) {
      _prePageRect.OffsetRect(0, -STATUS_ICON_SIZE);
      _nextPageRect.OffsetRect(0, -STATUS_ICON_SIZE);
    }
  }

  _contentRect.SetRect(0, 0, _contentSize.cx, _contentSize.cy);
  // background rect prepare for Hemispherical calculation
  CopyRect(_bgRect, _contentRect);
  _bgRect.DeflateRect(offsetX + 1, offsetY + 1);

  // Precompute preedit sub-rectangles
  _PrecomputePreeditRects(_preeditRect, _context.preedit, _preeditBeforeRect,
                          _preeditHiliteRect, _preeditAfterRect);

  // Precompute auxiliary sub-rectangles
  _PrecomputePreeditRects(_auxiliaryRect, _context.aux, _auxBeforeRect,
                          _auxHiliteRect, _auxAfterRect);

  // prepare round info
  _PrepareRoundInfo();

  // truely draw content size calculation
  _contentRect.DeflateRect(offsetX, offsetY);
}

void VHorizontalLayout::DoLayoutWithWrap() {
  int height = offsetY, width = offsetX + real_margin_x;
  int h = offsetY + real_margin_y;

  if ((_style.hilited_mark_color & 0xff000000) && candidates_count) {
    CSize sg;
    if (_style.mark_text.empty())
      _pD2D->GetTextSize(L"|", 1, _pD2D->pTextFormat, &sg);
    else
      _pD2D->GetTextSize(_style.mark_text, _style.mark_text.length(),
                         _pD2D->pTextFormat, &sg);

    mark_width = sg.cx;
    mark_height = sg.cy;
    if (_style.mark_text.empty()) {
      mark_height = mark_width / 7;
      if (_style.linespacing && _style.baseline)
        mark_height =
            (int)((float)mark_height / ((float)_style.linespacing / 100.0f));
      mark_height = MAX(mark_height, 6);
    }
    mark_gap = (_style.mark_text.empty()) ? mark_height
                                          : mark_height + _style.hilite_spacing;
  }
  int base_offset = ((_style.hilited_mark_color & 0xff000000)) ? mark_gap : 0;

  // calc page indicator
  int pgh = _pageEnabled
                ? _pagePrevSize.cy + _pageNextSize.cy + _style.hilite_spacing +
                      _style.hilite_padding_y * 2
                : 0;
  int pgw = _pageEnabled ? MAX(_pagePrevSize.cx, _pageNextSize.cx) : 0;

  /* Preedit */
  if (!IsInlinePreedit() && !_context.preedit.str.empty()) {
    size_t szx = MAX(_preeditSize.cx, pgw), szy = pgh;
    // icon size wider then preedit text
    int xoffset = ((size_t)STATUS_ICON_SIZE >= szx && ShouldDisplayStatusIcon())
                      ? (int)(STATUS_ICON_SIZE - szx) / 2
                      : 0;
    _preeditRect.SetRect(width + xoffset, h, width + xoffset + _preeditSize.cx,
                         h + _preeditSize.cy);
    width += _preeditSize.cx + xoffset * 2 + _style.spacing;
    height = static_cast<int>(
        MAX((size_t)height, offsetY + real_margin_y + _preeditSize.cy + szy));
    if (ShouldDisplayStatusIcon())
      height += STATUS_ICON_SIZE;
  }
  /* Auxiliary */
  if (!_context.aux.str.empty()) {
    // icon size wider then auxiliary text
    int xoffset = (STATUS_ICON_SIZE >= _auxSize.cx && ShouldDisplayStatusIcon())
                      ? (STATUS_ICON_SIZE - _auxSize.cx) / 2
                      : 0;
    _auxiliaryRect.SetRect(width + xoffset, h, width + xoffset + _auxSize.cx,
                           h + _auxSize.cy);
    width += _auxSize.cx + xoffset * 2 + _style.spacing;
    height = MAX(height, offsetY + real_margin_y + _auxSize.cy);
  }
  // candidates
  int col_cnt = 0;
  int max_height_of_cols = 0;
  std::vector<int> width_of_cols(candidates_count);
  std::vector<int> col_of_candidate(candidates_count);
  std::vector<int> minleft_of_cols(candidates_count);
  if (candidates_count) {
    h = offsetY + real_margin_y;
    for (auto i = 0; i < candidates_count && i < MAX_CANDIDATES_COUNT; i++) {
      int current_cand_height = 0;
      if (i > 0)
        h += _style.candidate_spacing;
      if (id == i)
        h += base_offset;
      /* Label */
      const auto& labelSize = _candidateLabelSizes[i];
      _candidateLabelRects[i].SetRect(width, h, width + labelSize.cx,
                                      h + labelSize.cy * labelFontValid);
      h += labelSize.cy * labelFontValid;
      current_cand_height += labelSize.cy * labelFontValid;

      /* Text */
      h += _style.hilite_spacing;
      const auto& textSize = _candidateTextSizes[i];
      _candidateTextRects[i].SetRect(width, h, width + textSize.cx,
                                     h + textSize.cy * textFontValid);
      h += textSize.cy * textFontValid;
      current_cand_height +=
          (textSize.cy + _style.hilite_spacing) * textFontValid;

      /* Comment */
      bool cmtFontNotTrans =
          (i == id && (_style.hilited_comment_text_color & 0xff000000)) ||
          (i != id && (_style.comment_text_color & 0xff000000));
      if (!comments.at(i).str.empty() && cmtFontValid && cmtFontNotTrans) {
        const auto& commentSize = _candidateCommentSizes[i];
        h += _style.hilite_spacing;
        _candidateCommentRects[i].SetRect(width, h, width + commentSize.cx,
                                          h + commentSize.cy * cmtFontValid);
        h += commentSize.cy * cmtFontValid;
        current_cand_height +=
            (commentSize.cy + _style.hilite_spacing) * cmtFontValid;
      } else
        _candidateCommentRects[i].SetRect(width, h, width + textSize.cx, h);
      int base_top = (i == id) ? _candidateLabelRects[i].top - base_offset
                               : _candidateLabelRects[i].top;
      if (_style.max_height > 0 && (base_top > real_margin_y + offsetY) &&
          (_candidateCommentRects[i].bottom - offsetY + real_margin_y >
           _style.max_height)) {
        max_height_of_cols =
            MAX(max_height_of_cols, _candidateCommentRects[i - 1].bottom);
        h = offsetY + real_margin_y + (i == id ? base_offset : 0);
        int ofy = h - _candidateLabelRects[i].top;
        int ofx = width_of_cols[col_cnt] + _style.candidate_spacing;
        _candidateLabelRects[i].OffsetRect(ofx, ofy);
        _candidateTextRects[i].OffsetRect(ofx, ofy);
        _candidateCommentRects[i].OffsetRect(ofx, ofy);
        max_height_of_cols =
            MAX(max_height_of_cols, _candidateCommentRects[i].bottom);
        minleft_of_cols[col_cnt] = width;
        width += ofx;
        h += current_cand_height;
        col_cnt++;
      } else
        max_height_of_cols = MAX(max_height_of_cols, h);
      minleft_of_cols[col_cnt] = width;
      width_of_cols[col_cnt] = MAX(
          width_of_cols[col_cnt], _candidateLabelRects[i].Width(),
          _candidateTextRects[i].Width(), _candidateCommentRects[i].Width());
      col_of_candidate[i] = col_cnt;
    }

    for (auto i = 0; i < candidates_count && i < MAX_CANDIDATES_COUNT; ++i) {
      int base_top = (i == id) ? _candidateLabelRects[i].top - base_offset
                               : _candidateLabelRects[i].top;
      _candidateRects[i].SetRect(minleft_of_cols[col_of_candidate[i]], base_top,
                                 minleft_of_cols[col_of_candidate[i]] +
                                     width_of_cols[col_of_candidate[i]],
                                 _candidateCommentRects[i].bottom);
      int ol = 0, ot = 0, oc = 0;
      if (_style.align_type == UIStyle::ALIGN_CENTER) {
        ol = (width_of_cols[col_of_candidate[i]] -
              _candidateLabelRects[i].Width()) /
             2;
        ot = (width_of_cols[col_of_candidate[i]] -
              _candidateTextRects[i].Width()) /
             2;
        oc = (width_of_cols[col_of_candidate[i]] -
              _candidateCommentRects[i].Width()) /
             2;
      } else if ((_style.align_type == UIStyle::ALIGN_BOTTOM) &&
                 _style.vertical_text_left_to_right) {
        ol = (width_of_cols[col_of_candidate[i]] -
              _candidateLabelRects[i].Width());
        ot = (width_of_cols[col_of_candidate[i]] -
              _candidateTextRects[i].Width());
        oc = (width_of_cols[col_of_candidate[i]] -
              _candidateCommentRects[i].Width());
      } else if ((_style.align_type == UIStyle::ALIGN_TOP) &&
                 (!_style.vertical_text_left_to_right)) {
        ol = (width_of_cols[col_of_candidate[i]] -
              _candidateLabelRects[i].Width());
        ot = (width_of_cols[col_of_candidate[i]] -
              _candidateTextRects[i].Width());
        oc = (width_of_cols[col_of_candidate[i]] -
              _candidateCommentRects[i].Width());
      }
      _candidateLabelRects[i].OffsetRect(ol, 0);
      _candidateTextRects[i].OffsetRect(ot, 0);
      _candidateCommentRects[i].OffsetRect(oc, 0);
      if ((i < candidates_count - 1 &&
           col_of_candidate[i] < col_of_candidate[i + 1]) ||
          (i == candidates_count - 1))
        _candidateRects[i].bottom = MAX(height, max_height_of_cols);
    }
    width = minleft_of_cols[col_cnt] + width_of_cols[col_cnt] - offsetX;
    height = MAX(height, max_height_of_cols);
    _highlightRect = _candidateRects[id];
  } else
    width -= _style.spacing + offsetX;
  // reposition if not left to right
  std::vector<int> first_cand_of_cols(candidates_count);
  std::vector<int> offset_of_cols(candidates_count);
  if (!_style.vertical_text_left_to_right) {
    // re position right to left
    int base_left;
    if ((!IsInlinePreedit() && !_context.preedit.str.empty()))
      base_left = _preeditRect.left;
    else if (!_context.aux.str.empty())
      base_left = _auxiliaryRect.left;
    else if (candidates_count)
      base_left = _candidateRects[0].left;
    if (candidates_count) {
      // calc offset for each col
      for (auto col_t = 0; col_t <= col_cnt; col_t++) {
        for (auto i = 0; i < candidates_count; i++) {
          if (col_of_candidate[i] == col_t) {
            first_cand_of_cols[col_t] = i;
            break;
          }
        }
      }
      for (auto i = col_cnt; i >= 0; i--) {
        int offset;
        if (i == col_cnt)
          offset = base_left - _candidateRects[first_cand_of_cols[i]].left;
        else
          offset = _candidateRects[first_cand_of_cols[i + 1]].right +
                   _style.candidate_spacing -
                   _candidateRects[first_cand_of_cols[i]].left;
        offset_of_cols[i] = offset;
        _candidateRects[first_cand_of_cols[i]].OffsetRect(offset_of_cols[i], 0);
      }
      for (auto i = 0; i < candidates_count; i++) {
        if (i != first_cand_of_cols[col_of_candidate[i]])
          _candidateRects[i].OffsetRect(offset_of_cols[col_of_candidate[i]], 0);
        _candidateLabelRects[i].OffsetRect(offset_of_cols[col_of_candidate[i]],
                                           0);
        _candidateTextRects[i].OffsetRect(offset_of_cols[col_of_candidate[i]],
                                          0);
        _candidateCommentRects[i].OffsetRect(
            offset_of_cols[col_of_candidate[i]], 0);
      }
      _highlightRect = _candidateRects[id];
      if (!IsInlinePreedit() && !_context.preedit.str.empty())
        _preeditRect.OffsetRect(
            _candidateRects[0].right + _style.spacing - _preeditRect.left, 0);
      if (!_context.aux.str.empty())
        _auxiliaryRect.OffsetRect(
            _candidateRects[0].right + _style.spacing - _auxiliaryRect.left, 0);
    }
  }

  width += real_margin_x;
  height += real_margin_y;
  if (candidates_count) {
    width = MAX(width, _style.min_width);
    height = MAX(height, _style.min_height);
  }
  _highlightRect = _candidateRects[id];
  _UpdateStatusIconLayout(&width, &height);
  _contentSize.SetSize(width + 2 * offsetX, height + offsetY);
  _contentRect.SetRect(0, 0, _contentSize.cx, _contentSize.cy);

  // calc page indicator
  if (_pageEnabled && candidates_count && !_style.inline_preedit) {
    int _prey = _contentSize.cy - offsetY - real_margin_y +
                _style.hilite_padding_y - pgh;
    int _prex =
        (_preeditRect.left + _preeditRect.right) / 2 - _pagePrevSize.cx / 2;
    _prePageRect.SetRect(_prex, _prey, _prex + _pagePrevSize.cx,
                         _prey + _pagePrevSize.cy);
    _nextPageRect.SetRect(
        _prex, _prePageRect.bottom + _style.hilite_spacing,
        _prex + _pageNextSize.cx,
        _prePageRect.bottom + _style.hilite_spacing + _pageNextSize.cy);
    if (ShouldDisplayStatusIcon()) {
      _prePageRect.OffsetRect(0, -STATUS_ICON_SIZE);
      _nextPageRect.OffsetRect(0, -STATUS_ICON_SIZE);
    }
  }

  // prepare temp rect _bgRect for roundinfo calculation
  CopyRect(_bgRect, _contentRect);
  _bgRect.DeflateRect(offsetX + 1, offsetY + 1);
  _PrepareRoundInfo();
  if (_style.vertical_text_left_to_right) {
    for (auto i = 0; i < candidates_count; i++) {
      _roundInfo[i].Hemispherical = _roundInfo[0].Hemispherical;
      if (_roundInfo[0].Hemispherical) {
        if (col_of_candidate[i] == col_cnt) {
          _roundInfo[i].IsTopRightNeedToRound = false;
          _roundInfo[i].IsBottomRightNeedToRound = false;
        }
        if (col_of_candidate[i] == 0) {
          _roundInfo[i].IsTopLeftNeedToRound = false;
          _roundInfo[i].IsBottomLeftNeedToRound = false;
        }
        if (i == 0) {
          _roundInfo[i].IsTopLeftNeedToRound = _style.inline_preedit;
          if (col_cnt == 0)
            _roundInfo[i].IsTopRightNeedToRound = true;
        }
        if (i == candidates_count - 1) {
          _roundInfo[i].IsBottomRightNeedToRound = true;
          if (col_cnt == 0)
            _roundInfo[i].IsBottomLeftNeedToRound = _style.inline_preedit;
        }

        if (col_of_candidate[i] == col_cnt && col_cnt > 0 && i > 0 &&
            col_of_candidate[i - 1] == (col_cnt - 1))
          _roundInfo[i].IsTopRightNeedToRound = true;
        if (col_of_candidate[i] == 0 && col_cnt > 0 &&
            i + 1 < candidates_count && col_of_candidate[i + 1] == 1)
          _roundInfo[i].IsBottomLeftNeedToRound = _style.inline_preedit;
      }
    }
  } else {
    for (auto i = 0; i < candidates_count; i++) {
      _roundInfo[i].Hemispherical = _roundInfo[0].Hemispherical;
      if (_roundInfo[0].Hemispherical) {
        if (col_of_candidate[i] == 0) {
          _roundInfo[i].IsTopRightNeedToRound = false;
          _roundInfo[i].IsBottomRightNeedToRound = false;
        }
        if (col_of_candidate[i] == col_cnt) {
          _roundInfo[i].IsTopLeftNeedToRound = false;
          _roundInfo[i].IsBottomLeftNeedToRound = false;
        }
        if (i == 0) {
          _roundInfo[i].IsTopRightNeedToRound = _style.inline_preedit;
          if (col_cnt == 0)
            _roundInfo[i].IsTopLeftNeedToRound = true;
        }
        if (i == candidates_count - 1) {
          _roundInfo[i].IsBottomLeftNeedToRound = true;
          if (col_cnt == 0)
            _roundInfo[i].IsBottomRightNeedToRound = _style.inline_preedit;
        }
        if (col_of_candidate[i] == col_cnt && col_cnt > 0 && i > 0 &&
            col_of_candidate[i - 1] == (col_cnt - 1))
          _roundInfo[i].IsTopLeftNeedToRound = true;
        if (col_of_candidate[i] == 0 && col_cnt > 0 &&
            i + 1 < candidates_count && col_of_candidate[i + 1] == 1)
          _roundInfo[i].IsBottomRightNeedToRound = _style.inline_preedit;
      }
    }
  }

  // Precompute preedit sub-rectangles
  _PrecomputePreeditRects(_preeditRect, _context.preedit, _preeditBeforeRect,
                          _preeditHiliteRect, _preeditAfterRect);

  // Precompute auxiliary sub-rectangles
  _PrecomputePreeditRects(_auxiliaryRect, _context.aux, _auxBeforeRect,
                          _auxHiliteRect, _auxAfterRect);

  // truely draw content size calculation
  _contentRect.DeflateRect(offsetX, offsetY);
}
