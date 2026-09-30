#pragma once

// Renders a mock horizontal candidate window for color scheme preview.
// Used by CustomSchemeDialog (live preview) and UIStyleSettingsDialog
// (persisted preview PNG for the "custom" scheme).

#include <cstdint>
#include <map>
#include <string>

namespace weasel_deployer {

// Color values used across the custom scheme editor.
// Each value is a 32-bit color in 0xRRGGBBAA order.
using RgbaColorMap = std::map<std::string, uint32_t>;

// Paints a mock candidate window into the given device context rectangle.
// Requires GDI+ to be started by the caller.
void PaintMockCandidateWindow(HDC hdc,
                              const RECT& target,
                              const RgbaColorMap& colors);

// Renders the mock candidate window and saves it as a PNG file.
// Requires GDI+ to be started by the caller.
bool SavePreviewPng(const std::wstring& png_path,
                    int width,
                    int height,
                    const RgbaColorMap& colors);

}  // namespace weasel_deployer
