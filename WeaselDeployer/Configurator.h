#pragma once

#include <string>
#include <vector>

class UIStyleSettings;

class Configurator {
 public:
  explicit Configurator();

  void Initialize();
  int Run(bool installing);
  int UpdateWorkspace(bool report_errors = false);
  int DictManagement();
  int DictManagement(const std::vector<std::wstring>& args);
  int SyncUserData();
};
