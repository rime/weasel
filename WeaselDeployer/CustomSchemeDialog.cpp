#include "stdafx.h"
#include "CustomSchemeDialog.h"

#include <WeaselUtility.h>
#include <rime_api.h>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <sstream>

#pragma comment(lib, "gdiplus.lib")

namespace {

using weasel_deployer::RgbaColorMap;

// ---------------------------------------------------------------------------
// UI language selection (chs / cht / en), matching the three .rc sections.

enum UiLang { kChs = 0, kCht = 1, kEn = 2 };

UiLang GetUiLang() {
  LANGID id = GetUserDefaultUILanguage();
  if (PRIMARYLANGID(id) == LANG_CHINESE) {
    switch (SUBLANGID(id)) {
      case SUBLANG_CHINESE_TRADITIONAL:
      case SUBLANG_CHINESE_HONGKONG:
      case SUBLANG_CHINESE_MACAU:
        return kCht;
      default:
        return kChs;
    }
  }
  return kEn;
}

const wchar_t* Tr(const wchar_t* chs, const wchar_t* cht, const wchar_t* en) {
  switch (GetUiLang()) {
    case kChs:
      return chs;
    case kCht:
      return cht;
    default:
      return en;
  }
}

// ---------------------------------------------------------------------------
// Editable color roles, grouped like the mock candidate window parts.
// The order matches the groups below; group headers are list-only items.

struct RoleDef {
  const char* key;
  const wchar_t* name_chs;
  const wchar_t* name_cht;
  const wchar_t* name_en;
};

struct GroupDef {
  const wchar_t* name_chs;
  const wchar_t* name_cht;
  const wchar_t* name_en;
  int first_role;
  int role_count;
};

// clang-format off
const RoleDef kRoles[] = {
    // window
    {"back_color",            L"背景色",       L"背景色",       L"Background"},
    {"border_color",          L"边框色",       L"邊框色",       L"Border"},
    {"shadow_color",          L"阴影色",       L"陰影色",       L"Shadow"},
    // preedit
    {"text_color",            L"编码文字",     L"編碼文字",     L"Preedit text"},
    {"hilited_text_color",    L"高亮编码",     L"高亮編碼",     L"Highlighted preedit"},
    {"hilited_back_color",    L"高亮编码背景", L"高亮編碼背景", L"Highlighted preedit bg"},
    {"hilited_shadow_color",  L"高亮编码阴影", L"高亮編碼陰影", L"Highlighted preedit shadow"},
    // candidates
    {"candidate_text_color",  L"候选文字",     L"候選文字",     L"Candidate text"},
    {"candidate_back_color",  L"候选背景",     L"候選背景",     L"Candidate background"},
    {"candidate_shadow_color", L"候选阴影",    L"候選陰影",     L"Candidate shadow"},
    {"candidate_border_color", L"候选边框",    L"候選邊框",     L"Candidate border"},
    {"label_color",           L"序号颜色",     L"序號顏色",     L"Label"},
    {"comment_text_color",    L"注释颜色",     L"註釋顏色",     L"Comment"},
    // highlighted candidate
    {"hilited_candidate_text_color",   L"高亮候选文字", L"高亮候選文字", L"Hilited candidate text"},
    {"hilited_candidate_back_color",   L"高亮候选背景", L"高亮候選背景", L"Hilited candidate bg"},
    {"hilited_candidate_shadow_color", L"高亮候选阴影", L"高亮候選陰影", L"Hilited candidate shadow"},
    {"hilited_candidate_border_color", L"高亮候选边框", L"高亮候選邊框", L"Hilited candidate border"},
    {"hilited_label_color",            L"高亮序号",     L"高亮序號",     L"Hilited label"},
    {"hilited_comment_text_color",     L"高亮注释",     L"高亮註釋",     L"Hilited comment"},
    // extras
    {"prevpage_color",        L"上一页箭头",   L"上一頁箭頭",   L"Prev-page arrow"},
    {"nextpage_color",        L"下一页箭头",   L"下一頁箭頭",   L"Next-page arrow"},
    {"hilited_mark_color",    L"高亮标记",     L"高亮標記",     L"Hilited mark"},
};
// clang-format on

const int kRoleCount = sizeof(kRoles) / sizeof(kRoles[0]);

const GroupDef kGroups[] = {
    {L"窗口", L"視窗", L"Window", 0, 3},
    {L"编码行", L"編碼行", L"Preedit", 3, 4},
    {L"候选区", L"候選區", L"Candidates", 7, 6},
    {L"高亮候选", L"高亮候選", L"Hilited candidate", 13, 6},
    {L"其他", L"其他", L"Extras", 19, 3},
};
const int kGroupCount = sizeof(kGroups) / sizeof(kGroups[0]);

std::vector<std::string> AllRoleKeys() {
  std::vector<std::string> keys;
  for (int i = 0; i < kRoleCount; ++i)
    keys.push_back(kRoles[i].key);
  return keys;
}

// ---------------------------------------------------------------------------
// Default values replicating the engine's fallback chain
// (see RimeWithWeasel.cpp _UpdateUIStyleColor), in 0xRRGGBBAA space.

uint32_t BlendColors(uint32_t frgba, uint32_t brgba) {
  double fa = (frgba & 0xff) / 255.0;
  double ba = (brgba & 0xff) / 255.0;
  double out_a = fa + ba * (1.0 - fa);
  if (out_a <= 0.0)
    return 0;
  auto ch = [](uint32_t v, int shift) { return (v >> shift) & 0xff; };
  auto mix = [&](double fc, double bc) -> uint32_t {
    return static_cast<uint32_t>((fc * fa + bc * ba * (1.0 - fa)) / out_a);
  };
  uint32_t r = mix(ch(frgba, 24), ch(brgba, 24));
  uint32_t g = mix(ch(frgba, 16), ch(brgba, 16));
  uint32_t b = mix(ch(frgba, 8), ch(brgba, 8));
  uint32_t a = static_cast<uint32_t>(out_a * 255.0);
  return (r << 24) | (g << 16) | (b << 8) | a;
}

void ResolveFallbacks(RgbaColorMap* colors) {
  RgbaColorMap& c = *colors;
  auto get = [&](const char* key, uint32_t fallback) -> uint32_t {
    auto it = c.find(key);
    uint32_t value = it != c.end() ? it->second : fallback;
    c[key] = value;
    return value;
  };
  uint32_t back = get("back_color", 0xffffffff);
  get("shadow_color", 0);
  get("prevpage_color", 0);
  get("nextpage_color", 0);
  uint32_t text = get("text_color", 0x000000ff);
  uint32_t cand_text = get("candidate_text_color", text);
  uint32_t cand_back = get("candidate_back_color", 0);
  get("border_color", text);
  uint32_t hi_text = get("hilited_text_color", text);
  uint32_t hi_back = get("hilited_back_color", back);
  uint32_t hi_cand_text = get("hilited_candidate_text_color", hi_text);
  uint32_t hi_cand_back = get("hilited_candidate_back_color", hi_back);
  get("hilited_candidate_shadow_color", 0);
  get("hilited_shadow_color", 0);
  get("candidate_shadow_color", 0);
  get("candidate_border_color", 0);
  get("hilited_candidate_border_color", 0);
  uint32_t label = get("label_color", BlendColors(cand_text, cand_back));
  uint32_t hi_label =
      get("hilited_label_color", BlendColors(hi_cand_text, hi_cand_back));
  get("comment_text_color", label);
  get("hilited_comment_text_color", hi_label);
  get("hilited_mark_color", 0);
}

// ---------------------------------------------------------------------------
// YAML snippet generation.

std::string HexRgba(uint32_t rgba) {
  char buf[16];
  snprintf(buf, sizeof buf, "0x%08X", rgba);
  return buf;
}

uint32_t RgbaToAbgr(uint32_t rgba) {
  return ((rgba & 0xff) << 24) | (((rgba >> 8) & 0xff) << 16) |
         (((rgba >> 16) & 0xff) << 8) | ((rgba >> 24) & 0xff);
}

bool CopyTextToClipboard(HWND owner, const std::wstring& text) {
  if (!OpenClipboard(owner))
    return false;
  EmptyClipboard();
  size_t size = (text.size() + 1) * sizeof(wchar_t);
  HGLOBAL handle = GlobalAlloc(GMEM_MOVEABLE, size);
  if (!handle) {
    CloseClipboard();
    return false;
  }
  void* data = GlobalLock(handle);
  if (!data) {
    GlobalFree(handle);
    CloseClipboard();
    return false;
  }
  memcpy(data, text.c_str(), size);
  GlobalUnlock(handle);
  if (!SetClipboardData(CF_UNICODETEXT, handle)) {
    GlobalFree(handle);
    CloseClipboard();
    return false;
  }
  CloseClipboard();
  return true;
}

}  // namespace

