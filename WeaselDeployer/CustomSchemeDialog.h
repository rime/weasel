#pragma once

// Custom color scheme editor dialog ("自定义配色").
// Lets the user edit every color of a preset_color_schemes/custom scheme
// with HSV pickers and an alpha slider, with a live mock candidate window
// preview, and export the scheme as weasel/squirrel YAML snippets.

#include "resource.h"
#include "ColorSchemePreview.h"
#include "UIStyleSettings.h"

#include <gdiplus.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

// Posted to the dialog when a picker control changes the current color.
// wParam: control id; lParam: unused.
#define WM_CUSTOM_COLOR_CHANGED (WM_APP + 100)

namespace weasel_deployer {

// Converts a 0xRRGGBBAA color to HSV. h in [0,360), s/v in [0,1].
inline void RgbaToHsv(uint32_t rgba, double* h, double* s, double* v) {
  double r = ((rgba >> 24) & 0xff) / 255.0;
  double g = ((rgba >> 16) & 0xff) / 255.0;
  double b = ((rgba >> 8) & 0xff) / 255.0;
  double hi = (std::max)(r, (std::max)(g, b));
  double lo = (std::min)(r, (std::min)(g, b));
  double delta = hi - lo;
  *v = hi;
  *s = hi > 0.0 ? delta / hi : 0.0;
  if (delta <= 0.0) {
    *h = 0.0;
  } else if (hi == r) {
    *h = 60.0 * fmod((g - b) / delta, 6.0);
  } else if (hi == g) {
    *h = 60.0 * ((b - r) / delta + 2.0);
  } else {
    *h = 60.0 * ((r - g) / delta + 4.0);
  }
  if (*h < 0.0)
    *h += 360.0;
}

// Converts HSV + alpha to a 0xRRGGBBAA color. h in [0,360), s/v in [0,1].
inline uint32_t HsvToRgba(double h, double s, double v, uint8_t a) {
  double c = v * s;
  double x = c * (1.0 - fabs(fmod(h / 60.0, 2.0) - 1.0));
  double m = v - c;
  double r = 0, g = 0, b = 0;
  if (h < 60) {
    r = c;
    g = x;
  } else if (h < 120) {
    r = x;
    g = c;
  } else if (h < 180) {
    g = c;
    b = x;
  } else if (h < 240) {
    g = x;
    b = c;
  } else if (h < 300) {
    r = x;
    b = c;
  } else {
    r = c;
    b = x;
  }
  uint32_t ri = static_cast<uint32_t>((r + m) * 255.0 + 0.5);
  uint32_t gi = static_cast<uint32_t>((g + m) * 255.0 + 0.5);
  uint32_t bi = static_cast<uint32_t>((b + m) * 255.0 + 0.5);
  return (ri << 24) | (gi << 16) | (bi << 8) | a;
}

}  // namespace weasel_deployer

// Saturation/brightness square picker for the current hue.
class CSvBox : public CWindowImpl<CSvBox, CStatic> {
 public:
  BEGIN_MSG_MAP(CSvBox)
  MESSAGE_HANDLER(WM_PAINT, OnPaint)
  MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
  MESSAGE_HANDLER(WM_NCHITTEST, OnNcHitTest)
  MESSAGE_HANDLER(WM_LBUTTONDOWN, OnMouseDown)
  MESSAGE_HANDLER(WM_LBUTTONUP, OnMouseUp)
  MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
  END_MSG_MAP()

  void SetHue(double hue) {
    hue_ = hue;
    if (IsWindow())
      Invalidate(FALSE);
  }
  void SetSv(double s, double v) {
    s_ = s;
    v_ = v;
    if (IsWindow())
      Invalidate(FALSE);
  }
  double saturation() const { return s_; }
  double brightness() const { return v_; }

 private:
  LRESULT OnPaint(UINT, WPARAM, LPARAM, BOOL&);
  LRESULT OnEraseBkgnd(UINT, WPARAM, LPARAM, BOOL&) { return 1; }
  // Static controls without SS_NOTIFY return HTTRANSPARENT and never receive
  // mouse input; report HTCLIENT so the pickers are clickable.
  LRESULT OnNcHitTest(UINT, WPARAM, LPARAM, BOOL&) { return HTCLIENT; }
  LRESULT OnMouseDown(UINT, WPARAM, LPARAM, BOOL&);
  LRESULT OnMouseUp(UINT, WPARAM, LPARAM, BOOL&);
  LRESULT OnMouseMove(UINT, WPARAM, LPARAM, BOOL&);
  void UpdateFromPoint(int x, int y);
  void NotifyChanged() {
    ::PostMessage(GetParent(), WM_CUSTOM_COLOR_CHANGED, GetDlgCtrlID(), 0);
  }

  double hue_ = 0.0;
  double s_ = 0.0;
  double v_ = 1.0;
  bool dragging_ = false;
};

// Horizontal slider for hue (rainbow) or alpha (checkerboard + color fade).
class CColorSliderBar : public CWindowImpl<CColorSliderBar, CStatic> {
 public:
  enum Mode { kHue, kAlpha };

  BEGIN_MSG_MAP(CColorSliderBar)
  MESSAGE_HANDLER(WM_PAINT, OnPaint)
  MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
  MESSAGE_HANDLER(WM_NCHITTEST, OnNcHitTest)
  MESSAGE_HANDLER(WM_LBUTTONDOWN, OnMouseDown)
  MESSAGE_HANDLER(WM_LBUTTONUP, OnMouseUp)
  MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
  END_MSG_MAP()

