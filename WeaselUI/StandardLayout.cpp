#include "StandardLayout.h"

using namespace weasel;

const wstring StandardLayout::_pre = L"<";
const wstring StandardLayout::_next = L">";

namespace {
wstring FormatCandidateLabel(const wstring& label, const wchar_t* format) {
  wchar_t buffer[128];
  swprintf_s<128>(buffer, format, label.c_str());
  return wstring(buffer);
}
}  // namespace

CSize StandardLayout::_GetPreeditSize(const Text& text,
                                      ComPtr<IDWriteTextFormat1>& pTextFormat) {
  const wstring& preedit = text.str;
  const vector<TextAttribute>& attrs = text.attributes;
  CSize size(0, 0);
  if (!preedit.empty()) {
    weasel::TextRange range;
    for (size_t j = 0; j < attrs.size(); ++j)
      if (attrs[j].type == weasel::HIGHLIGHTED)
        range = attrs[j].range;
    if (range.start < range.end) {
      wstring before_str = preedit.substr(0, range.start);
      wstring hilited_str =
          preedit.substr(range.start, range.end - range.start);
      wstring after_str = preedit.substr(range.end);
      CSize beforesz, hilitedsz, aftersz;
      _pD2D->GetTextSize(before_str, before_str.length(), pTextFormat,
                         &beforesz);
      _pD2D->GetTextSize(hilited_str, hilited_str.length(), pTextFormat,
                         &hilitedsz);
      _pD2D->GetTextSize(after_str, after_str.length(), pTextFormat, &aftersz);
      auto width_max = 0, height_max = 0;
      if (_style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT ||
          _style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT_FULLSCREEN) {
        width_max = MAX(width_max, beforesz.cx, hilitedsz.cx, aftersz.cx);
        height_max += beforesz.cy + (beforesz.cy > 0) * _style.hilite_spacing;
        height_max += hilitedsz.cy + (hilitedsz.cy > 0) * _style.hilite_spacing;
        height_max += aftersz.cy;
      } else {
        height_max = MAX(height_max, beforesz.cy, hilitedsz.cy, aftersz.cy);
        width_max += beforesz.cx + (beforesz.cx > 0) * _style.hilite_spacing;
        width_max += hilitedsz.cx + (hilitedsz.cx > 0) * _style.hilite_spacing;
        width_max += aftersz.cx;
      }
      size.cx = width_max;
      size.cy = height_max;
    } else
      _pD2D->GetTextSize(preedit, preedit.length(), pTextFormat, &size);
  }
  return size;
}

void StandardLayout::RecalculateSizes() {
  _preeditSize = _GetPreeditSize(_context.preedit, _pD2D->pPreeditFormat);
  _range = TextRange();
  for (size_t j = 0; j < _context.preedit.attributes.size(); ++j)
    if (_context.preedit.attributes[j].type == HIGHLIGHTED)
      _range = _context.preedit.attributes[j].range;
  _auxSize = _GetPreeditSize(_context.aux, _pD2D->pPreeditFormat);
  _pD2D->GetTextSize(_pre, _pre.length(), _pD2D->pPreeditFormat,
                     &_pagePrevSize);
  _pD2D->GetTextSize(_next, _next.length(), _pD2D->pPreeditFormat,
                     &_pageNextSize);

  // Precompute candidate sizes
  _candidateLabelSizes.clear();
  _candidateTextSizes.clear();
  _candidateCommentSizes.clear();
  for (size_t i = 0; i < candidates.size(); ++i) {
    CSize labelSize(0, 0), textSize(0, 0), commentSize(0, 0);
    if (labelFontValid) {
      const auto label = FormatCandidateLabel(labels.at(i).str,
                                              _style.label_text_format.c_str());
      _pD2D->GetTextSize(label, label.length(), _pD2D->pLabelFormat,
                         &labelSize);
    }
    if (textFontValid) {
      _pD2D->GetTextSize(candidates.at(i).str, candidates.at(i).str.length(),
                         _pD2D->pTextFormat, &textSize);
    }
    if (cmtFontValid) {
      _pD2D->GetTextSize(comments.at(i).str, comments.at(i).str.length(),
                         _pD2D->pCommentFormat, &commentSize);
    }
    _candidateLabelSizes.push_back(labelSize);
    _candidateTextSizes.push_back(textSize);
    _candidateCommentSizes.push_back(commentSize);
  }

  // Recalculate mark size
  if (!_style.mark_text.empty()) {
    _pD2D->GetTextSize(_style.mark_text, _style.mark_text.length(),
                       _pD2D->pTextFormat, &_markTextSize);
  } else {
    _markTextSize = CSize(0, 0);
  }
}

