#include "stdafx.h"
#include "DictManagementDialog.h"
#include "Configurator.h"
#include <WeaselUtility.h>
#include <rime_api.h>
#include "WeaselDeployer.h"

DictManagementDialog::DictManagementDialog() {
  api_ = (RimeLeversApi*)rime_get_api()->find_module("levers")->get_api();
}

DictManagementDialog::~DictManagementDialog() {}

void DictManagementDialog::Populate() {
  RimeUserDictIterator iter = {0};
  api_->user_dict_iterator_init(&iter);
  while (const char* dict = api_->next_user_dict(&iter)) {
    std::wstring txt = string_to_wstring(dict, CP_UTF8);
    user_dict_list_.AddString(txt.c_str());
  }
  api_->user_dict_iterator_destroy(&iter);
  user_dict_list_.SetCurSel(-1);
}

LRESULT DictManagementDialog::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&) {
  user_dict_list_.Attach(GetDlgItem(IDC_USER_DICT_LIST));
  backup_.Attach(GetDlgItem(IDC_BACKUP));
  backup_.EnableWindow(FALSE);
  restore_.Attach(GetDlgItem(IDC_RESTORE));
  restore_.EnableWindow(TRUE);
  export_.Attach(GetDlgItem(IDC_EXPORT));
  export_.EnableWindow(FALSE);
  import_.Attach(GetDlgItem(IDC_IMPORT));
  import_.EnableWindow(FALSE);

  Populate();

  CenterWindow();
  BringWindowToTop();
  return TRUE;
}

LRESULT DictManagementDialog::OnClose(UINT, WPARAM, LPARAM, BOOL&) {
  EndDialog(IDCANCEL);
  return 0;
}

LRESULT DictManagementDialog::OnBackup(WORD, WORD code, HWND, BOOL&) {
  int sel = user_dict_list_.GetCurSel();
  if (sel < 0 || sel >= user_dict_list_.GetCount()) {
    // MessageBox(L"請在左列選擇要導出的詞典名稱。", L":-(", MB_OK |
    // MB_ICONINFORMATION);
    MSG_BY_IDS(IDS_STR_SEL_EXPORT_DICT_NAME, IDS_STR_SAD,
               MB_OK | MB_ICONINFORMATION);
    return 0;
  }
  std::wstring path;
  {
    char dir[MAX_PATH] = {0};
    rime_get_api()->get_user_data_sync_dir(dir, _countof(dir));
    WCHAR wdir[MAX_PATH] = {0};
    MultiByteToWideChar(CP_ACP, 0, dir, -1, wdir, _countof(wdir));
    path = wdir;
  }
  if (_waccess_s(path.c_str(), 0) != 0 &&
      !CreateDirectoryW(path.c_str(), NULL) &&
      GetLastError() == ERROR_PATH_NOT_FOUND) {
    // MessageBox(L"未能完成導出操作。會不會是同步文件夾無法訪問？", L":-(",
    // MB_OK | MB_ICONERROR);
    MSG_BY_IDS(IDS_STR_ERREXPORT_SYNC_UV, IDS_STR_SAD, MB_OK | MB_ICONERROR);
    return 0;
  }
  WCHAR dict_name[100] = {0};
  user_dict_list_.GetText(sel, dict_name);
  path += std::wstring(L"\\") + dict_name + L".userdb.txt";
  std::string dict_name_str = wstring_to_string(dict_name, CP_UTF8);
  if (!api_->backup_user_dict(dict_name_str.c_str())) {
    // MessageBox(L"不知哪裏出錯了，未能完成導出操作。", L":-(", MB_OK |
    // MB_ICONERROR);
    MSG_BY_IDS(IDS_STR_ERR_EXPORT_UNKNOWN, IDS_STR_SAD, MB_OK | MB_ICONERROR);
    return 0;
  } else if (_waccess(path.c_str(), 0) != 0) {
    // MessageBox(L"咦，輸出的快照文件找不着了。", L":-(", MB_OK |
    // MB_ICONERROR);
    MSG_BY_IDS(IDS_STR_ERR_EXPORT_SNAP_LOST, IDS_STR_SAD, MB_OK | MB_ICONERROR);
    return 0;
  }
  std::wstring param = L"/select, \"" + path + L"\"";
  ShellExecute(NULL, L"open", L"explorer.exe", param.c_str(), NULL,
               SW_SHOWNORMAL);
  return 0;
}

LRESULT DictManagementDialog::OnRestore(WORD, WORD code, HWND, BOOL&) {
  CFileDialog dlg(TRUE, L"snapshot", NULL, OFN_HIDEREADONLY | OFN_FILEMUSTEXIST,
                  L"詞典快照\0*.userdb.txt\0KCSS格式詞典快照\0*.userdb.kct."
                  L"snapshot\0全部文件\0*.*\0");
  if (IDOK == dlg.DoModal()) {
    char path[MAX_PATH] = {0};
    WideCharToMultiByte(CP_ACP, 0, dlg.m_szFileName, -1, path, _countof(path),
                        NULL, NULL);
    if (!api_->restore_user_dict(path)) {
      // MessageBox(L"不知哪裏出錯了，未能完成操作。", L":-(", MB_OK |
      // MB_ICONERROR);
      MSG_BY_IDS(IDS_STR_ERR_UNKNOWN, IDS_STR_SAD, MB_OK | MB_ICONERROR);
    } else {
      // MessageBox(L"完成了。", L":-)", MB_OK | MB_ICONINFORMATION);
      MSG_BY_IDS(IDS_STR_ERR_SUCCESS, IDS_STR_HAPPY,
                 MB_OK | MB_ICONINFORMATION);
    }
  }
  return 0;
}

