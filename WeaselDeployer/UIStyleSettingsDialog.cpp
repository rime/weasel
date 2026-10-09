#include "stdafx.h"
#include "UIStyleSettingsDialog.h"
#include "CustomSchemeDialog.h"
#include "UIStyleSettings.h"
#include "Configurator.h"
#include <WeaselUtility.h>

UIStyleSettingsDialog::UIStyleSettingsDialog(UIStyleSettings* settings)
    : settings_(settings), loaded_(false) {}

UIStyleSettingsDialog::~UIStyleSettingsDialog() {
  image_.Destroy();
}

void UIStyleSettingsDialog::Populate(const std::string& select_id) {
  if (!settings_)
    return;
  color_schemes_.ResetContent();
  preset_.clear();
  std::string active(select_id.empty() ? settings_->GetActiveColorScheme()
                                       : select_id);
  int active_index = -1;
  settings_->GetPresetColorSchemes(&preset_);
  for (size_t i = 0; i < preset_.size(); ++i) {
    std::wstring txt = u8tow(preset_[i].name);
    color_schemes_.AddString(txt.c_str());
    if (preset_[i].color_scheme_id == active) {
      active_index = static_cast<int>(i);
    }
  }
  if (active_index < 0 && !select_id.empty()) {
    // The scheme (e.g. a just created "custom") may not be visible in the
    // staged configuration yet; show it so the selection is not lost.
    ColorSchemeInfo info;
    info.color_scheme_id = select_id;
    if (select_id == "custom") {
      const wchar_t* fallback_name = L"Custom";
      LANGID lang = GetUserDefaultUILanguage();
      if (PRIMARYLANGID(lang) == LANG_CHINESE) {
        fallback_name = (SUBLANGID(lang) == SUBLANG_CHINESE_TRADITIONAL ||
                         SUBLANGID(lang) == SUBLANG_CHINESE_HONGKONG ||
                         SUBLANGID(lang) == SUBLANG_CHINESE_MACAU)
                            ? L"自定義／Custom"
                            : L"自定义／Custom";
      }
      info.name = wtou8(fallback_name);
    } else {
      info.name = select_id;
    }
    preset_.push_back(info);
    color_schemes_.AddString(u8tow(info.name).c_str());
    active_index = static_cast<int>(preset_.size()) - 1;
  }
  if (active_index >= 0) {
    color_schemes_.SetCurSel(active_index);
    Preview(active_index);
  }
  loaded_ = true;
}

LRESULT UIStyleSettingsDialog::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&) {
  color_schemes_.Attach(GetDlgItem(IDC_COLOR_SCHEME));
  preview_.Attach(GetDlgItem(IDC_PREVIEW));
  select_font_.Attach(GetDlgItem(IDC_SELECT_FONT));
  select_font_.EnableWindow(FALSE);

  Populate();

  CenterWindow();
  BringWindowToTop();
  return TRUE;
}

LRESULT UIStyleSettingsDialog::OnClose(UINT, WPARAM, LPARAM, BOOL&) {
  EndDialog(IDCANCEL);
  return 0;
}

LRESULT UIStyleSettingsDialog::OnOK(WORD, WORD code, HWND, BOOL&) {
  EndDialog(code);
  return 0;
}

LRESULT UIStyleSettingsDialog::OnColorSchemeSelChange(WORD, WORD, HWND, BOOL&) {
  int index = color_schemes_.GetCurSel();
  if (index >= 0 && index < (int)preset_.size()) {
    settings_->SelectColorScheme(preset_[index].color_scheme_id);
    Preview(index);
  }
  return 0;
}

LRESULT UIStyleSettingsDialog::OnCustomize(WORD, WORD, HWND, BOOL&) {
  // Base the editor's starting colors on the scheme currently selected in
  // the list (fall back to the active scheme).
  int index = color_schemes_.GetCurSel();
  std::string base = (index >= 0 && index < (int)preset_.size())
                         ? preset_[index].color_scheme_id
                         : settings_->GetActiveColorScheme();
  if (base.empty())
    base = "aqua";
  CustomSchemeDialog dialog(settings_, base);
  if (dialog.DoModal() == IDOK) {
    // The custom scheme was staged and selected; rebuild the list around it.
    Populate("custom");
    loaded_ = true;
  }
  return 0;
}

void UIStyleSettingsDialog::Preview(int index) {
  if (index < 0 || index >= (int)preset_.size())
    return;
  const std::wstring file_path(
      settings_->GetColorSchemePreview(preset_[index].color_scheme_id));
  image_.Destroy();
  if (!file_path.empty())
    image_.Load(file_path.c_str());
  // Clear the stale bitmap when the preview image is missing or failed to
  // load, instead of keeping the previously shown scheme.
  preview_.SetBitmap(image_.IsNull() ? NULL : static_cast<HBITMAP>(image_));
}
