#pragma once

#include "StandardLayout.h"

namespace weasel {
class VerticalLayout : public StandardLayout {
 public:
  VerticalLayout(const UIStyle& style,
                 const Context& context,
                 const Status& status)
      : StandardLayout(style, context, status) {}
  virtual void DoLayout(CDCHandle dc, PDWR pDWR = NULL);
};
};  // namespace weasel
