#pragma once

#include "StandardLayout.h"

namespace weasel {
class VHorizontalLayout : public StandardLayout {
 public:
  VHorizontalLayout(const UIStyle& style,
                    const Context& context,
                    const Status& status,
                    an<D2D>& pD2D)
      : StandardLayout(style, context, status, pD2D) {}
  virtual void DoLayout();

 private:
  void DoLayoutWithWrap();
};
};  // namespace weasel
