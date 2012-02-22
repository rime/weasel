#include "stdafx.h"
#include "SwitcherSettingsDialog.h"
#include <rime/expl/switcher_settings.h>
#include <algorithm>
#include <set>
#include <boost/foreach.hpp>


static const WCHAR* utf8towcs(const char* utf8_str)
{
	const int buffer_len = 1024;
	static WCHAR buffer[buffer_len];
	memset(buffer, 0, sizeof(buffer));
	MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, buffer, buffer_len - 1);
	return buffer;
}

SwitcherSettingsDialog::SwitcherSettingsDialog(rime::SwitcherSettings* settings)
	: settings_(settings), loaded_(false), modified_(false)
{
}


SwitcherSettingsDialog::~SwitcherSettingsDialog()
{
}

void SwitcherSettingsDialog::Populate() {
	if (!settings_) return;
	const rime::SwitcherSettings::SchemaList& available(settings_->available());
	const rime::SwitcherSettings::Selection& selection(settings_->selection());
	schema_list_.DeleteAllItems();
	size_t i = 0;
	std::set<const rime::SchemaInfo*> recruited;
	BOOST_FOREACH(const std::string& schema_id, selection) {
		BOOST_FOREACH(const rime::SchemaInfo& info, available) {
			if (info.schema_id == schema_id && recruited.find(&info) == recruited.end()) {
				recruited.insert(&info);
				schema_list_.AddItem(i, 0, utf8towcs(info.name.c_str()));
				schema_list_.SetItemData(i, (DWORD_PTR)&info);
				schema_list_.SetCheckState(i, TRUE);
				++i;
				break;
			}
		}
	}
	BOOST_FOREACH(const rime::SchemaInfo& info, available) {
		if (recruited.find(&info) == recruited.end()) {
			recruited.insert(&info);
			schema_list_.AddItem(i, 0, utf8towcs(info.name.c_str()));
			schema_list_.SetItemData(i, (DWORD_PTR)&info);
			++i;
		}
	}
	hotkeys_.SetWindowTextW(utf8towcs(settings_->hotkeys().c_str()));
	loaded_ = true;
	modified_ = false;
}

void SwitcherSettingsDialog::ShowDetails(const rime::SchemaInfo* info) {
	if (!info) return;
	std::string details(info->name);
    if (!info->author.empty()) {
        details += "\n\n" + info->author;
    }
    if (!info->description.empty()) {
        details += "\n\n" + info->description;
    }
	description_.SetWindowTextW(utf8towcs(details.c_str()));
}

LRESULT SwitcherSettingsDialog::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&) {
	schema_list_.SubclassWindow(GetDlgItem(IDC_SCHEMA_LIST));
	schema_list_.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);
	schema_list_.AddColumn(L"方案名稱", 0);
	CRect rc;
	schema_list_.GetClientRect(&rc);
	schema_list_.SetColumnWidth(0, rc.Width() - 20);

	description_.Attach(GetDlgItem(IDC_SCHEMA_DESCRIPTION));
	
	hotkeys_.Attach(GetDlgItem(IDC_HOTKEYS));
	hotkeys_.EnableWindow(FALSE);
	
	Populate();
	
	CenterWindow();
	BringWindowToTop();
	return TRUE;
}

LRESULT SwitcherSettingsDialog::OnClose(UINT, WPARAM, LPARAM, BOOL&) {
	EndDialog(IDCANCEL);
	return 0;
}

LRESULT SwitcherSettingsDialog::OnOK(WORD, WORD code, HWND, BOOL&) {
	if (modified_ && settings_) {
		rime::SwitcherSettings::Selection selection;
		for (int i = 0; i < schema_list_.GetItemCount(); ++i) {
			if (!schema_list_.GetCheckState(i))
				continue;
			const rime::SchemaInfo* info = 
				(const rime::SchemaInfo*)(schema_list_.GetItemData(i));
			if (info) {
				selection.push_back(info->schema_id);
			}
		}
		if (selection.empty()) {
			MessageBox(L"至少要選用一項吧。", L"小狼毫不是這般用法", MB_OK | MB_ICONEXCLAMATION);
			return 0;
		}
		settings_->Select(selection);
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
		ShowDetails((const rime::SchemaInfo*)(schema_list_.GetItemData(lv->iItem)));
	}
	return 0;
}
