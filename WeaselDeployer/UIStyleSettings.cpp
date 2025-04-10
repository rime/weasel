#include "stdafx.h"
#include <WeaselUtility.h>
#include <WeaselConstants.h>
#include "UIStyleSettings.h"

UIStyleSettings::UIStyleSettings() {
  api_ = (RimeLeversApi*)rime_get_api()->find_module("levers")->get_api();
  settings_ = api_->custom_settings_init("weasel", "Weasel::UIStyleSettings");
  InitFontSettings();
}

void _Setup() {
  RIME_STRUCT(RimeTraits, weasel_traits);
  std::string shared_dir = wtou8(WeaselSharedDataPath().wstring());
  std::string user_dir = wtou8(WeaselUserDataPath().wstring());
  weasel_traits.shared_data_dir = shared_dir.c_str();
  weasel_traits.user_data_dir = user_dir.c_str();
  weasel_traits.prebuilt_data_dir = weasel_traits.shared_data_dir;
  std::string distribution_name = wtou8(get_weasel_ime_name());
  weasel_traits.distribution_name = distribution_name.c_str();
  weasel_traits.distribution_code_name = WEASEL_CODE_NAME;
  weasel_traits.distribution_version = WEASEL_VERSION;
  weasel_traits.app_name = "rime.weasel";
  std::string log_dir = WeaselLogPath().u8string();
  weasel_traits.log_dir = log_dir.c_str();
  rime_get_api()->setup(&weasel_traits);
  // rime_api->set_notification_handler(&RimeWithWeaselHandler::OnNotify, this);
}

void UIStyleSettings::InitFontSettings() {
  _Setup();
  RimeConfig config = {0};
  RimeApi* rime = rime_get_api();
  rime->config_open("weasel", &config);

  auto get_font = [&](wstring& value, const char* key) {
    char buffer[4096] = {0};
    rime->config_get_string(&config, key, buffer, _countof(buffer));
    value = u8tow(buffer);
  };
  get_font(font_face, "style/font_face");
  get_font(label_font_face, "style/label_font_face");
  get_font(comment_font_face, "style/comment_font_face");
  rime->config_get_int(&config, "style/font_point", &font_point);
  if (!rime->config_get_int(&config, "style/label_font_point",
                            &label_font_point))
    label_font_point = font_point;
  if (!rime->config_get_int(&config, "style/comment_font_point",
                            &comment_font_point))
    comment_font_point = font_point;
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

// check if a file exists
static inline bool IfFileExist(std::string filename) {
  DWORD dwAttrib = GetFileAttributes(acptow(filename).c_str());
  return (INVALID_FILE_ATTRIBUTES != dwAttrib &&
          0 == (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

// get preview image from user dir first, then shared_dir
std::string UIStyleSettings::GetColorSchemePreview(
    const std::string& color_scheme_id) {
  std::string shared_dir = rime_get_api()->get_shared_data_dir();
  std::string user_dir = rime_get_api()->get_user_data_dir();
  std::string filename =
      user_dir + "\\preview\\color_scheme_" + color_scheme_id + ".png";
  if (IfFileExist(filename))
    return filename;
  else
    return (shared_dir + "\\preview\\color_scheme_" + color_scheme_id + ".png");
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

bool UIStyleSettings::SetFontFace(const std::string& key,
                                  const std::string& font_face) {
  api_->customize_string(settings_, key.c_str(), font_face.c_str());
  return true;
}

bool UIStyleSettings::SetFontPoint(const std::string& key,
                                   const int font_point) {
  api_->customize_int(settings_, key.c_str(), font_point);
  return true;
}
