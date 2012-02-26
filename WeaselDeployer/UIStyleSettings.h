#pragma once

#include <string>
#include <vector>

#pragma warning(disable: 4995)
#pragma warning(disable: 4996)
#include <rime/expl/custom_settings.h>
#pragma warning(default: 4996)
#pragma warning(default: 4995)

namespace rime {
	class Deployer;
}

struct ColorSchemeInfo {
	std::string color_scheme_id;
	std::string name;
	std::string author;
};

class UIStyleSettings : public rime::CustomSettings {
public:
	UIStyleSettings(rime::Deployer* deployer);

	bool GetPresetColorSchemes(std::vector<ColorSchemeInfo>* result);
	const std::string GetColorSchemePreview(const std::string& color_scheme_id);
	const std::string GetActiveColorScheme();
	bool SelectColorScheme(const std::string& color_scheme_id);
};
