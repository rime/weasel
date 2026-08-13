#include "stdafx.h"
#include <WeaselUtility.h>
#include <RimeWithWeasel.h>
#include <WeaselStyleColor.h>
#include "UIStyleSettings.h"
#include <cstdlib>

UIStyleSettings::UIStyleSettings() {
  api_ = (RimeLeversApi*)rime_get_api()->find_module("levers")->get_api();
  settings_ = api_->custom_settings_init("weasel", "Weasel::UIStyleSettings");
  InitFontSettings();
}

UIStyleSettings::~UIStyleSettings() {
  if (api_ && settings_)
    api_->custom_settings_destroy(settings_);
}

void UIStyleSettings::InitFontSettings() {
  weasel::UIStyle style;
  if (LoadPreviewStyle(&style)) {
    font_face = style.font_face;
    label_font_face = style.label_font_face;
    comment_font_face = style.comment_font_face;
    font_point = style.font_point;
    label_font_point = style.label_font_point;
    comment_font_point = style.comment_font_point;
  }

  if (font_face.empty()) {
    RimeConfig config = {0};
    RimeApi* rime = rime_get_api();
    if (rime && rime->config_open("weasel", &config)) {
      const auto read_font = [&](const char* key, std::wstring* value) {
        char buffer[4096] = {0};
        if (rime->config_get_string(&config, key, buffer, _countof(buffer)))
          *value = u8tow(buffer);
      };
      read_font("style/font_face", &font_face);
      read_font("style/label_font_face", &label_font_face);
      read_font("style/comment_font_face", &comment_font_face);
      rime->config_get_int(&config, "style/font_point", &font_point);
      if (!rime->config_get_int(&config, "style/label_font_point",
                                &label_font_point))
        label_font_point = font_point;
      if (!rime->config_get_int(&config, "style/comment_font_point",
                                &comment_font_point))
        comment_font_point = font_point;
      rime->config_close(&config);
    }
  }
  if (font_face.empty())
    font_face = L"Microsoft YaHei";
  if (label_font_face.empty())
    label_font_face = font_face;
  if (comment_font_face.empty())
    comment_font_face = font_face;

  original_font_face_ = font_face;
  original_label_font_face_ = label_font_face;
  original_comment_font_face_ = comment_font_face;
  original_font_point_ = font_point;
  original_label_font_point_ = label_font_point;
  original_comment_font_point_ = comment_font_point;
}

bool UIStyleSettings::GetPresetColorSchemes(
    std::vector<ColorSchemeInfo>* result) {
  if (!result)
    return false;
  result->clear();
  RimeConfig config = {0};
  api_->settings_get_config(settings_, &config);
  RimeApi* rime = rime_get_api();
  RimeConfigIterator preset = {0};
  if (!rime->config_begin_map(&preset, &config, "preset_color_schemes"))
    return false;
  while (rime->config_next(&preset)) {
    std::string name_key(preset.path);
    name_key += "/name";
    const char* name = rime->config_get_cstring(&config, name_key.c_str());
    std::string author_key(preset.path);
    author_key += "/author";
    const char* author = rime->config_get_cstring(&config, author_key.c_str());
    if (!name)
      continue;
    ColorSchemeInfo info;
    info.color_scheme_id = preset.key;
    info.name = name;
    if (author)
      info.author = author;
    result->push_back(info);
  }
  return true;
}

bool UIStyleSettings::GetColorSettings(const std::string& color_scheme_id,
                                       std::vector<ColorSettingInfo>* result) {
  if (!result)
    return false;
  result->clear();
  RimeConfig config = {0};
  if (!api_->settings_get_config(settings_, &config))
    return false;
  RimeApi* rime = rime_get_api();
  const std::string prefix = "preset_color_schemes/" + color_scheme_id + "/";
  for (const char* key : weasel::GetColorKeys()) {
    ColorSettingInfo item{key, std::string(), false};
    std::string path = prefix + key;
    char value[256] = {0};
    if (rime->config_get_string(&config, path.c_str(), value, sizeof(value))) {
      item.value = value;
      item.defined = true;
    } else {
      int numeric = 0;
      if (rime->config_get_int(&config, path.c_str(), &numeric)) {
        item.value = std::to_string(numeric);
        item.defined = true;
      }
    }
    result->push_back(std::move(item));
  }
  auto overrides = preview_color_overrides_.find(color_scheme_id);
  if (overrides != preview_color_overrides_.end()) {
    for (auto& item : *result) {
      auto value = overrides->second.find(item.key);
      if (value != overrides->second.end()) {
        item.value = value->second;
        item.defined = true;
      }
    }
  }
  std::sort(result->begin(), result->end(),
            [](const ColorSettingInfo& left, const ColorSettingInfo& right) {
              return left.key < right.key;
            });
  return true;
}

