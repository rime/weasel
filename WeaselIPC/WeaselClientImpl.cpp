#include "stdafx.h"
#include "WeaselClientImpl.h"
#include <StringAlgorithm.hpp>

using namespace boost::interprocess;
using namespace weasel;

ClientImpl::ClientImpl()
	: session_id(0),
	  serverWnd(NULL),
	  is_ime(false)
{
	_InitializeClientInfo();
}

ClientImpl::~ClientImpl()
{
	if (_Connected())
		Disconnect();
}

//http://stackoverflow.com/questions/557081/how-do-i-get-the-hmodule-for-the-currently-executing-code
HMODULE GetCurrentModule()
{ // NB: XP+ solution!
  HMODULE hModule = NULL;
  GetModuleHandleEx(
    GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
    (LPCTSTR)GetCurrentModule,
    &hModule);

  return hModule;
}

void ClientImpl::_InitializeClientInfo()
{
	// get app name
	WCHAR exe_path[MAX_PATH] = {0};
	GetModuleFileName(NULL, exe_path, MAX_PATH);
	std::wstring path = exe_path;
	size_t separator_pos = path.find_last_of(L"\\/");
	if (separator_pos < path.size())
		app_name = path.substr(separator_pos + 1);
	else
		app_name = path;
	to_lower(app_name);
	// determine client type
	GetModuleFileName(GetCurrentModule(), exe_path, MAX_PATH);
	path = exe_path;
	to_lower(path);
	is_ime = ends_with(path, L".ime");
}

bool ClientImpl::Connect(ServerLauncher const& launcher)
{
	serverWnd = _GetServerWindow(WEASEL_IPC_WINDOW);
	if (!serverWnd && launcher)
	{
		// 启动服务进程
		if (!launcher())
		{
			serverWnd = NULL;
			return false;
		}
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
		SendMessage(serverWnd, WEASEL_IPC_SHUTDOWN_SERVER, 0, 0);
	}
}

bool ClientImpl::ProcessKeyEvent(KeyEvent const& keyEvent)
{
	if (!_Active())
		return false;

	LRESULT ret = SendMessage(serverWnd, WEASEL_IPC_PROCESS_KEY_EVENT, keyEvent, session_id);
	return ret != 0;
}

bool ClientImpl::CommitComposition()
{
	if (!_Active())
		return false;

	LRESULT ret = SendMessage(serverWnd, WEASEL_IPC_COMMIT_COMPOSITION, 0, session_id);
	return ret != 0;
}

bool ClientImpl::ClearComposition()
{
	if (!_Active())
		return false;

	LRESULT ret = SendMessage(serverWnd, WEASEL_IPC_CLEAR_COMPOSITION, 0, session_id);
	return ret != 0;
}

void ClientImpl::UpdateInputPosition(RECT const& rc)
{
	if (!_Active())
		return;
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
	int hi_res = static_cast<int>(rc.bottom - rc.top >= 128 || 
		rc.left < -2048 || rc.left >= 2048 || rc.top < -2048 || rc.top >= 2048);
	int left = max(-2048, min(2047, rc.left >> hi_res));
	int top = max(-2048, min(2047, rc.top >> hi_res));
	int height = max(0, min(127, (rc.bottom - rc.top) >> hi_res));
	DWORD compressed_rect = ((hi_res & 0x01) << 31) | ((height & 0x7f) << 24) | 
		                    ((top & 0xfff) << 12) | (left & 0xfff);
	PostMessage(serverWnd, WEASEL_IPC_UPDATE_INPUT_POS, compressed_rect, session_id);
}

void ClientImpl::FocusIn()
{
	DWORD client_caps = 0;  /* TODO */
	PostMessage(serverWnd, WEASEL_IPC_FOCUS_IN, client_caps, session_id);
}

void ClientImpl::FocusOut()
{
	PostMessage(serverWnd, WEASEL_IPC_FOCUS_OUT, 0, session_id);
}

