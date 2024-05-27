#pragma once

#include "StandardLayout.h"

namespace weasel {
class HorizontalLayout : public StandardLayout {
 public:
  HorizontalLayout(const UIStyle& style,
                   const Context& context,
                   const Status& status,
                   PDWR pDWR)
      : StandardLayout(style, context, status, pDWR) {}
  virtual void DoLayout(CDCHandle dc, PDWR pDWR = NULL);
};
};  // namespace weasel
