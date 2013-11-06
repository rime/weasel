#pragma once

class UIStyleSettings;

class Configurator
{
public:
	explicit Configurator();

	void Initialize();
	int Run(bool installing);
	int UpdateWorkspace(bool report_errors = false);
	int DictManagement();
	int SyncUserData();
};

const WCHAR* utf8towcs(const char* utf8_str);
