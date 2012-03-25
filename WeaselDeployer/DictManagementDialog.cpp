#include "stdafx.h"
#include "DictManagementDialog.h"
#include "Configurator.h"
#include <boost/format.hpp>

DictManagementDialog::DictManagementDialog(rime::Deployer* deployer)
	: mgr_(deployer)
{
}

DictManagementDialog::~DictManagementDialog()
{
}

void DictManagementDialog::Populate() {
	mgr_.GetUserDictList(&dicts_);
	for (size_t i = 0; i < dicts_.size(); ++i) {
		user_dict_list_.AddString(utf8towcs(dicts_[i].c_str()));
	}
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
	if (sel < 0 || sel >= dicts_.size()) {
		MessageBox(L"請在左列選擇要導出的詞典名稱。", L":-(", MB_OK | MB_ICONINFORMATION);
		return 0;
	}
	if (!mgr_.Backup(dicts_[sel])) {
		MessageBox(L"不知哪裏出錯了，未能完成操作。", L":-(", MB_OK | MB_ICONERROR);
	}
	else {
		WCHAR path[MAX_PATH] = {0};
		ExpandEnvironmentStrings(L"%AppData%\\Rime\\", path, _countof(path));
		MultiByteToWideChar(CP_ACP, 0, dicts_[sel].c_str(), -1, path + wcslen(path), _countof(path) - wcslen(path));
		wcscpy(path + wcslen(path), L".userdb.kct.snapshot");
		if (_waccess(path, 0) != 0) {
			MessageBox(L"咦，輸出的快照文件找不着了。", L":-(", MB_OK | MB_ICONERROR);
		}
		else {
			std::wstring param = L"/select, \"" + std::wstring(path) + L"\"";
			ShellExecute(NULL, L"open", L"explorer.exe", param.c_str(), NULL, SW_SHOWNORMAL);
		}
	}
	return 0;
}

LRESULT DictManagementDialog::OnRestore(WORD, WORD code, HWND, BOOL&) {
	CFileDialog dlg(TRUE, L"snapshot", NULL, OFN_HIDEREADONLY | OFN_FILEMUSTEXIST, L"詞典快照\0*.userdb.kct.snapshot\0全部文件\0*.*\0");
	if (IDOK == dlg.DoModal()) {
		char path[MAX_PATH] = {0};
		WideCharToMultiByte(CP_ACP, 0, dlg.m_szFileName, -1, path, _countof(path), NULL, NULL);
		if (!mgr_.Restore(path)) {
			MessageBox(L"不知哪裏出錯了，未能完成操作。", L":-(", MB_OK | MB_ICONERROR);
		}
		else {
			MessageBox(L"完成了。", L":-)", MB_OK | MB_ICONINFORMATION);
		}
	}
	return 0;
}

LRESULT DictManagementDialog::OnExport(WORD, WORD code, HWND, BOOL&) {
	int sel = user_dict_list_.GetCurSel();
	if (sel < 0 || sel >= dicts_.size()) {
		MessageBox(L"請在左列選擇要導出的詞典名稱。", L":-(", MB_OK | MB_ICONINFORMATION);
		return 0;
	}
	WCHAR name[MAX_PATH] = {0};
	MultiByteToWideChar(CP_ACP, 0, dicts_[sel].c_str(), -1, name, _countof(name));
	wcscpy(name + wcslen(name), L"_export.txt");
	CFileDialog dlg(FALSE, L"txt", name, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, L"文本文檔\0*.txt\0全部文件\0*.*\0");
	if (IDOK == dlg.DoModal()) {
		char path[MAX_PATH] = {0};
		WideCharToMultiByte(CP_ACP, 0, dlg.m_szFileName, -1, path, _countof(path), NULL, NULL);
		int result = mgr_.Export(dicts_[sel], path);
		if (result < 0) {
			MessageBox(L"不知哪裏出錯了，未能完成操作。", L":-(", MB_OK | MB_ICONERROR);
		}
		else if (_waccess(dlg.m_szFileName, 0) != 0) {
			MessageBox(L"咦，導出的文件找不着了。", L":-(", MB_OK | MB_ICONERROR);
		}
		else {
			std::wstring report(boost::str(boost::wformat(L"導出了 %d 條記錄。") % result));
			MessageBox(report.c_str(), L":-)", MB_OK | MB_ICONINFORMATION);
			std::wstring param = L"/select, \"" + std::wstring(dlg.m_szFileName) + L"\"";
			ShellExecute(NULL, L"open", L"explorer.exe", param.c_str(), NULL, SW_SHOWNORMAL);
		}
	}
	return 0;
}

LRESULT DictManagementDialog::OnImport(WORD, WORD code, HWND, BOOL&) {
	int sel = user_dict_list_.GetCurSel();
	if (sel < 0 || sel >= dicts_.size()) {
		MessageBox(L"請在左列選擇要導入的詞典名稱。", L":-(", MB_OK | MB_ICONINFORMATION);
		return 0;
	}
	WCHAR name[MAX_PATH] = {0};
	MultiByteToWideChar(CP_ACP, 0, dicts_[sel].c_str(), -1, name, _countof(name));
	wcscpy(name + wcslen(name), L"_export.txt");
	CFileDialog dlg(TRUE, L"txt", name, OFN_HIDEREADONLY | OFN_FILEMUSTEXIST, L"文本文檔\0*.txt\0全部文件\0*.*\0");
	if (IDOK == dlg.DoModal()) {
		char path[MAX_PATH] = {0};
		WideCharToMultiByte(CP_ACP, 0, dlg.m_szFileName, -1, path, _countof(path), NULL, NULL);
		int result = mgr_.Import(dicts_[sel], path);
		if (result < 0) {
			MessageBox(L"不知哪裏出錯了，未能完成操作。", L":-(", MB_OK | MB_ICONERROR);
		}
		else {
			std::wstring report(boost::str(boost::wformat(L"導入了 %d 條記錄。") % result));
			MessageBox(report.c_str(), L":-)", MB_OK | MB_ICONINFORMATION);
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
