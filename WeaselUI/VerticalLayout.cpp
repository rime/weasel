#include "VerticalLayout.h"

using namespace weasel;

void weasel::VerticalLayout::DoLayout() {
  const int space = _style.hilite_spacing;
  int width = 0, height = real_margin_y;

  int base_offset = _CalcMarkMetrics(false);

  // calc page indicator
  int pgw = 0, pgh = 0;
  _CalcPageIndicator(false, pgw, pgh);

  /*  preedit and auxiliary rectangle calc start */
  CSize size;
  /* Preedit */
  if (!IsInlinePreedit() && !_context.preedit.str.empty()) {
    _LayoutInlineRect(_preeditSize, false, true, pgw, pgh, real_margin_x, width,
                      height, _preeditRect);
    _preeditRect.OffsetRect(offsetX, offsetY);
  }

  /* Auxiliary */
  if (!_context.aux.str.empty()) {
    _LayoutInlineRect(_auxSize, false, false, pgw, pgh, real_margin_x, width,
                      height, _auxiliaryRect);
    _auxiliaryRect.OffsetRect(offsetX, offsetY);
  }
  /*  preedit and auxiliary rectangle calc end */

  /* Candidates */
  int comment_shift_width = 0; /* distance to the left of the candidate text */
  int max_candidate_width = 0; /* label + text */
  int max_comment_width = 0;   /* comment, or none */
  for (auto i = 0; i < candidates_count && i < MAX_CANDIDATES_COUNT; ++i) {
    if (i > 0)
      height += _style.candidate_spacing;

    int w = real_margin_x + base_offset, h = 0;
    int candidate_width = base_offset, comment_width = 0;
    /* Label */
    const auto& labelSize = _candidateLabelSizes[i];
    _candidateLabelRects[i].SetRect(w, height, w + labelSize.cx,
                                    height + labelSize.cy);
    _candidateLabelRects[i].OffsetRect(offsetX, offsetY);
    w += (labelSize.cx + space) * labelFontValid;
    h = MAX(h, labelSize.cy);
    candidate_width += (labelSize.cx + space) * labelFontValid;

    /* Text */
    const auto& textSize = _candidateTextSizes[i];
    _candidateTextRects[i].SetRect(w, height, w + textSize.cx,
                                   height + textSize.cy);
    _candidateTextRects[i].OffsetRect(offsetX, offsetY);
    w += textSize.cx * textFontValid;
    h = MAX(h, textSize.cy);
    candidate_width += textSize.cx * textFontValid;
    max_candidate_width = MAX(max_candidate_width, candidate_width);

    /* Comment */
    bool cmtFontNotTrans =
        (i == id && (_style.hilited_comment_text_color & 0xff000000)) ||
        (i != id && (_style.comment_text_color & 0xff000000));
    if (!comments.at(i).str.empty() && cmtFontValid && cmtFontNotTrans) {
      w += space;
      comment_shift_width = MAX(comment_shift_width, w);

      const auto& commentSize = _candidateCommentSizes[i];
      _candidateCommentRects[i].SetRect(0, height, commentSize.cx,
                                        height + commentSize.cy);
      _candidateCommentRects[i].OffsetRect(offsetX, offsetY);
      w += commentSize.cx * cmtFontValid;
      h = MAX(h, commentSize.cy);
      comment_width += commentSize.cx * cmtFontValid;
      max_comment_width = MAX(max_comment_width, comment_width);
    }
    if (_style.align_type != UIStyle::ALIGN_TOP) {
      int ol = 0, ot = 0, oc = 0;
      if (_style.align_type == UIStyle::ALIGN_CENTER) {
        ol = (h - _candidateLabelRects[i].Height()) / 2;
        ot = (h - _candidateTextRects[i].Height()) / 2;
        oc = (h - _candidateCommentRects[i].Height()) / 2;
      } else if (_style.align_type == UIStyle::ALIGN_BOTTOM) {
        ol = (h - _candidateLabelRects[i].Height());
        ot = (h - _candidateTextRects[i].Height());
        oc = (h - _candidateCommentRects[i].Height());
      }
      _candidateLabelRects[i].OffsetRect(0, ol);
      _candidateTextRects[i].OffsetRect(0, ot);
      _candidateCommentRects[i].OffsetRect(0, oc);
    }

    int hlTop = _candidateTextRects[i].top;
    int hlBot = _candidateTextRects[i].bottom;
    if (_candidateLabelRects[i].Height() > 0) {
      hlTop = MIN(_candidateLabelRects[i].top, hlTop);
      hlBot =
          MAX(_candidateLabelRects[i].bottom, _candidateTextRects[i].bottom);
    }
    if (_candidateCommentRects[i].Height() > 0) {
      hlTop = MIN(hlTop, _candidateCommentRects[i].top);
      hlBot = MAX(hlBot, _candidateCommentRects[i].bottom);
    }
    _candidateRects[i].SetRect(real_margin_x + offsetX, hlTop,
                               width - real_margin_x + offsetX, hlBot);

    width = MAX(width, w);
    height += h;
  }
  /* comments are left-aligned to the right of the longest candidate who has a
   * comment */
  int max_content_width =
      MAX(max_candidate_width, comment_shift_width + max_comment_width);
  width = MAX(width, max_content_width + 2 * real_margin_x);

  /* Align comments */
  for (auto i = 0; i < candidates_count && i < MAX_CANDIDATES_COUNT; ++i) {
    int hlTop = _candidateTextRects[i].top;
    int hlBot = _candidateTextRects[i].bottom;

    _candidateCommentRects[i].OffsetRect(real_margin_x + comment_shift_width,
                                         0);
    if (_candidateLabelRects[i].Height() > 0) {
      hlTop = MIN(_candidateLabelRects[i].top, hlTop);
      hlBot =
          MAX(_candidateLabelRects[i].bottom, _candidateTextRects[i].bottom);
    }
    if (_candidateCommentRects[i].Height() > 0) {
      hlTop = MIN(hlTop, _candidateCommentRects[i].top);
      hlBot = MAX(hlBot, _candidateCommentRects[i].bottom);
    }

    _candidateRects[i].SetRect(real_margin_x + offsetX, hlTop,
                               width - real_margin_x + offsetX, hlBot);
  }

  /* Trim the last spacing if no candidates */
  if (candidates_count == 0)
    height -= _style.spacing;

  height += real_margin_y;

  if (candidates_count) {
    width = MAX(width, _style.min_width);
    height = MAX(height, _style.min_height);
  }
  _UpdateStatusIconLayout(&width, &height);
  // candidate rectangle always align to right side, margin_x to the right edge
  for (auto i = 0; i < candidates_count && i < MAX_CANDIDATES_COUNT; ++i) {
    int right =
        MAX(_candidateRects[i].right,
            _candidateRects[i].left - real_margin_x + width - real_margin_x);
    _candidateCommentRects[i].OffsetRect(right - _candidateRects[i].right, 0);
    _candidateRects[i].right = right;
  }

  _contentSize.SetSize(width + offsetX * 2, height + offsetY * 2);

  /* Highlighted Candidate */

  _highlightRect = _candidateRects[id];
  if (_pageEnabled && candidates_count && !_style.inline_preedit) {
    int _prex = _contentSize.cx - offsetX - real_margin_x +
                _style.hilite_padding_x - pgw;
    int _prey =
        (_preeditRect.top + _preeditRect.bottom) / 2 - _pagePrevSize.cy / 2;
    _prePageRect.SetRect(_prex, _prey, _prex + _pagePrevSize.cx,
                         _prey + _pagePrevSize.cy);
    _nextPageRect.SetRect(
        _prePageRect.right + _style.hilite_spacing, _prey,
        _prePageRect.right + _style.hilite_spacing + _pageNextSize.cx,
        _prey + _pageNextSize.cy);
    if (ShouldDisplayStatusIcon()) {
      _prePageRect.OffsetRect(-STATUS_ICON_SIZE, 0);
      _nextPageRect.OffsetRect(-STATUS_ICON_SIZE, 0);
    }
  }

  // Precompute preedit sub-rectangles
  _PrecomputePreeditRects(_preeditRect, _context.preedit, _preeditBeforeRect,
                          _preeditHiliteRect, _preeditAfterRect);

  // Precompute auxiliary sub-rectangles
  _PrecomputePreeditRects(_auxiliaryRect, _context.aux, _auxBeforeRect,
                          _auxHiliteRect, _auxAfterRect);

  if (_style.vertical_right_to_left) {
    for (auto i = 0; i < candidates_count && i < MAX_CANDIDATES_COUNT; ++i) {
      size_t left = _candidateRects[i].left;
      size_t right = _candidateRects[i].right;
      auto& labelRect = _candidateLabelRects[i];
      auto& textRect = _candidateTextRects[i];
      auto& commentRect = _candidateCommentRects[i];
      labelRect.OffsetRect(right - labelRect.right, 0);
      textRect.OffsetRect(labelRect.left - _style.spacing - textRect.right, 0);
      commentRect.OffsetRect(left - commentRect.left, 0);
    }
  }
  // calc roundings start
  _contentRect.SetRect(0, 0, _contentSize.cx, _contentSize.cy);
  // background rect prepare for Hemispherical calculation
  CopyRect(_bgRect, _contentRect);
  _bgRect.DeflateRect(offsetX + 1, offsetY + 1);

  _PrepareRoundInfo();

  // truely draw content size calculation
  _contentRect.DeflateRect(offsetX, offsetY);
}
