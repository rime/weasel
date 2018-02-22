#include "stdafx.h"
#include "SecurityAttribute.h"

#define SECURITY_APP_PACKAGE_AUTHORITY              {0,0,0,0,0,15}
#define SECURITY_APP_PACKAGE_BASE_RID               (0x00000002L)
#define SECURITY_BUILTIN_APP_PACKAGE_RID_COUNT      (2L)
#define SECURITY_APP_PACKAGE_RID_COUNT              (8L)
#define SECURITY_CAPABILITY_BASE_RID                (0x00000003L)
#define SECURITY_BUILTIN_CAPABILITY_RID_COUNT       (2L)
#define SECURITY_CAPABILITY_RID_COUNT               (5L)
#define SECURITY_PARENT_PACKAGE_RID_COUNT           (SECURITY_APP_PACKAGE_RID_COUNT)
#define SECURITY_CHILD_PACKAGE_RID_COUNT            (12L)
#define SECURITY_BUILTIN_PACKAGE_ANY_PACKAGE        (0x00000001L)

namespace weasel {

	void SecurityAttribute::_Init()
	{
		memset(&ea, 0, sizeof(ea));

		// 对一般 desktop APP 的权限设置

		SID_IDENTIFIER_AUTHORITY worldSidAuthority = SECURITY_WORLD_SID_AUTHORITY;
		AllocateAndInitializeSid(&worldSidAuthority, 1,
			SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &sid_everyone);

		ea[0].grfAccessPermissions = GENERIC_ALL;
		ea[0].grfAccessMode = SET_ACCESS;
		ea[0].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
		ea[0].Trustee.pMultipleTrustee = NULL;
		ea[0].Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
		ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
		ea[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
		ea[0].Trustee.ptstrName = (LPTSTR)sid_everyone;

		// 对 winrt (UWP) APP 的权限设置
		//
		// Application Package Authority.
		//


		SID_IDENTIFIER_AUTHORITY appPackageAuthority = SECURITY_APP_PACKAGE_AUTHORITY;
		AllocateAndInitializeSid(&appPackageAuthority,
			SECURITY_BUILTIN_APP_PACKAGE_RID_COUNT,
			SECURITY_APP_PACKAGE_BASE_RID,
			SECURITY_BUILTIN_PACKAGE_ANY_PACKAGE,
			0, 0, 0, 0, 0, 0, &sid_all_apps);

		ea[1].grfAccessPermissions = GENERIC_ALL;
		ea[1].grfAccessMode = SET_ACCESS;
		ea[1].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
		ea[1].Trustee.pMultipleTrustee = NULL;
		ea[1].Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
		ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
		ea[1].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
		ea[1].Trustee.ptstrName = (LPTSTR)sid_all_apps;

		// create DACL
		DWORD err = SetEntriesInAcl(2, ea, NULL, &pacl);
		if (0 == err) {
			// security descriptor
			pd = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
			InitializeSecurityDescriptor(pd, SECURITY_DESCRIPTOR_REVISION);

			// Add the ACL to the security descriptor. 
			SetSecurityDescriptorDacl(pd, TRUE, pacl, FALSE);
		}

		sa.nLength = sizeof(SECURITY_ATTRIBUTES);
		sa.lpSecurityDescriptor = pd;
		sa.bInheritHandle = TRUE;
	}

	SECURITY_ATTRIBUTES *SecurityAttribute::get_attr()
	{
		return &sa;
	}
};
