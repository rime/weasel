#include "stdafx.h"
#include "UIStyleSettings.h"
#include <boost/filesystem.hpp>
#include <rime/config.h>
#include <rime/deployer.h>


UIStyleSettings::UIStyleSettings(rime::Deployer* deployer)
	: rime::CustomSettings(deployer, "weasel", "Weasel::UIStyleSettings")
{
}

bool UIStyleSettings::GetPresetColorSchemes(std::vector<ColorSchemeInfo>* result) {
	if (!result) return false;
	result->clear();
	rime::ConfigMapPtr preset = GetMap("preset_color_schemes");
	if (!preset) return false;
	for (rime::ConfigMap::Iterator it = preset->begin(); it != preset->end(); ++it) {
		rime::ConfigMapPtr definition = rime::As<rime::ConfigMap>(it->second);
		if (!definition) continue;
		rime::ConfigValuePtr name = definition->GetValue("name");
		rime::ConfigValuePtr author = definition->GetValue("author");
		ColorSchemeInfo info;
		info.color_scheme_id = it->first;
		if (!name) continue;
		info.name = name->str();
		if (author) info.author = author->str();
		result->push_back(info);
	}
	return true;
}

const std::string UIStyleSettings::GetColorSchemePreview(const std::string& color_scheme_id) {
	boost::filesystem::path preview_path(deployer_->shared_data_dir);
	preview_path /= "preview";
	preview_path /= "color_scheme_" + color_scheme_id + ".png";
	return preview_path.string();
}

const std::string UIStyleSettings::GetActiveColorScheme() {
	rime::ConfigValuePtr value = GetValue("style/color_scheme");
	if (!value) return std::string();
	return value->str();
}

bool UIStyleSettings::SelectColorScheme(const std::string& color_scheme_id) {
	Customize("style/color_scheme", rime::ConfigValuePtr(new rime::ConfigValue(color_scheme_id)));
	modified_ = true;
	return true;
}
