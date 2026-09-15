#pragma once

#include <string>
#include <vector>
#include <rime_levers_api.h>

struct ColorSchemeInfo {
  std::string color_scheme_id;
  std::string name;
  std::string author;
};

class UIStyleSettings {
 public:
  UIStyleSettings();
  ~UIStyleSettings();
  UIStyleSettings(const UIStyleSettings&) = delete;
  UIStyleSettings& operator=(const UIStyleSettings&) = delete;

  bool GetPresetColorSchemes(std::vector<ColorSchemeInfo>* result);
  std::string GetColorSchemePreview(const std::string& color_scheme_id);
  std::string GetActiveColorScheme();
  bool SelectColorScheme(const std::string& color_scheme_id);
  void InitFontSettings();
  bool SetFontFace(const std::string& key, const std::string& font_face);
  bool SetFontPoint(const std::string& key, const int font_point);

  RimeCustomSettings* settings() { return settings_; }
  std::wstring font_face;
  std::wstring label_font_face;
  std::wstring comment_font_face;
  int font_point = 12;
  int label_font_point = 12;
  int comment_font_point = 12;

 private:
  RimeLeversApi* api_;
  RimeCustomSettings* settings_;
};
