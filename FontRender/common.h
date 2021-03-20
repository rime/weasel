#ifndef UI_GFX_WIN_COMMON_H_
#define UI_GFX_WIN_COMMON_H_

/* #ifdef _DEBUG
	#include <crtdbg.h>

	inline void EnableMemLeakCheck()
	{
		_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
	}

	#define new  new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif */

#include <wrl/client.h>
#include <dwrite_3.h>
#include <d2d1.h>

#define D_INFO(msg) {}

#endif //UI_GFX_WIN_COMMON_H_