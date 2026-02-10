#pragma once

#include <optional>
#include <string>

class CredentialStore {
 public:
  bool Save(std::wstring const& target, std::wstring const& secret) const;
  std::optional<std::wstring> Load(std::wstring const& target) const;
  bool Remove(std::wstring const& target) const;
};
