#include "stdafx.h"
#include "SecurityAttribute.h"
#include <Sddl.h>

namespace weasel {

	void SecurityAttribute::_Init()
	{
		// Privilages for UWP and IE protected mode
		// https://stackoverflow.com/questions/39138674/accessing-named-pipe-servers-from-within-ie-epm-bho
		ConvertStringSecurityDescriptorToSecurityDescriptorW(
			L"S:(ML;;NW;;;LW)D:(A;;FA;;;SY)(A;;FA;;;WD)(A;;FA;;;AC)",
			SDDL_REVISION_1,
			&pd,
			NULL);

		sa.nLength = sizeof(SECURITY_ATTRIBUTES);
		sa.lpSecurityDescriptor = pd;
		sa.bInheritHandle = TRUE;
	}

	SECURITY_ATTRIBUTES *SecurityAttribute::get_attr()
	{
		return &sa;
	}
};
