#include "FullScreenLayout.h"

using namespace weasel;

void weasel::FullScreenLayout::DoLayout() {
  if (_context.empty()) {
    int width = 0, height = 0;
    _UpdateStatusIconLayout(&width, &height);
    _contentSize.SetSize(_statusIconRect.right, _statusIconRect.bottom);
    return;
  }

  CRect workArea;
  HMONITOR hMonitor = MonitorFromRect(&mr_inputPos, MONITOR_DEFAULTTONEAREST);
  if (hMonitor) {
    MONITORINFO info;
    info.cbSize = sizeof(MONITORINFO);
    if (GetMonitorInfo(hMonitor, &info)) {
      workArea = info.rcWork;
    }
  }

  int step = 32;
  bool adjusted = false;
  do {
    if (adjusted) {
      // Font changed, recalculate cached sizes
      static_cast<StandardLayout*>(m_layout.get())->RecalculateSizes();
    }
    m_layout->DoLayout();
    adjusted = AdjustFontPoint(workArea, step);
  } while (adjusted);

  mark_height = m_layout->mark_height;
  mark_width = m_layout->mark_width;
  mark_gap = m_layout->mark_gap;
  int offsetx = (workArea.Width() - m_layout->GetContentSize().cx) / 2;
  int offsety = (workArea.Height() - m_layout->GetContentSize().cy) / 2;
  _preeditRect = m_layout->GetPreeditRect();
  _preeditRect.OffsetRect(offsetx, offsety);
  _auxiliaryRect = m_layout->GetAuxiliaryRect();
  _auxiliaryRect.OffsetRect(offsetx, offsety);
  // walkaround to make auxiliary rect center of the ContentRect when auxiliary
  // text is not empty
  if (_style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT_FULLSCREEN &&
      !_context.aux.str.empty()) {
    int aux_center_x = (_auxiliaryRect.left + _auxiliaryRect.right) / 2;
    int aux_center_y = (_auxiliaryRect.top + _auxiliaryRect.bottom) / 2;
    int target_x = workArea.Width() / 2;
    int target_y = workArea.Height() / 2;
    int delta_x = target_x - aux_center_x;
    int delta_y = target_y - aux_center_y;
    _auxiliaryRect.OffsetRect(delta_x, delta_y);
    _auxBeforeRect.OffsetRect(delta_x, delta_y);
    _auxHiliteRect.OffsetRect(delta_x, delta_y);
    _auxAfterRect.OffsetRect(delta_x, delta_y);
    _statusIconRect.SetRect(0, 0, 0, 0);  // hide status icon
  }
  _highlightRect = m_layout->GetHighlightRect();
  _highlightRect.OffsetRect(offsetx, offsety);

  _prePageRect = m_layout->GetPrepageRect();
  _prePageRect.OffsetRect(offsetx, offsety);
  _nextPageRect = m_layout->GetNextpageRect();
  _nextPageRect.OffsetRect(offsetx, offsety);
  _range = m_layout->GetPreeditRange();

  for (auto i = 0, n = (int)_context.cinfo.candies.size();
       i < n && i < MAX_CANDIDATES_COUNT; ++i) {
    _candidateLabelRects[i] = m_layout->GetCandidateLabelRect(i);
    _candidateLabelRects[i].OffsetRect(offsetx, offsety);
    _candidateTextRects[i] = m_layout->GetCandidateTextRect(i);
    _candidateTextRects[i].OffsetRect(offsetx, offsety);
    _candidateCommentRects[i] = m_layout->GetCandidateCommentRect(i);
    _candidateCommentRects[i].OffsetRect(offsetx, offsety);
    _candidateRects[i] = m_layout->GetCandidateRect(i);
    _candidateRects[i].OffsetRect(offsetx, offsety);
  }
  _statusIconRect = m_layout->GetStatusIconRect();
  // VERTICAL_TEXT_FULLSCREEN with non-empty aux hides the icon;
  // only offset if it wasn't hidden.
  if (!_statusIconRect.IsRectNull())
    _statusIconRect.OffsetRect(offsetx, offsety);

  // Get precomputed preedit sub-rectangles from m_layout and apply offset
  _preeditBeforeRect = m_layout->GetPreeditBeforeRect();
  _preeditBeforeRect.OffsetRect(offsetx, offsety);
  _preeditHiliteRect = m_layout->GetPreeditHiliteRect();
  _preeditHiliteRect.OffsetRect(offsetx, offsety);
  _preeditAfterRect = m_layout->GetPreeditAfterRect();
  _preeditAfterRect.OffsetRect(offsetx, offsety);

  // Get precomputed auxiliary sub-rectangles from m_layout and apply offset
  _auxBeforeRect = m_layout->GetAuxBeforeRect();
  _auxBeforeRect.OffsetRect(offsetx, offsety);
  _auxHiliteRect = m_layout->GetAuxHiliteRect();
  _auxHiliteRect.OffsetRect(offsetx, offsety);
  _auxAfterRect = m_layout->GetAuxAfterRect();
  _auxAfterRect.OffsetRect(offsetx, offsety);

  _contentSize.SetSize(workArea.Width(), workArea.Height());
  _contentRect.SetRect(0, 0, workArea.Width(), workArea.Height());
  _contentRect.DeflateRect(offsetX, offsetY);
}