void StandardLayout::_UpdateStatusIconLayout(int* width, int* height) {
  // rule 1. status icon is middle-aligned with preedit text or auxiliary text,
  // whichever comes first rule 2. there is a spacing between preedit/aux text
  // and the status icon rule 3. status icon is right aligned in WeaselPanel,
  // when [margin_x + width(preedit/aux) + spacing + width(icon) + margin_x] <
  // style.min_width
  if (ShouldDisplayStatusIcon()) {
    if (_style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT ||
        _style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT_FULLSCREEN) {
      int top = 0, middle = 0;
      if (!_preeditRect.IsRectNull()) {
        top = _preeditRect.bottom + _style.spacing;
        middle = (_preeditRect.left + _preeditRect.right) / 2;
      } else if (!_auxiliaryRect.IsRectNull()) {
        top = _auxiliaryRect.bottom + _style.spacing;
        middle = (_auxiliaryRect.left + _auxiliaryRect.right) / 2;
      }
      if (top && middle) {
        int bottom_alignment = *height - real_margin_y - STATUS_ICON_SIZE;
        if (top > bottom_alignment) {
          *height = top + STATUS_ICON_SIZE + real_margin_y;
        } else {
          top = bottom_alignment;
        }
        _statusIconRect.SetRect(middle - STATUS_ICON_SIZE / 2 + 1, top,
                                middle + STATUS_ICON_SIZE / 2 + 1,
                                top + STATUS_ICON_SIZE + 1);
      } else {
        _statusIconRect.SetRect(0, 0, STATUS_ICON_SIZE, STATUS_ICON_SIZE);
        _statusIconRect.OffsetRect(offsetX, offsetY);
        *width = STATUS_ICON_SIZE;
        *height =
            (_style.vertical_text_with_wrap ? offsetY : 0) + STATUS_ICON_SIZE;
      }
    } else {
      int left = 0, middle = 0;
      if (!_preeditRect.IsRectNull()) {
        left = _preeditRect.right + _style.spacing;
        middle = (_preeditRect.top + _preeditRect.bottom) / 2;
      } else if (!_auxiliaryRect.IsRectNull()) {
        left = _auxiliaryRect.right + _style.spacing;
        middle = (_auxiliaryRect.top + _auxiliaryRect.bottom) / 2;
      }
      if (left && middle) {
        int right_alignment = *width - real_margin_x - STATUS_ICON_SIZE;
        if (left > right_alignment) {
          *width = left + STATUS_ICON_SIZE + real_margin_x;
        } else {
          left = right_alignment;
        }
        _statusIconRect.SetRect(left, middle - STATUS_ICON_SIZE / 2 + 1,
                                left + STATUS_ICON_SIZE,
                                middle + STATUS_ICON_SIZE / 2 + 1);
      } else {
        _statusIconRect.SetRect(0, 0, STATUS_ICON_SIZE, STATUS_ICON_SIZE);
        _statusIconRect.OffsetRect(offsetX, offsetY);
        *width = *height = STATUS_ICON_SIZE;
      }
    }
  }
  if (IS_FULLSCREENLAYOUT(_style))
    _statusIconRect.OffsetRect(-_style.border, -_style.border);
}

void StandardLayout::_CalcPageIndicator(bool vertical_text_layout,
                                        int& pgw,
                                        int& pgh) {
  CSize pgszl, pgszr;
  if (!IsInlinePreedit()) {
    _pD2D->GetTextSize(_pre, _pre.length(), _pD2D->pPreeditFormat, &pgszl);
    _pD2D->GetTextSize(_next, _next.length(), _pD2D->pPreeditFormat, &pgszr);
  }
  if (!_pageEnabled) {
    pgw = 0;
    pgh = 0;
    return;
  }
  if (vertical_text_layout) {
    pgh = pgszl.cy + pgszr.cy + _style.hilite_spacing +
          _style.hilite_padding_y * 2;
    pgw = MAX(pgszl.cx, pgszr.cx);
  } else {
    pgw = pgszl.cx + pgszr.cx + _style.hilite_spacing +
          _style.hilite_padding_x * 2;
    pgh = MAX(pgszl.cy, pgszr.cy);
  }
}