bool UIStyleSettings::SetColorSetting(const std::string& color_scheme_id,
                                      const std::string& key,
                                      const std::string& value) {
  preview_color_scheme_ = color_scheme_id;
  auto& overrides = preview_color_overrides_[color_scheme_id];
  if (value.empty()) {
    overrides.erase(key);
    if (overrides.empty())
      preview_color_overrides_.erase(color_scheme_id);
  } else {
    overrides[key] = value;
  }
  return true;
}

bool UIStyleSettings::ApplyColorPatches(
    const std::map<std::string, std::vector<ColorSettingInfo>>& originals,
    const std::string& original_color_scheme) {
  if (!preview_color_scheme_.empty() &&
      preview_color_scheme_ != original_color_scheme &&
      !api_->customize_string(settings_, "style/color_scheme",
                              preview_color_scheme_.c_str()))
    return false;
  for (const auto& scheme : preview_color_overrides_) {
    auto baseline = originals.find(scheme.first);
    if (baseline == originals.end())
      continue;
    for (const auto& override_value : scheme.second) {
      auto original =
          std::find_if(baseline->second.begin(), baseline->second.end(),
                       [&](const ColorSettingInfo& item) {
                         return item.key == override_value.first;
                       });
      if (original == baseline->second.end() ||
          (original->defined && original->value == override_value.second))
        continue;
      std::string path =
          "preset_color_schemes/" + scheme.first + "/" + override_value.first;
      if (!api_->customize_string(settings_, path.c_str(),
                                  override_value.second.c_str()))
        return false;
    }
  }
  return true;
}

std::string UIStyleSettings::GetActiveColorScheme() {
  RimeConfig config = {0};
  api_->settings_get_config(settings_, &config);
  const char* value =
      rime_get_api()->config_get_cstring(&config, "style/color_scheme");
  return value ? std::string(value) : std::string();
}

std::string UIStyleSettings::GetDarkColorScheme() {
  RimeConfig config = {0};
  api_->settings_get_config(settings_, &config);
  const char* value =
      rime_get_api()->config_get_cstring(&config, "style/color_scheme_dark");
  return value ? std::string(value) : std::string();
}

namespace {
// Defined below; applies an edited value string onto a resolved UIStyle.
bool SetStyleFieldText(weasel::UIStyle& style,
                       const std::string& key,
                       const std::string& value);
}  // namespace

bool UIStyleSettings::SelectColorScheme(const std::string& color_scheme_id) {
  preview_color_scheme_ = color_scheme_id;
  return true;
}

bool UIStyleSettings::SetFontFace(const std::string& key,
                                  const std::string& font_face_value) {
  std::wstring* current = nullptr;
  const std::wstring* original = nullptr;
  if (key == "style/font_face") {
    current = &font_face;
    original = &original_font_face_;
  } else if (key == "style/label_font_face") {
    current = &label_font_face;
    original = &original_label_font_face_;
  } else if (key == "style/comment_font_face") {
    current = &comment_font_face;
    original = &original_comment_font_face_;
  } else {
    return false;
  }

  const std::wstring value = u8tow(font_face_value);
  *current = value;
  if (value == *original)
    preview_style_overrides_.erase(key);
  else
    preview_style_overrides_[key] = font_face_value;
  return true;
}

bool UIStyleSettings::SetFontPoint(const std::string& key,
                                   int font_point_value) {
  int* current = nullptr;
  int original = 0;
  if (key == "style/font_point") {
    current = &font_point;
    original = original_font_point_;
  } else if (key == "style/label_font_point") {
    current = &label_font_point;
    original = original_label_font_point_;
  } else if (key == "style/comment_font_point") {
    current = &comment_font_point;
    original = original_comment_font_point_;
  } else {
    return false;
  }

  *current = font_point_value;
  if (font_point_value == original)
    preview_style_overrides_.erase(key);
  else
    preview_style_overrides_[key] = std::to_string(font_point_value);
  return true;
}

