#include "Layout.h"
using namespace weasel;

Layout::Layout(const UIStyle& style,
               const Context& context,
               const Status& status,
               an<D2D>& pD2D)
    : _style(style),
      _context(context),
      _status(status),
      candidates(_context.cinfo.candies),
      comments(_context.cinfo.comments),
      labels(_context.cinfo.labels),
      id((_context.cinfo.highlighted >= 0 &&
          _context.cinfo.highlighted <
              MIN((int)_context.cinfo.candies.size(), MAX_CANDIDATES_COUNT))
             ? _context.cinfo.highlighted
             : 0),
      candidates_count(MIN((int)candidates.size(), MAX_CANDIDATES_COUNT)),
      labelFontValid(!!(_style.label_font_point > 0)),
      textFontValid(!!(_style.font_point > 0)),
      _pD2D(pD2D),
      cmtFontValid(!!(_style.comment_font_point > 0)) {
  if (pD2D) {
    float scale = pD2D->m_dpiScaleLayout;
    _style.min_width = (int)(_style.min_width * scale);
    _style.min_height = (int)(_style.min_height * scale);
    _style.max_width = (int)(_style.max_width * scale);
    _style.max_height = (int)(_style.max_height * scale);
    _style.border = (int)(_style.border * scale);
    _style.margin_x = (int)(_style.margin_x * scale);
    _style.margin_y = (int)(_style.margin_y * scale);
    _style.spacing = (int)(_style.spacing * scale);
    _style.candidate_spacing = (int)(_style.candidate_spacing * scale);
    _style.hilite_spacing = (int)(_style.hilite_spacing * scale);
    _style.hilite_padding_x = (int)(_style.hilite_padding_x * scale);
    _style.hilite_padding_y = (int)(_style.hilite_padding_y * scale);
    _style.round_corner = (int)(_style.round_corner * scale);
    _style.round_corner_ex = (int)(_style.round_corner_ex * scale);
    _style.shadow_radius = (int)(_style.shadow_radius * scale);
    _style.shadow_offset_x = (int)(_style.shadow_offset_x * scale);
    _style.shadow_offset_y = (int)(_style.shadow_offset_y * scale);
  }
  real_margin_x = ((abs(_style.margin_x) > _style.hilite_padding_x)
                       ? abs(_style.margin_x)
                       : _style.hilite_padding_x);
  real_margin_y = ((abs(_style.margin_y) > _style.hilite_padding_y)
                       ? abs(_style.margin_y)
                       : _style.hilite_padding_y);
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