// ---------------------------------------------------------------------------
// CSvBox

LRESULT CSvBox::OnPaint(UINT, WPARAM, LPARAM, BOOL&) {
  CPaintDC dc(m_hWnd);
  CRect rc;
  GetClientRect(&rc);
  HDC mem_dc = CreateCompatibleDC(dc);
  HBITMAP mem_bmp = CreateCompatibleBitmap(dc, rc.Width(), rc.Height());
  HBITMAP old_bmp = static_cast<HBITMAP>(SelectObject(mem_dc, mem_bmp));
  {
    Gdiplus::Graphics g(mem_dc);
    uint32_t pure = weasel_deployer::HsvToRgba(hue_, 1.0, 1.0, 0xff);
    Gdiplus::SolidBrush white(Gdiplus::Color(255, 255, 255, 255));
    g.FillRectangle(&white, 0, 0, rc.Width(), rc.Height());
    auto argb = [](uint32_t rgba) {
      return Gdiplus::Color(255, static_cast<BYTE>((rgba >> 24) & 0xff),
                            static_cast<BYTE>((rgba >> 16) & 0xff),
                            static_cast<BYTE>((rgba >> 8) & 0xff));
    };
    Gdiplus::LinearGradientBrush hue_brush(
        Gdiplus::Point(0, 0), Gdiplus::Point(rc.Width(), 0),
        Gdiplus::Color(255, 255, 255, 255), argb(pure));
    g.FillRectangle(&hue_brush, 0, 0, rc.Width(), rc.Height());
    Gdiplus::LinearGradientBrush black_brush(
        Gdiplus::Point(0, 0), Gdiplus::Point(0, rc.Height()),
        Gdiplus::Color(0, 0, 0, 0), Gdiplus::Color(255, 0, 0, 0));
    g.FillRectangle(&black_brush, 0, 0, rc.Width(), rc.Height());
    // Thumb.
    float x = static_cast<float>(s_ * (rc.Width() - 1));
    float y = static_cast<float>((1.0 - v_) * (rc.Height() - 1));
    Gdiplus::Pen outline(Gdiplus::Color(255, 0, 0, 0), 3.0f);
    Gdiplus::Pen inner(Gdiplus::Color(255, 255, 255, 255), 1.5f);
    float r = 6.0f;
    g.DrawEllipse(&outline, x - r, y - r, r * 2, r * 2);
    g.DrawEllipse(&inner, x - r, y - r, r * 2, r * 2);
  }
  dc.BitBlt(0, 0, rc.Width(), rc.Height(), mem_dc, 0, 0, SRCCOPY);
  SelectObject(mem_dc, old_bmp);
  DeleteObject(mem_bmp);
  DeleteDC(mem_dc);
  return 0;
}

