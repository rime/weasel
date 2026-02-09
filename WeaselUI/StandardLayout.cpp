#include "stdafx.h"
#include "StandardLayout.h"

using namespace weasel;

StandardLayout::MarkMetrics StandardLayout::ComputeMarkMetrics() {
  MarkMetrics m;
  if ((_style.hilited_mark_color & 0xff000000)) {
    CSize sg;
    if (candidates_count) {
      if (_style.mark_text.empty())
        GetTextSizeDW(L"|", 1, pDWR_->pTextFormat, &sg);
      else
        GetTextSizeDW(_style.mark_text, _style.mark_text.length(),
                      pDWR_->pTextFormat, &sg);
    }

    m.mark_width = sg.cx;
    m.mark_height = sg.cy;
    if (_style.mark_text.empty()) {
      if (_style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT) {
        m.mark_height = m.mark_width / 7;
        if (_style.linespacing && _style.baseline)
          m.mark_height = (int)((float)m.mark_height /
                                ((float)_style.linespacing / 100.0f));
        m.mark_height = max(m.mark_height, 6);
      } else {
        m.mark_width = m.mark_height / 7;
        if (_style.linespacing && _style.baseline)
          m.mark_width =
              (int)((float)m.mark_width / ((float)_style.linespacing / 100.0f));
        m.mark_width = max(m.mark_width, 6);
      }
    }
    const int base = (_style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT)
                         ? m.mark_height
                         : m.mark_width;
    m.mark_gap =
        (_style.mark_text.empty())
            ? base
            : base + (_style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT
                          ? _style.hilite_spacing
                          : _style.hilite_spacing);
    m.base_offset = m.mark_gap;
  }

  // keep member values in sync for downstream code
  mark_width = m.mark_width;
  mark_height = m.mark_height;
  mark_gap = m.mark_gap;
  return m;
}

StandardLayout::PagerMetrics StandardLayout::ComputePagerMetrics() {
  PagerMetrics pager;
  if (!IsInlinePreedit()) {
    GetTextSizeDW(pre, pre.length(), pDWR_->pPreeditTextFormat, &pager.pgszl);
    GetTextSizeDW(next, next.length(), pDWR_->pPreeditTextFormat, &pager.pgszr);
  }
  pager.page_en = (_style.prevpage_color & 0xff000000) &&
                  (_style.nextpage_color & 0xff000000);
  if (_style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT) {
    pager.pgh = pager.page_en
                    ? pager.pgszl.cy + pager.pgszr.cy + _style.hilite_spacing +
                          _style.hilite_padding_y * 2
                    : 0;
    pager.pgw = pager.page_en ? max(pager.pgszl.cx, pager.pgszr.cx) : 0;
  } else {
    pager.pgw = pager.page_en
                    ? (pager.pgszl.cx + pager.pgszr.cx + _style.hilite_spacing +
                       _style.hilite_padding_x * 2)
                    : 0;
    pager.pgh = pager.page_en ? max(pager.pgszl.cy, pager.pgszr.cy) : 0;
  }
  return pager;
}

void StandardLayout::PlacePagerHorizontal(const PagerMetrics& pager,
                                          int contentWidth,
                                          int contentHeight) {
  if (!pager.page_en || !candidates_count || _style.inline_preedit)
    return;
  int _prex = contentWidth - offsetX - real_margin_x + _style.hilite_padding_x -
              pager.pgw;
  int _prey = (_preeditRect.top + _preeditRect.bottom) / 2 - pager.pgszl.cy / 2;
  _prePageRect.SetRect(_prex, _prey, _prex + pager.pgszl.cx,
                       _prey + pager.pgszl.cy);
  _nextPageRect.SetRect(
      _prePageRect.right + _style.hilite_spacing, _prey,
      _prePageRect.right + _style.hilite_spacing + pager.pgszr.cx,
      _prey + pager.pgszr.cy);
  if (ShouldDisplayStatusIcon()) {
    _prePageRect.OffsetRect(-STATUS_ICON_SIZE, 0);
    _nextPageRect.OffsetRect(-STATUS_ICON_SIZE, 0);
  }
}

void StandardLayout::PlacePagerVerticalText(const PagerMetrics& pager,
                                            int contentWidth,
                                            int contentHeight) {
  if (!pager.page_en || !candidates_count || _style.inline_preedit)
    return;
  int _prey = contentHeight - offsetY - real_margin_y +
              _style.hilite_padding_y - pager.pgh;
  int _prex = (_preeditRect.left + _preeditRect.right) / 2 - pager.pgszl.cx / 2;
  _prePageRect.SetRect(_prex, _prey, _prex + pager.pgszl.cx,
                       _prey + pager.pgszl.cy);
  _nextPageRect.SetRect(
      _prex, _prePageRect.bottom + _style.hilite_spacing,
      _prex + pager.pgszr.cx,
      _prePageRect.bottom + _style.hilite_spacing + pager.pgszr.cy);
  if (ShouldDisplayStatusIcon()) {
    _prePageRect.OffsetRect(0, -STATUS_ICON_SIZE);
    _nextPageRect.OffsetRect(0, -STATUS_ICON_SIZE);
  }
}

