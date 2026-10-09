#pragma once

#include "StandardLayout.h"

namespace weasel {
class FullScreenLayout : public StandardLayout {
 public:
  FullScreenLayout(const UIStyle& style,
                   const Context& context,
                   const Status& status,
                   const CRect& inputPos,
                   the<Layout> layout,
                   an<D2D>& pD2D)
      : StandardLayout(style, context, status, pD2D),
        mr_inputPos(inputPos),
        m_layout(std::move(layout)) {}

  virtual void DoLayout();

 private:
  bool AdjustFontPoint(const CRect& workArea, int& step);

  const CRect& mr_inputPos;
  the<Layout> m_layout;
};
};  // namespace weasel
