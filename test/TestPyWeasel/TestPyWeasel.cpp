// TestPyWeasel.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <boost/detail/lightweight_test.hpp>
#include <iostream>

#include <WeaselIPC.h>
#include <PyWeasel.h>

using namespace std;

const char* wcstomb(const wchar_t* wcs)
{
	const int buffer_len = 8192;
	static char buffer[buffer_len];
	WideCharToMultiByte(CP_OEMCP, NULL, wcs, -1, buffer, buffer_len, NULL, FALSE);
	return buffer;
}

void test_pyweasel()
{
	WCHAR buffer[WEASEL_IPC_BUFFER_LENGTH];

	weasel::RequestHandler* handler = new PyWeaselHandler;
	BOOST_ASSERT(handler);

	handler->Initialize();

	memset(buffer, 0, sizeof(buffer));
	// 成功创建会话，返回sessionID；失败返回0
	UINT sessionID = handler->AddSession(buffer);
	BOOST_ASSERT(sessionID);
	cout << wcstomb(buffer) << endl;

	// 会话存在返回sessionID，不存在返回0
	BOOST_ASSERT(sessionID == handler->FindSession(sessionID));

	memset(buffer, 0, sizeof(buffer));
	// 输入 a，ProcessKeyEvent应返回TRUE，并将回应串写入buffer
	BOOL taken = handler->ProcessKeyEvent(weasel::KeyEvent(L'a', 0), sessionID, buffer);
	BOOST_ASSERT(taken);
	// Windows控制台不能直接显示WideChar中文串，转为MultiByteString
	cout << wcstomb(buffer) << endl;
	
	// 成功销毁会话，返回sessionID；失败返回0
	BOOST_ASSERT(sessionID == handler->RemoveSession(sessionID));

	handler->Finalize();

	delete handler;
}

int _tmain(int argc, _TCHAR* argv[])
{
  test_pyweasel();
  
  system("pause");
  return boost::report_errors();
}

