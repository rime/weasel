#include "stdafx.h"
#include "FontSettingDialog.h"

const wstring patStyle(L"(\\s*:\\s*italic|\\s*:\\s*oblique|\\s*:\\s*normal)");
const wstring patWeight(
    L"(\\s*:\\s*thin|\\s*:\\s*extra_light|\\s*:\\s*ultra_light|\\s*:\\s*light|"
    L"\\s*:\\s*semi_light|\\s*:\\s*medium|\\s*:\\s*demi_"
    L"bold|"
    L"\\s*:\\s*semi_bold|\\s*:\\s*bold|\\s*:\\s*extra_bold|\\s*:\\s*ultra_bold|"
    L"\\s*:\\s*black|\\s*:\\s*heavy|\\s*:\\s*extra_black|\\s*:\\s*"
    L"ultra_black)");

const map<wstring, DWRITE_FONT_STYLE> _mapStyle = {
    {L"italic", DWRITE_FONT_STYLE_ITALIC},
    {L"oblique", DWRITE_FONT_STYLE_OBLIQUE},
    {L"normal", DWRITE_FONT_STYLE_NORMAL},
};

const map<wstring, DWRITE_FONT_WEIGHT> _mapWeight = {
    {L"thin", DWRITE_FONT_WEIGHT_THIN},
    {L"extra_light", DWRITE_FONT_WEIGHT_EXTRA_LIGHT},
    {L"ultra_light", DWRITE_FONT_WEIGHT_ULTRA_LIGHT},
    {L"light", DWRITE_FONT_WEIGHT_LIGHT},
    {L"semi_light", DWRITE_FONT_WEIGHT_SEMI_LIGHT},
    {L"medium", DWRITE_FONT_WEIGHT_MEDIUM},
    {L"demi_bold", DWRITE_FONT_WEIGHT_DEMI_BOLD},
    {L"semi_bold", DWRITE_FONT_WEIGHT_SEMI_BOLD},
    {L"bold", DWRITE_FONT_WEIGHT_BOLD},
    {L"extra_bold", DWRITE_FONT_WEIGHT_EXTRA_BOLD},
    {L"ultra_bold", DWRITE_FONT_WEIGHT_ULTRA_BOLD},
    {L"black", DWRITE_FONT_WEIGHT_BLACK},
    {L"heavy", DWRITE_FONT_WEIGHT_HEAVY},
    {L"extra_black", DWRITE_FONT_WEIGHT_EXTRA_BLACK},
    {L"regular", DWRITE_FONT_WEIGHT_NORMAL},
    {L"ultra_black", DWRITE_FONT_WEIGHT_ULTRA_BLACK}};

const map<wstring, float> _mapDpiScale = {
    {L"100%", 1.0f}, {L"110%", 1.1f}, {L"120%", 1.2f}, {L"150%", 1.5f},
    {L"200%", 2.0f}, {L"250%", 2.5f}, {L"400%", 4.0f},
};

std::vector<wstring> WstrSplit(const wstring& in, const wstring& delim) {
  wregex re{delim};
  return std::vector<wstring>{
      wsregex_token_iterator(in.begin(), in.end(), re, -1),
      wsregex_token_iterator()};
}

wstring MatchWordsOutLowerCaseTrim1st(const wstring& wstr, const wstring& pat) {
  wstring mat = L"";
  wsmatch mc;
  wregex pattern(pat, wregex::icase);
  wstring::const_iterator iter = wstr.cbegin();
  wstring::const_iterator end = wstr.cend();
  while (regex_search(iter, end, mc, pattern)) {
    for (const auto& m : mc) {
      mat = m;
      mat = mat.substr(1);
      break;
    }
    iter = mc.suffix().first;
  }
  wstring res;
  transform(mat.begin(), mat.end(), back_inserter(res), ::tolower);
  return res;
}

