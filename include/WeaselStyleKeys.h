#pragma once

#include <cstddef>
#include <iterator>
#include <vector>

#include <windows.h>

namespace weasel {

// Value type of a style/... configuration key. Drives which editor the
// style-settings dialog shows for a given key.
enum class StyleValueType { Boolean, Enum, Int, Str, Theme };

// A display label localized for the three UI languages the deployer supports.
// The deployer resolves the language once at startup (get_language_id() +
// SetThreadUILanguage), so GetThreadUILanguage() reflects the user's choice.
struct LocalizedLabel {
  const char* zh_hans;  // Simplified Chinese
  const char* zh_hant;  // Traditional Chinese
  const char* en;       // English
};

enum class UiLang { kHans, kHant, kEnglish };

inline UiLang GetUiLang() {
  const LANGID id = GetThreadUILanguage();
  switch (PRIMARYLANGID(id)) {
    case LANG_CHINESE:
      if (SUBLANGID(id) == SUBLANG_CHINESE_TRADITIONAL ||
          SUBLANGID(id) == SUBLANG_CHINESE_HONGKONG ||
          SUBLANGID(id) == SUBLANG_CHINESE_MACAU)
        return UiLang::kHant;
      return UiLang::kHans;
    default:
      return UiLang::kEnglish;
  }
}

inline const char* Localize(const LocalizedLabel& label) {
  switch (GetUiLang()) {
    case UiLang::kHant:
      return label.zh_hant;
    case UiLang::kEnglish:
      return label.en;
    case UiLang::kHans:
    default:
      return label.zh_hans;
  }
}

struct StyleChoice {
  const char* value;     // canonical config value (UTF-8)
  LocalizedLabel label;  // display label
};

struct StyleKeyInfo {
  const char* key;       // canonical config path, e.g. "style/layout/type"
  LocalizedLabel label;  // display label
  StyleValueType type;
  const StyleChoice* choices;  // enum choices (Enum type only)
  std::size_t choice_count;
};

// Single source of truth for the style keys exposed in the settings dialog.
// Order controls the order shown in the list.
inline const std::vector<StyleKeyInfo>& GetStyleKeys() {
  static const StyleChoice kLayoutTypeChoices[] = {
      {"vertical", {"竖直", "垂直", "Vertical"}},
      {"horizontal", {"水平", "水平", "Horizontal"}},
      {"vertical_text", {"竖直 + 文字竖排", "垂直 + 直書", "Vertical text"}},
      {"vertical+fullscreen",
       {"竖直 + 全屏", "垂直 + 全螢幕", "Vertical + fullscreen"}},
      {"horizontal+fullscreen",
       {"水平 + 全屏", "水平 + 全螢幕", "Horizontal + fullscreen"}},
      {"vertical_text+fullscreen",
       {"竖直 + 文字竖排 + 全屏", "直書 + 全螢幕",
        "Vertical text + fullscreen"}},
  };
  static const StyleChoice kAlignTypeChoices[] = {
      {"top", {"顶部", "頂部", "Top"}},
      {"center", {"居中", "居中", "Center"}},
      {"bottom", {"底部", "底部", "Bottom"}},
  };
  static const StyleChoice kPreeditTypeChoices[] = {
      {"composition", {"编码", "編碼", "Composition"}},
      {"preview", {"预览", "預覽", "Preview"}},
      {"preview_all", {"预览全部", "預覽全部", "Preview all"}},
  };
  static const StyleChoice kAntialiasChoices[] = {
      {"default", {"默认", "預設", "Default"}},
      {"cleartype", {"ClearType", "ClearType", "ClearType"}},
      {"grayscale", {"灰度", "灰階", "Grayscale"}},
      {"aliased", {"无抗锯齿", "無抗鋸齒", "Aliased"}},
      {"force_dword",
       {"强制 DWORD 对齐", "強制 DWORD 對齊", "Force DWORD alignment"}},
  };
  static const StyleChoice kHoverTypeChoices[] = {
      {"none", {"无", "無", "None"}},
      {"semi_hilite", {"半高亮", "半高亮", "Semi highlight"}},
      {"hilite", {"高亮", "高亮", "Highlight"}},
  };

  static const std::vector<StyleKeyInfo> kKeys = {
      {"style/color_scheme",
       {"配色方案", "配色方案", "Color scheme"},
       StyleValueType::Theme,
       nullptr,
       0},
      {"style/color_scheme_dark",
       {"暗色主题", "暗色主題", "Dark theme"},
       StyleValueType::Theme,
       nullptr,
       0},
      {"style/layout/type",
       {"布局方向", "佈局方向", "Layout direction"},
       StyleValueType::Enum,
       kLayoutTypeChoices,
       std::size(kLayoutTypeChoices)},
      {"style/layout/align_type",
       {"对齐方式", "對齊方式", "Alignment"},
       StyleValueType::Enum,
       kAlignTypeChoices,
       std::size(kAlignTypeChoices)},
      {"style/preedit_type",
       {"编码显示方式", "編碼顯示方式", "Preedit display"},
       StyleValueType::Enum,
       kPreeditTypeChoices,
       std::size(kPreeditTypeChoices)},
      {"style/antialias_mode",
       {"字体渲染", "字型渲染", "Font rendering"},
       StyleValueType::Enum,
       kAntialiasChoices,
       std::size(kAntialiasChoices)},
      {"style/hover_type",
       {"悬停效果", "懸停效果", "Hover effect"},
       StyleValueType::Enum,
       kHoverTypeChoices,
       std::size(kHoverTypeChoices)},
      {"style/inline_preedit",
       {"内联编码", "內聯編碼", "Inline preedit"},
       StyleValueType::Boolean,
       nullptr,
       0},
      {"style/display_tray_icon",
       {"显示托盘图标", "顯示系統匣圖示", "Show tray icon"},
       StyleValueType::Boolean,
       nullptr,
       0},
      {"style/ascii_tip_follow_cursor",
       {"英文提示跟随光标", "英文提示跟隨游標", "ASCII tip follows cursor"},
       StyleValueType::Boolean,
       nullptr,
       0},
      {"style/paging_on_scroll",
       {"滚轮翻页", "滾輪翻頁", "Page on scroll"},
       StyleValueType::Boolean,
       nullptr,
       0},
      {"style/click_to_capture",
       {"点击截屏", "點擊截屏", "Click to capture"},
       StyleValueType::Boolean,
       nullptr,
       0},
      {"style/enhanced_position",
       {"增强定位", "增強定位", "Enhanced positioning"},
       StyleValueType::Boolean,
       nullptr,
       0},
      {"style/vertical_auto_reverse",
       {"竖直自动换向", "垂直自動換向", "Vertical auto reverse"},
       StyleValueType::Boolean,
       nullptr,
       0},
      {"style/vertical_text_left_to_right",
       {"竖排从左到右", "直書從左到右", "Vertical text LTR"},
       StyleValueType::Boolean,
       nullptr,
       0},
      {"style/vertical_text_with_wrap",
       {"竖排自动换行", "直書自動換行", "Vertical text wrap"},
       StyleValueType::Boolean,
       nullptr,
       0},
      {"style/vertical_right_to_left",
       {"竖直布局从右到左", "垂直佈局從右到左", "Vertical layout RTL"},
       StyleValueType::Boolean,
       nullptr,
       0},
      {"style/candidate_abbreviate_length",
       {"候选缩写长度", "候選縮寫長度", "Candidate abbreviation length"},
       StyleValueType::Int,
       nullptr,
       0},
      {"style/label_format",
       {"序号格式", "序號格式", "Label format"},
       StyleValueType::Str,
       nullptr,
       0},
      {"style/mark_text",
       {"标记文本", "標記文本", "Mark text"},
       StyleValueType::Str,
       nullptr,
       0},
      {"style/layout/border",
       {"边框宽度", "邊框寬度", "Border width"},
       StyleValueType::Int,
       nullptr,
       0},
      {"style/layout/margin_x",
       {"左右边距", "左右邊距", "Horizontal margin"},
       StyleValueType::Int,
       nullptr,
       0},
      {"style/layout/margin_y",
       {"上下边距", "上下邊距", "Vertical margin"},
       StyleValueType::Int,
       nullptr,
       0},
      {"style/layout/spacing",
       {"间距", "間距", "Spacing"},
       StyleValueType::Int,
       nullptr,
       0},
      {"style/layout/candidate_spacing",
       {"候选间距", "候選間距", "Candidate spacing"},
       StyleValueType::Int,
       nullptr,
       0},
      {"style/layout/hilite_spacing",
       {"高亮间距", "高亮間距", "Highlight spacing"},
       StyleValueType::Int,
       nullptr,
       0},
      {"style/layout/hilite_padding_x",
       {"高亮内边距（水平）", "高亮內邊距（水平）", "Highlight padding (X)"},
       StyleValueType::Int,
       nullptr,
       0},
      {"style/layout/hilite_padding_y",
       {"高亮内边距（垂直）", "高亮內邊距（垂直）", "Highlight padding (Y)"},
       StyleValueType::Int,
       nullptr,
       0},
      {"style/layout/hilited_corner_radius",
       {"高亮圆角半径", "高亮圓角半徑", "Highlight corner radius"},
       StyleValueType::Int,
       nullptr,
       0},
      {"style/layout/corner_radius",
       {"圆角半径", "圓角半徑", "Corner radius"},
       StyleValueType::Int,
       nullptr,
       0},
      {"style/layout/shadow_radius",
       {"阴影半径", "陰影半徑", "Shadow radius"},
       StyleValueType::Int,
       nullptr,
       0},
      {"style/layout/shadow_offset_x",
       {"阴影水平偏移", "陰影水平偏移", "Shadow offset X"},
       StyleValueType::Int,
       nullptr,
       0},
      {"style/layout/shadow_offset_y",
       {"阴影垂直偏移", "陰影垂直偏移", "Shadow offset Y"},
       StyleValueType::Int,
       nullptr,
       0},
      {"style/layout/min_width",
       {"最小宽度", "最小寬度", "Min width"},
       StyleValueType::Int,
       nullptr,
       0},
      {"style/layout/max_width",
       {"最大宽度", "最大寬度", "Max width"},
       StyleValueType::Int,
       nullptr,
       0},
      {"style/layout/min_height",
       {"最小高度", "最小高度", "Min height"},
       StyleValueType::Int,
       nullptr,
       0},
      {"style/layout/max_height",
       {"最大高度", "最大高度", "Max height"},
       StyleValueType::Int,
       nullptr,
       0},
      {"style/layout/baseline",
       {"基线偏移（%）", "基線偏移（%）", "Baseline (%)"},
       StyleValueType::Int,
       nullptr,
       0},
      {"style/layout/linespacing",
       {"行距（%）", "行距（%）", "Line spacing (%)"},
       StyleValueType::Int,
       nullptr,
       0},
  };
  return kKeys;
}

}  // namespace weasel
