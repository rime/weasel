#pragma once

#include "StandardLayout.h"

namespace weasel {
class VHorizontalLayout : public StandardLayout {
 public:
  VHorizontalLayout(const UIStyle& style,
                    const Context& context,
                    const Status& status,
                    PDWR pDWR)
      : StandardLayout(style, context, status, pDWR) {}
  virtual void DoLayout(CDCHandle dc, PDWR pDWR = NULL);

 private:
  void DoLayoutWithWrap(CDCHandle dc, PDWR pDWR = NULL);
};
};  // namespace weasel
