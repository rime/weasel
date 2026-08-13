#pragma once

#include "resource.h"
#include "UIStyleSettings.h"
#include <CDialogDpiAware.h>
#include <WeaselUI.h>
#include <map>
#include <set>

class UIStyleSettingsDialog : public CDialogDpiAware<UIStyleSettingsDialog> {
 public:
  enum { IDD = IDD_STYLE_SETTING };

  UIStyleSettingsDialog(UIStyleSettings* settings);
  ~UIStyleSettingsDialog();

 protected:
  BEGIN_MSG_MAP(UIStyleSettingsDialog)
  CHAIN_MSG_MAP(CDialogDpiAware<UIStyleSettingsDialog>)
  MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
  MESSAGE_HANDLER(WM_WINDOWPOSCHANGED, OnWindowPosChanged)
  MESSAGE_HANDLER(WM_MEASUREITEM, OnMeasureItem)
  MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
  MESSAGE_HANDLER(WM_CLOSE, OnClose)
  COMMAND_ID_HANDLER(IDOK, OnOK)
  COMMAND_ID_HANDLER(IDC_SELECT_FONT, OnSelectFont)
  COMMAND_HANDLER(IDC_COLOR_SCHEME, CBN_SELCHANGE, OnColorSchemeSelChange)
  COMMAND_HANDLER(IDC_COLOR_LIST, LBN_SELCHANGE, OnColorSettingSelChange)
  COMMAND_HANDLER(IDC_SET_COLOR, BN_CLICKED, OnSetColor)
  COMMAND_HANDLER(IDC_STYLE_LIST, LBN_SELCHANGE, OnStyleListSelChange)
  COMMAND_HANDLER(IDC_STYLE_VALUE_COMBO, CBN_SELCHANGE, OnStyleValueSelChange)
  COMMAND_HANDLER(IDC_SET_STYLE, BN_CLICKED, OnSetStyle)
  END_MSG_MAP()

  LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL&);
  LRESULT OnWindowPosChanged(UINT, WPARAM, LPARAM, BOOL&);
  LRESULT OnMeasureItem(UINT, WPARAM, LPARAM, BOOL&);
  LRESULT OnDrawItem(UINT, WPARAM, LPARAM, BOOL&);
  LRESULT OnClose(UINT, WPARAM, LPARAM, BOOL&);
  LRESULT OnOK(WORD, WORD code, HWND, BOOL&);
  LRESULT OnSelectFont(WORD, WORD code, HWND, BOOL&);
  LRESULT OnColorSchemeSelChange(WORD, WORD, HWND, BOOL&);
  LRESULT OnColorSettingSelChange(WORD, WORD, HWND, BOOL&);
  LRESULT OnSetColor(WORD, WORD, HWND, BOOL&);
  LRESULT OnStyleListSelChange(WORD, WORD, HWND, BOOL&);
  LRESULT OnStyleValueSelChange(WORD, WORD, HWND, BOOL&);
  LRESULT OnSetStyle(WORD, WORD, HWND, BOOL&);

  void Populate();
  void UpdatePreview();
  void PopulateColorSettings();
  void UpdateColorValue();
  void PopulateStyleKeys();
  void UpdateStyleValue();
  void RefreshStyleList();
  bool ApplyStyleValue(const weasel::StyleKeyInfo& info,
                       const std::string& value);
  const weasel::StyleKeyInfo* SelectedStyleKey() const;

  UIStyleSettings* settings_;
  bool loaded_;
  std::vector<ColorSchemeInfo> preset_;
  std::vector<ColorSettingInfo> color_settings_;
  std::set<std::string> modified_schemes_;
  std::map<std::string, std::vector<ColorSettingInfo>> original_colors_;
  std::string original_color_scheme_;
  // Baseline values for exposed style keys, captured when the dialog opens;
  // a key is "modified" (marked with "+") while its current value differs.
  std::map<std::string, std::string> original_style_values_;

  CComboBox color_schemes_;
  CListBox color_list_;
  CEdit color_value_;
  CButton set_color_;
  CListBox style_list_;
  CComboBox style_value_combo_;
  CEdit style_value_edit_;
  CButton set_style_;
  std::unique_ptr<weasel::UI> preview_ui_;
  weasel::Context preview_context_;
  weasel::Status preview_status_;
};
