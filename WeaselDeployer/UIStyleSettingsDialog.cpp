#include "stdafx.h"
#include "UIStyleSettingsDialog.h"
#include "UIStyleSettings.h"
#include "Configurator.h"
#include "FontSettingDialog.h"
#include <WeaselUtility.h>
#include <WeaselStyleColor.h>

static bool IsValidColorValue(const std::string& value) {
  unsigned int parsed = 0;
  return weasel::ParseColorValue(value, weasel::COLOR_ABGR, &parsed);
}

namespace {

void DrawColorSwatch(HDC hdc, const RECT& rc, int color) {
  const int alpha = (color >> 24) & 0xFF;
  const int red = color & 0xFF;
  const int green = (color >> 8) & 0xFF;
  const int blue = (color >> 16) & 0xFF;

  const int dpi = GetDeviceCaps(hdc, LOGPIXELSY);
  const int cell = MulDiv(4, dpi, 96);
  if (cell <= 0)
    return;

  auto blend = [alpha, red, green, blue](COLORREF base) -> COLORREF {
    const int br = GetRValue(base);
    const int bg = GetGValue(base);
    const int bb = GetBValue(base);
    const int out_r = (red * alpha + br * (255 - alpha)) / 255;
    const int out_g = (green * alpha + bg * (255 - alpha)) / 255;
    const int out_b = (blue * alpha + bb * (255 - alpha)) / 255;
    return RGB(out_r, out_g, out_b);
  };

  const COLORREF light = RGB(255, 255, 255);
  const COLORREF dark = RGB(198, 198, 198);

  int row = 0;
  for (LONG y = rc.top; y < rc.bottom; y += cell) {
    int col = 0;
    for (LONG x = rc.left; x < rc.right; x += cell) {
      const COLORREF base = ((row + col) % 2 == 0) ? light : dark;
      RECT cell_rc = {x, y, (x + cell < rc.right) ? x + cell : rc.right,
                      (y + cell < rc.bottom) ? y + cell : rc.bottom};
      HBRUSH brush = CreateSolidBrush(blend(base));
      FillRect(hdc, &cell_rc, brush);
      DeleteObject(brush);
      ++col;
    }
    ++row;
  }

  HGDIOBJ old_pen = SelectObject(hdc, GetStockObject(BLACK_PEN));
  HGDIOBJ old_brush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
  Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
  SelectObject(hdc, old_pen);
  SelectObject(hdc, old_brush);
}

}  // namespace

LRESULT UIStyleSettingsDialog::OnMeasureItem(UINT,
                                             WPARAM,
                                             LPARAM lParam,
                                             BOOL& bHandled) {
  MEASUREITEMSTRUCT* mis = reinterpret_cast<MEASUREITEMSTRUCT*>(lParam);
  if (!mis || (mis->CtlID != IDC_COLOR_LIST && mis->CtlID != IDC_STYLE_LIST))
    return 0;
  HDC hdc = ::GetDC(m_hWnd);
  const int dpi = ::GetDeviceCaps(hdc, LOGPIXELSY);
  ::ReleaseDC(m_hWnd, hdc);
  mis->itemHeight = MulDiv(20, dpi, 96);
  bHandled = TRUE;
  return TRUE;
}

