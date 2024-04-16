#include "stdafx.h"
#include "DictManagementDialog.h"
#include "Configurator.h"
#include <WeaselUtility.h>
#include <rime_api.h>
#include "WeaselDeployer.h"
#include <regex>

void static OpenFolderAndSelectItem(std::wstring filepath) {
  filepath = std::regex_replace(filepath, std::wregex(L"/"), L"\\");
  std::wstring directory;
  const size_t last_slash_idx = filepath.rfind('\\');
  if (std::string::npos != last_slash_idx) {
    directory = filepath.substr(0, last_slash_idx);
  }

  HRESULT hr;
  hr = CoInitializeEx(0, COINIT_MULTITHREADED);

  ITEMIDLIST* folder = ILCreateFromPath(directory.c_str());
  std::vector<LPITEMIDLIST> v;
  v.push_back(ILCreateFromPath(filepath.c_str()));

  SHOpenFolderAndSelectItems(folder, v.size(), (LPCITEMIDLIST*)v.data(), 0);

  for (auto idl : v) {
    ILFree(idl);
  }
  ILFree(folder);
}

static wchar_t* DoFileDialog(HWND hwndOwner,
                             BOOL bOpenFileDialog,
                             LPCWSTR title,
                             UINT size,
                             COMDLG_FILTERSPEC filter[],
                             LPCWSTR filename,
                             LPCWSTR defExt) {
  IFileDialog* pfd = NULL;
  IShellItem* psi = NULL;
  wchar_t* path = NULL;
  HRESULT hr = NULL;

  CoInitialize(NULL);
  if (bOpenFileDialog) {
    hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER,
                          IID_IFileOpenDialog, (void**)(&pfd));
  } else {
    hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_INPROC_SERVER,
                          IID_IFileSaveDialog, (void**)(&pfd));
  }
  hr = pfd->SetFileTypes(size, filter);
  hr = pfd->SetTitle(title);
  if (filename != NULL) {
    hr = pfd->SetFileName(filename);
  }
  hr = pfd->SetDefaultExtension(defExt);
  hr = pfd->Show(hwndOwner);
  hr = pfd->GetResult(&psi);
  if (SUCCEEDED(hr)) {
    hr = psi->GetDisplayName(SIGDN_FILESYSPATH, &path);
    if (SUCCEEDED(hr)) {
      return path;
    }
    psi->Release();
  }
  pfd->Release();
  CoUninitialize();
  return NULL;
}

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
    MSG_BY_IDS(IDS_STR_ERREXPORT_SYNC_UV, IDS_STR_SAD, MB_OK | MB_ICONERROR);
    return 0;
  }
  WCHAR dict_name[100] = {0};
  user_dict_list_.GetText(sel, dict_name);
  path += std::wstring(L"\\") + dict_name + L".userdb.txt";
  std::string dict_name_str = wstring_to_string(dict_name, CP_UTF8);
  if (!api_->backup_user_dict(dict_name_str.c_str())) {
    MSG_BY_IDS(IDS_STR_ERR_EXPORT_UNKNOWN, IDS_STR_SAD, MB_OK | MB_ICONERROR);
    return 0;
  } else if (_waccess(path.c_str(), 0) != 0) {
    MSG_BY_IDS(IDS_STR_ERR_EXPORT_SNAP_LOST, IDS_STR_SAD, MB_OK | MB_ICONERROR);
    return 0;
  }
  OpenFolderAndSelectItem(path);
  return 0;
}

LRESULT DictManagementDialog::OnRestore(WORD, WORD code, HWND, BOOL&) {
  COMDLG_FILTERSPEC filter[3] = {{L"詞典快照", L"*.userdb.txt"},
                                 {L"KCSS格式詞典快照",
                                  L"*.userdb.kct."
                                  L"snapshot"},
                                 {L"全部文件", L"*.*"}};

  wchar_t* selectedPath = DoFileDialog(m_hWnd, TRUE, L"導入", ARRAYSIZE(filter),
                                       filter, NULL, L"snapshot");
  if (selectedPath != NULL) {
    char path[MAX_PATH] = {0};
    WideCharToMultiByte(CP_ACP, 0, selectedPath, -1, path, _countof(path), NULL,
                        NULL);
    if (!api_->restore_user_dict(path)) {
      MSG_BY_IDS(IDS_STR_ERR_UNKNOWN, IDS_STR_SAD, MB_OK | MB_ICONERROR);
    } else {
      MSG_BY_IDS(IDS_STR_ERR_SUCCESS, IDS_STR_HAPPY,
                 MB_OK | MB_ICONINFORMATION);
    }
    CoTaskMemFree(selectedPath);
  }
  return 0;
}

