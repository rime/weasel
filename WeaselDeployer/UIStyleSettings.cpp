#include "stdafx.h"
#include <WeaselUtility.h>
#include "UIStyleSettings.h"

#include <cctype>
#include <cstdio>
#include <filesystem>

UIStyleSettings::UIStyleSettings() {
  api_ = (RimeLeversApi*)rime_get_api()->find_module("levers")->get_api();
  settings_ = api_->custom_settings_init("weasel", "Weasel::UIStyleSettings");
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
  if (!rime->config_begin_map(&preset, &config, "preset_color_schemes")) {
    return false;
  }
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

// get preview image from user dir first, then shared_dir
std::wstring UIStyleSettings::GetColorSchemePreview(
    const std::string& color_scheme_id) {
  // Resolve the data directories through WeaselUtility instead of the rime
  // API: the latter round-trips paths through narrow strings, which corrupts
  // non-ASCII user data directories (e.g. CJK paths) on Windows.
  const std::wstring filename =
      L"color_scheme_" + acptow(color_scheme_id) + L".png";
  const std::filesystem::path user_path =
      WeaselUserDataPath() / L"preview" / filename;
  DWORD attr = GetFileAttributesW(user_path.c_str());
  if (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY))
    return user_path.wstring();
  return (WeaselSharedDataPath() / L"preview" / filename).wstring();
}

std::string UIStyleSettings::GetActiveColorScheme() {
  RimeConfig config = {0};
  api_->settings_get_config(settings_, &config);
  const char* value =
      rime_get_api()->config_get_cstring(&config, "style/color_scheme");
  if (!value)
    return std::string();
  return std::string(value);
}

bool UIStyleSettings::SelectColorScheme(const std::string& color_scheme_id) {
  api_->customize_string(settings_, "style/color_scheme",
                         color_scheme_id.c_str());
  return true;
}

namespace {

bool IntToRgba(uint32_t value,
               bool no_alpha,
               const std::string& format,
               uint32_t* rgba_out) {
  if (format == "rgba") {
    if (no_alpha)
      value = (value << 8) | 0xff;
  } else if (format == "argb") {
    if (no_alpha)
      value |= 0xff000000;
    // 0xAARRGGBB -> 0xRRGGBBAA
    *rgba_out = ((value & 0x00ff0000) << 8) | ((value & 0x0000ff00) << 8) |
                ((value & 0x000000ff) << 8) | ((value & 0xff000000) >> 24);
    return true;
  } else {  // abgr
    if (no_alpha)
      value |= 0xff000000;
    // 0xAABBGGRR -> 0xRRGGBBAA
    *rgba_out = ((value & 0x000000ff) << 24) | ((value & 0x0000ff00) << 8) |
                ((value & 0x00ff0000) >> 8) | ((value & 0xff000000) >> 24);
    return true;
  }
  *rgba_out = value;
  return true;
}

// Parses a color string ("0x...", "0X..." or "#...", 3/4/6/8 hex digits) or
// an integer value into 0xRRGGBBAA, honoring the given color format
// ("abgr" default, "argb" or "rgba"). Mirrors _RimeGetColor in
// RimeWithWeasel.cpp.
bool ParseColorToRgba(const std::string& str,
                      const std::string& format,
                      uint32_t* rgba_out) {
  if (str.empty())
    return false;
  size_t start = 0;
  if (str[0] == '#') {
    start = 1;
  } else if (str.size() >= 2 &&
             (str.compare(0, 2, "0x") == 0 || str.compare(0, 2, "0X") == 0)) {
    start = 2;
  } else {
    return false;
  }
  std::string hex = str.substr(start);
  if (hex.length() != 3 && hex.length() != 4 && hex.length() != 6 &&
      hex.length() != 8)
    return false;
  for (char c : hex) {
    if (!isxdigit(static_cast<unsigned char>(c)))
      return false;
  }
  if (hex.length() == 3 || hex.length() == 4) {
    std::string expanded;
    for (char c : hex) {
      expanded += c;
      expanded += c;
    }
    hex = expanded;
  }
  uint32_t value = std::stoul(hex, nullptr, 16);
  return IntToRgba(value, hex.length() == 6, format, rgba_out);
}

}  // namespace

bool UIStyleSettings::GetSchemeColors(
    const std::string& color_scheme_id,
    const std::vector<std::string>& keys,
    std::map<std::string, uint32_t>* rgba_out) {
  if (!rgba_out)
    return false;
  RimeConfig config = {0};
  if (!api_->settings_get_config(settings_, &config))
    return false;
  RimeApi* rime = rime_get_api();
  const std::string prefix = "preset_color_schemes/" + color_scheme_id + "/";
  char buffer[64] = {0};
  std::string format("abgr");
  if (rime->config_get_string(&config, (prefix + "color_format").c_str(),
                              buffer, sizeof buffer)) {
    format = buffer;
  }
  for (const std::string& key : keys) {
    const std::string path = prefix + key;
    uint32_t rgba = 0;
    bool ok = false;
    if (rime->config_get_string(&config, path.c_str(), buffer, sizeof buffer)) {
      ok = ParseColorToRgba(buffer, format, &rgba);
    }
    if (!ok) {
      int int_value = 0;
      if (rime->config_get_int(&config, path.c_str(), &int_value)) {
        // Compare as signed int, mirroring _RimeGetColor: values without an
        // alpha byte (including negatives from oversized decimals) are made
        // opaque.
        uint32_t value = static_cast<uint32_t>(int_value);
        ok = IntToRgba(value, int_value <= 0xffffff, format, &rgba);
      }
    }
    if (ok)
      (*rgba_out)[key] = rgba;
  }
  return true;
}

bool UIStyleSettings::WriteCustomScheme(
    const std::map<std::string, uint32_t>& rgba,
    const std::string& name) {
  bool ok = true;
  char buffer[16];
  for (const auto& pair : rgba) {
    snprintf(buffer, sizeof buffer, "0x%08X", pair.second);
    const std::string path = "preset_color_schemes/custom/" + pair.first;
    ok = api_->customize_string(settings_, path.c_str(), buffer) && ok;
  }
  ok = api_->customize_string(settings_, "preset_color_schemes/custom/name",
                              name.c_str()) &&
       ok;
  ok = api_->customize_string(
           settings_, "preset_color_schemes/custom/color_format", "rgba") &&
       ok;
  ok = api_->customize_string(settings_, "style/color_scheme", "custom") && ok;
  return ok;
}
