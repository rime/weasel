#include "stdafx.h"
#include "InstallOptionsDlg.h"
#include <atlstr.h>
#include <ShlObj.h>
#pragma comment(lib, "Shell32.lib")

int uninstall(bool silent);

InstallOptionsDialog::InstallOptionsDialog()
    : installed(false), profile(L"hans"), user_dir() {}

InstallOptionsDialog::~InstallOptionsDialog() {}

LRESULT InstallOptionsDialog::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&) {
  profile_.Attach(GetDlgItem(IDC_COMBO_PROFILE));
  remove_.Attach(GetDlgItem(IDC_REMOVE));
  default_dir_.Attach(GetDlgItem(IDC_RADIO_DEFAULT_DIR));
  custom_dir_.Attach(GetDlgItem(IDC_RADIO_CUSTOM_DIR));
  dir_.Attach(GetDlgItem(IDC_EDIT_DIR));

  struct ProfileOption {
    const wchar_t* value;
    const wchar_t* zhHansText;
    const wchar_t* zhHantText;
    const wchar_t* enText;
  };
  const ProfileOption options[] = {
      {L"hans", L"中文（简体，中国大陆）", L"中文（簡體，中國大陸）",
       L"Chinese (Simplified, China)"},
      {L"hant", L"中文（繁体，中国台湾）", L"中文（繁體，中國臺灣）",
       L"Chinese (Traditional, Taiwan)"},
      {L"hongkong", L"中文（繁体，中国香港）", L"中文（繁體，中國香港）",
       L"Chinese (Traditional, Hong Kong)"},
      {L"macau", L"中文（繁体，中国澳门）", L"中文（繁體，中國澳門）",
       L"Chinese (Traditional, Macao)"},
      {L"singapore", L"中文（简体，新加坡）", L"中文（簡體，新加坡）",
       L"Chinese (Simplified, Singapore)"},
  };
  const auto uiLang = PRIMARYLANGID(GetThreadUILanguage());
  const auto uiSubLang = SUBLANGID(GetThreadUILanguage());

  int selected = 0;
  for (int i = 0; i < static_cast<int>(_countof(options)); ++i) {
    const wchar_t* label = options[i].enText;
    if (uiLang == LANG_CHINESE) {
      const bool isTraditional = (uiSubLang == SUBLANG_CHINESE_TRADITIONAL ||
                                  uiSubLang == SUBLANG_CHINESE_HONGKONG ||
                                  uiSubLang == SUBLANG_CHINESE_MACAU);
      label = isTraditional ? options[i].zhHantText : options[i].zhHansText;
    }
    int index = profile_.AddString(label);
    profile_.SetItemData(index, i);
    if (profile == options[i].value) {
      selected = index;
    }
  }
  profile_.SetCurSel(selected);

  CheckRadioButton(
      IDC_RADIO_DEFAULT_DIR, IDC_RADIO_CUSTOM_DIR,
      (user_dir.empty() ? IDC_RADIO_DEFAULT_DIR : IDC_RADIO_CUSTOM_DIR));
  dir_.SetWindowTextW(user_dir.c_str());

  profile_.EnableWindow(!installed);
  remove_.EnableWindow(installed);
  dir_.EnableWindow(user_dir.empty() ? FALSE : TRUE);

  button_custom_dir_.Attach(GetDlgItem(IDC_BUTTON_CUSTOM_DIR));
  button_custom_dir_.EnableWindow(user_dir.empty() ? FALSE : TRUE);

  ok_.Attach(GetDlgItem(IDOK));
  if (installed) {
    CString str;
    str.LoadStringW(IDS_STRING_MODIFY);
    ok_.SetWindowTextW(str);
  }

  CenterWindow();
  return 0;
}

LRESULT InstallOptionsDialog::OnClose(UINT, WPARAM, LPARAM, BOOL&) {
  EndDialog(IDCANCEL);
  return 0;
}

LRESULT InstallOptionsDialog::OnOK(WORD, WORD code, HWND, BOOL&) {
  int selected = profile_.GetCurSel();
  if (selected == CB_ERR) {
    selected = 0;
  }
  switch (static_cast<int>(profile_.GetItemData(selected))) {
    case 1:
      profile = L"hant";
      break;
    case 2:
      profile = L"hongkong";
      break;
    case 3:
      profile = L"macau";
      break;
    case 4:
      profile = L"singapore";
      break;
    default:
      profile = L"hans";
      break;
  }
  if (IsDlgButtonChecked(IDC_RADIO_CUSTOM_DIR) == BST_CHECKED) {
    CStringW text;
    dir_.GetWindowTextW(text);
    user_dir = text;
  } else {
    user_dir.clear();
  }
  EndDialog(IDOK);
  return 0;
}

LRESULT InstallOptionsDialog::OnRemove(WORD, WORD code, HWND, BOOL&) {
  const bool non_silent = false;
  uninstall(non_silent);
  installed = false;
  CString str;
  str.LoadStringW(IDS_STRING_INSTALL);
  ok_.SetWindowTextW(str);
  profile_.EnableWindow(!installed);
  remove_.EnableWindow(installed);
  return 0;
}

LRESULT InstallOptionsDialog::OnUseDefaultDir(WORD, WORD code, HWND, BOOL&) {
  dir_.EnableWindow(FALSE);
  dir_.SetWindowTextW(L"");
  button_custom_dir_.EnableWindow(FALSE);
  return 0;
}

LRESULT InstallOptionsDialog::OnUseCustomDir(WORD, WORD code, HWND, BOOL&) {
  CShellFileOpenDialog fileOpenDlg(
      NULL, FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_PICKFOLDERS);
  CStringW text;
  dir_.GetWindowTextW(text);
  if (!text.IsEmpty()) {
    PIDLIST_ABSOLUTE pidl;
    HRESULT hr = SHParseDisplayName(text, NULL, &pidl, 0, NULL);
    if (SUCCEEDED(hr)) {
      IShellItem* psi;
      hr = SHCreateShellItem(NULL, NULL, pidl, &psi);
      if (SUCCEEDED(hr)) {
        fileOpenDlg.GetPtr()->SetFolder(psi);
        psi->Release();
      }
      CoTaskMemFree(pidl);
    }
  }
  if (fileOpenDlg.DoModal(m_hWnd) == IDOK) {
    CComPtr<IShellItem> psi;
    if (SUCCEEDED(fileOpenDlg.GetPtr()->GetResult(&psi))) {
      LPWSTR path;
      if (SUCCEEDED(psi->GetDisplayName(SIGDN_FILESYSPATH, &path))) {
        dir_.SetWindowTextW(path);
        CoTaskMemFree(path);
      }
    }
  }
  dir_.EnableWindow(TRUE);
  button_custom_dir_.EnableWindow(TRUE);
  ok_.SetFocus();
  return 0;
}