bool UIStyleSettings::LoadPreviewStyle(weasel::UIStyle* style) const {
  if (!style)
    return false;
  RimeConfig config = {0};
  if (!api_->settings_get_config(settings_, &config))
    return false;
  LoadWeaselUIStyle(&config, *style, true, preview_color_scheme_);
  const auto apply_font_override = [&](const char* key, std::wstring* face,
                                       int* point) {
    const auto override_value = preview_style_overrides_.find(key);
    if (override_value == preview_style_overrides_.end())
      return;
    if (face)
      *face = u8tow(override_value->second);
    else if (point)
      *point = std::stoi(override_value->second);
  };
  apply_font_override("style/font_face", &style->font_face, nullptr);
  apply_font_override("style/label_font_face", &style->label_font_face,
                      nullptr);
  apply_font_override("style/comment_font_face", &style->comment_font_face,
                      nullptr);
  apply_font_override("style/font_point", nullptr, &style->font_point);
  apply_font_override("style/label_font_point", nullptr,
                      &style->label_font_point);
  apply_font_override("style/comment_font_point", nullptr,
                      &style->comment_font_point);
  const auto scheme = preview_color_overrides_.find(preview_color_scheme_);
  if (scheme != preview_color_overrides_.end()) {
    weasel::ColorFormat format = weasel::COLOR_ABGR;
    std::string format_path =
        "preset_color_schemes/" + preview_color_scheme_ + "/color_format";
    char format_buffer[32] = {0};
    if (rime_get_api()->config_get_string(&config, format_path.c_str(),
                                          format_buffer,
                                          sizeof(format_buffer))) {
      const std::string format_str = format_buffer;
      if (format_str == "argb")
        format = weasel::COLOR_ARGB;
      else if (format_str == "rgba")
        format = weasel::COLOR_RGBA;
    }
    for (const auto& override_value : scheme->second) {
      unsigned int parsed = 0;
      if (weasel::ParseColorValue(override_value.second, format, &parsed)) {
        if (int* field =
                weasel::GetStyleColorField(*style, override_value.first))
          *field = static_cast<int>(parsed);
      }
    }
  }
  // Apply in-session style-key edits so the preview reflects unsaved changes.
  for (const auto& override_value : preview_style_overrides_) {
    if (override_value.first == "style/color_scheme_dark")
      continue;  // dark scheme does not affect the active preview
    SetStyleFieldText(*style, override_value.first, override_value.second);
  }
  return true;
}