void CSvBox::UpdateFromPoint(int x, int y) {
  CRect rc;
  GetClientRect(&rc);
  double w = (std::max)(1, rc.Width() - 1);
  double h = (std::max)(1, rc.Height() - 1);
  s_ = (std::max)(0.0, (std::min)(1.0, x / w));
  v_ = 1.0 - (std::max)(0.0, (std::min)(1.0, y / h));
  Invalidate(FALSE);
}

LRESULT CSvBox::OnMouseDown(UINT, WPARAM, LPARAM lparam, BOOL&) {
  dragging_ = true;
  SetCapture();
  UpdateFromPoint(((int)(short)LOWORD(lparam)), ((int)(short)HIWORD(lparam)));
  NotifyChanged();
  return 0;
}

LRESULT CSvBox::OnMouseUp(UINT, WPARAM, LPARAM, BOOL&) {
  if (dragging_) {
    dragging_ = false;
    ReleaseCapture();
  }
  return 0;
}

LRESULT CSvBox::OnMouseMove(UINT, WPARAM, LPARAM lparam, BOOL&) {
  if (dragging_) {
    UpdateFromPoint(((int)(short)LOWORD(lparam)), ((int)(short)HIWORD(lparam)));
    NotifyChanged();
  }
  return 0;
}

// ---------------------------------------------------------------------------
// CColorSliderBar

