// TestWeaselIPC.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <WeaselIPC.h>

#include <boost/bind.hpp>
#include <boost/interprocess/streams/bufferstream.hpp>
using namespace boost::interprocess;

#include <iostream>
using namespace std;

CAppModule _Module;

int console_main();
int client_main();
int server_main();

// usage: TestWeaselIPC.exe [/start | /stop | /console]

int _tmain(int argc, _TCHAR* argv[])
{
	if (argc == 1)  // no args
	{
		return client_main();
	}
	else if(argc > 1 && !wcscmp(L"/start", argv[1]))
	{
		return server_main();
	}
	else if (argc > 1 && !wcscmp(L"/stop", argv[1]))
	{
		weasel::Client client;
		if (!client.Connect())
		{
			cerr << "server not running." << endl;
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
	int ret = (int)ShellExecute( NULL, L"open", L"TestWeaselIPC.exe", L"/start", NULL, SW_HIDE );
	if (ret <= 32)
	{
		cerr << "failed to launch server." << endl;
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
	weasel::Client client;
	if (!client.Connect())
	{
		cerr << "failed to connect to server." << endl;
		return -2;
	}
	client.StartSession();
	if (!client.Echo())
	{
		cerr << "failed to start session." << endl;
		return -3;
	}
	
	while (cin.good())
	{
		int ch = cin.get();
		if (!cin.good())
			break;
		bool eaten = client.ProcessKeyEvent(weasel::KeyEvent(ch, 0));
		cout << "server replies: " << eaten << endl;
		if (eaten)
		{
			WCHAR response[WEASEL_IPC_BUFFER_LENGTH];
			bool ret = client.GetResponseData(boost::bind<bool>(read_buffer, _1, _2, boost::ref(response)));
			cout << "get response data: " << ret << endl;
			cout << "buffer reads: " << endl << wcstomb(response) << endl;
		}
	}

	client.EndSession();
	
	return 0;
}

int client_main()
{
	weasel::Client client;
	if (!client.Connect(launch_server))
	{
		cerr << "failed to connect to server." << endl;
		return -2;
	}
	client.StartSession();
	if (!client.Echo())
	{
		cerr << "failed to login." << endl;
		return -3;
	}
	bool eaten = client.ProcessKeyEvent(weasel::KeyEvent(L'A', 0));
	cout << "server replies: " << eaten << endl;
	if (eaten)
	{
		WCHAR response[WEASEL_IPC_BUFFER_LENGTH];
		bool ret = client.GetResponseData(boost::bind<bool>(read_buffer, _1, _2, boost::ref(response)));
		cout << "get response data: " << ret << endl;
		cout << "buffer reads: " << endl << wcstomb(response) << endl;
	}
	client.EndSession();

	system("pause");
	return 0;
}

class TestRequestHandler : public weasel::RequestHandler
{
public:
	TestRequestHandler() : m_counter(0)
	{
		cerr << "handler ctor." << endl;
	}
	virtual ~TestRequestHandler()
	{
		cerr << "handler dtor: " << m_counter << endl;
	}
	virtual UINT FindSession(UINT sessionID)
	{
		cerr << "FindSession: " << sessionID << endl;
		return (sessionID <= m_counter ? sessionID : 0);
	}
	virtual UINT AddSession()
	{
		cerr << "AddSession: " << m_counter + 1 << endl;
		return ++m_counter;
	}
	virtual UINT RemoveSession(UINT sessionID)
	{
		cerr << "RemoveClient: " << sessionID << endl;
		return 0;
	}
	virtual BOOL ProcessKeyEvent(weasel::KeyEvent keyEvent, UINT sessionID, LPWSTR buffer) {
		cerr << "ProcessKeyEvent: " << sessionID 
			  << " keycode: " << keyEvent.keycode 
			  << " mask: " << keyEvent.mask 
			  << endl;
		wsprintf(buffer, L"Greeting=Hello, Ð¡ÀÇºÁ.\n");
		return TRUE;
	}
private:
	unsigned int m_counter;
};

int server_main()
{
	HRESULT hRes = _Module.Init(NULL, GetModuleHandle(NULL));
	ATLASSERT(SUCCEEDED(hRes));

	weasel::Server server(new TestRequestHandler());
	if (!server.Start())
		return -4;
	cerr << "server running." << endl;
	int ret = server.Run();
	cerr << "server quitting." << endl;
	return ret;
}
