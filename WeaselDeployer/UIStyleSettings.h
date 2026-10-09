#pragma once

#include <cstdint>
#include <map>
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

  bool GetPresetColorSchemes(std::vector<ColorSchemeInfo>* result);
  std::wstring GetColorSchemePreview(const std::string& color_scheme_id);
  std::string GetActiveColorScheme();
  bool SelectColorScheme(const std::string& color_scheme_id);

  // Reads the explicitly set colors of a scheme, converted to 0xRRGGBBAA
  // (the scheme's own color_format is honored). Keys not set by the scheme
  // are absent from the result.
  bool GetSchemeColors(const std::string& color_scheme_id,
                       const std::vector<std::string>& keys,
                       std::map<std::string, uint32_t>* rgba_out);
  // Writes (or overwrites) preset_color_schemes/custom with every given
  // color in 0xRRGGBBAA form plus color_format: rgba, then selects it.
  bool WriteCustomScheme(const std::map<std::string, uint32_t>& rgba,
                         const std::string& name);

  RimeCustomSettings* settings() { return settings_; }

 private:
  RimeLeversApi* api_;
  RimeCustomSettings* settings_;
};
