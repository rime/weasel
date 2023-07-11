#include "pch.h"
#include <weasel/util.h>
#include <weasel/log.h>
#include <lmcons.h>
#include <Sddl.h>

#define LOW_INTEGRITY_SDDL_SACL      SDDL_SACL             \
                                     SDDL_DELIMINATOR      \
                                     SDDL_ACE_BEGIN        \
                                     SDDL_MANDATORY_LABEL  \
                                     SDDL_SEPERATOR        \
                                     SDDL_SEPERATOR        \
                                     SDDL_NO_WRITE_UP      \
                                     SDDL_SEPERATOR        \
                                     SDDL_SEPERATOR        \
                                     SDDL_SEPERATOR        \
                                     SDDL_ML_LOW           \
                                     SDDL_ACE_END

#define LOCAL_SYSTEM_FILE_ACCESS     SDDL_ACE_BEGIN        \
                                     SDDL_ACCESS_ALLOWED   \
                                     SDDL_SEPERATOR        \
                                     SDDL_SEPERATOR        \
                                     SDDL_FILE_ALL         \
                                     SDDL_SEPERATOR        \
                                     SDDL_SEPERATOR        \
                                     SDDL_SEPERATOR        \
                                     SDDL_LOCAL_SYSTEM     \
                                     SDDL_ACE_END

#define EVERYONE_FILE_ACCESS         SDDL_ACE_BEGIN        \
                                     SDDL_ACCESS_ALLOWED   \
                                     SDDL_SEPERATOR        \
                                     SDDL_SEPERATOR        \
                                     SDDL_FILE_ALL         \
                                     SDDL_SEPERATOR        \
                                     SDDL_SEPERATOR        \
                                     SDDL_SEPERATOR        \
                                     SDDL_EVERYONE         \
                                     SDDL_ACE_END

#define ALL_APP_PACKAGES_FILE_ACCESS SDDL_ACE_BEGIN        \
                                     SDDL_ACCESS_ALLOWED   \
                                     SDDL_SEPERATOR        \
                                     SDDL_SEPERATOR        \
                                     SDDL_FILE_ALL         \
                                     SDDL_SEPERATOR        \
                                     SDDL_SEPERATOR        \
                                     SDDL_SEPERATOR        \
                                     SDDL_ALL_APP_PACKAGES \
                                     SDDL_ACE_END

namespace weasel::utils
{

namespace
{
std::wstring wusername;
std::wstring winstall_dir;
std::wstring wuserdata_dir;
}

SECURITY_ATTRIBUTES make_security_attributes()
{
  // Privileges for UWP and IE protected mode
  // https://stackoverflow.com/questions/39138674/accessing-named-pipe-servers-from-within-ie-epm-bho

  PSECURITY_DESCRIPTOR pd;
  SECURITY_ATTRIBUTES sa;

  const BOOL r = ConvertStringSecurityDescriptorToSecurityDescriptor(
      LOW_INTEGRITY_SDDL_SACL
      SDDL_DACL
      SDDL_DELIMINATOR
      LOCAL_SYSTEM_FILE_ACCESS
      EVERYONE_FILE_ACCESS
      ALL_APP_PACKAGES_FILE_ACCESS ,
      SDDL_REVISION_1,
      &pd,
      NULL);

  if (r == FALSE)
  {
    LOG_LASTERROR(err, "ConvertStrToSD Failed");
    LocalFree(pd);
    THROW_IF_WIN32_BOOL_FALSE(FALSE);
  }

  sa.nLength = sizeof(SECURITY_ATTRIBUTES);
  sa.lpSecurityDescriptor = pd;
  sa.bInheritHandle = TRUE;

  return sa;
}

std::wstring get_wusername()
{
  if (!wusername.empty()) return wusername;

  wchar_t wun[ UNLEN + 1 ] = L"default";
  DWORD name_size = UNLEN + 1;
  const BOOL result = GetUserNameW(wun, &name_size);

  if (result == FALSE)
  {
    LOG_LASTERROR(err, "GetUserNameW failed");
  }

  wusername = wun;
  return wusername;
}

std::wstring install_dir()
{
  if (!winstall_dir.empty()) return winstall_dir;
  
  WCHAR exe_path[MAX_PATH] = {0};
  DWORD len = GetModuleFileNameW(GetModuleHandle(NULL), exe_path, ARRAYSIZE(exe_path));
  PWSTR start = exe_path + len;
  while ((start > exe_path) && (*(start - 1) != '\\')) start--;
  start[0] = L'\0';
  
  winstall_dir = exe_path;
  return winstall_dir;
}

std::wstring user_data_dir()
{
  if (!wuserdata_dir.empty()) return wuserdata_dir;
  WCHAR path[MAX_PATH] = {0};
  const WCHAR key[] = LR"(Software\Rime\Weasel)";
  wil::unique_hkey hkey;
  auto ret = RegOpenKeyW(HKEY_CURRENT_USER, key, &hkey);
  if (ret == ERROR_SUCCESS)
  {
    DWORD len = sizeof(path);
    DWORD type = 0;
    ret = RegQueryValueExW(hkey.get(), L"RimeUserDir", NULL, &type, reinterpret_cast<LPBYTE>(path), &len);
    if (ret == ERROR_SUCCESS && type == REG_SZ && path[0])
    {
      return path;
    }
  }
  ExpandEnvironmentStringsW(LR"(%AppData%\Rime)", path, ARRAYSIZE(path));
  return path;
}

}