  void SetMode(Mode mode) { mode_ = mode; }
  // value: hue in [0,360) for kHue, alpha in [0,255] for kAlpha.
  void SetValue(double value) {
    value_ = value;
    if (IsWindow())
      Invalidate(FALSE);
  }
  // Base opaque color used by the alpha gradient.
  void SetBaseColor(uint32_t rgba) {
    base_rgb_ = rgba & 0xffffff00;
    if (IsWindow())
      Invalidate(FALSE);
  }
  double value() const { return value_; }

 private:
  LRESULT OnPaint(UINT, WPARAM, LPARAM, BOOL&);
  LRESULT OnEraseBkgnd(UINT, WPARAM, LPARAM, BOOL&) { return 1; }
  LRESULT OnNcHitTest(UINT, WPARAM, LPARAM, BOOL&) { return HTCLIENT; }
  LRESULT OnMouseDown(UINT, WPARAM, LPARAM, BOOL&);
  LRESULT OnMouseUp(UINT, WPARAM, LPARAM, BOOL&);
  LRESULT OnMouseMove(UINT, WPARAM, LPARAM, BOOL&);
  void UpdateFromPoint(int x);
  void NotifyChanged() {
    ::PostMessage(GetParent(), WM_CUSTOM_COLOR_CHANGED, GetDlgCtrlID(), 0);
  }

  Mode mode_ = kHue;
  double value_ = 0.0;
  uint32_t base_rgb_ = 0xff000000;
  bool dragging_ = false;
};

// Live mock candidate window preview pane.
class CPreviewPane : public CWindowImpl<CPreviewPane, CStatic> {
 public:
  BEGIN_MSG_MAP(CPreviewPane)
  MESSAGE_HANDLER(WM_PAINT, OnPaint)
  MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
  END_MSG_MAP()

  void SetColors(const weasel_deployer::RgbaColorMap* colors) {
    colors_ = colors;
    if (IsWindow())
      Invalidate(FALSE);
  }

 private:
  LRESULT OnPaint(UINT, WPARAM, LPARAM, BOOL&);
  LRESULT OnEraseBkgnd(UINT, WPARAM, LPARAM, BOOL&) { return 1; }

  const weasel_deployer::RgbaColorMap* colors_ = nullptr;
};

class CustomSchemeDialog : public CDialogImpl<CustomSchemeDialog> {
 public:
  enum { IDD = IDD_CUSTOM_SCHEME };

  // base_scheme: id of the preset scheme used as the starting point when the
  // user has no saved custom scheme yet.
  CustomSchemeDialog(UIStyleSettings* settings, const std::string& base_scheme);
  ~CustomSchemeDialog();

 protected:
  BEGIN_MSG_MAP(CustomSchemeDialog)
  MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
  MESSAGE_HANDLER(WM_CLOSE, OnClose)
  MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
  MESSAGE_HANDLER(WM_MEASUREITEM, OnMeasureItem)
  MESSAGE_HANDLER(WM_CUSTOM_COLOR_CHANGED, OnPickerChanged)
  COMMAND_ID_HANDLER(IDOK, OnOK)
  COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
  COMMAND_HANDLER(IDC_ROLE_LIST, LBN_SELCHANGE, OnRoleSelChange)
  COMMAND_HANDLER(IDC_HEX_EDIT, EN_CHANGE, OnHexChange)
  COMMAND_HANDLER(IDC_HEX_EDIT, EN_KILLFOCUS, OnHexKillFocus)
  COMMAND_HANDLER(IDC_EXPORT_YAML, BN_CLICKED, OnExportYaml)
  END_MSG_MAP()

  LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL&);
  LRESULT OnClose(UINT, WPARAM, LPARAM, BOOL&);
  LRESULT OnDrawItem(UINT, WPARAM, LPARAM, BOOL&);
  LRESULT OnMeasureItem(UINT, WPARAM, LPARAM, BOOL&);
  LRESULT OnPickerChanged(UINT, WPARAM, LPARAM, BOOL&);
  LRESULT OnOK(WORD, WORD, HWND, BOOL&);
  LRESULT OnCancel(WORD, WORD, HWND, BOOL&);
  LRESULT OnRoleSelChange(WORD, WORD, HWND, BOOL&);
  LRESULT OnHexChange(WORD, WORD, HWND, BOOL&);
  LRESULT OnHexKillFocus(WORD, WORD, HWND, BOOL&);
  LRESULT OnExportYaml(WORD, WORD, HWND, BOOL&);

  void BuildRoleList();
  void LoadColors();
  void SelectRole(int role_index);
  void ApplyPickersToColor();
  void SyncPickersFromColor();
  void UpdateHexText();
  bool ApplyHexText();
  void RefreshPreview();
  bool SaveSchemeAndPreview();
  void ExportYaml(bool for_squirrel);

  static std::string WeaselYaml(const weasel_deployer::RgbaColorMap& colors,
                                const std::string& name);
  static std::string SquirrelYaml(const weasel_deployer::RgbaColorMap& colors,
                                  const std::string& name);

  UIStyleSettings* settings_;
  std::string base_scheme_;
  weasel_deployer::RgbaColorMap colors_;
  int current_role_ = -1;
  double hue_ = 0.0, sat_ = 0.0, val_ = 1.0;
  uint8_t alpha_ = 0xff;
  bool syncing_ = false;

  ULONG_PTR gdiplus_token_ = 0;

  CListBox role_list_;
  CSvBox sv_box_;
  CColorSliderBar hue_slider_;
  CColorSliderBar alpha_slider_;
  CEdit hex_edit_;
  CPreviewPane preview_pane_;
};
