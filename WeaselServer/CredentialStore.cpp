#include "CredentialStore.h"

#include <windows.h>
#include <wincred.h>

bool CredentialStore::Save(std::wstring const& target,
                           std::wstring const& secret) const {
  if (target.empty()) {
    return false;
  }

  CREDENTIALW cred = {};
  cred.Type = CRED_TYPE_GENERIC;
  cred.TargetName = const_cast<LPWSTR>(target.c_str());
  cred.Persist = CRED_PERSIST_LOCAL_MACHINE;
  cred.CredentialBlobSize =
      static_cast<DWORD>(secret.size() * sizeof(wchar_t));
  cred.CredentialBlob =
      reinterpret_cast<LPBYTE>(const_cast<wchar_t*>(secret.data()));

  return CredWriteW(&cred, 0) == TRUE;
}

std::optional<std::wstring> CredentialStore::Load(
    std::wstring const& target) const {
  if (target.empty()) {
    return std::nullopt;
  }

  PCREDENTIALW pcred = nullptr;
  if (!CredReadW(target.c_str(), CRED_TYPE_GENERIC, 0, &pcred)) {
    return std::nullopt;
  }

  std::wstring secret;
  if (pcred->CredentialBlob && pcred->CredentialBlobSize > 0) {
    auto len = pcred->CredentialBlobSize / sizeof(wchar_t);
    auto ptr = reinterpret_cast<wchar_t const*>(pcred->CredentialBlob);
    secret.assign(ptr, ptr + len);
  }
  CredFree(pcred);
  return secret;
}

bool CredentialStore::Remove(std::wstring const& target) const {
  if (target.empty()) {
    return false;
  }
  return CredDeleteW(target.c_str(), CRED_TYPE_GENERIC, 0) == TRUE;
}