namespace {

std::string LayoutTypeToString(weasel::UIStyle::LayoutType type) {
  switch (type) {
    case weasel::UIStyle::LAYOUT_HORIZONTAL:
      return "horizontal";
    case weasel::UIStyle::LAYOUT_VERTICAL_TEXT:
      return "vertical_text";
    case weasel::UIStyle::LAYOUT_VERTICAL_FULLSCREEN:
      return "vertical+fullscreen";
    case weasel::UIStyle::LAYOUT_HORIZONTAL_FULLSCREEN:
      return "horizontal+fullscreen";
    case weasel::UIStyle::LAYOUT_VERTICAL_TEXT_FULLSCREEN:
      return "vertical_text+fullscreen";
    case weasel::UIStyle::LAYOUT_VERTICAL:
    default:
      return "vertical";
  }
}

// Extract the effective value of a style key from a resolved UIStyle. This
// keeps the displayed value consistent with the live preview, including alias
// resolution (e.g. style/horizontal -> layout_type).
bool GetStyleFieldText(const weasel::UIStyle& style,
                       const std::string& key,
                       std::string* out) {
  if (key == "style/candidate_abbreviate_length")
    *out = std::to_string(style.candidate_abbreviate_length);
  else if (key == "style/inline_preedit")
    *out = style.inline_preedit ? "true" : "false";
  else if (key == "style/display_tray_icon")
    *out = style.display_tray_icon ? "true" : "false";
  else if (key == "style/ascii_tip_follow_cursor")
    *out = style.ascii_tip_follow_cursor ? "true" : "false";
  else if (key == "style/paging_on_scroll")
    *out = style.paging_on_scroll ? "true" : "false";
  else if (key == "style/click_to_capture")
    *out = style.click_to_capture ? "true" : "false";
  else if (key == "style/enhanced_position")
    *out = style.enhanced_position ? "true" : "false";
  else if (key == "style/vertical_auto_reverse")
    *out = style.vertical_auto_reverse ? "true" : "false";
  else if (key == "style/vertical_text_left_to_right")
    *out = style.vertical_text_left_to_right ? "true" : "false";
  else if (key == "style/vertical_text_with_wrap")
    *out = style.vertical_text_with_wrap ? "true" : "false";
  else if (key == "style/vertical_right_to_left")
    *out = style.vertical_right_to_left ? "true" : "false";
  else if (key == "style/label_format")
    *out = wtou8(style.label_text_format);
  else if (key == "style/mark_text")
    *out = wtou8(style.mark_text);
  else if (key == "style/preedit_type") {
    switch (style.preedit_type) {
      case weasel::UIStyle::PREVIEW:
        *out = "preview";
        break;
      case weasel::UIStyle::PREVIEW_ALL:
        *out = "preview_all";
        break;
      case weasel::UIStyle::COMPOSITION:
      default:
        *out = "composition";
        break;
    }
  } else if (key == "style/antialias_mode") {
    switch (style.antialias_mode) {
      case weasel::UIStyle::CLEARTYPE:
        *out = "cleartype";
        break;
      case weasel::UIStyle::GRAYSCALE:
        *out = "grayscale";
        break;
      case weasel::UIStyle::ALIASED:
        *out = "aliased";
        break;
      case weasel::UIStyle::FORCE_DWORD:
        *out = "force_dword";
        break;
      case weasel::UIStyle::DEFAULT:
      default:
        *out = "default";
        break;
    }
  } else if (key == "style/hover_type") {
    switch (style.hover_type) {
      case weasel::UIStyle::SEMI_HILITE:
        *out = "semi_hilite";
        break;
      case weasel::UIStyle::HILITE:
        *out = "hilite";
        break;
      case weasel::UIStyle::NONE:
      default:
        *out = "none";
        break;
    }
  } else if (key == "style/layout/type") {
    *out = LayoutTypeToString(style.layout_type);
  } else if (key == "style/layout/align_type") {
    switch (style.align_type) {
      case weasel::UIStyle::ALIGN_TOP:
        *out = "top";
        break;
      case weasel::UIStyle::ALIGN_CENTER:
        *out = "center";
        break;
      case weasel::UIStyle::ALIGN_BOTTOM:
      default:
        *out = "bottom";
        break;
    }
  } else if (key == "style/layout/border") {
    *out = std::to_string(style.border);
  } else if (key == "style/layout/margin_x") {
    *out = std::to_string(style.margin_x);
  } else if (key == "style/layout/margin_y") {
    *out = std::to_string(style.margin_y);
  } else if (key == "style/layout/spacing") {
    *out = std::to_string(style.spacing);
  } else if (key == "style/layout/candidate_spacing") {
    *out = std::to_string(style.candidate_spacing);
  } else if (key == "style/layout/hilite_spacing") {
    *out = std::to_string(style.hilite_spacing);
  } else if (key == "style/layout/hilite_padding_x") {
    *out = std::to_string(style.hilite_padding_x);
  } else if (key == "style/layout/hilite_padding_y") {
    *out = std::to_string(style.hilite_padding_y);
  } else if (key == "style/layout/hilited_corner_radius") {
    *out = std::to_string(style.round_corner);
  } else if (key == "style/layout/corner_radius") {
    *out = std::to_string(style.round_corner_ex);
  } else if (key == "style/layout/shadow_radius") {
    *out = std::to_string(style.shadow_radius);
  } else if (key == "style/layout/shadow_offset_x") {
    *out = std::to_string(style.shadow_offset_x);
  } else if (key == "style/layout/shadow_offset_y") {
    *out = std::to_string(style.shadow_offset_y);
  } else if (key == "style/layout/min_width") {
    *out = std::to_string(style.min_width);
  } else if (key == "style/layout/max_width") {
    *out = std::to_string(style.max_width);
  } else if (key == "style/layout/min_height") {
    *out = std::to_string(style.min_height);
  } else if (key == "style/layout/max_height") {
    *out = std::to_string(style.max_height);
  } else if (key == "style/layout/baseline") {
    *out = std::to_string(style.baseline);
  } else if (key == "style/layout/linespacing") {
    *out = std::to_string(style.linespacing);
  } else {
    return false;
  }
  return true;
}

// Inverse of GetStyleFieldText: apply an edited value string onto a resolved
// UIStyle so the live preview reflects unsaved style edits.
bool SetStyleFieldText(weasel::UIStyle& style,
                       const std::string& key,
                       const std::string& value) {
  const bool truthy = (value == "true");

  auto to_int = [&](int* target) {
    char* end = nullptr;
    const long parsed = strtol(value.c_str(), &end, 10);
    if (end == value.c_str() || *end != '\0')
      return false;
    *target = static_cast<int>(parsed);
    return true;
  };

  if (key == "style/candidate_abbreviate_length")
    return to_int(&style.candidate_abbreviate_length);
  else if (key == "style/inline_preedit")
    style.inline_preedit = truthy;
  else if (key == "style/display_tray_icon")
    style.display_tray_icon = truthy;
  else if (key == "style/ascii_tip_follow_cursor")
    style.ascii_tip_follow_cursor = truthy;
  else if (key == "style/paging_on_scroll")
    style.paging_on_scroll = truthy;
  else if (key == "style/click_to_capture")
    style.click_to_capture = truthy;
  else if (key == "style/enhanced_position")
    style.enhanced_position = truthy;
  else if (key == "style/vertical_auto_reverse")
    style.vertical_auto_reverse = truthy;
  else if (key == "style/vertical_text_left_to_right")
    style.vertical_text_left_to_right = truthy;
  else if (key == "style/vertical_text_with_wrap")
    style.vertical_text_with_wrap = truthy;
  else if (key == "style/vertical_right_to_left")
    style.vertical_right_to_left = truthy;
  else if (key == "style/label_format")
    style.label_text_format = u8tow(value);
  else if (key == "style/mark_text")
    style.mark_text = u8tow(value);
  else if (key == "style/preedit_type") {
    if (value == "preview")
      style.preedit_type = weasel::UIStyle::PREVIEW;
    else if (value == "preview_all")
      style.preedit_type = weasel::UIStyle::PREVIEW_ALL;
    else
      style.preedit_type = weasel::UIStyle::COMPOSITION;
  } else if (key == "style/antialias_mode") {
    if (value == "cleartype")
      style.antialias_mode = weasel::UIStyle::CLEARTYPE;
    else if (value == "grayscale")
      style.antialias_mode = weasel::UIStyle::GRAYSCALE;
    else if (value == "aliased")
      style.antialias_mode = weasel::UIStyle::ALIASED;
    else if (value == "force_dword")
      style.antialias_mode = weasel::UIStyle::FORCE_DWORD;
    else
      style.antialias_mode = weasel::UIStyle::DEFAULT;
  } else if (key == "style/hover_type") {
    if (value == "semi_hilite")
      style.hover_type = weasel::UIStyle::SEMI_HILITE;
    else if (value == "hilite")
      style.hover_type = weasel::UIStyle::HILITE;
    else
      style.hover_type = weasel::UIStyle::NONE;
  } else if (key == "style/layout/type") {
    if (value == "horizontal")
      style.layout_type = weasel::UIStyle::LAYOUT_HORIZONTAL;
    else if (value == "vertical_text")
      style.layout_type = weasel::UIStyle::LAYOUT_VERTICAL_TEXT;
    else if (value == "vertical+fullscreen")
      style.layout_type = weasel::UIStyle::LAYOUT_VERTICAL_FULLSCREEN;
    else if (value == "horizontal+fullscreen")
      style.layout_type = weasel::UIStyle::LAYOUT_HORIZONTAL_FULLSCREEN;
    else if (value == "vertical_text+fullscreen")
      style.layout_type = weasel::UIStyle::LAYOUT_VERTICAL_TEXT_FULLSCREEN;
    else
      style.layout_type = weasel::UIStyle::LAYOUT_VERTICAL;
  } else if (key == "style/layout/align_type") {
    if (value == "top")
      style.align_type = weasel::UIStyle::ALIGN_TOP;
    else if (value == "center")
      style.align_type = weasel::UIStyle::ALIGN_CENTER;
    else
      style.align_type = weasel::UIStyle::ALIGN_BOTTOM;
  } else if (key == "style/layout/border")
    return to_int(&style.border);
  else if (key == "style/layout/margin_x")
    return to_int(&style.margin_x);
  else if (key == "style/layout/margin_y")
    return to_int(&style.margin_y);
  else if (key == "style/layout/spacing")
    return to_int(&style.spacing);
  else if (key == "style/layout/candidate_spacing")
    return to_int(&style.candidate_spacing);
  else if (key == "style/layout/hilite_spacing")
    return to_int(&style.hilite_spacing);
  else if (key == "style/layout/hilite_padding_x")
    return to_int(&style.hilite_padding_x);
  else if (key == "style/layout/hilite_padding_y")
    return to_int(&style.hilite_padding_y);
  else if (key == "style/layout/hilited_corner_radius")
    return to_int(&style.round_corner);
  else if (key == "style/layout/corner_radius")
    return to_int(&style.round_corner_ex);
  else if (key == "style/layout/shadow_radius")
    return to_int(&style.shadow_radius);
  else if (key == "style/layout/shadow_offset_x")
    return to_int(&style.shadow_offset_x);
  else if (key == "style/layout/shadow_offset_y")
    return to_int(&style.shadow_offset_y);
  else if (key == "style/layout/min_width")
    return to_int(&style.min_width);
  else if (key == "style/layout/max_width")
    return to_int(&style.max_width);
  else if (key == "style/layout/min_height")
    return to_int(&style.min_height);
  else if (key == "style/layout/max_height")
    return to_int(&style.max_height);
  else if (key == "style/layout/baseline")
    return to_int(&style.baseline);
  else if (key == "style/layout/linespacing")
    return to_int(&style.linespacing);
  else
    return false;
  return true;
}

}  // namespace