LRESULT CColorSliderBar::OnPaint(UINT, WPARAM, LPARAM, BOOL&) {
  CPaintDC dc(m_hWnd);
  CRect rc;
  GetClientRect(&rc);
  HDC mem_dc = CreateCompatibleDC(dc);
  HBITMAP mem_bmp = CreateCompatibleBitmap(dc, rc.Width(), rc.Height());
  HBITMAP old_bmp = static_cast<HBITMAP>(SelectObject(mem_dc, mem_bmp));
  {
    Gdiplus::Graphics g(mem_dc);
    Gdiplus::Rect rect(0, 0, rc.Width(), rc.Height());
    if (mode_ == kHue) {
      Gdiplus::LinearGradientBrush brush(rect, Gdiplus::Color(255, 255, 0, 0),
                                         Gdiplus::Color(255, 255, 0, 0),
                                         Gdiplus::LinearGradientModeHorizontal);
      const int stops = 7;
      Gdiplus::Color colors[stops];
      Gdiplus::REAL positions[stops];
      for (int i = 0; i < stops; ++i) {
        uint32_t rgb = weasel_deployer::HsvToRgba(i * 60.0, 1.0, 1.0, 0xff);
        colors[i] = Gdiplus::Color(255, static_cast<BYTE>((rgb >> 24) & 0xff),
                                   static_cast<BYTE>((rgb >> 16) & 0xff),
                                   static_cast<BYTE>((rgb >> 8) & 0xff));
        positions[i] = i / (Gdiplus::REAL)(stops - 1);
      }
      brush.SetInterpolationColors(colors, positions, stops);
      g.FillRectangle(&brush, rect);
    } else {
      // Checkerboard, then a fade from transparent to the opaque base color.
      const float cell = 6.0f;
      Gdiplus::SolidBrush light(Gdiplus::Color(255, 235, 235, 235));
      Gdiplus::SolidBrush dark(Gdiplus::Color(255, 200, 200, 200));
      g.FillRectangle(&light, rect);
      for (int y = 0; y < rect.Height; y += static_cast<int>(cell)) {
        for (int x = 0; x < rect.Width; x += static_cast<int>(cell)) {
          if ((x / static_cast<int>(cell) + y / static_cast<int>(cell)) % 2)
            g.FillRectangle(&dark, static_cast<float>(x), static_cast<float>(y),
                            cell, cell);
        }
      }
      Gdiplus::LinearGradientBrush brush(
          rect,
          Gdiplus::Color(0, static_cast<BYTE>((base_rgb_ >> 24) & 0xff),
                         static_cast<BYTE>((base_rgb_ >> 16) & 0xff),
                         static_cast<BYTE>((base_rgb_ >> 8) & 0xff)),
          Gdiplus::Color(255, static_cast<BYTE>((base_rgb_ >> 24) & 0xff),
                         static_cast<BYTE>((base_rgb_ >> 16) & 0xff),
                         static_cast<BYTE>((base_rgb_ >> 8) & 0xff)),
          Gdiplus::LinearGradientModeHorizontal);
      g.FillRectangle(&brush, rect);
    }
    // Thumb.
    double ratio = mode_ == kHue ? value_ / 360.0 : value_ / 255.0;
    float x = static_cast<float>(ratio * (rc.Width() - 1));
    Gdiplus::Pen outline(Gdiplus::Color(255, 0, 0, 0), 1.0f);
    Gdiplus::SolidBrush thumb(Gdiplus::Color(255, 255, 255, 255));
    Gdiplus::RectF thumb_rect(x - 3.0f, 0.0f, 6.0f,
                              static_cast<Gdiplus::REAL>(rc.Height() - 1));
    g.FillRectangle(&thumb, thumb_rect);
    g.DrawRectangle(&outline, thumb_rect);
  }
  dc.BitBlt(0, 0, rc.Width(), rc.Height(), mem_dc, 0, 0, SRCCOPY);
  SelectObject(mem_dc, old_bmp);
  DeleteObject(mem_bmp);
  DeleteDC(mem_dc);
  return 0;
}

void CColorSliderBar::UpdateFromPoint(int x) {
  CRect rc;
  GetClientRect(&rc);
  double w = (std::max)(1, rc.Width() - 1);
  double ratio = (std::max)(0.0, (std::min)(1.0, x / w));
  value_ = mode_ == kHue ? ratio * 360.0 : ratio * 255.0;
  if (mode_ == kHue && value_ >= 360.0)
    value_ = 359.999;
  Invalidate(FALSE);
}

LRESULT CColorSliderBar::OnMouseDown(UINT, WPARAM, LPARAM lparam, BOOL&) {
  dragging_ = true;
  SetCapture();
  UpdateFromPoint(((int)(short)LOWORD(lparam)));
  NotifyChanged();
  return 0;
}

LRESULT CColorSliderBar::OnMouseUp(UINT, WPARAM, LPARAM, BOOL&) {
  if (dragging_) {
    dragging_ = false;
    ReleaseCapture();
  }
  return 0;
}

LRESULT CColorSliderBar::OnMouseMove(UINT, WPARAM, LPARAM lparam, BOOL&) {
  if (dragging_) {
    UpdateFromPoint(((int)(short)LOWORD(lparam)));
    NotifyChanged();
  }
  return 0;
}

// ---------------------------------------------------------------------------
// CPreviewPane

