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

template <typename T>
LSTATUS SetRegKeyValue(HKEY rootKey,
                       const wchar_t* subpath,
                       const wchar_t* key,
                       const T& value,
                       DWORD type,
                       bool disable_reg_redirect = false) {
  HKEY hKey;
  auto flag_wow64 = (disable_reg_redirect && is_wow64()) ? KEY_WOW64_64KEY : 0;
  LSTATUS ret =
      RegOpenKeyEx(rootKey, subpath, 0, KEY_ALL_ACCESS | flag_wow64, &hKey);
  if (ret != ERROR_SUCCESS) {
    ret = RegCreateKeyEx(rootKey, subpath, 0, NULL, 0,
                         KEY_ALL_ACCESS | flag_wow64, 0, &hKey, NULL);
    if (ret != ERROR_SUCCESS)
      return ret;
  }
  if (ret == ERROR_SUCCESS) {
    DWORD dataSize;
    const BYTE* dataPtr;
    if constexpr (std::is_same<T, std::wstring>::value) {
      dataSize = (value.size() + 1) * sizeof(wchar_t);
      dataPtr = reinterpret_cast<const BYTE*>(value.c_str());
    } else if constexpr (std::is_same<T, const wchar_t*>::value) {
      dataSize = (wcslen((wchar_t*)value) + 1) * sizeof(wchar_t);
      dataPtr = reinterpret_cast<const BYTE*>(value);
    } else {
      dataSize = sizeof(T);
      dataPtr = reinterpret_cast<const BYTE*>(&value);
    }
    ret = RegSetValueEx(hKey, key, 0, type, dataPtr, dataSize);
    RegCloseKey(hKey);
  }
  return ret;
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
  COMMAND_ID_HANDLER(IDC_BUTTON_CUSTOM_DIR, OnUseCustomDir)
  END_MSG_MAP()

  LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL&);
  LRESULT OnClose(UINT, WPARAM, LPARAM, BOOL&);
  LRESULT OnOK(WORD, WORD code, HWND, BOOL&);
  LRESULT OnRemove(WORD, WORD code, HWND, BOOL&);
  LRESULT OnUseDefaultDir(WORD, WORD code, HWND, BOOL&);
  LRESULT OnUseCustomDir(WORD, WORD code, HWND, BOOL&);

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