void StandardLayout::ReAdjustAlignment(size_t limit,
                                       int index,
                                       bool alignToEnd,
                                       bool endIsEnd) {
  int ol = 0, ot = 0, oc = 0;
  bool isVerticalText = (_style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT);
  size_t textSpan = isVerticalText ? _candidateTextRects[index].Width()
                                   : _candidateTextRects[index].Height();
  size_t labelSpan = isVerticalText ? _candidateLabelRects[index].Width()
                                    : _candidateLabelRects[index].Height();
  size_t commentSpan = isVerticalText ? _candidateCommentRects[index].Width()
                                      : _candidateCommentRects[index].Height();

  const bool alignCenter = _style.align_type == UIStyle::ALIGN_CENTER;
  const bool alignEnd =
      ((endIsEnd && _style.align_type == UIStyle::ALIGN_BOTTOM) || alignToEnd);

  if (alignCenter) {
    ol = static_cast<int>((limit - labelSpan) / 2);
    ot = static_cast<int>((limit - textSpan) / 2);
    oc = static_cast<int>((limit - commentSpan) / 2);
  } else if (alignEnd) {
    ol = static_cast<int>(limit - labelSpan);
    ot = static_cast<int>(limit - textSpan);
    oc = static_cast<int>(limit - commentSpan);
  }

  int factor = isVerticalText ? 1 : 0;
  _candidateLabelRects[index].OffsetRect(ol * factor, ol * (1 - factor));
  _candidateTextRects[index].OffsetRect(ot * factor, ot * (1 - factor));
  _candidateCommentRects[index].OffsetRect(oc * factor, oc * (1 - factor));
}

std::wstring StandardLayout::GetLabelText(const std::vector<Text>& labels,
                                          int id,
                                          const wchar_t* format) const {
  wchar_t buffer[128];
  swprintf_s<128>(buffer, format, labels.at(id).str.c_str());
  return std::wstring(buffer);
}

void weasel::StandardLayout::GetTextSizeDW(
    const std::wstring& text,
    size_t nCount,
    ComPtr<IDWriteTextFormat1>& pTextFormat,
    LPSIZE lpSize) const {
  D2D1_SIZE_F sz;
  HRESULT hr = S_OK;

  if (pTextFormat == NULL) {
    lpSize->cx = 0;
    lpSize->cy = 0;
    return;
  }
  // 创建文本布局
  if (_style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT)
    HR(pDWR_->CreateTextLayout(text.c_str(), (int)nCount, pTextFormat.Get(),
                               0.0f, (float)_style.max_height));
  else
    HR(pDWR_->CreateTextLayout(text.c_str(), (int)nCount, pTextFormat.Get(),
                               (float)_style.max_width, 0));

  if (_style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT) {
    DWRITE_FLOW_DIRECTION flow = _style.vertical_text_left_to_right
                                     ? DWRITE_FLOW_DIRECTION_LEFT_TO_RIGHT
                                     : DWRITE_FLOW_DIRECTION_RIGHT_TO_LEFT;
    HR(pDWR_->SetLayoutReadingDirection(
        DWRITE_READING_DIRECTION_TOP_TO_BOTTOM));
    HR(pDWR_->SetLayoutFlowDirection(flow));
  }
  // 获取文本尺寸
  DWRITE_TEXT_METRICS textMetrics;
  HR(pDWR_->GetLayoutMetrics(&textMetrics));
  sz = D2D1::SizeF(ceil(textMetrics.widthIncludingTrailingWhitespace),
                   ceil(textMetrics.height));

  lpSize->cx = (int)sz.width;
  lpSize->cy = (int)sz.height;

  if (_style.layout_type != UIStyle::LAYOUT_VERTICAL_TEXT) {
    auto max_width = _style.max_width == 0
                         ? textMetrics.widthIncludingTrailingWhitespace
                         : _style.max_width;
    HR(pDWR_->CreateTextLayout(text.c_str(), (int)nCount, pTextFormat.Get(),
                               max_width, textMetrics.height));
  } else {
    auto max_height =
        _style.max_height == 0 ? textMetrics.height : _style.max_height;
    HR(pDWR_->CreateTextLayout(text.c_str(), (int)nCount, pTextFormat.Get(),
                               textMetrics.widthIncludingTrailingWhitespace,
                               max_height));
  }

  if (_style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT) {
    HR(pDWR_->SetLayoutReadingDirection(
        DWRITE_READING_DIRECTION_TOP_TO_BOTTOM));
    HR(pDWR_->SetLayoutFlowDirection(DWRITE_FLOW_DIRECTION_RIGHT_TO_LEFT));
  }
  DWRITE_OVERHANG_METRICS overhangMetrics;
  HR(pDWR_->GetLayoutOverhangMetrics(&overhangMetrics));
  {
    if (overhangMetrics.left > 0)
      lpSize->cx += (LONG)(overhangMetrics.left + 1);
    if (overhangMetrics.right > 0)
      lpSize->cx += (LONG)(overhangMetrics.right + 1);
    if (overhangMetrics.top > 0)
      lpSize->cy += (LONG)(overhangMetrics.top + 1);
    if (overhangMetrics.bottom > 0)
      lpSize->cy += (LONG)(overhangMetrics.bottom + 1);
  }
}

