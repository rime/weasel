#include "stdafx.h"
#include "WeaselServerImpl.h"

using namespace weasel;

extern CAppModule _Module;


SharedMemory::SharedMemory()
{
	m_pShm.reset(new windows_shared_memory(create_only, 
										   WEASEL_IPC_SHARED_MEMORY, 
										   read_write, 
										   WEASEL_IPC_SHARED_MEMORY_SIZE));
	m_pRegion.reset(new mapped_region(*m_pShm, read_write));
}

SharedMemory::~SharedMemory()
{
}

IPCMetadata* SharedMemory::GetMetadata()
{
	return reinterpret_cast<IPCMetadata*>(m_pRegion->get_address());
}

LPWSTR SharedMemory::GetBuffer()
{
	return reinterpret_cast<LPWSTR>((char*)m_pRegion->get_address() + WEASEL_IPC_METADATA_SIZE);
}

ServerImpl::ServerImpl(RequestHandler* pHandler)
: m_pHandler(pHandler), m_pSharedMemory()
{
}

ServerImpl::~ServerImpl()
{
}

LRESULT ServerImpl::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	// not neccessary...
	::SetWindowText(m_hWnd, WEASEL_IPC_WINDOW); 

	// notify clients that we are ready serving
	HANDLE hEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, WEASEL_IPC_READY_EVENT);
	if (hEvent != INVALID_HANDLE_VALUE)
	{
		SetEvent(hEvent);
		CloseHandle(hEvent);
	}

	return 0;
}

LRESULT ServerImpl::OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	Stop();
	return 0;
}

LRESULT ServerImpl::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	bHandled = FALSE;
	return 1;
}

int ServerImpl::Start()
{
	// assure single instance
	if (FindWindow(WEASEL_IPC_WINDOW, NULL) != NULL)
	{
		return 0;
	}

	HWND hwnd = Create(NULL);
	try
	{
		m_pSharedMemory.reset(new SharedMemory());
		IPCMetadata *metadata = m_pSharedMemory->GetMetadata();
		if (metadata)
		{
			metadata->server_hwnd = reinterpret_cast<UINT32>(hwnd);
			wcsncpy(metadata->server_window_class, WEASEL_IPC_WINDOW, IPCMetadata::WINDOW_CLASS_LENGTH);
		}
		else
			return 0;
	}
	catch (interprocess_exception& /*ex*/)
	{
		m_pSharedMemory.reset();
		return 0;
	}

	if (m_pHandler)
		m_pHandler->Initialize();

	return (int)hwnd;
}

int ServerImpl::Stop()
{
	if (m_pHandler)
	{
		m_pHandler->Finalize();
		m_pHandler.reset();
	}
	if (m_pSharedMemory)
	{
		m_pSharedMemory.reset();
	}

	if (!IsWindow())
	{
		return 0;
	}
	DestroyWindow();
	//quit the server
	::PostQuitMessage(0);
	return 0;
}

int ServerImpl::Run()
{
	CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);
	
	int nRet = theLoop.Run();

	_Module.RemoveMessageLoop();
	return nRet;
}

LRESULT ServerImpl::OnEcho(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if (!m_pHandler)
		return 0;
	return m_pHandler->FindSession(lParam);
}

LRESULT ServerImpl::OnStartSession(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if (!m_pHandler)
		return 0;
	return m_pHandler->AddSession(m_pSharedMemory->GetBuffer());
}

LRESULT ServerImpl::OnEndSession(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if (!m_pHandler)
		return 0;
	return m_pHandler->RemoveSession(lParam);
}

LRESULT ServerImpl::OnKeyEvent(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if (!m_pHandler || !m_pSharedMemory)
		return 0;
	return m_pHandler->ProcessKeyEvent(KeyEvent(wParam), lParam, m_pSharedMemory->GetBuffer());
}

LRESULT ServerImpl::OnShutdownServer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	Stop();
	return 0;
}

// weasel::Server

Server::Server(RequestHandler* pHandler)
	: m_pImpl(new ServerImpl(pHandler))
{}

Server::~Server()
{
	if (m_pImpl)
		delete m_pImpl;
}

int Server::Start()
{
	return m_pImpl->Start();
}

int Server::Stop()
{
	return m_pImpl->Stop();
}

int Server::Run()
{
	return m_pImpl->Run();
}