bool UIStyleSettings::GetStyleValue(const weasel::StyleKeyInfo& info,
                                    std::string* value) {
  if (!value)
    return false;
  value->clear();

  // In-session edits are not yet visible through the base config; prefer the
  // recorded override so the list and preview stay consistent.
  auto it = preview_style_overrides_.find(info.key);
  if (it != preview_style_overrides_.end()) {
    *value = it->second;
    return true;
  }

  if (info.type == weasel::StyleValueType::Theme) {
    if (std::string(info.key) == "style/color_scheme_dark")
      *value = GetDarkColorScheme();
    else
      *value = preview_color_scheme_.empty() ? GetActiveColorScheme()
                                             : preview_color_scheme_;
    return true;
  }
  weasel::UIStyle style;
  if (!LoadPreviewStyle(&style))
    return false;
  return GetStyleFieldText(style, info.key, value);
}

bool UIStyleSettings::SetStyleValue(const weasel::StyleKeyInfo& info,
                                    const std::string& value) {
  if (info.type == weasel::StyleValueType::Theme) {
    if (std::string(info.key) == "style/color_scheme_dark") {
      preview_style_overrides_[info.key] = value;
      return true;
    }
    return SelectColorScheme(value);
  }

  switch (info.type) {
    case weasel::StyleValueType::Boolean: {
      if (value != "true" && value != "false")
        return false;
      break;
    }
    case weasel::StyleValueType::Int: {
      if (value.empty())
        return false;
      char* end = nullptr;
      const long parsed = strtol(value.c_str(), &end, 10);
      if (end == value.c_str() || *end != '\0')
        return false;
      break;
    }
    case weasel::StyleValueType::Enum:
    case weasel::StyleValueType::Str: {
      break;
    }
    default:
      return false;
  }

  preview_style_overrides_[info.key] = value;
  return true;
}

