#pragma once

#include "StandardLayout.h"

namespace weasel {
class VHorizontalLayout : public StandardLayout {
 public:
  VHorizontalLayout(const UIStyle& style,
                    const Context& context,
                    const Status& status,
                    DirectWriteResources* pDWR)
      : StandardLayout(style, context, status, pDWR) {}
  virtual void DoLayout(CDCHandle dc, DirectWriteResources* pDWR = NULL);

 private:
  void DoLayoutWithWrap(CDCHandle dc, DirectWriteResources* pDWR = NULL);
};
};  // namespace weasel
