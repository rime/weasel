#pragma once

#include "resource.h"

namespace rime {

struct SchemaInfo;
class SwitcherSettings;

}  // namespace rime

class SwitcherSettingsDialog : public CDialogImpl<SwitcherSettingsDialog> {
public:
	enum { IDD = IDD_SWITCHER_SETTING };

	SwitcherSettingsDialog(rime::SwitcherSettings* settings);
	~SwitcherSettingsDialog();

protected:
	BEGIN_MSG_MAP(SwitcherSettingsDialog)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_CLOSE, OnClose)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		NOTIFY_HANDLER(IDC_SCHEMA_LIST, LVN_ITEMCHANGED, OnSchemaListItemChanged)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL&);
	LRESULT OnClose(UINT, WPARAM, LPARAM, BOOL&);
	LRESULT OnOK(WORD, WORD code, HWND, BOOL&);
	LRESULT OnSchemaListItemChanged(int, LPNMHDR, BOOL&);

	void Populate();
	void ShowDetails(const rime::SchemaInfo* info);

	rime::SwitcherSettings* settings_;
	bool loaded_;
	bool modified_;

	CCheckListViewCtrl schema_list_;
	CStatic description_;
	CEdit hotkeys_; 
};

