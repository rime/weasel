#pragma once
#include "windows.h"
#include<fstream>
#include<strsafe.h>

#pragma warning(disable:4353)

#if defined(_DEBUG)
#
#	define _DUMP_MSG_WRITE_DISK 0

#	define _traceXX(fmt, ...)					\
	{											\
		TCHAR buffer[512] = { 0 };				\
		wsprintf(buffer, fmt, ##__VA_ARGS__);   \
		OutputDebugString(buffer);				\
	};
#
#	if(_DUMP_MSG_WRITE_DISK)
#		ifndef _DUMP_FILE
#			error "Please define _DUMP_FILE."
#		endif
		static std::wofstream g_dumpFile;
		static WNDPROC g_fnWndProcOrg = nullptr;
		const TCHAR* g_prefix = TEXT("DUMP_MSG:");
		void _dumpMsg(const TCHAR* fmt, ...)
		{
			TCHAR buffer[512] = { 0 };
			va_list args;
			va_start(args, fmt);
			StringCchVPrintf(buffer, 512, fmt, args);
			va_end(args);
			assert(g_dumpFile.is_open());
			if (g_dumpFile.is_open()) {
				g_dumpFile << std::wstring(buffer);
				g_dumpFile.flush();
			}
		};
#		define MSGDUMP_PREFIX  g_prefix
#		define MSGDUMP_TPRINTF _dumpMsg
#		define OPEN_DUMP_FILE()	     { g_dumpFile.open(_DUMP_FILE,std::ios::out|std::ios::app); }
#		define CLOSE_DUMP_FILE()     { if(g_dumpFile.is_open()) g_dumpFile.close(); }
#		define DUMP_FILE_IS_OPEN()   { assert(g_dumpFile.is_open()); }
#		define MSG_DUMP(hWnd)        { _HookWndProc(hWnd);		    }
#		define MSG_DUMP2(wndProcOrg) { g_fnWndProcOrg=wndProcOrg; wndProcOrg=_WrapWindowProc;}

#		include "msgdump.h"

		static LRESULT CALLBACK _WrapWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
		{
			if (InSendMessage())
				g_prefix = TEXT("DUMP_MSG:S: ");
			else
				g_prefix = TEXT("DUMP_MSG:P: ");
			MD_msgdump(hwnd, uMsg, wParam, lParam);
			LRESULT lResult = CallWindowProc(g_fnWndProcOrg, hwnd, uMsg, wParam, lParam);
			g_prefix = TEXT("DUMP_MSG:R: ");
			MD_msgresult(hwnd, uMsg, wParam, lParam, lResult);
			return lResult;
		};
		static void _HookWndProc(HWND hWnd) {
			g_fnWndProcOrg = (WNDPROC)SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LPARAM)(WNDPROC)(_WrapWindowProc));
		};
#	else
#		undef _DUMP_FILE
#		define OPEN_DUMP_FILE()	    ((void)0)
#		define CLOSE_DUMP_FILE()    ((void)0)
#		define DUMP_FILE_IS_OPEN()  ((void)0)
#		define MSG_DUMP				((void)0)
#		define MSG_DUMP2			((void)0)
#	endif
#else
#	define _traceXX   ((void)0)
#endif // _DEBUG