LRESULT DictManagementDialog::OnExport(WORD, WORD code, HWND, BOOL&) {
  int sel = user_dict_list_.GetCurSel();
  if (sel < 0 || sel >= user_dict_list_.GetCount()) {
    // MessageBox(L"請在左列選擇要導出的詞典名稱。", L":-(", MB_OK |
    // MB_ICONINFORMATION);
    MSG_BY_IDS(IDS_STR_SEL_EXPORT_DICT_NAME, IDS_STR_SAD,
               MB_OK | MB_ICONINFORMATION);
    return 0;
  }
  WCHAR dict_name[MAX_PATH] = {0};
  user_dict_list_.GetText(sel, dict_name);
  std::wstring file_name(dict_name);
  file_name += L"_export.txt";
  CFileDialog dlg(FALSE, L"txt", file_name.c_str(),
                  OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
                  L"文本文檔\0*.txt\0全部文件\0*.*\0");
  if (IDOK == dlg.DoModal()) {
    char path[MAX_PATH] = {0};
    WideCharToMultiByte(CP_ACP, 0, dlg.m_szFileName, -1, path, _countof(path),
                        NULL, NULL);
    std::string dict_name_str = wstring_to_string(dict_name, CP_UTF8);
    int result = api_->export_user_dict(dict_name_str.c_str(), path);
    if (result < 0) {
      // MessageBox(L"不知哪裏出錯了，未能完成操作。", L":-(", MB_OK |
      // MB_ICONERROR);
      MSG_BY_IDS(IDS_STR_ERR_UNKNOWN, IDS_STR_SAD, MB_OK | MB_ICONERROR);
    } else if (_waccess(dlg.m_szFileName, 0) != 0) {
      // MessageBox(L"咦，導出的文件找不着了。", L":-(", MB_OK | MB_ICONERROR);
      MSG_BY_IDS(IDS_STR_ERR_EXPORT_FILE_LOST, IDS_STR_SAD,
                 MB_OK | MB_ICONERROR);
    } else {
      std::wstring report(L"導出了 " + std::to_wstring(result) + L" 條記錄。");
      // MessageBox(report.c_str(), L":-)", MB_OK | MB_ICONINFORMATION);
      MSG_ID_CAP(report.c_str(), IDS_STR_SAD, MB_OK | MB_ICONINFORMATION);
      std::wstring param =
          L"/select, \"" + std::wstring(dlg.m_szFileName) + L"\"";
      ShellExecute(NULL, L"open", L"explorer.exe", param.c_str(), NULL,
                   SW_SHOWNORMAL);
    }
  }
  return 0;
}

LRESULT DictManagementDialog::OnImport(WORD, WORD code, HWND, BOOL&) {
  int sel = user_dict_list_.GetCurSel();
  if (sel < 0 || sel >= user_dict_list_.GetCount()) {
    // MessageBox(L"請在左列選擇要導入的詞典名稱。", L":-(", MB_OK |
    // MB_ICONINFORMATION);
    MSG_BY_IDS(IDS_STR_SEL_IMPORT_DICT_NAME, IDS_STR_SAD,
               MB_OK | MB_ICONINFORMATION);
    return 0;
  }
  WCHAR dict_name[MAX_PATH] = {0};
  user_dict_list_.GetText(sel, dict_name);
  std::wstring file_name(dict_name);
  file_name += L"_export.txt";
  CFileDialog dlg(TRUE, L"txt", file_name.c_str(),
                  OFN_HIDEREADONLY | OFN_FILEMUSTEXIST,
                  L"文本文檔\0*.txt\0全部文件\0*.*\0");
  if (IDOK == dlg.DoModal()) {
    char path[MAX_PATH] = {0};
    WideCharToMultiByte(CP_ACP, 0, dlg.m_szFileName, -1, path, _countof(path),
                        NULL, NULL);
    int result = api_->import_user_dict(
        wstring_to_string(dict_name, CP_UTF8).c_str(), path);
    if (result < 0) {
      // MessageBox(L"不知哪裏出錯了，未能完成操作。", L":-(", MB_OK |
      // MB_ICONERROR);
      MSG_BY_IDS(IDS_STR_ERR_UNKNOWN, IDS_STR_SAD, MB_OK | MB_ICONERROR);
    } else {
      std::wstring report(L"導入了 " + std::to_wstring(result) + L" 條記錄。");
      // MessageBox(report.c_str(), L":-)", MB_OK | MB_ICONINFORMATION);
      MSG_ID_CAP(report.c_str(), IDS_STR_SAD, MB_OK | MB_ICONINFORMATION);
    }
  }
  return 0;
}

LRESULT DictManagementDialog::OnUserDictListSelChange(WORD, WORD, HWND, BOOL&) {
  int index = user_dict_list_.GetCurSel();
  BOOL enabled = index < 0 ? FALSE : TRUE;
  backup_.EnableWindow(enabled);
  export_.EnableWindow(enabled);
  import_.EnableWindow(enabled);
  return 0;
}