CSize StandardLayout::GetPreeditSize(CDCHandle dc,
                                     const weasel::Text& text,
                                     ComPtr<IDWriteTextFormat1> pTextFormat) {
  const std::wstring& preedit = text.str;
  const std::vector<weasel::TextAttribute>& attrs = text.attributes;
  CSize size(0, 0);
  if (!preedit.empty()) {
    weasel::TextRange range;
    for (size_t j = 0; j < attrs.size(); ++j)
      if (attrs[j].type == weasel::HIGHLIGHTED)
        _range = attrs[j].range;
    if (_range.start < _range.end) {
      std::wstring before_str = preedit.substr(0, _range.start);
      std::wstring hilited_str = preedit.substr(_range.start, _range.end);
      std::wstring after_str = preedit.substr(_range.end);
      GetTextSizeDW(before_str, before_str.length(), pTextFormat, &_beforesz);
      GetTextSizeDW(hilited_str, hilited_str.length(), pTextFormat,
                    &_hilitedsz);
      GetTextSizeDW(after_str, after_str.length(), pTextFormat, &_aftersz);
      auto width_max = 0, height_max = 0;
      if (_style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT) {
        width_max = max(width_max, _beforesz.cx);
        width_max = max(width_max, _hilitedsz.cx);
        width_max = max(width_max, _aftersz.cx);
        height_max += _beforesz.cy + (_beforesz.cy > 0) * _style.hilite_spacing;
        height_max +=
            _hilitedsz.cy + (_hilitedsz.cy > 0) * _style.hilite_spacing;
        height_max += _aftersz.cy;
      } else {
        height_max = max(height_max, _beforesz.cy);
        height_max = max(height_max, _hilitedsz.cy);
        height_max = max(height_max, _aftersz.cy);
        width_max += _beforesz.cx + (_beforesz.cx > 0) * _style.hilite_spacing;
        width_max +=
            _hilitedsz.cx + (_hilitedsz.cx > 0) * _style.hilite_spacing;
        width_max += _aftersz.cx;
      }
      size.cx = width_max;
      size.cy = height_max;
    } else
      GetTextSizeDW(preedit, preedit.length(), pTextFormat, &size);
  }
  return size;
}

// check if a candidate back path over _bgRect path
bool weasel::StandardLayout::_IsHighlightOverCandidateWindow(CRect& rc,
                                                             CDCHandle& dc) {
  GraphicsRoundRectPath bgPath(_bgRect, _style.round_corner_ex);
  GraphicsRoundRectPath hlPath(rc, _style.round_corner);

  Gdiplus::Region bgRegion(&bgPath);
  Gdiplus::Region hlRegion(&hlPath);
  Gdiplus::Region* tmpRegion = hlRegion.Clone();

  if (!tmpRegion)
    return false;
  tmpRegion->Xor(&bgRegion);
  tmpRegion->Exclude(&bgRegion);

  Gdiplus::Graphics g(dc);
  bool res = !tmpRegion->IsEmpty(&g);
  delete tmpRegion;
  tmpRegion = NULL;
  return res;
}

// prepare Hemispherical rounding info
void weasel::StandardLayout::_PrepareRoundInfo(CDCHandle& dc) {
  const int tmp[5] = {UIStyle::LAYOUT_VERTICAL, UIStyle::LAYOUT_HORIZONTAL,
                      UIStyle::LAYOUT_VERTICAL_TEXT, UIStyle::LAYOUT_VERTICAL,
                      UIStyle::LAYOUT_HORIZONTAL};
  int layout_type = tmp[_style.layout_type];
  bool textHemispherical = false, cand0Hemispherical = false;
  if (!_style.inline_preedit) {
    CRect textRect(_preeditRect);
    textRect.InflateRect(_style.hilite_padding_x, _style.hilite_padding_y);
    textHemispherical = _IsHighlightOverCandidateWindow(textRect, dc);
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
    cand0Hemispherical = _IsHighlightOverCandidateWindow(cand0Rect, dc);
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
            _IsHighlightOverCandidateWindow(hilite_rect, dc);
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

void StandardLayout::UpdateStatusIconLayout(int* width, int* height) {
  // rule 1. status icon is middle-aligned with preedit text or auxiliary text,
  // whichever comes first rule 2. there is a spacing between preedit/aux text
  // and the status icon rule 3. status icon is right aligned in WeaselPanel,
  // when [margin_x + width(preedit/aux) + spacing + width(icon) + margin_x] <
  // style.min_width
  if (ShouldDisplayStatusIcon()) {
    if (_style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT) {
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
                                middle + STATUS_ICON_SIZE,
                                top + STATUS_ICON_SIZE / 2 + 1);
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
            _style.layout_type == UIStyle::LAYOUT_VERTICAL_FULLSCREEN) &&
           !_context.aux.empty());
}
