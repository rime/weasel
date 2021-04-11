#pragma once
#include "windows.h"
#include<fstream>

#if defined(_DEBUG)
#
#	define _DUMP_MSG_WRITE_DISK 0
#
#	if(_DUMP_MSG_WRITE_DISK)
#		ifndef _DUMP_FILE
#			error "Please define _DUMP_FILE."
#		endif
		static std::wofstream g_dumpFile;
		const TCHAR* g_prefix = TEXT("DUMP_MSG:");
#		define MSGDUMP_PREFIX  g_prefix
#		define MSGDUMP_TPRINTF _dumpMsg
#		define OPEN_DUMP_FILE()	    { g_dumpFile.open(_DUMP_FILE,std::ios::out|std::ios::app); }
#		define CLOSE_DUMP_FILE()    { if(g_dumpFile.is_open()) g_dumpFile.close(); }
#		define DUMP_FILE_IS_OPEN()  { assert(g_dumpFile.is_open()); }
#		define MSG_DUMP   MD_msgdump
#		define MSG_RESULT MD_msgresult

		void _dumpMsg(const TCHAR* fmt, ...)
		{
			TCHAR buffer[512] = { 0 };
			va_list args;
			va_start(args, fmt);
			vswprintf(buffer, fmt, args);
			va_end(args);
			if (g_dumpFile.is_open()) {
				g_dumpFile << std::wstring(buffer);
			}
		};
#		include "msgdump.h"
#	else
#		undef _DUMP_FILE
#		define OPEN_DUMP_FILE()	    (void)0
#		define CLOSE_DUMP_FILE()    (void)0
#		define DUMP_FILE_IS_OPEN()  (void)0
#		define MSG_DUMP   (void)0
#		define MSG_RESULT (void)0
#	endif
#endif // _DEBUG