bool FullScreenLayout::AdjustFontPoint(const CRect& workArea, int& step) {
  if (_context.empty() || step == 0)
    return false;
  {
    int fontPointLabel;
    int fontPoint;
    int fontPointComment;

    if (_pD2D->pLabelFormat != NULL)
      fontPointLabel = (int)(_pD2D->pLabelFormat->GetFontSize() /
                             _pD2D->m_dpiScaleFontPoint);
    else
      fontPointLabel = 0;
    if (_pD2D->pTextFormat != NULL)
      fontPoint =
          (int)(_pD2D->pTextFormat->GetFontSize() / _pD2D->m_dpiScaleFontPoint);
    else
      fontPoint = 0;
    if (_pD2D->pCommentFormat != NULL)
      fontPointComment = (int)(_pD2D->pCommentFormat->GetFontSize() /
                               _pD2D->m_dpiScaleFontPoint);
    else
      fontPointComment = 0;
    CSize sz = m_layout->GetContentSize();
    if (sz.cx > ((CRect)workArea).Width() - offsetX * 2 ||
        sz.cy > ((CRect)workArea).Height() - offsetY * 2) {
      if (step > 0) {
        step = -(step >> 1);
      }
      fontPoint += step;
      fontPointLabel += step;
      fontPointComment += step;
      if (fontPoint < 1)
        fontPoint = 1;
      if (fontPointLabel < 1)
        fontPointLabel = 1;
      if (fontPointComment < 1)
        fontPointComment = 1;
      _pD2D->InitFontFormats(_style.label_font_face, fontPointLabel,
                             _style.font_face, fontPoint,
                             _style.comment_font_face, fontPointComment);
      return true;
    } else if (sz.cx <= (((CRect)workArea).Width() - offsetX * 2) * 31 / 32 &&
               sz.cy <= (((CRect)workArea).Height() - offsetY * 2) * 31 / 32) {
      if (step < 0) {
        step = -step >> 1;
      }
      fontPoint += step;
      fontPointLabel += step;
      fontPointComment += step;
      if (fontPoint < 1)
        fontPoint = 1;
      if (fontPointLabel < 1)
        fontPointLabel = 1;
      if (fontPointComment < 1)
        fontPointComment = 1;
      _pD2D->InitFontFormats(_style.label_font_face, fontPointLabel,
                             _style.font_face, fontPoint,
                             _style.comment_font_face, fontPointComment);
      return true;
    }

    return false;
  }
}