LRESULT CPreviewPane::OnPaint(UINT, WPARAM, LPARAM, BOOL&) {
  CPaintDC dc(m_hWnd);
  CRect rc;
  GetClientRect(&rc);
  HBRUSH bg = GetSysColorBrush(COLOR_BTNFACE);
  dc.FillRect(&rc, bg);
  CRect inner = rc;
  inner.DeflateRect(1, 1);
  if (colors_)
    weasel_deployer::PaintMockCandidateWindow(dc, inner, *colors_);
  return 0;
}

// ---------------------------------------------------------------------------
// CustomSchemeDialog

CustomSchemeDialog::CustomSchemeDialog(UIStyleSettings* settings,
                                       const std::string& base_scheme)
    : settings_(settings), base_scheme_(base_scheme) {
  Gdiplus::GdiplusStartupInput input;
  Gdiplus::GdiplusStartup(&gdiplus_token_, &input, nullptr);
}

CustomSchemeDialog::~CustomSchemeDialog() {
  if (gdiplus_token_)
    Gdiplus::GdiplusShutdown(gdiplus_token_);
}

void CustomSchemeDialog::LoadColors() {
  colors_.clear();
  std::vector<std::string> keys = AllRoleKeys();
  // Start from the base preset's explicitly set colors, then overlay the
  // user's previously saved custom scheme (if any), then resolve fallbacks.
  settings_->GetSchemeColors(base_scheme_, keys, &colors_);
  settings_->GetSchemeColors("custom", keys, &colors_);
  ResolveFallbacks(&colors_);
}

void CustomSchemeDialog::BuildRoleList() {
  role_list_.ResetContent();
  for (int g = 0; g < kGroupCount; ++g) {
    const GroupDef& group = kGroups[g];
    std::wstring header = L"【";
    header += Tr(group.name_chs, group.name_cht, group.name_en);
    header += L"】";
    int header_index = role_list_.AddString(header.c_str());
    role_list_.SetItemData(header_index, static_cast<DWORD_PTR>(-1));
    for (int i = 0; i < group.role_count; ++i) {
      const RoleDef& role = kRoles[group.first_role + i];
      std::wstring name = Tr(role.name_chs, role.name_cht, role.name_en);
      int index = role_list_.AddString(name.c_str());
      role_list_.SetItemData(index,
                             static_cast<DWORD_PTR>(group.first_role + i));
    }
  }
}

LRESULT CustomSchemeDialog::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&) {
  role_list_.Attach(GetDlgItem(IDC_ROLE_LIST));
  sv_box_.SubclassWindow(GetDlgItem(IDC_SV_BOX));
  hue_slider_.SubclassWindow(GetDlgItem(IDC_HUE_SLIDER));
  alpha_slider_.SubclassWindow(GetDlgItem(IDC_ALPHA_SLIDER));
  hex_edit_.Attach(GetDlgItem(IDC_HEX_EDIT));
  preview_pane_.SubclassWindow(GetDlgItem(IDC_CUSTOM_PREVIEW));

  hue_slider_.SetMode(CColorSliderBar::kHue);
  alpha_slider_.SetMode(CColorSliderBar::kAlpha);
  preview_pane_.SetColors(&colors_);

  LoadColors();
  BuildRoleList();

  // Select the first real role (skip the group header).
  role_list_.SetCurSel(1);
  int role = static_cast<int>(role_list_.GetItemData(1));
  SelectRole(role);

  CenterWindow();
  BringWindowToTop();
  return TRUE;
}

LRESULT CustomSchemeDialog::OnClose(UINT, WPARAM, LPARAM, BOOL&) {
  EndDialog(IDCANCEL);
  return 0;
}

LRESULT CustomSchemeDialog::OnCancel(WORD, WORD, HWND, BOOL&) {
  EndDialog(IDCANCEL);
  return 0;
}

void CustomSchemeDialog::SelectRole(int role_index) {
  if (role_index < 0 || role_index >= kRoleCount)
    return;
  current_role_ = role_index;
  uint32_t rgba = colors_[kRoles[role_index].key];
  alpha_ = static_cast<uint8_t>(rgba & 0xff);
  weasel_deployer::RgbaToHsv(rgba, &hue_, &sat_, &val_);
  SyncPickersFromColor();
  UpdateHexText();
}

void CustomSchemeDialog::SyncPickersFromColor() {
  syncing_ = true;
  sv_box_.SetHue(hue_);
  sv_box_.SetSv(sat_, val_);
  hue_slider_.SetValue(hue_);
  uint32_t rgb = weasel_deployer::HsvToRgba(hue_, sat_, val_, 0xff);
  alpha_slider_.SetBaseColor(rgb);
  alpha_slider_.SetValue(alpha_);
  syncing_ = false;
}

