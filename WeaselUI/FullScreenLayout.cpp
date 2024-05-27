#include "stdafx.h"
#include "FullScreenLayout.h"

using namespace weasel;

void weasel::FullScreenLayout::DoLayout(CDCHandle dc, PDWR pDWR) {
  if (_context.empty()) {
    int width = 0, height = 0;
    UpdateStatusIconLayout(&width, &height);
    _contentSize.SetSize(width, height);
    return;
  }

  CRect workArea;
  HMONITOR hMonitor = MonitorFromRect(mr_inputPos, MONITOR_DEFAULTTONEAREST);
  if (hMonitor) {
    MONITORINFO info;
    info.cbSize = sizeof(MONITORINFO);
    if (GetMonitorInfo(hMonitor, &info)) {
      workArea = info.rcWork;
    }
  }

  int step = 32;
  do {
    m_layout->DoLayout(dc, pDWR);
    if ((_style.hilited_mark_color & 0xff000000)) {
      CSize sg;
      if (candidates_count) {
        if (_style.mark_text.empty())
          GetTextSizeDW(L"|", 1, pDWR->pTextFormat, pDWR, &sg);
        else
          GetTextSizeDW(_style.mark_text, _style.mark_text.length(),
                        pDWR->pTextFormat, pDWR, &sg);
      }
      mark_width = sg.cx;
      mark_height = sg.cy;
      if (_style.mark_text.empty()) {
        mark_width = mark_height / 7;
        if (_style.linespacing && _style.baseline)
          mark_width =
              (int)((float)mark_width / ((float)_style.linespacing / 100.0f));
        mark_width = max(mark_width, 6);
      }
      mark_gap = (_style.mark_text.empty())
                     ? mark_width
                     : mark_width + _style.hilite_spacing;
    }
  } while (AdjustFontPoint(dc, workArea, step, pDWR));

  int offsetx = (workArea.Width() - m_layout->GetContentSize().cx) / 2;
  int offsety = (workArea.Height() - m_layout->GetContentSize().cy) / 2;
  _preeditRect = m_layout->GetPreeditRect();
  _preeditRect.OffsetRect(offsetx, offsety);
  _auxiliaryRect = m_layout->GetAuxiliaryRect();
  _auxiliaryRect.OffsetRect(offsetx, offsety);
  _highlightRect = m_layout->GetHighlightRect();
  _highlightRect.OffsetRect(offsetx, offsety);

  _prePageRect = m_layout->GetPrepageRect();
  _prePageRect.OffsetRect(offsetx, offsety);
  _nextPageRect = m_layout->GetNextpageRect();
  _nextPageRect.OffsetRect(offsetx, offsety);

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

  _contentSize.SetSize(workArea.Width(), workArea.Height());
  _contentRect.SetRect(0, 0, workArea.Width(), workArea.Height());
  _contentRect.DeflateRect(offsetX, offsetY);
}

bool FullScreenLayout::AdjustFontPoint(CDCHandle dc,
                                       const CRect& workArea,
                                       int& step,
                                       PDWR pDWR) {
  if (_context.empty() || step == 0)
    return false;
  {
    int fontPointLabel;
    int fontPoint;
    int fontPointComment;

    if (pDWR->pLabelTextFormat != NULL)
      fontPointLabel = (int)(pDWR->pLabelTextFormat->GetFontSize() /
                             pDWR->dpiScaleFontPoint);
    else
      fontPointLabel = 0;
    if (pDWR->pTextFormat != NULL)
      fontPoint =
          (int)(pDWR->pTextFormat->GetFontSize() / pDWR->dpiScaleFontPoint);
    else
      fontPoint = 0;
    if (pDWR->pCommentTextFormat != NULL)
      fontPointComment = (int)(pDWR->pCommentTextFormat->GetFontSize() /
                               pDWR->dpiScaleFontPoint);
    else
      fontPointComment = 0;
    CSize sz = m_layout->GetContentSize();
    if (sz.cx > workArea.Width() - offsetX * 2 ||
        sz.cy > workArea.Height() - offsetY * 2) {
      if (step > 0) {
        step = -(step >> 1);
      }
      fontPoint += step;
      fontPointLabel += step;
      fontPointComment += step;
      pDWR->InitResources(_style.label_font_face, fontPointLabel,
                          _style.font_face, fontPoint, _style.comment_font_face,
                          fontPointComment);
      return true;
    } else if (sz.cx <= (workArea.Width() - offsetX * 2) * 31 / 32 &&
               sz.cy <= (workArea.Height() - offsetY * 2) * 31 / 32) {
      if (step < 0) {
        step = -step >> 1;
      }
      fontPoint += step;
      fontPointLabel += step;
      fontPointComment += step;
      pDWR->InitResources(_style.label_font_face, fontPointLabel,
                          _style.font_face, fontPoint, _style.comment_font_face,
                          fontPointComment);
      return true;
    }

    return false;
  }
}