LRESULT UIStyleSettingsDialog::OnDrawItem(UINT,
                                          WPARAM,
                                          LPARAM lParam,
                                          BOOL& bHandled) {
  DRAWITEMSTRUCT* dis = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
  if (!dis)
    return 0;

  if (dis->CtlID == IDC_STYLE_LIST) {
    const auto& keys = weasel::GetStyleKeys();
    if (dis->itemID == (UINT)-1 ||
        dis->itemID >= static_cast<UINT>(keys.size()))
      return 0;

    HDC hdc = dis->hDC;
    const RECT& rc = dis->rcItem;
    const bool selected = (dis->itemState & ODS_SELECTED) != 0;
    FillRect(hdc, &rc,
             GetSysColorBrush(selected ? COLOR_HIGHLIGHT : COLOR_WINDOW));

    const weasel::StyleKeyInfo& info = keys[dis->itemID];
    std::string current;
    settings_->GetStyleValue(info, &current);
    const auto baseline = original_style_values_.find(info.key);
    const bool modified = (baseline != original_style_values_.end() &&
                           baseline->second != current);

    std::wstring label = u8tow(weasel::Localize(info.label));
    std::wstring detail = u8tow(std::string(info.key) + ": " + current);
    if (modified)
      detail += L" +";

    const int dpi = GetDeviceCaps(hdc, LOGPIXELSY);
    const int pad = MulDiv(6, dpi, 96);
    const int split = rc.left + MulDiv(120, dpi, 96);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(
        hdc, GetSysColor(selected ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT));

    RECT label_rc = rc;
    label_rc.left += pad;
    label_rc.right = split;
    if (label_rc.right > label_rc.left)
      DrawText(hdc, label.c_str(), static_cast<int>(label.size()), &label_rc,
               DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

    RECT detail_rc = rc;
    detail_rc.left = split;
    detail_rc.right -= pad;
    if (detail_rc.right > detail_rc.left)
      DrawText(hdc, detail.c_str(), static_cast<int>(detail.size()), &detail_rc,
               DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX |
                   DT_END_ELLIPSIS);

    if (dis->itemState & ODS_FOCUS)
      DrawFocusRect(hdc, &rc);
    bHandled = TRUE;
    return TRUE;
  }

  if (dis->CtlID != IDC_COLOR_LIST)
    return 0;
  if (dis->itemID == (UINT)-1 ||
      dis->itemID >= static_cast<UINT>(color_settings_.size()))
    return 0;

  HDC hdc = dis->hDC;
  const RECT& rc = dis->rcItem;
  const bool selected = (dis->itemState & ODS_SELECTED) != 0;

  FillRect(hdc, &rc,
           GetSysColorBrush(selected ? COLOR_HIGHLIGHT : COLOR_WINDOW));

  const ColorSettingInfo& item = color_settings_[dis->itemID];
  std::wstring text = u8tow(item.key);
  if (!item.defined)
    text += L" *";

  const int dpi = GetDeviceCaps(hdc, LOGPIXELSY);
  const int swatch = MulDiv(16, dpi, 96);
  const int pad = MulDiv(4, dpi, 96);

  SetBkMode(hdc, TRANSPARENT);
  SetTextColor(hdc,
               GetSysColor(selected ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT));
  RECT text_rc = rc;
  text_rc.left += pad;
  text_rc.right -= swatch + pad * 2;
  if (text_rc.right > text_rc.left)
    DrawText(hdc, text.c_str(), static_cast<int>(text.size()), &text_rc,
             DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

  RECT swatch_rc = {rc.right - swatch - pad,
                    rc.top + (rc.bottom - rc.top - swatch) / 2, rc.right - pad,
                    rc.top + (rc.bottom - rc.top + swatch) / 2};
  int color = 0;
  if (preview_ui_) {
    if (const int* resolved =
            weasel::GetStyleColorField(preview_ui_->style(), item.key))
      color = *resolved;
  }
  DrawColorSwatch(hdc, swatch_rc, color);

  if (dis->itemState & ODS_FOCUS)
    DrawFocusRect(hdc, &rc);

  bHandled = TRUE;
  return TRUE;
}

UIStyleSettingsDialog::UIStyleSettingsDialog(UIStyleSettings* settings)
    : settings_(settings), loaded_(false) {}

UIStyleSettingsDialog::~UIStyleSettingsDialog() {
  if (preview_ui_)
    preview_ui_->Destroy(true);
}

void UIStyleSettingsDialog::Populate() {
  if (!settings_)
    return;
  std::string active(settings_->GetActiveColorScheme());
  original_color_scheme_ = active;
  int active_index = -1;
  color_schemes_.ResetContent();
  settings_->GetPresetColorSchemes(&preset_);
  for (size_t i = 0; i < preset_.size(); ++i) {
    std::wstring txt = u8tow(preset_[i].name);
    if (modified_schemes_.count(preset_[i].color_scheme_id))
      txt += L" +";
    color_schemes_.AddString(txt.c_str());
    if (preset_[i].color_scheme_id == active) {
      active_index = static_cast<int>(i);
    }
  }
  if (active_index >= 0) {
    color_schemes_.SetCurSel(active_index);
    PopulateColorSettings();
    UpdatePreview();
  }
  PopulateStyleKeys();
  loaded_ = true;
}

LRESULT UIStyleSettingsDialog::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&) {
  color_schemes_.Attach(GetDlgItem(IDC_COLOR_SCHEME));
  color_list_.Attach(GetDlgItem(IDC_COLOR_LIST));
  color_value_.Attach(GetDlgItem(IDC_COLOR_VALUE));
  set_color_.Attach(GetDlgItem(IDC_SET_COLOR));
  style_list_.Attach(GetDlgItem(IDC_STYLE_LIST));
  style_value_combo_.Attach(GetDlgItem(IDC_STYLE_VALUE_COMBO));
  style_value_edit_.Attach(GetDlgItem(IDC_STYLE_VALUE_EDIT));
  set_style_.Attach(GetDlgItem(IDC_SET_STYLE));
  preview_context_.preedit = weasel::Text(L"小狼毫pei se\u2038");
  weasel::TextAttribute preedit_attribute(3, 9, weasel::HIGHLIGHTED);
  preedit_attribute.range.cursor = 9;
  preview_context_.preedit.attributes.push_back(preedit_attribute);
  preview_context_.cinfo.currentPage = 0;
  preview_context_.cinfo.totalPages = 2;
  preview_context_.cinfo.is_last_page = false;
  preview_context_.cinfo.highlighted = 0;
  preview_context_.cinfo.labels = {weasel::Text(L"1"), weasel::Text(L"2"),
                                   weasel::Text(L"3"), weasel::Text(L"4"),
                                   weasel::Text(L"5")};
  preview_context_.cinfo.candies = {weasel::Text(L"配色"), weasel::Text(L"陪"),
                                    weasel::Text(L"配"), weasel::Text(L"賠"),
                                    weasel::Text(L"培")};
  preview_context_.cinfo.comments = {
      weasel::Text(L"pei se"), weasel::Text(L"pei"), weasel::Text(L"pei"),
      weasel::Text(L"pei"), weasel::Text(L"pei")};
  preview_status_.composing = true;
  preview_status_.schema_name = L"朙月拼音";
  preview_status_.schema_id = L"luna_pinyin";
  preview_ui_ = std::make_unique<weasel::UI>();
  weasel::UIStyle initial_style;
  if (settings_->LoadPreviewStyle(&initial_style)) {
    initial_style.client_caps |= weasel::INLINE_PREEDIT_CAPABLE;
    preview_ui_->style() = initial_style;
  }
  if (!preview_ui_->Create(m_hWnd, true))
    preview_ui_.reset();

  Populate();
  UpdatePreview();

  InitCtrlRects();
  CenterWindow();
  BringWindowToTop();
  return TRUE;
}

LRESULT UIStyleSettingsDialog::OnWindowPosChanged(UINT,
                                                  WPARAM wParam,
                                                  LPARAM lParam,
                                                  BOOL&) {
  LRESULT result = ::DefWindowProc(m_hWnd, WM_WINDOWPOSCHANGED, wParam, lParam);
  if (preview_ui_)
    preview_ui_->RepositionPreview();
  return result;
}

LRESULT UIStyleSettingsDialog::OnClose(UINT, WPARAM, LPARAM, BOOL&) {
  EndDialog(IDCANCEL);
  return 0;
}

LRESULT UIStyleSettingsDialog::OnOK(WORD, WORD code, HWND, BOOL&) {
  if (settings_ && (!settings_->ApplyStylePatches() ||
                    !settings_->ApplyColorPatches(original_colors_,
                                                  original_color_scheme_))) {
    MessageBox(L"保存样式设置失败。", L"样式设置", MB_ICONWARNING | MB_OK);
    return 0;
  }
  EndDialog(code);
  return 0;
}

LRESULT UIStyleSettingsDialog::OnSelectFont(WORD, WORD, HWND, BOOL&) {
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
    UpdatePreview();
  }
  return 0;
}