void CustomSchemeDialog::ApplyPickersToColor() {
  if (current_role_ < 0)
    return;
  uint32_t rgba = weasel_deployer::HsvToRgba(hue_, sat_, val_, alpha_);
  colors_[kRoles[current_role_].key] = rgba;
  UpdateHexText();
  RefreshPreview();
  // Repaint the swatch of the current role.
  int sel = role_list_.GetCurSel();
  if (sel >= 0) {
    CRect rc;
    role_list_.GetItemRect(sel, &rc);
    role_list_.InvalidateRect(&rc, FALSE);
  }
}

LRESULT CustomSchemeDialog::OnPickerChanged(UINT,
                                            WPARAM wparam,
                                            LPARAM,
                                            BOOL&) {
  if (syncing_)
    return 0;
  switch (wparam) {
    case IDC_SV_BOX:
      sat_ = sv_box_.saturation();
      val_ = sv_box_.brightness();
      break;
    case IDC_HUE_SLIDER:
      hue_ = hue_slider_.value();
      sv_box_.SetHue(hue_);
      break;
    case IDC_ALPHA_SLIDER:
      alpha_ = static_cast<uint8_t>(alpha_slider_.value() + 0.5);
      break;
  }
  // Keep the alpha slider's base color in sync with the current RGB.
  uint32_t rgb = weasel_deployer::HsvToRgba(hue_, sat_, val_, 0xff);
  alpha_slider_.SetBaseColor(rgb);
  ApplyPickersToColor();
  return 0;
}

LRESULT CustomSchemeDialog::OnRoleSelChange(WORD, WORD, HWND, BOOL&) {
  int sel = role_list_.GetCurSel();
  if (sel < 0)
    return 0;
  DWORD_PTR data = role_list_.GetItemData(sel);
  if (data == static_cast<DWORD_PTR>(-1)) {
    // Group header: not selectable, move to the next role.
    if (sel + 1 < role_list_.GetCount()) {
      role_list_.SetCurSel(sel + 1);
      data = role_list_.GetItemData(sel + 1);
    } else {
      return 0;
    }
  }
  SelectRole(static_cast<int>(data));
  return 0;
}

void CustomSchemeDialog::UpdateHexText() {
  if (current_role_ < 0)
    return;
  char buf[16];
  snprintf(buf, sizeof buf, "#%08X", colors_[kRoles[current_role_].key]);
  syncing_ = true;
  hex_edit_.SetWindowText(u8tow(buf).c_str());
  syncing_ = false;
}

bool CustomSchemeDialog::ApplyHexText() {
  if (current_role_ < 0)
    return false;
  CStringW text;
  hex_edit_.GetWindowText(text);
  std::string str = wtou8(text.GetString());
  size_t start = 0;
  if (str.compare(0, 1, "#") == 0)
    start = 1;
  else if (str.compare(0, 2, "0x") == 0 || str.compare(0, 2, "0X") == 0)
    start = 2;
  std::string hex = str.substr(start);
  if (hex.length() != 6 && hex.length() != 8)
    return false;
  for (char c : hex) {
    if (!isxdigit(static_cast<unsigned char>(c)))
      return false;
  }
  uint32_t value = std::stoul(hex, nullptr, 16);
  if (hex.length() == 6)
    value = (value << 8) | 0xff;
  colors_[kRoles[current_role_].key] = value;
  alpha_ = static_cast<uint8_t>(value & 0xff);
  weasel_deployer::RgbaToHsv(value, &hue_, &sat_, &val_);
  SyncPickersFromColor();
  RefreshPreview();
  int sel = role_list_.GetCurSel();
  if (sel >= 0) {
    CRect rc;
    role_list_.GetItemRect(sel, &rc);
    role_list_.InvalidateRect(&rc, FALSE);
  }
  return true;
}

LRESULT CustomSchemeDialog::OnHexChange(WORD, WORD, HWND, BOOL&) {
  // Live-apply as soon as the text forms a complete valid color (6/8 hex
  // digits); partial input is simply ignored until it parses.
  if (!syncing_)
    ApplyHexText();
  return 0;
}

LRESULT CustomSchemeDialog::OnHexKillFocus(WORD, WORD, HWND, BOOL&) {
  if (!syncing_)
    ApplyHexText();
  return 0;
}

void CustomSchemeDialog::RefreshPreview() {
  if (preview_pane_.IsWindow())
    preview_pane_.Invalidate(FALSE);
}