bool StandardLayout::IsInlinePreedit() const {
  return _style.inline_preedit &&
         (_style.client_caps & weasel::INLINE_PREEDIT_CAPABLE) != 0 &&
         _style.layout_type != UIStyle::LAYOUT_VERTICAL_FULLSCREEN &&
         _style.layout_type != UIStyle::LAYOUT_HORIZONTAL_FULLSCREEN;
}

bool StandardLayout::ShouldDisplayStatusIcon() const {
  // rule 1. emphasis ascii mode
  // rule 2. show status icon when switching mode
  // rule 3. always show status icon with tips
  // rule 4. rule 3 excluding tips FullScreenLayout with strings
  return ((_status.ascii_mode && !_style.inline_preedit) ||
          !_status.composing || !_context.aux.empty()) &&
         !((_style.layout_type == UIStyle::LAYOUT_HORIZONTAL_FULLSCREEN ||
            _style.layout_type == UIStyle::LAYOUT_VERTICAL_FULLSCREEN ||
            _style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT_FULLSCREEN) &&
           !_context.aux.empty());
}

bool StandardLayout::_IsHighlightOverCandidateWindow(const CRect& rc) {
  if (!_pD2D || !_pD2D->d2Factory)
    return false;
  ComPtr<ID2D1PathGeometry> bgPathGeometry;
  HR(_pD2D->CreateRoundedRectanglePath(_bgRect, _style.round_corner_ex,
                                       IsToRoundStruct(), bgPathGeometry));
  // check if the center of arcs is out of _bgRect
  auto offset = _style.round_corner * 0.414213562f;
  D2D1_POINT_2F lt = D2D1::Point2F(rc.left + offset, rc.top + offset);
  D2D1_POINT_2F lb = D2D1::Point2F(rc.left + offset, rc.bottom - offset);
  D2D1_POINT_2F rb = D2D1::Point2F(rc.right - offset, rc.bottom - offset);
  D2D1_POINT_2F rt = D2D1::Point2F(rc.right - offset, rc.top + offset);
  static const D2D1_MATRIX_3X2_F wtf = D2D1::Matrix3x2F::Identity();
  static const float flatteningTolerance = 1.0f;
  BOOL inside = FALSE;
  bgPathGeometry->FillContainsPoint(lt, &wtf, flatteningTolerance, &inside);
  if (!inside)
    return true;
  bgPathGeometry->FillContainsPoint(lb, &wtf, flatteningTolerance, &inside);
  if (!inside)
    return true;
  bgPathGeometry->FillContainsPoint(rb, &wtf, flatteningTolerance, &inside);
  if (!inside)
    return true;
  bgPathGeometry->FillContainsPoint(rt, &wtf, flatteningTolerance, &inside);
  if (!inside)
    return true;
  // check if the rc out of _bgRect
  return (rc.left <= _bgRect.left || rc.right >= _bgRect.right ||
          rc.top <= _bgRect.top || rc.bottom >= _bgRect.bottom);
}

