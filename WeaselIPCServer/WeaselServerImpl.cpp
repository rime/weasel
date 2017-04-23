﻿#include "stdafx.h"
#include "WeaselServerImpl.h"
#include <Windows.h>
#include <ShellScalingApi.h>

using namespace weasel;

extern CAppModule _Module;


SharedMemory::SharedMemory()
{
	boost::interprocess::permissions perms;
	perms.set_unrestricted();
	m_pShm.reset(new windows_shared_memory(create_only, 
										   WEASEL_IPC_SHARED_MEMORY, 
										   read_write, 
										   WEASEL_IPC_SHARED_MEMORY_SIZE,
										   perms));
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

ServerImpl::ServerImpl()
: m_pRequestHandler(NULL), m_pSharedMemory()
{
}

ServerImpl::~ServerImpl()
{
}

LRESULT ServerImpl::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	// not neccessary...
	::SetWindowText(m_hWnd, WEASEL_IPC_WINDOW); 
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

LRESULT ServerImpl::OnQueryEndSystemSession(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	return TRUE;
}

LRESULT ServerImpl::OnEndSystemSession(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if (m_pRequestHandler)
    {
        m_pRequestHandler->Finalize();
    }
	return 0;
}

LRESULT ServerImpl::OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	UINT uID = LOWORD(wParam);
	std::map<UINT, CommandHandler>::iterator it = m_MenuHandlers.find(uID);
	if (it == m_MenuHandlers.end())
	{
		bHandled = FALSE;
		return 0;
	}
	it->second();  // execute command
	return 0;
}

extern "C" BOOL ( STDAPICALLTYPE *pChangeWindowMessageFilter )( UINT,DWORD ) = NULL;

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

	// 使用「消息免疫過濾」繞過IE9的用戶界面特權隔離機制
	HMODULE hMod = 0;
	if ( ( hMod = ::LoadLibrary( _T( "user32.dll" ) ) ) != 0 )
	{
		pChangeWindowMessageFilter = (BOOL (__stdcall *)( UINT,DWORD ) )::GetProcAddress( hMod, "ChangeWindowMessageFilter" );
		if ( pChangeWindowMessageFilter )
		{
			for (UINT cmd = WEASEL_IPC_ECHO; cmd < WEASEL_IPC_LAST_COMMAND; ++cmd)
			{
				pChangeWindowMessageFilter(cmd, MSGFLT_ADD);
			}
		}
		FreeLibrary(hMod);
	}

	return (int)hwnd;
}

int ServerImpl::Stop()
{
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
	if (!m_pRequestHandler)
		return 0;
	return m_pRequestHandler->FindSession(lParam);
}

LRESULT ServerImpl::OnStartSession(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if (!m_pRequestHandler)
		return 0;
	return m_pRequestHandler->AddSession(m_pSharedMemory->GetBuffer());
}

LRESULT ServerImpl::OnEndSession(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if (!m_pRequestHandler)
		return 0;
	return m_pRequestHandler->RemoveSession(lParam);
}

LRESULT ServerImpl::OnKeyEvent(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if (!m_pRequestHandler || !m_pSharedMemory)
		return 0;
	return m_pRequestHandler->ProcessKeyEvent(KeyEvent(wParam), lParam, m_pSharedMemory->GetBuffer());
}

LRESULT ServerImpl::OnShutdownServer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	Stop();
	return 0;
}

LRESULT ServerImpl::OnFocusIn(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if (!m_pRequestHandler)
		return 0;
	m_pRequestHandler->FocusIn(wParam, lParam);
	return 0;
}

LRESULT ServerImpl::OnFocusOut(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if (!m_pRequestHandler)
		return 0;
	m_pRequestHandler->FocusOut(wParam, lParam);
	return 0;
}

LRESULT ServerImpl::OnUpdateInputPosition(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if (!m_pRequestHandler)
		return 0;
	/*
	移位标志 = 1bit == 0
	height:0~127 = 7bit
	top:-2048~2047 = 12bit（有符号）
	left:-2048~2047 = 12bit（有符号）

	高解析度下：
	移位标志 = 1bit == 1
	height:0~254 = 7bit（舍弃低1位）
	top:-4096~4094 = 12bit（有符号，舍弃低1位）
	left:-4096~4094 = 12bit（有符号，舍弃低1位）
	*/
	RECT rc;
	int hi_res = (wParam >> 31) & 0x01;
	rc.left = ((wParam & 0x7ff) - (wParam & 0x800)) << hi_res;
	rc.top = (((wParam >> 12) & 0x7ff) - ((wParam >> 12) & 0x800)) << hi_res;
	const int width = 6;
	int height = ((wParam >> 24) & 0x7f) << hi_res;
	rc.right = rc.left + width;
	rc.bottom = rc.top + height;

	POINT lt = { rc.left, rc.top };
	POINT rb = { rc.right, rc.bottom };
	PhysicalToLogicalPointForPerMonitorDPI(NULL, &lt);
	PhysicalToLogicalPointForPerMonitorDPI(NULL, &rb);

	m_pRequestHandler->UpdateInputPosition({ lt.x, lt.y, rb.x, rb.y }, lParam);
	return 0;
}

LRESULT ServerImpl::OnStartMaintenance(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if (m_pRequestHandler)
		m_pRequestHandler->StartMaintenance();
	return 0;
}

LRESULT ServerImpl::OnEndMaintenance(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if (m_pRequestHandler)
		m_pRequestHandler->EndMaintenance();
	return 0;
}

LRESULT ServerImpl::OnCommitComposition(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if (m_pRequestHandler)
		m_pRequestHandler->CommitComposition(lParam);
	return 0;
}

LRESULT ServerImpl::OnClearComposition(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if (m_pRequestHandler)
		m_pRequestHandler->ClearComposition(lParam);
	return 0;
}

// weasel::Server

Server::Server()
	: m_pImpl(new ServerImpl)
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

void Server::SetRequestHandler(RequestHandler* pHandler)
{
	m_pImpl->SetRequestHandler(pHandler);
}

void Server::AddMenuHandler(UINT uID, CommandHandler handler)
{
	m_pImpl->AddMenuHandler(uID, handler);
}

HWND Server::GetHWnd()
{
	return m_pImpl->m_hWnd;
}
