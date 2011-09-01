#include "stdafx.h"
#include "WeaselClientImpl.h"

using namespace boost::interprocess;
using namespace weasel;

ClientImpl::ClientImpl()
	: sessionID(0),
	  serverWnd(NULL)
{
}

ClientImpl::~ClientImpl()
{
	if (_Connected())
		Disconnect();
}

bool ClientImpl::Connect(ServerLauncher const& launcher)
{
	serverWnd = _GetServerWindow(WEASEL_IPC_WINDOW);
	if ( !serverWnd && !launcher.empty() )
	{
		HANDLE hEvent = CreateEvent( NULL, TRUE, FALSE, WEASEL_IPC_READY_EVENT );
		// 启动服务进程
		if (!launcher())
		{
			CloseHandle(hEvent);
			serverWnd = NULL;
			return false;
		}
		WaitForSingleObject( hEvent, 2000 );
		CloseHandle(hEvent);
		serverWnd = _GetServerWindow(WEASEL_IPC_WINDOW);
	}
	return _Connected();
}

void ClientImpl::Disconnect()
{
	if (_Active())
		EndSession();
	serverWnd = NULL;
}

void ClientImpl::ShutdownServer()
{
	if (_Connected())
	{
		SendMessage( serverWnd, WEASEL_IPC_SHUTDOWN_SERVER, 0, 0 );
	}
}

bool ClientImpl::ProcessKeyEvent(KeyEvent keyEvent)
{
	if (!_Active())
		return false;

	LRESULT ret = SendMessage( serverWnd, WEASEL_IPC_PROCESS_KEY_EVENT, keyEvent, sessionID );
	return ret != 0;
}

void ClientImpl::StartSession()
{
	if (!_Connected())
		return;

	if (_Active())
		EndSession();

	UINT ret = SendMessage( serverWnd, WEASEL_IPC_START_SESSION, 0, 0 );
	sessionID = ret;
}

void ClientImpl::EndSession()
{
	if (_Connected())
		PostMessage( serverWnd, WEASEL_IPC_END_SESSION, 0, sessionID );
	sessionID = 0;
}

bool ClientImpl::Echo()
{
	if (!_Active())
		return false;

	UINT serverEcho = SendMessage( serverWnd, WEASEL_IPC_ECHO, 0, sessionID );
	return (serverEcho == sessionID);
}

bool ClientImpl::GetResponseData(ResponseHandler const& handler)
{
	if (handler.empty())
	{
		return false;
	}
	try
	{
		windows_shared_memory shm(open_only, WEASEL_IPC_SHARED_MEMORY, read_only);
		mapped_region region(shm, read_only, WEASEL_IPC_METADATA_SIZE);
		return handler((LPWSTR)region.get_address(), WEASEL_IPC_BUFFER_LENGTH);
	}
	catch (interprocess_exception& /*ex*/)
	{
		return false;
	}

	return false;
}

HWND ClientImpl::_GetServerWindow(LPCWSTR windowClass)
{
	if (!windowClass)
	{
		return NULL;
	}
	try
	{
		windows_shared_memory shm(open_only, WEASEL_IPC_SHARED_MEMORY, read_only);
		mapped_region region(shm, read_only, 0, WEASEL_IPC_METADATA_SIZE);
		IPCMetadata *metadata = reinterpret_cast<IPCMetadata*>(region.get_address());
		if (!metadata || wcsncmp(metadata->server_window_class, windowClass, IPCMetadata::WINDOW_CLASS_LENGTH))
		{
			return NULL;
		}
		return reinterpret_cast<HWND>(metadata->server_hwnd);
	}
	catch (interprocess_exception& /*ex*/)
	{
		return NULL;
	}
	return NULL;
}

// weasel::Client

Client::Client() 
	: m_pImpl(new ClientImpl())
{}

Client::~Client()
{
	if (m_pImpl)
		delete m_pImpl;
}

bool Client::Connect(ServerLauncher launcher)
{
	return m_pImpl->Connect(launcher);
}

void Client::Disconnect()
{
	m_pImpl->Disconnect();
}

void Client::ShutdownServer()
{
	m_pImpl->ShutdownServer();
}

bool Client::ProcessKeyEvent(KeyEvent keyEvent)
{
	return m_pImpl->ProcessKeyEvent(keyEvent);
}

void Client::StartSession()
{
	m_pImpl->StartSession();
}

void Client::EndSession()
{
	m_pImpl->EndSession();
}

bool Client::Echo()
{
	return m_pImpl->Echo();
}

bool Client::GetResponseData(ResponseHandler handler)
{
	return m_pImpl->GetResponseData(handler);
}
