#include "stdafx.h"
#include "Layout.h"
using namespace weasel;

Layout::Layout(const UIStyle& style,
               const Context& context,
               const Status& status)
    : _style(style),
      _context(context),
      _status(status),
      candidates(_context.cinfo.candies),
      comments(_context.cinfo.comments),
      labels(_context.cinfo.labels),
      id(_context.cinfo.highlighted),
      candidates_count(candidates.size()),
      real_margin_x((abs(_style.margin_x) > _style.hilite_padding_x)
                        ? abs(_style.margin_x)
                        : _style.hilite_padding_x),
      real_margin_y((abs(_style.margin_y) > _style.hilite_padding_y)
                        ? abs(_style.margin_y)
                        : _style.hilite_padding_y),
      labelFontValid(!!(_style.label_font_point > 0)),
      textFontValid(!!(_style.font_point > 0)),
      cmtFontValid(!!(_style.comment_font_point > 0)) {
  offsetX = offsetY = 0;
  if (_style.shadow_radius != 0) {
    offsetX = abs(_style.shadow_offset_x) + _style.shadow_radius * 2;
    offsetY = abs(_style.shadow_offset_y) + _style.shadow_radius * 2;
    if ((_style.shadow_offset_x != 0) || (_style.shadow_offset_y != 0)) {
      offsetX -= _style.shadow_radius / 2;
      offsetY -= _style.shadow_radius / 2;
    }
  }
  offsetX += _style.border * 2;
  offsetY += _style.border * 2;
}

GraphicsRoundRectPath::GraphicsRoundRectPath(const CRect rc,
                                             int corner,
                                             bool roundTopLeft,
                                             bool roundTopRight,
                                             bool roundBottomRight,
                                             bool roundBottomLeft) {
  if (!(roundTopLeft || roundTopRight || roundBottomRight || roundBottomLeft) ||
      corner <= 0) {
    Gdiplus::Rect& rcp =
        Gdiplus::Rect(rc.left, rc.top, rc.Width(), rc.Height());
    AddRectangle(rcp);
  } else {
    int cnx = ((corner * 2 <= rc.Width()) ? corner : (rc.Width() / 2));
    int cny = ((corner * 2 <= rc.Height()) ? corner : (rc.Height() / 2));
    int elWid = 2 * cnx;
    int elHei = 2 * cny;
    AddArc(rc.left, rc.top, elWid * roundTopLeft, elHei * roundTopLeft, 180,
           90);
    AddLine(rc.left + cnx * roundTopLeft, rc.top,
            rc.right - cnx * roundTopRight, rc.top);

    AddArc(rc.right - elWid * roundTopRight, rc.top, elWid * roundTopRight,
           elHei * roundTopRight, 270, 90);
    AddLine(rc.right, rc.top + cny * roundTopRight, rc.right,
            rc.bottom - cny * roundBottomRight);

    AddArc(rc.right - elWid * roundBottomRight,
           rc.bottom - elHei * roundBottomRight, elWid * roundBottomRight,
           elHei * roundBottomRight, 0, 90);
    AddLine(rc.right - cnx * roundBottomRight, rc.bottom,
            rc.left + cnx * roundBottomLeft, rc.bottom);

    AddArc(rc.left, rc.bottom - elHei * roundBottomLeft,
           elWid * roundBottomLeft, elHei * roundBottomLeft, 90, 90);
    AddLine(rc.left, rc.top + cny * roundTopLeft, rc.left,
            rc.bottom - cny * roundBottomLeft);
  }
}

void GraphicsRoundRectPath::AddRoundRect(int left,
                                         int top,
                                         int width,
                                         int height,
                                         int cornerx,
                                         int cornery) {
  if (cornery > 0 && cornerx > 0) {
    int cnx = ((cornerx * 2 <= width) ? cornerx : (width / 2));
    int cny = ((cornery * 2 <= height) ? cornery : (height / 2));
    int elWid = 2 * cnx;
    int elHei = 2 * cny;
    AddArc(left, top, elWid, elHei, 180, 90);
    AddLine(left + cnx, top, left + width - cnx, top);

    AddArc(left + width - elWid, top, elWid, elHei, 270, 90);
    AddLine(left + width, top + cny, left + width, top + height - cny);

    AddArc(left + width - elWid, top + height - elHei, elWid, elHei, 0, 90);
    AddLine(left + width - cnx, top + height, left + cnx, top + height);

    AddArc(left, top + height - elHei, elWid, elHei, 90, 90);
    AddLine(left, top + cny, left, top + height - cny);
  } else {
    Gdiplus::Rect& rc = Gdiplus::Rect(left, top, width, height);
    AddRectangle(rc);
  }
}