LRESULT UIStyleSettingsDialog::OnColorSchemeSelChange(WORD, WORD, HWND, BOOL&) {
  int index = color_schemes_.GetCurSel();
  if (index >= 0 && index < (int)preset_.size()) {
    settings_->SelectColorScheme(preset_[index].color_scheme_id);
    PopulateColorSettings();
    RefreshStyleList();
    UpdatePreview();
  }
  return 0;
}

LRESULT UIStyleSettingsDialog::OnColorSettingSelChange(WORD,
                                                       WORD,
                                                       HWND,
                                                       BOOL&) {
  UpdateColorValue();
  return 0;
}

LRESULT UIStyleSettingsDialog::OnSetColor(WORD, WORD, HWND, BOOL&) {
  int scheme_index = color_schemes_.GetCurSel();
  int color_index = color_list_.GetCurSel();
  if (scheme_index < 0 || color_index < 0 ||
      scheme_index >= (int)preset_.size() ||
      color_index >= (int)color_settings_.size())
    return 0;
  CString value;
  color_value_.GetWindowText(value);
  std::wstring value_w(value.GetString());
  std::string value_u8 = wtou8(value_w);
  if (!value_u8.empty() && !IsValidColorValue(value_u8)) {
    MessageBox(
        L"颜色值格式无效。请输入普通整数，或 3、4、6、8 位十六进制颜色值，可带 "
        L"# 或 0x 前缀。",
        L"颜色设置", MB_ICONWARNING | MB_OK);
    return 0;
  }
  if (settings_->SetColorSetting(preset_[scheme_index].color_scheme_id,
                                 color_settings_[color_index].key, value_u8)) {
    modified_schemes_.insert(preset_[scheme_index].color_scheme_id);
    if (value_u8.empty()) {
      const std::string& scheme_id = preset_[scheme_index].color_scheme_id;
      const auto baseline = original_colors_.find(scheme_id);
      if (baseline != original_colors_.end()) {
        auto original =
            std::find_if(baseline->second.begin(), baseline->second.end(),
                         [&](const ColorSettingInfo& item) {
                           return item.key == color_settings_[color_index].key;
                         });
        if (original != baseline->second.end())
          color_settings_[color_index] = *original;
        else {
          color_settings_[color_index].value.clear();
          color_settings_[color_index].defined = false;
        }
      } else {
        color_settings_[color_index].value.clear();
        color_settings_[color_index].defined = false;
      }
    } else {
      color_settings_[color_index].value = value_u8;
      color_settings_[color_index].defined = true;
    }
    std::wstring color_name = u8tow(color_settings_[color_index].key);
    if (!color_settings_[color_index].defined)
      color_name += L" *";
    color_list_.DeleteString(color_index);
    color_list_.InsertString(color_index, color_name.c_str());
    color_list_.SetCurSel(color_index);
    UpdateColorValue();
    UpdatePreview();
    const std::string& scheme_id = preset_[scheme_index].color_scheme_id;
    bool modified = false;
    const auto baseline = original_colors_.find(scheme_id);
    if (baseline != original_colors_.end() &&
        baseline->second.size() == color_settings_.size()) {
      for (const auto& item : color_settings_) {
        auto original =
            std::find_if(baseline->second.begin(), baseline->second.end(),
                         [&](const ColorSettingInfo& baseline_item) {
                           return baseline_item.key == item.key;
                         });
        if (original == baseline->second.end() ||
            item.defined != original->defined ||
            item.value != original->value) {
          modified = true;
          break;
        }
      }
    } else {
      modified = true;
    }
    if (modified)
      modified_schemes_.insert(scheme_id);
    else
      modified_schemes_.erase(scheme_id);
    int text_length = color_schemes_.GetLBTextLen(scheme_index);
    if (text_length >= 0) {
      std::wstring scheme_name(text_length + 1, L'\0');
      color_schemes_.GetLBText(scheme_index, scheme_name.data());
      scheme_name.resize(text_length);
      size_t marker = scheme_name.rfind(L" +");
      if (marker != std::wstring::npos)
        scheme_name.resize(marker);
      if (modified)
        scheme_name += L" +";
      color_schemes_.DeleteString(scheme_index);
      color_schemes_.InsertString(scheme_index, scheme_name.c_str());
      color_schemes_.SetCurSel(scheme_index);
    }
  }
  return 0;
}

