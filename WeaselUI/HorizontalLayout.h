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

 private:
  void LayoutWithCommentOnRight(int& w,
                                int base_offset,
                                weasel::PDWR& pDWR,
                                CSize& size,
                                int& height,
                                int& row_cnt,
                                int& max_width_of_rows,
                                int height_of_rows[100],
                                int row_of_candidate[100],
                                int mintop_of_rows[100]);

  void LayoutWithCommentOnTop(int& w,
                              int base_offset,
                              weasel::PDWR& pDWR,
                              CSize& size,
                              int& height,
                              int& row_cnt,
                              int& max_width_of_rows,
                              int height_of_rows[100],
                              int row_of_candidate[100],
                              int mintop_of_rows[100]);
};
};  // namespace weasel
