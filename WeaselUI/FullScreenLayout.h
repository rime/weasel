#pragma once

#include "StandardLayout.h"

namespace weasel {
class FullScreenLayout : public StandardLayout {
 public:
  FullScreenLayout(const UIStyle& style,
                   const Context& context,
                   const Status& status,
                   const CRect& inputPos,
                   Layout* layout,
                   PDWR pDWR)
      : StandardLayout(style, context, status, pDWR),
        mr_inputPos(inputPos),
        m_layout(layout) {}
  virtual ~FullScreenLayout() { delete m_layout; }

  virtual void DoLayout(CDCHandle dc, PDWR pDWR = NULL);

 private:
  bool AdjustFontPoint(CDCHandle dc,
                       const CRect& workArea,
                       int& step,
                       PDWR pDWR = NULL);

  const CRect& mr_inputPos;
  Layout* m_layout;
};
};  // namespace weasel
