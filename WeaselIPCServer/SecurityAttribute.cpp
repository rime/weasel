#include "stdafx.h"
#include "SecurityAttribute.h"
#include <Sddl.h>

#ifndef SDDL_ALL_APP_PACKAGES
#define SDDL_ALL_APP_PACKAGES TEXT("AC")
#endif

#define LOW_INTEGRITY_SDDL_SACL \
  SDDL_SACL                     \
  SDDL_DELIMINATOR              \
  SDDL_ACE_BEGIN                \
  SDDL_MANDATORY_LABEL          \
  SDDL_SEPERATOR                \
  SDDL_SEPERATOR                \
  SDDL_NO_WRITE_UP              \
  SDDL_SEPERATOR                \
  SDDL_SEPERATOR                \
  SDDL_SEPERATOR                \
  SDDL_ML_LOW                   \
  SDDL_ACE_END

#define LOCAL_SYSTEM_FILE_ACCESS \
  SDDL_ACE_BEGIN                 \
  SDDL_ACCESS_ALLOWED            \
  SDDL_SEPERATOR                 \
  SDDL_SEPERATOR                 \
  SDDL_FILE_ALL                  \
  SDDL_SEPERATOR                 \
  SDDL_SEPERATOR                 \
  SDDL_SEPERATOR                 \
  SDDL_LOCAL_SYSTEM              \
  SDDL_ACE_END

#define EVERYONE_FILE_ACCESS \
  SDDL_ACE_BEGIN             \
  SDDL_ACCESS_ALLOWED        \
  SDDL_SEPERATOR             \
  SDDL_SEPERATOR             \
  SDDL_FILE_ALL              \
  SDDL_SEPERATOR             \
  SDDL_SEPERATOR             \
  SDDL_SEPERATOR             \
  SDDL_EVERYONE              \
  SDDL_ACE_END

#define ALL_APP_PACKAGES_FILE_ACCESS \
  SDDL_ACE_BEGIN                     \
  SDDL_ACCESS_ALLOWED                \
  SDDL_SEPERATOR                     \
  SDDL_SEPERATOR                     \
  SDDL_FILE_ALL                      \
  SDDL_SEPERATOR                     \
  SDDL_SEPERATOR                     \
  SDDL_SEPERATOR                     \
  SDDL_ALL_APP_PACKAGES              \
  SDDL_ACE_END

namespace weasel {

void SecurityAttribute::_Init() {
  // Privileges for UWP and IE protected mode
  // https://stackoverflow.com/questions/39138674/accessing-named-pipe-servers-from-within-ie-epm-bho

  ConvertStringSecurityDescriptorToSecurityDescriptor(
      LOW_INTEGRITY_SDDL_SACL SDDL_DACL SDDL_DELIMINATOR
          LOCAL_SYSTEM_FILE_ACCESS EVERYONE_FILE_ACCESS
              ALL_APP_PACKAGES_FILE_ACCESS,
      SDDL_REVISION_1, &pd, NULL);
  sa.nLength = sizeof(SECURITY_ATTRIBUTES);
  sa.lpSecurityDescriptor = pd;
  sa.bInheritHandle = TRUE;
}

SECURITY_ATTRIBUTES* SecurityAttribute::get_attr() {
  return &sa;
}
};  // namespace weasel