bool UIStyleSettings::UnsetStyleValue(const std::string& key) {
  preview_style_overrides_.erase(key);
  // Keep persisted settings untouched until the dialog is confirmed.
  return true;
}

bool UIStyleSettings::ApplyStylePatches() {
  const auto apply_layout_compatibility = [&]() {
    return api_->customize_bool(settings_, "style/fullscreen", False) &&
           api_->customize_bool(settings_, "style/vertical_text", False) &&
           api_->customize_string(settings_, "style/text_orientation", "");
  };

  for (const auto& override_value : preview_style_overrides_) {
    const std::string& key = override_value.first;
    const std::string& value = override_value.second;
    if (key == "style/font_face" || key == "style/label_font_face" ||
        key == "style/comment_font_face" || key == "style/label_format" ||
        key == "style/mark_text" || key == "style/preedit_type" ||
        key == "style/antialias_mode" || key == "style/hover_type" ||
        key == "style/layout/type" || key == "style/layout/align_type" ||
        key == "style/color_scheme_dark") {
      if (!api_->customize_string(settings_, key.c_str(), value.c_str()))
        return false;
    } else if (key == "style/font_point" || key == "style/label_font_point" ||
               key == "style/comment_font_point" ||
               key == "style/candidate_abbreviate_length" ||
               key.find("style/layout/") == 0) {
      char* end = nullptr;
      const long parsed = strtol(value.c_str(), &end, 10);
      if (end == value.c_str() || *end != '\0' ||
          !api_->customize_int(settings_, key.c_str(),
                               static_cast<int>(parsed)))
        return false;
    } else {
      const Bool b = (value == "true") ? True : False;
      if ((value != "true" && value != "false") ||
          !api_->customize_bool(settings_, key.c_str(), b))
        return false;
    }

    if (key == "style/layout/type") {
      if (!apply_layout_compatibility())
        return false;
    }
  }
  return true;
}
