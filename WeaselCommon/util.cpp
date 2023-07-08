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

namespace weasel
{

namespace
{
std::wstring wusername;
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
    LOG_LASTERROR("ConvertStrToSD Failed");
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
    LOG_LASTERROR("GetUserNameW failed");
  }

  wusername = wun;
  return wusername;
}

}
