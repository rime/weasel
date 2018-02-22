#pragma once
#include <winnt.h> // for security attributes constants
#include <aclapi.h> // for ACL

namespace weasel {
	class SecurityAttribute {
	private:
		PSECURITY_DESCRIPTOR pd;
		SECURITY_ATTRIBUTES sa;
		PACL pacl;
		EXPLICIT_ACCESS ea[2];
		PSID sid_everyone;
		PSID sid_all_apps;
		void _Init();
	public:
		SecurityAttribute() { _Init(); }
		SECURITY_ATTRIBUTES *get_attr();
	};
};