void StandardLayout::_PrepareRoundInfo() {
  const int tmp[6] = {
      UIStyle::LAYOUT_VERTICAL,      UIStyle::LAYOUT_HORIZONTAL,
      UIStyle::LAYOUT_VERTICAL_TEXT, UIStyle::LAYOUT_VERTICAL,
      UIStyle::LAYOUT_HORIZONTAL,    UIStyle::LAYOUT_VERTICAL_TEXT};
  int style_layout = _style.layout_type;
  if (style_layout < UIStyle::LAYOUT_VERTICAL ||
      style_layout >= UIStyle::LAYOUT_TYPE_LAST) {
    style_layout = UIStyle::LAYOUT_VERTICAL;
  }
  int layout_type = tmp[style_layout];
  bool textHemispherical = false, cand0Hemispherical = false;
  if (!_style.inline_preedit) {
    CRect textRect(_preeditRect);
    textRect.InflateRect(_style.hilite_padding_x, _style.hilite_padding_y);
    textHemispherical = _IsHighlightOverCandidateWindow(textRect);
    const bool hilite_rd_info[3][2][4] = {
        // vertical
        {{textHemispherical, textHemispherical && (!candidates_count), false,
          false},
         {true, true, true, true}},
        // horizontal
        {{textHemispherical, textHemispherical && (!candidates_count), false,
          false},
         {true, true, true, true}},
        // vertical text
        {{textHemispherical && (!candidates_count), false, textHemispherical,
          false},
         {true, true, true, true}}};

    _textRoundInfo.IsTopLeftNeedToRound =
        hilite_rd_info[layout_type][!textHemispherical][0];
    _textRoundInfo.IsBottomLeftNeedToRound =
        hilite_rd_info[layout_type][!textHemispherical][1];
    _textRoundInfo.IsTopRightNeedToRound =
        hilite_rd_info[layout_type][!textHemispherical][2];
    _textRoundInfo.IsBottomRightNeedToRound =
        hilite_rd_info[layout_type][!textHemispherical][3];
    _textRoundInfo.Hemispherical = textHemispherical;
    if (_style.vertical_text_left_to_right &&
        _style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT) {
      std::swap(_textRoundInfo.IsTopLeftNeedToRound,
                _textRoundInfo.IsTopRightNeedToRound);
    }
  }

  if (candidates_count) {
    CRect cand0Rect(_candidateRects[0]);
    cand0Rect.InflateRect(_style.hilite_padding_x, _style.hilite_padding_y);
    cand0Hemispherical = _IsHighlightOverCandidateWindow(cand0Rect);
    if (textHemispherical || cand0Hemispherical) {
      // if (current_hemispherical_dome_status) _textRoundInfo.Hemispherical =
      // true;
      //  level 0: m_style.layout_type == UIStyle::LAYOUT_HORIZONTAL
      //  level 1: m_style.inline_preedit
      //  level 2: BackType
      //  level 3: IsTopLeftNeedToRound, IsBottomLeftNeedToRound,
      //  IsTopRightNeedToRound, IsBottomRightNeedToRound
      const static bool is_to_round_corner[3][2][5][4] = {
          // LAYOUT_VERTICAL
          {// not inline_preedit
           {
               {false, false, false, false},  // FIRST_CAND
               {false, false, false, false},  // MID_CAND
               {false, true, false, true},    // LAST_CAND
               {false, true, false, true},    // ONLY_CAND
           },
           // inline_preedit
           {
               {true, false, true, false},    // FIRST_CAND
               {false, false, false, false},  // MID_CAND
               {false, true, false, true},    // LAST_CAND
               {true, true, true, true},      // ONLY_CAND
           }},
          // LAYOUT_HORIZONTAL
          {// not inline_preedit
           {
               {false, true, false, false},   // FIRST_CAND
               {false, false, false, false},  // MID_CAND
               {false, false, false, true},   // LAST_CAND
               {false, true, false, true},    // ONLY_CAND
           },
           // inline_preedit
           {
               {true, true, false, false},    // FIRST_CAND
               {false, false, false, false},  // MID_CAND
               {false, false, true, true},    // LAST_CAND
               {true, true, true, true},      // ONLY_CAND
           }},
          // LAYOUT_VERTICAL_TEXT
          {// not inline_preedit
           {
               {false, false, false, false},  // FIRST_CAND
               {false, false, false, false},  // MID_CAND
               {true, true, false, false},    // LAST_CAND
               {false, false, true, true},    // ONLY_CAND
           },
           // inline_preedit
           {
               {false, false, true, true},    // FIRST_CAND
               {false, false, false, false},  // MID_CAND
               {true, true, false, false},    // LAST_CAND
               {true, true, true, true},      // ONLY_CAND
           }},
      };
      for (auto i = 0; i < candidates_count; i++) {
        CRect hilite_rect(_candidateRects[i]);
        hilite_rect.InflateRect(_style.hilite_padding_x,
                                _style.hilite_padding_y);
        bool current_hemispherical_dome_status =
            _IsHighlightOverCandidateWindow(hilite_rect);
        int type = 0;               // default FIRST_CAND
        if (candidates_count == 1)  // ONLY_CAND
          type = 3;
        else if (i != 0 && i != candidates_count - 1)  // MID_CAND
          type = 1;
        else if (i == candidates_count - 1)  // LAST_CAND
          type = 2;
        _roundInfo[i].IsTopLeftNeedToRound =
            is_to_round_corner[layout_type][_style.inline_preedit][type][0];
        _roundInfo[i].IsBottomLeftNeedToRound =
            is_to_round_corner[layout_type][_style.inline_preedit][type][1];
        _roundInfo[i].IsTopRightNeedToRound =
            is_to_round_corner[layout_type][_style.inline_preedit][type][2];
        _roundInfo[i].IsBottomRightNeedToRound =
            is_to_round_corner[layout_type][_style.inline_preedit][type][3];
        _roundInfo[i].Hemispherical = current_hemispherical_dome_status;
      }
      // fix round info for vetical text layout when vertical_text_left_to_right
      // is set
      if (_style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT &&
          _style.vertical_text_left_to_right) {
        if (_style.inline_preedit) {
          if (candidates_count > 1) {
            _roundInfo[candidates_count - 1].IsTopLeftNeedToRound =
                !_roundInfo[candidates_count - 1].IsTopLeftNeedToRound;
            _roundInfo[candidates_count - 1].IsTopRightNeedToRound =
                !_roundInfo[candidates_count - 1].IsTopRightNeedToRound;
            _roundInfo[candidates_count - 1].IsBottomLeftNeedToRound =
                !_roundInfo[candidates_count - 1].IsBottomLeftNeedToRound;
            _roundInfo[candidates_count - 1].IsBottomRightNeedToRound =
                !_roundInfo[candidates_count - 1].IsBottomRightNeedToRound;
            _roundInfo[0].IsTopLeftNeedToRound =
                !_roundInfo[0].IsTopLeftNeedToRound;
            _roundInfo[0].IsTopRightNeedToRound =
                !_roundInfo[0].IsTopRightNeedToRound;
            _roundInfo[0].IsBottomLeftNeedToRound =
                !_roundInfo[0].IsBottomLeftNeedToRound;
            _roundInfo[0].IsBottomRightNeedToRound =
                !_roundInfo[0].IsBottomRightNeedToRound;
          }
        } else {
          if (candidates_count > 1) {
            _roundInfo[candidates_count - 1].IsTopLeftNeedToRound =
                !_roundInfo[candidates_count - 1].IsTopLeftNeedToRound;
            _roundInfo[candidates_count - 1].IsTopRightNeedToRound =
                !_roundInfo[candidates_count - 1].IsTopRightNeedToRound;
            _roundInfo[candidates_count - 1].IsBottomLeftNeedToRound =
                !_roundInfo[candidates_count - 1].IsBottomLeftNeedToRound;
            _roundInfo[candidates_count - 1].IsBottomRightNeedToRound =
                !_roundInfo[candidates_count - 1].IsBottomRightNeedToRound;
          }
        }
      }
    }
  }
}

