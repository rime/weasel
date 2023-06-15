#pragma once

#include "resource.h"

class InstallOptionsDialog : public CDialogImpl<InstallOptionsDialog>
{
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
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL&);
	LRESULT OnClose(UINT, WPARAM, LPARAM, BOOL&);
	LRESULT OnOK(WORD, WORD code, HWND, BOOL&);
	LRESULT OnRemove(WORD, WORD code, HWND, BOOL&);
	LRESULT OnUseDefaultDir(WORD, WORD code, HWND, BOOL&);
	LRESULT OnUseCustomDir(WORD, WORD code, HWND, BOOL&);

	void UpdateWidgets();

	CButton cn_;
	CButton tw_;
	CButton remove_;
	CButton default_dir_;
	CButton custom_dir_;
	CEdit dir_;
};
