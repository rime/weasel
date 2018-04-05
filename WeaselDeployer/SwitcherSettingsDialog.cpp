﻿#include "stdafx.h"
#include "SwitcherSettingsDialog.h"
#include "Configurator.h"
#include <algorithm>
#include <set>
#include <rime_levers_api.h>
#include <WeaselUtility.h>


SwitcherSettingsDialog::SwitcherSettingsDialog(RimeSwitcherSettings* settings)
	: settings_(settings), loaded_(false), modified_(false)
{
	api_ = (RimeLeversApi*)rime_get_api()->find_module("levers")->get_api();
}


SwitcherSettingsDialog::~SwitcherSettingsDialog()
{
}

void SwitcherSettingsDialog::Populate() {
	if (!settings_) return;
	RimeSchemaList available = {0};
	api_->get_available_schema_list(settings_, &available);
	RimeSchemaList selected = {0};
	api_->get_selected_schema_list(settings_, &selected);
	schema_list_.DeleteAllItems();
	size_t k = 0;
	std::set<RimeSchemaInfo*> recruited;
	for (size_t i = 0; i < selected.size; ++i) {
		const char* schema_id = selected.list[i].schema_id;
		for (size_t j = 0; j < available.size; ++j) {
			RimeSchemaListItem& item(available.list[j]);
			RimeSchemaInfo* info = (RimeSchemaInfo*)item.reserved;
			if (!strcmp(item.schema_id, schema_id) && recruited.find(info) == recruited.end()) {
				recruited.insert(info);
				schema_list_.AddItem(k, 0, utf8towcs(item.name));
				schema_list_.SetItemData(k, (DWORD_PTR)info);
				schema_list_.SetCheckState(k, TRUE);
				++k;
				break;
			}
		}
	}
	for (size_t i = 0; i < available.size; ++i) {
		RimeSchemaListItem& item(available.list[i]);
		RimeSchemaInfo* info = (RimeSchemaInfo*)item.reserved;
		if (recruited.find(info) == recruited.end()) {
			recruited.insert(info);
			schema_list_.AddItem(k, 0, utf8towcs(item.name));
			schema_list_.SetItemData(k, (DWORD_PTR)info);
			++k;
		}
	}
	hotkeys_.SetWindowTextW(utf8towcs(api_->get_hotkeys(settings_)));
	loaded_ = true;
	modified_ = false;
}

void SwitcherSettingsDialog::ShowDetails(RimeSchemaInfo* info) {
	if (!info) return;
	std::string details;
	if (const char* name = api_->get_schema_name(info)) {
		details += name;
	}
    if (const char* author = api_->get_schema_author(info)) {
        (details += "\n\n") += author;
    }
    if (const char* description = api_->get_schema_description(info)) {
        (details += "\n\n") += description;
    }
	description_.SetWindowTextW(utf8towcs(details.c_str()));
}

LRESULT SwitcherSettingsDialog::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&) {
	schema_list_.SubclassWindow(GetDlgItem(IDC_SCHEMA_LIST));
	schema_list_.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);
	schema_list_.AddColumn(L"方案名稱", 0);
	WTL::CRect rc;
	schema_list_.GetClientRect(&rc);
	schema_list_.SetColumnWidth(0, rc.Width() - 20);

	description_.Attach(GetDlgItem(IDC_SCHEMA_DESCRIPTION));
	
	hotkeys_.Attach(GetDlgItem(IDC_HOTKEYS));
	hotkeys_.EnableWindow(FALSE);

	get_schemata_.Attach(GetDlgItem(IDC_GET_SCHEMATA));
	get_schemata_.EnableWindow(TRUE);
	
	Populate();
	
	CenterWindow();
	BringWindowToTop();
	return TRUE;
}

LRESULT SwitcherSettingsDialog::OnClose(UINT, WPARAM, LPARAM, BOOL&) {
	EndDialog(IDCANCEL);
	return 0;
}

LRESULT SwitcherSettingsDialog::OnGetSchemata(WORD, WORD, HWND hWndCtl, BOOL&) {
	std::wstring parameter = std::wstring(L"/k (set plum_dir=") + WeaselUserDataPath() + L"\\plum) && (set git_mirror=taobao) && cd \"";
	HKEY hKey;
	LSTATUS ret = RegOpenKey(HKEY_LOCAL_MACHINE, _T("Software\\Rime\\Weasel"), &hKey);
	if (ret == ERROR_SUCCESS) {
		WCHAR value[MAX_PATH];
		DWORD len = sizeof(value);
		DWORD type = 0;
		DWORD data = 0;
		ret = RegQueryValueExW(hKey, L"WeaselRoot", NULL, &type, (LPBYTE)value, &len);
		if (ret == ERROR_SUCCESS && type == REG_SZ)
		{
			ShellExecuteW(hWndCtl, L"open", L"cmd", (parameter + value + L"\" && rime-install.bat").c_str(), NULL, SW_SHOW);
		}
	}
	RegCloseKey(hKey);
	return 0;
}

LRESULT SwitcherSettingsDialog::OnOK(WORD, WORD code, HWND, BOOL&) {
	if (modified_ && settings_ && schema_list_.GetItemCount() != 0) {
		const char** selection = new const char*[schema_list_.GetItemCount()];
		int count = 0;
		for (int i = 0; i < schema_list_.GetItemCount(); ++i) {
			if (!schema_list_.GetCheckState(i))
				continue;
			RimeSchemaInfo* info = (RimeSchemaInfo*)(schema_list_.GetItemData(i));
			if (info) {
				selection[count++] = api_->get_schema_id(info);
			}
		}
		if (count == 0) {
			MessageBox(_T("至少要選用一項吧。"), _T("小狼毫不是這般用法"), MB_OK | MB_ICONEXCLAMATION);
			delete selection;
			return 0;
		}
		api_->select_schemas(settings_, selection, count);
		delete selection;
	}
	EndDialog(code);
	return 0;
}

LRESULT SwitcherSettingsDialog::OnSchemaListItemChanged(int, LPNMHDR p, BOOL&) {
	LPNMLISTVIEW lv = reinterpret_cast<LPNMLISTVIEW>(p);
	if (!loaded_ || !lv || lv->iItem < 0 && lv->iItem >= schema_list_.GetItemCount())
		return 0;
	if ((lv->uNewState & LVIS_STATEIMAGEMASK) != (lv->uOldState & LVIS_STATEIMAGEMASK)) {
		modified_ = true;
	}
	else if ((lv->uNewState & LVIS_SELECTED) && !(lv->uOldState & LVIS_SELECTED)) {
		ShowDetails((RimeSchemaInfo*)(schema_list_.GetItemData(lv->iItem)));
	}
	return 0;
}