void UIStyleSettingsDialog::PopulateColorSettings() {
  int scheme_index = color_schemes_.GetCurSel();
  color_list_.ResetContent();
  color_settings_.clear();
  if (scheme_index < 0 || scheme_index >= (int)preset_.size())
    return;
  settings_->GetColorSettings(preset_[scheme_index].color_scheme_id,
                              &color_settings_);
  const std::string& scheme_id = preset_[scheme_index].color_scheme_id;
  if (original_colors_.find(scheme_id) == original_colors_.end())
    original_colors_[scheme_id] = color_settings_;
  for (const auto& item : color_settings_) {
    std::wstring text = u8tow(item.key);
    if (!item.defined)
      text += L" *";
    color_list_.AddString(text.c_str());
  }
  if (!color_settings_.empty()) {
    color_list_.SetCurSel(0);
    UpdateColorValue();
  }
}

void UIStyleSettingsDialog::UpdateColorValue() {
  int index = color_list_.GetCurSel();
  if (index >= 0 && index < (int)color_settings_.size()) {
    color_value_.SetWindowText(u8tow(color_settings_[index].value).c_str());
    set_color_.EnableWindow(TRUE);
  } else {
    color_value_.SetWindowText(L"");
    set_color_.EnableWindow(FALSE);
  }
}