void StandardLayout::_PrecomputePreeditRects(const CRect& baseRect,
                                             const Text& text,
                                             CRect& beforeRect,
                                             CRect& hiliteRect,
                                             CRect& afterRect) {
  beforeRect = hiliteRect = afterRect = CRect(0, 0, 0, 0);

  if (baseRect.left >= baseRect.right || baseRect.top >= baseRect.bottom ||
      text.str.empty())
    return;

  TextRange range;
  for (size_t j = 0; j < text.attributes.size(); ++j)
    if (text.attributes[j].type == HIGHLIGHTED)
      range = text.attributes[j].range;
  if (range.start >= range.end)
    return;

  CSize beforesz, hilitedsz, aftersz;
  auto& fmt = _pD2D->pPreeditFormat;

  int x = baseRect.left, y = baseRect.top;

  // Before part
  if (range.start > 0) {
    _pD2D->GetTextSize(text.str.substr(0, range.start), range.start, fmt,
                       &beforesz);
    if (beforesz.cx > 0 && beforesz.cy > 0) {
      if (_style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT ||
          _style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT_FULLSCREEN)
        beforeRect = CRect(baseRect.left, y, baseRect.right, y + beforesz.cy);
      else
        beforeRect = CRect(x, baseRect.top, x + beforesz.cx, baseRect.bottom);

      if (_style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT ||
          _style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT_FULLSCREEN)
        y += beforesz.cy + _style.hilite_spacing;
      else
        x += beforesz.cx + _style.hilite_spacing;
    }
  }

  // Highlighted part
  {
    _pD2D->GetTextSize(text.str.substr(range.start, range.end - range.start),
                       range.end - range.start, fmt, &hilitedsz);
    if (hilitedsz.cx > 0 && hilitedsz.cy > 0) {
      if (_style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT ||
          _style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT_FULLSCREEN)
        hiliteRect = CRect(baseRect.left, y, baseRect.right, y + hilitedsz.cy);
      else
        hiliteRect = CRect(x, baseRect.top, x + hilitedsz.cx, baseRect.bottom);

      if (_style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT ||
          _style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT_FULLSCREEN)
        y += hilitedsz.cy + _style.hilite_spacing;
      else
        x += hilitedsz.cx + _style.hilite_spacing;
    }
  }

  // After part
  if (range.end < static_cast<int>(text.str.length())) {
    _pD2D->GetTextSize(text.str.substr(range.end),
                       text.str.length() - range.end, fmt, &aftersz);
    if (aftersz.cx > 0 && aftersz.cy > 0) {
      if (_style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT ||
          _style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT_FULLSCREEN)
        afterRect = CRect(baseRect.left, y, baseRect.right, y + aftersz.cy);
      else
        afterRect = CRect(x, baseRect.top, x + aftersz.cx, baseRect.bottom);
    }
  }
}