LRESULT CustomSchemeDialog::OnMeasureItem(UINT, WPARAM, LPARAM lparam, BOOL&) {
  auto* measure = reinterpret_cast<LPMEASUREITEMSTRUCT>(lparam);
  if (measure->CtlID == IDC_ROLE_LIST) {
    HDC dc = GetDC();
    int dpi = dc ? GetDeviceCaps(dc, LOGPIXELSY) : 96;
    if (dc)
      ReleaseDC(dc);
    measure->itemHeight = MulDiv(18, dpi, 96);
    return TRUE;
  }
  return FALSE;
}

LRESULT CustomSchemeDialog::OnDrawItem(UINT, WPARAM, LPARAM lparam, BOOL&) {
  auto* draw = reinterpret_cast<LPDRAWITEMSTRUCT>(lparam);
  if (draw->CtlID != IDC_ROLE_LIST)
    return FALSE;
  if (draw->itemID == static_cast<UINT>(-1))
    return TRUE;
  // CDCHandle (not CDC): the DC in a DRAWITEMSTRUCT is owned by the system.
  CDCHandle dc(draw->hDC);
  bool is_header = draw->itemData == static_cast<DWORD_PTR>(-1);
  bool selected = (draw->itemState & ODS_SELECTED) != 0;
  COLORREF bg =
      selected ? GetSysColor(COLOR_HIGHLIGHT) : GetSysColor(COLOR_WINDOW);
  if (is_header)
    bg = GetSysColor(COLOR_BTNFACE);
  dc.FillSolidRect(&draw->rcItem, bg);

  int text_len = role_list_.GetTextLen(draw->itemID);
  std::wstring text(static_cast<size_t>(text_len) + 1, L'\0');
  role_list_.GetText(draw->itemID, &text[0]);
  text.resize(text_len);

  int left = draw->rcItem.left + 4;
  if (!is_header) {
    // Color swatch; translucent colors are blended over a checkerboard.
    int role = static_cast<int>(draw->itemData);
    if (role >= 0 && role < kRoleCount) {
      uint32_t rgba = colors_[kRoles[role].key];
      CRect swatch(draw->rcItem.left + 4, draw->rcItem.top + 2,
                   draw->rcItem.left + 18, draw->rcItem.bottom - 2);
      double a = (rgba & 0xff) / 255.0;
      auto channel = [&](int shift, int cell_shade) -> int {
        double fg = (rgba >> shift) & 0xff;
        return static_cast<int>(fg * a + cell_shade * (1.0 - a) + 0.5);
      };
      // Two-tone checkered background to hint at translucency.
      CRect top_half = swatch, bottom_half = swatch;
      top_half.bottom = (swatch.top + swatch.bottom) / 2;
      bottom_half.top = top_half.bottom;
      dc.FillSolidRect(
          &top_half, RGB(channel(24, 235), channel(16, 235), channel(8, 235)));
      dc.FillSolidRect(&bottom_half, RGB(channel(24, 200), channel(16, 200),
                                         channel(8, 200)));
      dc.DrawEdge(&swatch, EDGE_SUNKEN, BF_RECT);
      left = swatch.right + 6;
    }
  }
  dc.SetBkMode(TRANSPARENT);
  dc.SetTextColor(is_header ? GetSysColor(COLOR_GRAYTEXT)
                            : (selected ? GetSysColor(COLOR_HIGHLIGHTTEXT)
                                        : GetSysColor(COLOR_WINDOWTEXT)));
  CRect text_rc = draw->rcItem;
  text_rc.left = left;
  dc.DrawText(text.c_str(), static_cast<int>(text.size()), &text_rc,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE);
  return TRUE;
}

bool CustomSchemeDialog::SaveSchemeAndPreview() {
  std::string name = wtou8(Tr(L"自定义／Custom", L"自定義／Custom", L"Custom"));
  if (!settings_->WriteCustomScheme(colors_, name))
    return false;
  // Persist a preview image; UIStyleSettingsDialog looks in the user data
  // directory first, so this reuses the existing preview mechanism. Resolve
  // the directory through WeaselUserDataPath() rather than the rime API,
  // whose narrow-string round-trip corrupts non-ASCII paths on Windows.
  std::wstring dir = (WeaselUserDataPath() / L"preview").wstring();
  CreateDirectoryW(dir.c_str(), nullptr);
  weasel_deployer::SavePreviewPng(dir + L"\\color_scheme_custom.png", 336, 168,
                                  colors_);
  return true;
}