void ClientImpl::StartSession()
{
	if (!_Connected())
		return;

	if (_Active() && Echo())
		return;

	_WriteClientInfo();
	UINT ret = SendMessage(serverWnd, WEASEL_IPC_START_SESSION, 0, 0);
	session_id = ret;
}

void ClientImpl::EndSession()
{
	if (_Connected())
		PostMessage(serverWnd, WEASEL_IPC_END_SESSION, 0, session_id);
	session_id = 0;
}

void ClientImpl::StartMaintenance()
{
	if (_Connected())
		SendMessage(serverWnd, WEASEL_IPC_START_MAINTENANCE, 0, 0);
	session_id = 0;
}

void ClientImpl::EndMaintenance()
{
	if (_Connected())
		SendMessage(serverWnd, WEASEL_IPC_END_MAINTENANCE, 0, 0);
	session_id = 0;
}

bool ClientImpl::Echo()
{
	if (!_Active())
		return false;

	UINT serverEcho = SendMessage(serverWnd, WEASEL_IPC_ECHO, 0, session_id);
	return (serverEcho == session_id);
}

bool ClientImpl::GetResponseData(ResponseHandler const& handler)
{
	if (!handler)
	{
		return false;
	}
	try
	{
		windows_shared_memory shm(open_only, WEASEL_IPC_SHARED_MEMORY, read_only);
		mapped_region region(shm, read_only, WEASEL_IPC_METADATA_SIZE,
			WEASEL_IPC_SHARED_MEMORY_SIZE - WEASEL_IPC_METADATA_SIZE);
		return handler((LPWSTR)region.get_address(), WEASEL_IPC_BUFFER_LENGTH);
	}
	catch (interprocess_exception& /*ex*/)
	{
		return false;
	}

	return false;
}

bool ClientImpl::_WriteClientInfo()
{
	WCHAR* buffer = NULL;
	try
	{
		windows_shared_memory shm(open_only, WEASEL_IPC_SHARED_MEMORY, read_write);
		mapped_region region(shm, read_write, WEASEL_IPC_METADATA_SIZE,
			WEASEL_IPC_SHARED_MEMORY_SIZE - WEASEL_IPC_METADATA_SIZE);
		buffer = (LPWSTR)region.get_address();
		if (!buffer)
		{
			return false;
		}
		memset(buffer, 0, WEASEL_IPC_BUFFER_SIZE);
		wbufferstream bs(buffer, WEASEL_IPC_BUFFER_LENGTH);
		bs << L"action=session\n";
		bs << L"session.client_app=" << app_name.c_str() << L"\n";
		bs << L"session.client_type=" << (is_ime ? L"ime" : L"tsf") << L"\n";
		bs << L".\n";
		if (!bs.good())
		{
			// response text toooo long!
			return false;
		}
	}
	catch (interprocess_exception& /*ex*/)
	{
		return false;
	}

	return true;
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

bool Client::ProcessKeyEvent(KeyEvent const& keyEvent)
{
	return m_pImpl->ProcessKeyEvent(keyEvent);
}

bool Client::CommitComposition()
{
	return m_pImpl->CommitComposition();
}

bool Client::ClearComposition()
{
	return m_pImpl->ClearComposition();
}

void Client::UpdateInputPosition(RECT const& rc)
{
	m_pImpl->UpdateInputPosition(rc);
}

void Client::FocusIn()
{
	m_pImpl->FocusIn();
}

void Client::FocusOut()
{
	m_pImpl->FocusOut();
}

void Client::StartSession()
{
	m_pImpl->StartSession();
}

void Client::EndSession()
{
	m_pImpl->EndSession();
}

void Client::StartMaintenance()
{
	m_pImpl->StartMaintenance();
}

void Client::EndMaintenance()
{
	m_pImpl->EndMaintenance();
}

bool Client::Echo()
{
	return m_pImpl->Echo();
}

bool Client::GetResponseData(ResponseHandler handler)
{
	return m_pImpl->GetResponseData(handler);
}