int StandardLayout::_CalcMarkMetrics(bool vertical_text_layout) {
  if (!(_style.hilited_mark_color & 0xff000000) || !candidates_count) {
    mark_width = 0;
    mark_height = 0;
    mark_gap = 0;
    return 0;
  }
  CSize sg;
  if (_style.mark_text.empty())
    _pD2D->GetTextSize(L"|", 1, _pD2D->pTextFormat, &sg);
  else
    sg = _markTextSize;
  mark_width = sg.cx;
  mark_height = sg.cy;
  if (_style.mark_text.empty()) {
    if (vertical_text_layout) {
      mark_height = mark_width / 7;
      if (_style.linespacing && _style.baseline)
        mark_height =
            (int)((float)mark_height / ((float)_style.linespacing / 100.0f));
      mark_height = MAX(mark_height, 6);
    } else {
      mark_width = mark_height / 7;
      if (_style.linespacing && _style.baseline)
        mark_width =
            (int)((float)mark_width / ((float)_style.linespacing / 100.0f));
      mark_width = MAX(mark_width, 6);
    }
  }
  mark_gap = (_style.mark_text.empty())
                 ? (vertical_text_layout ? mark_height : mark_width)
                 : (vertical_text_layout ? mark_height : mark_width) +
                       _style.hilite_spacing;
  return mark_gap;
}

// CalcPageIndicator moved to StandardStandardLayout

int StandardLayout::_CalcStatusIconOffset(int extent) const {
  if (!ShouldDisplayStatusIcon())
    return 0;
  return (STATUS_ICON_SIZE >= extent) ? (STATUS_ICON_SIZE - extent) / 2 : 0;
}

void StandardLayout::_LayoutInlineRect(const CSize& size,
                                       bool vertical_text_layout,
                                       bool is_preedit,
                                       int pgw,
                                       int pgh,
                                       int base_coord,
                                       int& width,
                                       int& height,
                                       CRect& rect) {
  if (vertical_text_layout) {
    int szx = is_preedit ? MAX(size.cx, pgw) : size.cx;
    int szy = is_preedit ? pgh : size.cy;
    int xoffset = _CalcStatusIconOffset(szx);
    rect.SetRect(width + xoffset, base_coord, width + xoffset + size.cx,
                 base_coord + size.cy);
    width += size.cx + xoffset * 2 + _style.spacing;
    height = MAX(height, offsetY + real_margin_y + size.cy + szy);
    if (ShouldDisplayStatusIcon() && is_preedit)
      height += STATUS_ICON_SIZE;
  } else {
    int szx = is_preedit ? pgw : size.cx;
    int szy = is_preedit ? MAX(size.cy, pgh) : size.cy;
    int yoffset = _CalcStatusIconOffset(szy);
    rect.SetRect(base_coord, height + yoffset, base_coord + size.cx,
                 height + yoffset + size.cy);
    height += size.cy + 2 * yoffset + _style.spacing;
    width = MAX(width, real_margin_x * 2 + size.cx + (is_preedit ? szx : 0));
    if (ShouldDisplayStatusIcon() && is_preedit)
      width += STATUS_ICON_SIZE;
  }
}
