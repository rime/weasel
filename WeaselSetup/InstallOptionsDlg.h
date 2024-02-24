#pragma once

#include "resource.h"
#include <atlstr.h>

#define MSG_BY_IDS(idInfo, idCap, uType)           \
  {                                                \
    CString info, cap;                             \
    info.LoadStringW(idInfo);                      \
    cap.LoadStringW(idCap);                        \
    LANGID langID = GetThreadUILanguage();         \
    MessageBoxExW(NULL, info, cap, uType, langID); \
  }

#define MSG_ID_CAP(info, idCap, uType)             \
  {                                                \
    CString cap;                                   \
    cap.LoadStringW(idCap);                        \
    LANGID langID = GetThreadUILanguage();         \
    MessageBoxExW(NULL, info, cap, uType, langID); \
  }

#define MSG_NOT_SILENT_BY_IDS(silent, idInfo, idCap, uType) \
  {                                                         \
    if (!silent)                                            \
      MSG_BY_IDS(idInfo, idCap, uType);                     \
  }
#define MSG_NOT_SILENT_ID_CAP(silent, info, idCap, uType) \
  {                                                       \
    if (!silent)                                          \
      MSG_ID_CAP(info, idCap, uType);                     \
  }

class InstallOptionsDialog : public CDialogImpl<InstallOptionsDialog> {
 public:
  enum { IDD = IDD_INSTALL_OPTIONS };

  InstallOptionsDialog();
  ~InstallOptionsDialog();

  bool installed;
  bool hant;
  bool old_ime_support;
  std::wstring user_dir;

 protected:
  BEGIN_MSG_MAP(InstallOptionsDialog)
  MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
  MESSAGE_HANDLER(WM_CLOSE, OnClose)
  COMMAND_ID_HANDLER(IDOK, OnOK)
  COMMAND_ID_HANDLER(IDC_REMOVE, OnRemove)
  COMMAND_ID_HANDLER(IDC_RADIO_DEFAULT_DIR, OnUseDefaultDir)
  COMMAND_ID_HANDLER(IDC_RADIO_CUSTOM_DIR, OnUseCustomDir)
  COMMAND_ID_HANDLER(IDC_BUTTON_CUSTOM_DIR, OnBtnCustomDir)
  END_MSG_MAP()

  LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL&);
  LRESULT OnClose(UINT, WPARAM, LPARAM, BOOL&);
  LRESULT OnOK(WORD, WORD code, HWND, BOOL&);
  LRESULT OnRemove(WORD, WORD code, HWND, BOOL&);
  LRESULT OnUseDefaultDir(WORD, WORD code, HWND, BOOL&);
  LRESULT OnUseCustomDir(WORD, WORD code, HWND, BOOL&);
  LRESULT OnBtnCustomDir(WORD, WORD code, HWND, BOOL&);

  CButton cn_;
  CButton tw_;
  CButton remove_;
  CButton default_dir_;
  CButton custom_dir_;
  CButton ok_;
  CButton ime_;
  CButton button_custom_dir_;
  CEdit dir_;
};
