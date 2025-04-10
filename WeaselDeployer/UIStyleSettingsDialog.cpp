#include "stdafx.h"
#include "UIStyleSettingsDialog.h"
#include "UIStyleSettings.h"
#include "Configurator.h"
#include <WeaselUtility.h>
#include "FontSettingDialog.h"

UIStyleSettingsDialog::UIStyleSettingsDialog(UIStyleSettings* settings)
    : settings_(settings), loaded_(false) {}

UIStyleSettingsDialog::~UIStyleSettingsDialog() {
  image_.Destroy();
}

void UIStyleSettingsDialog::Populate() {
  if (!settings_)
    return;
  std::string active(settings_->GetActiveColorScheme());
  int active_index = -1;
  settings_->GetPresetColorSchemes(&preset_);
  for (size_t i = 0; i < preset_.size(); ++i) {
    std::wstring txt = u8tow(preset_[i].name);
    color_schemes_.AddString(txt.c_str());
    if (preset_[i].color_scheme_id == active) {
      active_index = i;
    }
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
  select_font_.EnableWindow(TRUE);

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

LRESULT UIStyleSettingsDialog::OnSelectFont(WORD, WORD code, HWND, BOOL&) {
  FontSettingDialog dialog(settings_, m_hWnd);

  if (dialog.ShowDialog() == IDOK) {
    settings_->SetFontFace("style/font_face", wtou8(dialog.m_font_face));
    settings_->SetFontFace("style/label_font_face",
                           wtou8(dialog.m_label_font_face));
    settings_->SetFontFace("style/comment_font_face",
                           wtou8(dialog.m_comment_font_face));
    settings_->SetFontPoint("style/font_point", dialog.m_font_point);
    settings_->SetFontPoint("style/label_font_point",
                            dialog.m_label_font_point);
    settings_->SetFontPoint("style/comment_font_point",
                            dialog.m_comment_font_point);
  }
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

void UIStyleSettingsDialog::Preview(int index) {
  if (index < 0 || index >= (int)preset_.size())
    return;
  const std::string file_path(
      settings_->GetColorSchemePreview(preset_[index].color_scheme_id));
  if (file_path.empty())
    return;
  image_.Destroy();
  // it is from ansi coding, not utf8
  image_.Load(acptow(file_path).c_str());
  if (!image_.IsNull()) {
    preview_.SetBitmap(image_);
  }
}