void ParseFontFace(const wstring& fontFaceStr,
                   DWRITE_FONT_WEIGHT& fontWeight,
                   DWRITE_FONT_STYLE& fontStyle) {
  const wstring weight = MatchWordsOutLowerCaseTrim1st(fontFaceStr, patWeight);
  const auto it = _mapWeight.find(weight);
  fontWeight =
      (it != _mapWeight.end()) ? it->second : DWRITE_FONT_WEIGHT_NORMAL;

  const wstring style = MatchWordsOutLowerCaseTrim1st(fontFaceStr, patStyle);
  const auto it2 = _mapStyle.find(style);
  fontStyle = (it2 != _mapStyle.end()) ? it2->second : DWRITE_FONT_STYLE_NORMAL;
}

void RemoveSpaceAround(wstring& str) {
  str = regex_replace(str, wregex(L",$"), L"");
  str = regex_replace(str, wregex(L"\\s*(,|:|^|$)\\s*"), L"$1");
}

INT_PTR FontSettingDialog::ShowDialog() {
  return DialogBoxParam(hInstance_, MAKEINTRESOURCE(IDD_DIALOG_FONT), hParent,
                        DialogProc, reinterpret_cast<LPARAM>(this));
}

INT_PTR CALLBACK FontSettingDialog::DialogProc(HWND hDlg,
                                               UINT message,
                                               WPARAM wParam,
                                               LPARAM lParam) {
  FontSettingDialog* pThis = nullptr;
  if (message == WM_INITDIALOG) {
    pThis = reinterpret_cast<FontSettingDialog*>(lParam);
    SetWindowLongPtr(hDlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
    pThis->hDlg_ = hDlg;
    return pThis->OnInitDialog();
  } else {
    pThis = reinterpret_cast<FontSettingDialog*>(
        GetWindowLongPtr(hDlg, GWLP_USERDATA));
    if (pThis) {
      return pThis->HandleMsg(hDlg, message, wParam, lParam);
    }
  }
  return (INT_PTR)FALSE;
}

INT_PTR FontSettingDialog::HandleMsg(HWND hDlg,
                                     UINT message,
                                     WPARAM wParam,
                                     LPARAM lParam) {
  switch (message) {
    case WM_COMMAND:
      return OnCommand(wParam);
    case WM_DPICHANGED:
    case WM_MOVE: {
      if (!(m_pD2D && m_pD2D->GetDpi()))
        UpdatePreview();
      return 0;
    }
    case WM_PAINT: {
      PAINTSTRUCT ps;
      HDC hdc = BeginPaint(hDlg_, &ps);
      RECT rect;
      GetClientRect(hDlg_, &rect);
      FillRect(hdc, &rect, (HBRUSH)(COLOR_BTNFACE + 1));
      UpdatePreview();
      EndPaint(hDlg_, &ps);
      return 0;
    }
  }
  return (INT_PTR)FALSE;
}

INT_PTR FontSettingDialog::OnInitDialog() {
  // 在这里初始化窗口控件
  HICON hIconSmall = (HICON)LoadIcon(hInstance_, MAKEINTRESOURCE(IDI_SMALL));
  SendMessage(hDlg_, WM_SETICON, ICON_SMALL, (LPARAM)hIconSmall);

  m_hFontPickerCbb = GetDlgItem(hDlg_, IDC_CBB_FONTPICKER);
  InitFontPickerItems();

  m_hFontFaceNameCbb = GetDlgItem(hDlg_, IDC_CBB_FONTFACE_NAME);
  const wstring fontFaceName[] = {L"label_font_face", L"font_face",
                                  L"comment_font_face"};
  for (const auto& name : fontFaceName)
    SendMessage(m_hFontFaceNameCbb, CB_ADDSTRING, 0, (LPARAM)name.c_str());
  SendMessage(m_hFontFaceNameCbb, CB_SETCURSEL, 1, 0);

  m_hComboBoxFontPoint = GetDlgItem(hDlg_, IDC_CBB_FONTPOINT);
  for (auto i = 0; i < 100; i++) {
    auto font_point = std::to_wstring(i);
    SendMessage(m_hComboBoxFontPoint, CB_ADDSTRING, 0,
                (LPARAM)font_point.c_str());
  }

  m_hCheckBoxRangeEn = GetDlgItem(hDlg_, IDC_CKB_RANGEEN);
  m_hEditRangeEnd = GetDlgItem(hDlg_, IDC_EDIT_RANGEEND);
  m_hEditRangeStart = GetDlgItem(hDlg_, IDC_EDIT_RANGESTART);
  // set start L"0" and end L"10ffff"
  SetDlgItemText(hDlg_, IDC_EDIT_RANGESTART, L"0");
  SetDlgItemText(hDlg_, IDC_EDIT_RANGEEND, L"10ffff");

  m_font_face_ptr = &m_font_face;
  m_font_point_ptr = &m_font_point;
  int sel = (*m_font_point_ptr) >= 0 ? (*m_font_point_ptr) : 0;
  SendMessage(m_hComboBoxFontPoint, CB_SETCURSEL, sel, 0);

  m_hFontStyleCbb = GetDlgItem(hDlg_, IDC_CBB_FONTSTYLE);
  for (auto& it : _mapStyle) {
    SendMessage(m_hFontStyleCbb, CB_ADDSTRING, 0, (LPARAM)it.first.c_str());
  }
  m_hFontWeightCbb = GetDlgItem(hDlg_, IDC_CBB_FONTWEIGHT);
  for (auto& it : _mapWeight) {
    SendMessage(m_hFontWeightCbb, CB_ADDSTRING, 0, (LPARAM)it.first.c_str());
  }

  wstring style_str = MatchWordsOutLowerCaseTrim1st(*m_font_face_ptr, patStyle);
  wstring weight_str =
      MatchWordsOutLowerCaseTrim1st(*m_font_face_ptr, patWeight);
  if (style_str.empty())
    style_str = L"normal";
  if (weight_str.empty())
    weight_str = L"regular";
  SendMessage(m_hFontStyleCbb, CB_SELECTSTRING, 0, (LPARAM)style_str.c_str());
  SendMessage(m_hFontWeightCbb, CB_SELECTSTRING, 0, (LPARAM)weight_str.c_str());

  m_hEditFontFace = GetDlgItem(hDlg_, IDC_EDIT_FONTFACE);
  m_hEditPreviewText = GetDlgItem(hDlg_, IDC_EDIT_PREVIEW_TEXT);
  SetDlgItemText(hDlg_, IDC_EDIT_PREVIEW_TEXT, m_text.c_str());
  SetDlgItemText(hDlg_, IDC_EDIT_FONTFACE, m_font_face_ptr->c_str());

  m_hPreview = GetDlgItem(hDlg_, IDC_STATIC_PREVIEW);
  m_pD2D = make_unique<D2D>(m_hPreview, m_text);
  UpdatePreview();

  return (INT_PTR)TRUE;
}

wstring FontSettingDialog::GetComboBoxSelectStr(HWND target) {
  int selectedIndex = (int)SendMessage(target, CB_GETCURSEL, 0, 0);
  if (selectedIndex != CB_ERR) {
    int textLength =
        (int)SendMessage(target, CB_GETLBTEXTLEN, selectedIndex, 0);
    if (textLength != CB_ERR) {
      wchar_t buffer[4096] = {0};
      SendMessage(target, CB_GETLBTEXT, selectedIndex, (LPARAM)buffer);
      return wstring(buffer);
    }
  }
  return L"";
}

wstring FontSettingDialog::GetTextOfEdit(HWND& hwndEdit) {
  wchar_t buffer[4096] = {0};
  LRESULT textLength = SendMessage(hwndEdit, WM_GETTEXTLENGTH, 0, 0);
  if (textLength > 0 && textLength <= (_countof(buffer) - 1)) {
    SendMessage(hwndEdit, WM_GETTEXT, sizeof(buffer) / sizeof(wchar_t),
                (LPARAM)buffer);
    return wstring(buffer);
  }
  return wstring(L"");
}

void FontSettingDialog::UpdatePreview() {
  if (m_pD2D) {
    *m_font_face_ptr = GetTextOfEdit(m_hEditFontFace);
    auto font_point_str = GetComboBoxSelectStr(m_hComboBoxFontPoint);
    *m_font_point_ptr = std::stoi(font_point_str);
    m_text = GetTextOfEdit(m_hEditPreviewText);
    RemoveSpaceAround(*m_font_face_ptr);
    HR(m_pD2D->InitFontFormat(*m_font_face_ptr, *m_font_point_ptr));
    m_pD2D->Draw();
  }
}

void FontSettingDialog::OnAddFont() {
  auto ret = GetComboBoxSelectStr(m_hFontPickerCbb);
  auto rangeEnabled = SendMessage(m_hCheckBoxRangeEn, BM_GETCHECK, 0, 0);
  if (rangeEnabled == BST_CHECKED) {
    auto start_str = GetTextOfEdit(m_hEditRangeStart);
    auto end_str = GetTextOfEdit(m_hEditRangeEnd);
    start_str = regex_replace(start_str, wregex(L"^0+$"), L"0");
    end_str = regex_replace(end_str, wregex(L"^0+$"), L"0");
    if (start_str == L"0") {
      if (!end_str.empty())
        ret += L"::" + end_str;
    } else {
      ret += L":" + start_str;
      if (!end_str.empty())
        ret += L":" + end_str;
    }
  }
  auto font_face = GetTextOfEdit(m_hEditFontFace);
  if (!font_face.empty())
    font_face += L", ";
  font_face += ret;
  SendMessage(m_hEditFontFace, WM_SETTEXT, 0, (LPARAM)font_face.c_str());
  ResetRange();
}

void FontSettingDialog::ResetRange() {
  SendMessage(m_hCheckBoxRangeEn, BM_SETCHECK, false, 0);
  SetDlgItemText(hDlg_, IDC_EDIT_RANGESTART, L"0");
  SetDlgItemText(hDlg_, IDC_EDIT_RANGEEND, L"10ffff");
  EnableWindow(m_hEditRangeStart, false);
  EnableWindow(m_hEditRangeEnd, false);
}

void FontSettingDialog::OnChangeFontFace() {
  auto curFontFaceName = GetComboBoxSelectStr(m_hFontFaceNameCbb);
  if (curFontFaceName == L"label_font_face") {
    m_font_face_ptr = &m_label_font_face;
    SendMessage(m_hComboBoxFontPoint, CB_SETCURSEL, m_label_font_point, 0);
    m_font_point_ptr = &m_label_font_point;
  } else if (curFontFaceName == L"font_face") {
    m_font_face_ptr = &m_font_face;
    SendMessage(m_hComboBoxFontPoint, CB_SETCURSEL, m_font_point, 0);
    m_font_point_ptr = &m_font_point;
  } else if (curFontFaceName == L"comment_font_face") {
    m_font_face_ptr = &m_comment_font_face;
    SendMessage(m_hComboBoxFontPoint, CB_SETCURSEL, m_comment_font_point, 0);
    m_font_point_ptr = &m_comment_font_point;
  }

  auto style_str = MatchWordsOutLowerCaseTrim1st(*m_font_face_ptr, patStyle);
  auto weight_str = MatchWordsOutLowerCaseTrim1st(*m_font_face_ptr, patWeight);
  if (style_str.empty())
    style_str = L"normal";
  if (weight_str.empty())
    weight_str = L"regular";
  SendMessage(m_hFontStyleCbb, CB_SELECTSTRING, 0, (LPARAM)style_str.c_str());
  SendMessage(m_hFontWeightCbb, CB_SELECTSTRING, 0, (LPARAM)weight_str.c_str());
  SendMessage(m_hEditFontFace, WM_SETTEXT, 0, (LPARAM)m_font_face_ptr->c_str());
}

void FontSettingDialog::OnRangeToggle() {
  int checkState = (int)SendMessage(m_hCheckBoxRangeEn, BM_GETCHECK, 0, 0);
  auto status = (checkState == BST_CHECKED) ? BST_CHECKED : BST_UNCHECKED;
  SendMessage(m_hCheckBoxRangeEn, BM_SETCHECK, status, 0);
  EnableWindow(m_hEditRangeStart, (checkState == BST_CHECKED));
  EnableWindow(m_hEditRangeEnd, (checkState == BST_CHECKED));
}

INT_PTR FontSettingDialog::OnChangeFontPoint() {
  auto font_point_str = GetComboBoxSelectStr(m_hComboBoxFontPoint);
  if (font_point_str.empty())
    return (INT_PTR)FALSE;
  auto font_point = std::stoi(font_point_str);
  *m_font_point_ptr = font_point;
  UpdatePreview();
  return TRUE;
}

void FontSettingDialog::OnChangeFontFaceEdit() {
  *m_font_face_ptr = GetTextOfEdit(m_hEditFontFace);
  wstring curfont = L"";
  if (m_font_face_ptr == &m_label_font_face) {
    curfont = L"label_font_face";
  } else if (m_font_face_ptr == &m_comment_font_face) {
    curfont = L"comment_font_face";
  } else if (m_font_face_ptr == &m_font_face) {
    curfont = L"font_face";
  }
  UpdatePreview();
}

void FontSettingDialog::HandleRangeInput(int controlId,
                                         const wchar_t* defaultValue) {
  wchar_t range[10] = {0};
  GetDlgItemText(hDlg_, controlId, range, 10);
  std::wstring rangeStr(range);
  std::wregex hexRegex(L"^[0-9a-fA-F]+$");

  if (rangeStr.empty() || !std::regex_match(rangeStr, hexRegex)) {
    SetDlgItemText(hDlg_, controlId, defaultValue);
  }
}

void FontSettingDialog::OnStyleOrWeightChanged(WPARAM wParam) {
  if (HIWORD(wParam) == CBN_SELCHANGE) {
    bool isWeight = (LOWORD(wParam) == IDC_CBB_FONTWEIGHT);
    HWND target = isWeight ? m_hFontWeightCbb : m_hFontStyleCbb;
    const wregex pat = isWeight ? wregex(patWeight) : wregex(patStyle);
    auto defaultValue = isWeight ? L"regular" : L"normal";
    int selectedIndex = (int)SendMessage(target, CB_GETCURSEL, 0, 0);
    auto str = GetComboBoxSelectStr(target);
    if (str.empty())
      return;
    *m_font_face_ptr = GetTextOfEdit(m_hEditFontFace);
    // 清除原来的字体样式或字重
    *m_font_face_ptr = regex_replace(*m_font_face_ptr, pat, L"");
    // 如果不是normal/regular, 使用正则表达式将 buffer 插入到第一个 ":" 之前
    if (str != defaultValue) {
      auto pat = wregex(L"(^\\s*(\\w| )+)(\\s*:\\s*)?");
      const auto replaceStr = L"$1:" + str + L"$3";
      *m_font_face_ptr = regex_replace(*m_font_face_ptr, pat, replaceStr);
    }
    // 更新编辑框内容
    SendMessage(m_hEditFontFace, WM_SETTEXT, 0,
                (LPARAM)m_font_face_ptr->c_str());
  }
}

INT_PTR FontSettingDialog::OnCommand(WPARAM wParam) {
  auto commandId = LOWORD(wParam);
  if (commandId == IDOK || commandId == IDCANCEL) {
    EndDialog(hDlg_, commandId);
    return (INT_PTR)TRUE;
  } else if (commandId == IDC_BTN_ADDFONT) {
    OnAddFont();
  } else if (commandId == IDC_CKB_RANGEEN && (HIWORD(wParam) == BN_CLICKED)) {
    OnRangeToggle();
  } else if (commandId == IDC_CBB_FONTFACE_NAME &&
             (HIWORD(wParam) == CBN_SELCHANGE)) {
    OnChangeFontFace();
  } else if (commandId == IDC_CBB_FONTPOINT &&
             HIWORD(wParam) == CBN_SELCHANGE) {
    return OnChangeFontPoint();
  } else if (commandId == IDC_EDIT_RANGESTART) {
    HandleRangeInput(IDC_EDIT_RANGESTART, L"0");
  } else if (commandId == IDC_EDIT_RANGEEND) {
    HandleRangeInput(IDC_EDIT_RANGEEND, L"10ffff");
  } else if (commandId == IDC_EDIT_FONTFACE && HIWORD(wParam) == EN_CHANGE) {
    OnChangeFontFaceEdit();
  } else if (commandId == IDC_EDIT_PREVIEW_TEXT &&
             HIWORD(wParam) == EN_CHANGE) {
    UpdatePreview();
  } else if ((commandId == IDC_CBB_FONTSTYLE) ||
             (commandId == IDC_CBB_FONTWEIGHT)) {
    OnStyleOrWeightChanged(wParam);
  } else if ((commandId == IDC_CBB_FONTPICKER) &&
             HIWORD(wParam) == CBN_SELCHANGE) {
    ResetRange();
  }
  return (INT_PTR)FALSE;
}

void FontSettingDialog::InitFontPickerItems() {
  wchar_t locale[LOCALE_NAME_MAX_LENGTH] = {0};
  if (!GetUserDefaultLocaleName(locale, LOCALE_NAME_MAX_LENGTH)) {
    DWORD errorCode = GetLastError();
    string errorMsgA = (HRESULTToString(errorCode));
    throw std::runtime_error(
        "Error Code: " + std::to_string(errorCode) +
        ", Error Message: " + std::string(errorMsgA.begin(), errorMsgA.end()));
  }
  ComPtr<IDWriteFactory> pDWriteFactory;
  HR(DWriteCreateFactory(
      DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
      reinterpret_cast<IUnknown**>(pDWriteFactory.ReleaseAndGetAddressOf())));
  ComPtr<IDWriteFontCollection> pFontCollection;
  // Get the system font collection.
  HR(pDWriteFactory->GetSystemFontCollection(
      pFontCollection.ReleaseAndGetAddressOf()));
  UINT32 familyCount = 0;
  // Get the number of font families in the collection.
  familyCount = pFontCollection->GetFontFamilyCount();
  ComPtr<IDWriteFontFamily> pFontFamily;
  ComPtr<IDWriteLocalizedStrings> pFamilyNames;
  const auto func = [&](const wchar_t* localeName) -> wstring {
    UINT32 index = 0;
    BOOL exists = false;
    HR(pFamilyNames->FindLocaleName(localeName, &index, &exists));
    // If the specified locale doesn't exist, select the first on the list.
    index = !exists ? 0 : index;
    UINT32 length = 0;
    HR(pFamilyNames->GetStringLength(index, &length));
    std::unique_ptr<wchar_t[]> name(new wchar_t[length + 1]);
    HR(!name ? E_OUTOFMEMORY : S_OK);
    HR(pFamilyNames->GetString(index, name.get(), length + 1));
    return wstring(name.get());
  };
  std::vector<wstring> fonts;
  for (UINT32 i = 0; i < familyCount; ++i) {
    HR(pFontCollection->GetFontFamily(i, pFontFamily.ReleaseAndGetAddressOf()));
    HR(pFontFamily->GetFamilyNames(pFamilyNames.ReleaseAndGetAddressOf()));
    auto font_family_name = func(locale);
    fonts.push_back(font_family_name);
  }
  // std::sort(fonts.begin(), fonts.end());
  //  sort fonts reverse order
  std::sort(fonts.begin(), fonts.end(), std::greater<wstring>());
  for (const auto& font : fonts) {
    SendMessage(m_hFontPickerCbb, CB_ADDSTRING, 0, (LPARAM)font.c_str());
  }
  SendMessage(m_hFontPickerCbb, CB_SETCURSEL, 0, 0);
}
