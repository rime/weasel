#pragma once

#include <WeaselIPCData.h>
#include <array>
#include <stdexcept>
#include <string>

namespace weasel {

enum ColorFormat { COLOR_ABGR = 0, COLOR_ARGB, COLOR_RGBA };

inline unsigned int ARGB2ABGR(unsigned int value) {
  return (value & 0xff000000) | ((value & 0x000000ff) << 16) |
         (value & 0x0000ff00) | ((value & 0x00ff0000) >> 16);
}

inline unsigned int RGBA2ABGR(unsigned int value) {
  return ((value & 0xff) << 24) | ((value & 0xff000000) >> 24) |
         ((value & 0x00ff0000) >> 8) | ((value & 0x0000ff00) << 8);
}

// Parses a color value into a 32-bit ABGR COLORREF, honoring `fmt`.
// Accepted forms:
//   - "#RGB", "#ARGB", "#RRGGBB", "#AARRGGBB" (hex, case-insensitive)
//   - "0x..." / "0X..." (same as "#...")
//   - a plain decimal integer, e.g. "16777215"
// Returns false when the string is not a valid color value.
inline bool ParseColorValue(const std::string& text,
                            ColorFormat fmt,
                            unsigned int* out) {
  if (!out || text.empty())
    return false;
  size_t start = 0;
  if (text[0] == '#') {
    start = 1;
  } else if (text.size() >= 2 && text[0] == '0' &&
             (text[1] == 'x' || text[1] == 'X')) {
    start = 2;
  }
  const bool hexadecimal = start != 0;
  const std::string digits = text.substr(start);
  if (digits.empty())
    return false;

  unsigned int value = 0;
  if (hexadecimal) {
    for (char c : digits) {
      if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
            (c >= 'A' && c <= 'F')))
        return false;
    }
    if (digits.size() != 3 && digits.size() != 4 && digits.size() != 6 &&
        digits.size() != 8)
      return false;
    std::string expanded = digits;
    if (expanded.size() == 3)
      expanded = std::string(2, expanded[0]) + std::string(2, expanded[1]) +
                 std::string(2, expanded[2]);
    else if (expanded.size() == 4)
      expanded = std::string(2, expanded[0]) + std::string(2, expanded[1]) +
                 std::string(2, expanded[2]) + std::string(2, expanded[3]);
    try {
      value = static_cast<unsigned int>(std::stoul(expanded, nullptr, 16));
    } catch (const std::invalid_argument&) {
      return false;
    } catch (const std::out_of_range&) {
      return false;
    }
    if (expanded.size() == 6) {
      // Short form carries no alpha channel; make it opaque.
      if (fmt != COLOR_RGBA)
        value |= 0xff000000;
      else
        value = (value << 8) | 0x000000ff;
    }
  } else {
    for (char c : digits) {
      if (c < '0' || c > '9')
        return false;
    }
    if (digits.size() > 10)
      return false;
    try {
      value = static_cast<unsigned int>(std::stoul(digits, nullptr, 10));
    } catch (const std::invalid_argument&) {
      return false;
    } catch (const std::out_of_range&) {
      return false;
    }
    if (value <= 0xffffff) {
      // Decimal values with no alpha bits default to opaque.
      if (fmt != COLOR_RGBA)
        value |= 0xff000000;
      else
        value = (value << 8) | 0x000000ff;
    }
  }

  if (fmt == COLOR_ARGB)
    value = ARGB2ABGR(value);
  else if (fmt == COLOR_RGBA)
    value = RGBA2ABGR(value);
  value &= 0xffffffff;
  *out = value;
  return true;
}

// The canonical list of user-editable color keys, in display order.
inline const std::array<const char*, 22>& GetColorKeys() {
  static const std::array<const char*, 22> keys = {
      "back_color",
      "shadow_color",
      "prevpage_color",
      "nextpage_color",
      "text_color",
      "candidate_text_color",
      "candidate_back_color",
      "border_color",
      "hilited_text_color",
      "hilited_back_color",
      "hilited_candidate_text_color",
      "hilited_candidate_back_color",
      "hilited_candidate_shadow_color",
      "hilited_shadow_color",
      "candidate_shadow_color",
      "candidate_border_color",
      "hilited_candidate_border_color",
      "label_color",
      "hilited_label_color",
      "comment_text_color",
      "hilited_comment_text_color",
      "hilited_mark_color"};
  return keys;
}

// Maps a color key to the corresponding UIStyle field, or nullptr when the key
// is unknown. This is the single source of truth for key-to-field resolution.
inline const int* GetStyleColorField(const UIStyle& style,
                                     const std::string& key) {
  if (key == "back_color")
    return &style.back_color;
  if (key == "shadow_color")
    return &style.shadow_color;
  if (key == "prevpage_color")
    return &style.prevpage_color;
  if (key == "nextpage_color")
    return &style.nextpage_color;
  if (key == "text_color")
    return &style.text_color;
  if (key == "candidate_text_color")
    return &style.candidate_text_color;
  if (key == "candidate_back_color")
    return &style.candidate_back_color;
  if (key == "border_color")
    return &style.border_color;
  if (key == "hilited_text_color")
    return &style.hilited_text_color;
  if (key == "hilited_back_color")
    return &style.hilited_back_color;
  if (key == "hilited_candidate_text_color")
    return &style.hilited_candidate_text_color;
  if (key == "hilited_candidate_back_color")
    return &style.hilited_candidate_back_color;
  if (key == "hilited_candidate_shadow_color")
    return &style.hilited_candidate_shadow_color;
  if (key == "hilited_shadow_color")
    return &style.hilited_shadow_color;
  if (key == "candidate_shadow_color")
    return &style.candidate_shadow_color;
  if (key == "candidate_border_color")
    return &style.candidate_border_color;
  if (key == "hilited_candidate_border_color")
    return &style.hilited_candidate_border_color;
  if (key == "label_color")
    return &style.label_text_color;
  if (key == "hilited_label_color")
    return &style.hilited_label_text_color;
  if (key == "comment_text_color")
    return &style.comment_text_color;
  if (key == "hilited_comment_text_color")
    return &style.hilited_comment_text_color;
  if (key == "hilited_mark_color")
    return &style.hilited_mark_color;
  return nullptr;
}

inline int* GetStyleColorField(UIStyle& style, const std::string& key) {
  return const_cast<int*>(
      GetStyleColorField(static_cast<const UIStyle&>(style), key));
}

}  // namespace weasel
