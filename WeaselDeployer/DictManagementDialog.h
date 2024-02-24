#pragma once

#include "resource.h"

#include <rime_levers_api.h>

class DictManagementDialog : public CDialogImpl<DictManagementDialog> {
 public:
  enum { IDD = IDD_DICT_MANAGEMENT };

  DictManagementDialog();
  ~DictManagementDialog();

 protected:
  BEGIN_MSG_MAP(DictManagementDialog)
  MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
  MESSAGE_HANDLER(WM_CLOSE, OnClose)
  COMMAND_ID_HANDLER(IDC_BACKUP, OnBackup)
  COMMAND_ID_HANDLER(IDC_RESTORE, OnRestore)
  COMMAND_ID_HANDLER(IDC_EXPORT, OnExport)
  COMMAND_ID_HANDLER(IDC_IMPORT, OnImport)
  COMMAND_HANDLER(IDC_USER_DICT_LIST, LBN_SELCHANGE, OnUserDictListSelChange)
  END_MSG_MAP()

  LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL&);
  LRESULT OnClose(UINT, WPARAM, LPARAM, BOOL&);
  LRESULT OnBackup(WORD, WORD code, HWND, BOOL&);
  LRESULT OnRestore(WORD, WORD code, HWND, BOOL&);
  LRESULT OnExport(WORD, WORD code, HWND, BOOL&);
  LRESULT OnImport(WORD, WORD code, HWND, BOOL&);
  LRESULT OnUserDictListSelChange(WORD, WORD, HWND, BOOL&);

  void Populate();

  CListBox user_dict_list_;
  CButton backup_;
  CButton restore_;
  CButton export_;
  CButton import_;

  RimeLeversApi* api_;
};