LRESULT DictManagementDialog::OnExport(WORD, WORD code, HWND, BOOL&) {
  int sel = user_dict_list_.GetCurSel();
  if (sel < 0 || sel >= user_dict_list_.GetCount()) {
    MSG_BY_IDS(IDS_STR_SEL_EXPORT_DICT_NAME, IDS_STR_SAD,
               MB_OK | MB_ICONINFORMATION);
    return 0;
  }
  WCHAR dict_name[MAX_PATH] = {0};
  user_dict_list_.GetText(sel, dict_name);
  std::wstring file_name(dict_name);
  file_name += L"_export.txt";

  COMDLG_FILTERSPEC filter[2] = {{L"文本文檔", L"*.txt"},
                                 {L"全部文件", L"*.*"}};

  wchar_t* selectedPath =
      DoFileDialog(m_hWnd, FALSE, L"導出", ARRAYSIZE(filter), filter,
                   file_name.c_str(), L"txt");
  if (selectedPath != NULL) {
    char path[MAX_PATH] = {0};
    WideCharToMultiByte(CP_ACP, 0, selectedPath, -1, path, _countof(path), NULL,
                        NULL);
    std::string dict_name_str = wstring_to_string(dict_name, CP_UTF8);
    int result = api_->export_user_dict(dict_name_str.c_str(), path);
    if (result < 0) {
      MSG_BY_IDS(IDS_STR_ERR_UNKNOWN, IDS_STR_SAD, MB_OK | MB_ICONERROR);
    } else if (_waccess(selectedPath, 0) != 0) {
      MSG_BY_IDS(IDS_STR_ERR_EXPORT_FILE_LOST, IDS_STR_SAD,
                 MB_OK | MB_ICONERROR);
    } else {
      std::wstring report(L"導出了 " + std::to_wstring(result) + L" 條記錄。");
      MSG_ID_CAP(report.c_str(), IDS_STR_SAD, MB_OK | MB_ICONINFORMATION);
      OpenFolderAndSelectItem(selectedPath);
    }
    CoTaskMemFree(selectedPath);
  }
  return 0;
}

LRESULT DictManagementDialog::OnImport(WORD, WORD code, HWND, BOOL&) {
  int sel = user_dict_list_.GetCurSel();
  if (sel < 0 || sel >= user_dict_list_.GetCount()) {
    MSG_BY_IDS(IDS_STR_SEL_IMPORT_DICT_NAME, IDS_STR_SAD,
               MB_OK | MB_ICONINFORMATION);
    return 0;
  }
  WCHAR dict_name[MAX_PATH] = {0};
  user_dict_list_.GetText(sel, dict_name);
  std::wstring file_name(dict_name);
  file_name += L"_export.txt";

  COMDLG_FILTERSPEC filter[2] = {{L"文本文檔", L"*.txt"},
                                 {L"全部文件", L"*.*"}};

  wchar_t* selectedPath = DoFileDialog(m_hWnd, TRUE, L"導入", ARRAYSIZE(filter),
                                       filter, file_name.c_str(), L"txt");
  if (selectedPath != NULL) {
    char path[MAX_PATH] = {0};
    WideCharToMultiByte(CP_ACP, 0, selectedPath, -1, path, _countof(path), NULL,
                        NULL);
    int result = api_->import_user_dict(
        wstring_to_string(dict_name, CP_UTF8).c_str(), path);
    if (result < 0) {
      MSG_BY_IDS(IDS_STR_ERR_UNKNOWN, IDS_STR_SAD, MB_OK | MB_ICONERROR);
    } else {
      std::wstring report(L"導入了 " + std::to_wstring(result) + L" 條記錄。");
      MSG_ID_CAP(report.c_str(), IDS_STR_SAD, MB_OK | MB_ICONINFORMATION);
    }
    CoTaskMemFree(selectedPath);
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
