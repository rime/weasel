#pragma once

#include <string>
#include <vector>
#include <map>
#include <rime_levers_api.h>
#include <WeaselIPCData.h>
#include <WeaselStyleKeys.h>

struct ColorSchemeInfo {
  std::string color_scheme_id;
  std::string name;
  std::string author;
};

struct ColorSettingInfo {
  std::string key;
  std::string value;
  bool defined;
};

class UIStyleSettings {
 public:
  UIStyleSettings();
  ~UIStyleSettings();
  UIStyleSettings(const UIStyleSettings&) = delete;
  UIStyleSettings& operator=(const UIStyleSettings&) = delete;
  void InitFontSettings();
  bool SetFontFace(const std::string& key, const std::string& font_face);
  bool SetFontPoint(const std::string& key, int font_point);

  bool GetPresetColorSchemes(std::vector<ColorSchemeInfo>* result);
  std::string GetActiveColorScheme();
  bool SelectColorScheme(const std::string& color_scheme_id);
  bool LoadPreviewStyle(weasel::UIStyle* style) const;
  bool GetColorSettings(const std::string& color_scheme_id,
                        std::vector<ColorSettingInfo>* result);
  bool SetColorSetting(const std::string& color_scheme_id,
                       const std::string& key,
                       const std::string& value);
  bool GetStyleValue(const weasel::StyleKeyInfo& info, std::string* value);
  bool SetStyleValue(const weasel::StyleKeyInfo& info,
                     const std::string& value);
  bool UnsetStyleValue(const std::string& key);
  std::string GetDarkColorScheme();
  bool ApplyColorPatches(
      const std::map<std::string, std::vector<ColorSettingInfo>>& originals,
      const std::string& original_color_scheme);
  bool ApplyStylePatches();

  RimeCustomSettings* settings() { return settings_; }
  std::wstring font_face;
  std::wstring label_font_face;
  std::wstring comment_font_face;
  int font_point = 12;
  int label_font_point = 12;
  int comment_font_point = 12;

 private:
  std::wstring original_font_face_;
  std::wstring original_label_font_face_;
  std::wstring original_comment_font_face_;
  int original_font_point_ = 12;
  int original_label_font_point_ = 12;
  int original_comment_font_point_ = 12;
  RimeLeversApi* api_;
  RimeCustomSettings* settings_;
  std::string preview_color_scheme_;
  std::map<std::string, std::map<std::string, std::string>>
      preview_color_overrides_;
  // In-memory overrides for style/... keys edited this session. They drive
  // readback and live preview until the dialog commits them.
  std::map<std::string, std::string> preview_style_overrides_;
};
