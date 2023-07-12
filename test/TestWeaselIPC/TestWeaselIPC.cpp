// TestWeaselIPC.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <RimeWithWeasel.h>

#include <boost/interprocess/streams/bufferstream.hpp>
using namespace boost::interprocess;

#include <iostream>
#include <memory>

CAppModule _Module;

int console_main();
int client_main();
int server_main();

// usage: TestWeaselIPC.exe [/start | /stop | /console]

int _tmain(int argc, _TCHAR* argv[])
{
		return client_main();
	if (argc == 1)  // no args
	{
	}
	else if (argc > 1 && !wcscmp(L"/stop", argv[1]))
	{
		weasel::Client client;
		if (!client.Connect())
		{
			std::cerr << "server not running." << std::endl;
			return 0;
		}
		client.ShutdownServer();
		return 0;
	}
	else if (argc > 1 && !wcscmp(L"/console", argv[1]))
	{
		return console_main();
		return 0;
	}

	return -1;
}

bool launch_server()
{
	int ret = (int)ShellExecute( NULL, L"open", L"TestWeaselIPC.exe", L"/start", NULL, SW_NORMAL);
	if (ret <= 32)
	{
		std::cerr << "failed to launch server." << std::endl;
		return false;
	}
	return true;
}

bool read_buffer(LPWSTR buffer, UINT length, LPWSTR dest)
{
	wbufferstream bs(buffer, length);
	bs.read(dest, WEASEL_IPC_BUFFER_LENGTH);
	return bs.good();
}

const char* wcstomb(const wchar_t* wcs)
{
	const int buffer_len = 8192;
	static char buffer[buffer_len];
	WideCharToMultiByte(CP_OEMCP, NULL, wcs, -1, buffer, buffer_len, NULL, FALSE);
	return buffer;
}

int console_main()
{
	weasel::ipc::client client;
	client.start_session();
	if (!client.echo())
	{
		std::cerr << "failed to start session." << std::endl;
		return -3;
	}
	
	while (std::cin.good())
	{
		int ch = std::cin.get();
		if (!std::cin.good())
			break;
		bool eaten = client.process_key_event(weasel::ipc::key_event(ch, 0));
		std::cout << "server replies: " << eaten << std::endl;
		if (eaten)
		{
			WCHAR response[WEASEL_IPC_BUFFER_LENGTH];
			bool ret = client.get_response_data(std::bind<bool>(read_buffer, std::placeholders::_1, std::placeholders::_2, std::ref(response)));
			std::cout << "get response data: " << ret << std::endl;
			std::cout << "buffer reads: " << std::endl << wcstomb(response) << std::endl;
		}
	}

	client.end_session();
	
	return 0;
}

int client_main()
{
	//launch_server();
	// Sleep(1000);
	weasel::ipc::client client;
	client.start_session();
	if (!client.echo())
	{
		std::cerr << "failed to login." << std::endl;
		return -3;
	}
  for (auto i = 0; i < 3; ++i)
  {
    bool eaten = client.process_key_event(weasel::ipc::key_event(L'A', 0));
    std::cout << "server replies: " << eaten << std::endl;
    if (eaten)
    {
      WCHAR response[WEASEL_IPC_BUFFER_LENGTH];
      bool ret = client.get_response_data(std::bind<bool>(read_buffer, std::placeholders::_1, std::placeholders::_2, std::ref(response)));
      std::cout << "get response data: " << ret << std::endl;
      std::cout << "buffer reads: " << std::endl << wcstomb(response) << std::endl;
    }
  }
	client.end_session();

	system("pause");
	return 0;
}

class TestRequestHandler : public weasel::RequestHandler
{
public:
	TestRequestHandler() : m_counter(0)
	{
		std::cerr << "handler ctor." << std::endl;
	}
	virtual ~TestRequestHandler()
	{
		std::cerr << "handler dtor: " << m_counter << std::endl;
	}
	virtual UINT FindSession(UINT session_id)
	{
		std::cerr << "FindSession: " << session_id << std::endl;
		return (session_id <= m_counter ? session_id : 0);
	}
	virtual UINT AddSession(LPWSTR buffer)
	{
		std::cerr << "AddSession: " << m_counter + 1 << std::endl;
		return ++m_counter;
	}
	virtual UINT RemoveSession(UINT session_id)
	{
		std::cerr << "RemoveClient: " << session_id << std::endl;
		return 0;
	}
	virtual BOOL ProcessKeyEvent(weasel::KeyEvent keyEvent, UINT session_id, EatLine eat) {
		std::cerr << "ProcessKeyEvent: " << session_id 
			  << " keycode: " << keyEvent.keycode 
			  << " mask: " << keyEvent.mask 
			  << std::endl;
		eat(std::wstring(L"Greeting=Hello, 小狼毫.\n"));
		return TRUE;
	}
private:
	unsigned int m_counter;
};