LRESULT CustomSchemeDialog::OnOK(WORD, WORD, HWND, BOOL&) {
  ApplyHexText();
  if (!SaveSchemeAndPreview()) {
    MessageBox(Tr(L"保存自定义配色失败，请检查用户目录是否可写。",
                  L"保存自定義配色失敗，請檢查使用者目錄是否可寫。",
                  L"Failed to save the custom color scheme. Please check "
                  L"that the user data directory is writable."),
               Tr(L"【小狼毫】自定义配色", L"【小狼毫】自定義配色",
                  L"Weasel Custom Color Scheme"),
               MB_OK | MB_ICONWARNING);
    return 0;
  }
  EndDialog(IDOK);
  return 0;
}

std::string CustomSchemeDialog::WeaselYaml(const RgbaColorMap& colors,
                                           const std::string& name) {
  std::ostringstream out;
  out << "# 小狼毫：粘贴到 weasel.custom.yaml，然后重新部署\n"
      << "patch:\n"
      << "  preset_color_schemes/custom:\n"
      << "    name: " << name << "\n"
      << "    color_format: rgba\n";
  for (int i = 0; i < kRoleCount; ++i) {
    auto it = colors.find(kRoles[i].key);
    if (it != colors.end())
      out << "    " << kRoles[i].key << ": " << HexRgba(it->second) << "\n";
  }
  out << "  style/color_scheme: custom\n";
  return out.str();
}

std::string CustomSchemeDialog::SquirrelYaml(const RgbaColorMap& colors,
                                             const std::string& name) {
  // Weasel keys that have a Squirrel equivalent (some renamed).
  static const std::pair<const char*, const char*> kKeyMap[] = {
      {"back_color", "back_color"},
      {"border_color", "border_color"},
      {"text_color", "text_color"},
      {"hilited_text_color", "hilited_text_color"},
      {"hilited_back_color", "hilited_back_color"},
      {"candidate_text_color", "candidate_text_color"},
      {"candidate_back_color", "candidate_back_color"},
      {"label_color", "label_color"},
      {"comment_text_color", "comment_text_color"},
      {"hilited_candidate_text_color", "hilited_candidate_text_color"},
      {"hilited_candidate_back_color", "hilited_candidate_back_color"},
      {"hilited_label_color", "hilited_candidate_label_color"},
      {"hilited_comment_text_color", "hilited_comment_text_color"},
  };
  std::ostringstream out;
  out << "# 鼠须管：粘贴到 squirrel.custom.yaml，然后重新部署\n"
      << "# 注意：鼠须管色值顺序为 0xAABBGGRR；阴影/候选边框/翻页箭头/高亮标记 "
         "不支持，已省略\n"
      << "patch:\n"
      << "  preset_color_schemes/custom:\n"
      << "    name: " << name << "\n";
  for (const auto& pair : kKeyMap) {
    auto it = colors.find(pair.first);
    if (it == colors.end())
      continue;
    char buf[16];
    snprintf(buf, sizeof buf, "0x%08X", RgbaToAbgr(it->second));
    out << "    " << pair.second << ": " << buf << "\n";
  }
  out << "  style/color_scheme: custom\n";
  return out.str();
}

void CustomSchemeDialog::ExportYaml(bool for_squirrel) {
  std::string name = wtou8(Tr(L"自定义／Custom", L"自定義／Custom", L"Custom"));
  std::string yaml =
      for_squirrel ? SquirrelYaml(colors_, name) : WeaselYaml(colors_, name);
  if (CopyTextToClipboard(m_hWnd, u8tow(yaml))) {
    MessageBox(Tr(L"YAML 片段已复制到剪贴板。", L"YAML 片段已複製到剪貼簿。",
                  L"The YAML snippet has been copied to the clipboard."),
               Tr(L"【小狼毫】自定义配色", L"【小狼毫】自定義配色",
                  L"Weasel Custom Color Scheme"),
               MB_OK | MB_ICONINFORMATION);
  }
}

LRESULT CustomSchemeDialog::OnExportYaml(WORD, WORD, HWND, BOOL&) {
  HMENU menu = CreatePopupMenu();
  if (!menu)
    return 0;
  AppendMenuW(menu, MF_STRING, 1,
              Tr(L"小狼毫 weasel.yaml", L"小狼毫 weasel.yaml",
                 L"Weasel (weasel.yaml)"));
  AppendMenuW(menu, MF_STRING, 2,
              Tr(L"鼠须管 squirrel.yaml", L"鼠鬚管 squirrel.yaml",
                 L"Squirrel (squirrel.yaml)"));
  CRect rc;
  CWindow(GetDlgItem(IDC_EXPORT_YAML)).GetWindowRect(&rc);
  UINT cmd = TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD,
                            rc.left, rc.bottom, 0, m_hWnd, nullptr);
  DestroyMenu(menu);
  if (cmd == 1)
    ExportYaml(false);
  else if (cmd == 2)
    ExportYaml(true);
  return 0;
}