void UIStyleSettingsDialog::UpdatePreview() {
  if (!preview_ui_)
    return;
  weasel::UIStyle style;
  if (!settings_->LoadPreviewStyle(&style))
    return;
  // The real frontend advertises inline-preedit support per session.
  style.client_caps |= weasel::INLINE_PREEDIT_CAPABLE;
  preview_ui_->style() = style;
  preview_ui_->Update(preview_context_, preview_status_);
  preview_ui_->Refresh();
  // Layout-affecting keys (layout/type, margins, spacing, min/max size, ...)
  // change the preview's content size, so re-center it above the dialog.
  preview_ui_->RepositionPreview();
  preview_ui_->Show();
}

const weasel::StyleKeyInfo* UIStyleSettingsDialog::SelectedStyleKey() const {
  const int index = style_list_.GetCurSel();
  const auto& keys = weasel::GetStyleKeys();
  if (index < 0 || index >= static_cast<int>(keys.size()))
    return nullptr;
  return &keys[index];
}

void UIStyleSettingsDialog::PopulateStyleKeys() {
  style_list_.ResetContent();
  original_style_values_.clear();
  const auto& keys = weasel::GetStyleKeys();
  for (const auto& key : keys) {
    std::string original;
    if (settings_->GetStyleValue(key, &original))
      original_style_values_[key.key] = original;
    style_list_.AddString(u8tow(weasel::Localize(key.label)).c_str());
  }
  if (!keys.empty()) {
    style_list_.SetCurSel(0);
    UpdateStyleValue();
  }
}

void UIStyleSettingsDialog::RefreshStyleList() {
  // Items are owner-drawn from the live settings, so a redraw is enough to
  // refresh the "key: value" detail and the modified "+" marker.
  style_list_.Invalidate(FALSE);
}

bool UIStyleSettingsDialog::ApplyStyleValue(const weasel::StyleKeyInfo& info,
                                            const std::string& value) {
  const auto baseline = original_style_values_.find(info.key);
  const bool restored =
      baseline != original_style_values_.end() && baseline->second == value;
  const bool ok = restored ? settings_->UnsetStyleValue(info.key)
                           : settings_->SetStyleValue(info, value);
  if (ok) {
    RefreshStyleList();
    UpdatePreview();
  }
  return ok;
}

void UIStyleSettingsDialog::UpdateStyleValue() {
  const weasel::StyleKeyInfo* info = SelectedStyleKey();
  if (!info) {
    style_value_combo_.ShowWindow(SW_HIDE);
    style_value_edit_.ShowWindow(SW_HIDE);
    set_style_.EnableWindow(FALSE);
    return;
  }

  std::string current;
  settings_->GetStyleValue(*info, &current);

  if (info->type == weasel::StyleValueType::Int ||
      info->type == weasel::StyleValueType::Str) {
    style_value_combo_.ShowWindow(SW_HIDE);
    set_style_.ShowWindow(SW_SHOW);
    set_style_.EnableWindow(TRUE);
    style_value_edit_.ShowWindow(SW_SHOW);
    style_value_edit_.SetWindowText(u8tow(current).c_str());
    return;
  }

  // Bool / Enum / Theme use a read-only dropdown.
  style_value_edit_.ShowWindow(SW_HIDE);
  set_style_.ShowWindow(SW_HIDE);
  style_value_combo_.ShowWindow(SW_SHOW);
  style_value_combo_.ResetContent();

  int select = -1;
  if (info->type == weasel::StyleValueType::Boolean) {
    style_value_combo_.AddString(L"true");
    style_value_combo_.AddString(L"false");
    select = (current == "true") ? 0 : 1;
  } else if (info->type == weasel::StyleValueType::Theme) {
    for (size_t i = 0; i < preset_.size(); ++i) {
      style_value_combo_.AddString(u8tow(preset_[i].name).c_str());
      if (preset_[i].color_scheme_id == current)
        select = static_cast<int>(i);
    }
  } else {  // Enum
    for (size_t i = 0; i < info->choice_count; ++i) {
      style_value_combo_.AddString(
          u8tow(weasel::Localize(info->choices[i].label)).c_str());
      if (info->choices[i].value == current)
        select = static_cast<int>(i);
    }
  }
  style_value_combo_.SetCurSel(select < 0 ? 0 : select);
}

LRESULT UIStyleSettingsDialog::OnStyleListSelChange(WORD, WORD, HWND, BOOL&) {
  UpdateStyleValue();
  return 0;
}

LRESULT UIStyleSettingsDialog::OnStyleValueSelChange(WORD, WORD, HWND, BOOL&) {
  const weasel::StyleKeyInfo* info = SelectedStyleKey();
  if (!info)
    return 0;
  const int index = style_value_combo_.GetCurSel();
  if (index < 0)
    return 0;

  if (info->type == weasel::StyleValueType::Theme) {
    if (index >= static_cast<int>(preset_.size()))
      return 0;
    const std::string& id = preset_[index].color_scheme_id;
    // The dark scheme is stored as a plain style key; it does not switch the
    // preview's active (light) color scheme.
    if (std::string(info->key) == "style/color_scheme_dark") {
      ApplyStyleValue(*info, id);
      return 0;
    }
    settings_->SelectColorScheme(id);
    color_schemes_.SetCurSel(index);
    PopulateColorSettings();
    RefreshStyleList();
    UpdatePreview();
    return 0;
  }

  std::string value;
  if (info->type == weasel::StyleValueType::Boolean) {
    value = (index == 0) ? "true" : "false";
  } else {  // Enum
    if (index >= static_cast<int>(info->choice_count))
      return 0;
    value = info->choices[index].value;
  }
  ApplyStyleValue(*info, value);
  return 0;
}

LRESULT UIStyleSettingsDialog::OnSetStyle(WORD, WORD, HWND, BOOL&) {
  const weasel::StyleKeyInfo* info = SelectedStyleKey();
  if (!info || (info->type != weasel::StyleValueType::Int &&
                info->type != weasel::StyleValueType::Str))
    return 0;
  CString value;
  style_value_edit_.GetWindowText(value);
  const std::string value_u8 = wtou8(std::wstring(value.GetString()));
  if (!ApplyStyleValue(*info, value_u8)) {
    MessageBox(L"输入的值无效。", L"样式设置", MB_ICONWARNING | MB_OK);
  }
  return 0;
}
