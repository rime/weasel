#include "stdafx.h"
#include "WeaselServerImpl.h"
#include <Windows.h>
#include <boost/thread.hpp>

using namespace weasel;

extern CAppModule _Module;

typedef BOOL(STDAPICALLTYPE * PhysicalToLogicalPointForPerMonitorDPI_API)(HWND, LPPOINT);

ServerImpl::ServerImpl()
: m_pRequestHandler(NULL), m_hUser32Module(NULL)
{
	InitSecurityAttr();
	buffer = std::make_unique<char[]>(WEASEL_IPC_SHARED_MEMORY_SIZE);
}

ServerImpl::~ServerImpl()
{
	if (m_hUser32Module != NULL)
	{
		FreeLibrary(m_hUser32Module);
	}
	if (pipeThread != nullptr) {
		pipeThread->interrupt();
	}
	if (IsWindow())
	{
		DestroyWindow();
	}

	m_pRequestHandler->Finalize();
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

	m_hUser32Module = ::LoadLibrary(_T("user32.dll"));

	return (int)hwnd;
}

int ServerImpl::Stop()
{
	if (m_hUser32Module != NULL)
	{
		FreeLibrary(m_hUser32Module);
	}
	if (pipeThread != nullptr) {
		pipeThread->interrupt();
	}
	if (IsWindow())
	{
		DestroyWindow();
	}
	
	// quit the server
	::ExitProcess(0);
	return 0;
}



int ServerImpl::Run()
{
	pipeThread = std::make_unique<boost::thread>(boost::bind(&ServerImpl::ListenPipe, this));
	CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);
	int nRet = theLoop.Run();
	_Module.RemoveMessageLoop();
	return nRet;
}


DWORD ServerImpl::OnEcho(WEASEL_IPC_COMMAND uMsg, DWORD wParam, DWORD lParam, BOOL& hasResp)
{
	if (!m_pRequestHandler)
		return 0;
	return m_pRequestHandler->FindSession(lParam);
}

DWORD ServerImpl::OnStartSession(WEASEL_IPC_COMMAND uMsg, DWORD wParam, DWORD lParam, BOOL& hasResp)
{
	if (!m_pRequestHandler)
		return 0;
	return m_pRequestHandler->AddSession(reinterpret_cast<LPWSTR>(buffer.get()));
}

DWORD ServerImpl::OnEndSession(WEASEL_IPC_COMMAND uMsg, DWORD wParam, DWORD lParam, BOOL& hasResp)
{
	if (!m_pRequestHandler)
		return 0;
	return m_pRequestHandler->RemoveSession(lParam);
}

DWORD ServerImpl::OnKeyEvent(WEASEL_IPC_COMMAND uMsg, DWORD wParam, DWORD lParam, BOOL& hasResp)
{
	LPWSTR buffer_ptr = reinterpret_cast<LPWSTR>((char *)buffer.get() + sizeof(DWORD));
	if (!m_pRequestHandler/* || !m_pSharedMemory*/)
		return 0;
	hasResp = true;
	return m_pRequestHandler->ProcessKeyEvent(KeyEvent(wParam), lParam, buffer_ptr);
}

DWORD ServerImpl::OnShutdownServer(WEASEL_IPC_COMMAND uMsg, DWORD wParam, DWORD lParam, BOOL& hasResp)
{
	Stop();
	return 0;
}

DWORD ServerImpl::OnFocusIn(WEASEL_IPC_COMMAND uMsg, DWORD wParam, DWORD lParam, BOOL& hasResp)
{
	if (!m_pRequestHandler)
		return 0;
	m_pRequestHandler->FocusIn(wParam, lParam);
	return 0;
}

DWORD ServerImpl::OnFocusOut(WEASEL_IPC_COMMAND uMsg, DWORD wParam, DWORD lParam, BOOL& hasResp)
{
	if (!m_pRequestHandler)
		return 0;
	m_pRequestHandler->FocusOut(wParam, lParam);
	return 0;
}

DWORD ServerImpl::OnUpdateInputPosition(WEASEL_IPC_COMMAND uMsg, DWORD wParam, DWORD lParam, BOOL& hasResp)
{
	if (!m_pRequestHandler)
		return 0;
	/*
	 * 移位标志 = 1bit == 0
	 * height: 0~127 = 7bit
	 * top:-2048~2047 = 12bit（有符号）
	 * left:-2048~2047 = 12bit（有符号）
	 *
	 * 高解析度下：
	 * 移位标志 = 1bit == 1
	 * height: 0~254 = 7bit（舍弃低1位）
	 * top: -4096~4094 = 12bit（有符号，舍弃低1位）
	 * left: -4096~4094 = 12bit（有符号，舍弃低1位）
	 */
	RECT rc;
	int hi_res = (wParam >> 31) & 0x01;
	rc.left = ((wParam & 0x7ff) - (wParam & 0x800)) << hi_res;
	rc.top = (((wParam >> 12) & 0x7ff) - ((wParam >> 12) & 0x800)) << hi_res;
	const int width = 6;
	int height = ((wParam >> 24) & 0x7f) << hi_res;
	rc.right = rc.left + width;
	rc.bottom = rc.top + height;

	if (m_hUser32Module != NULL)
	{
		PhysicalToLogicalPointForPerMonitorDPI_API p2lPonit = (PhysicalToLogicalPointForPerMonitorDPI_API)::GetProcAddress(m_hUser32Module, "PhysicalToLogicalPointForPerMonitorDPI");
		if (p2lPonit)
		{
			POINT lt = { rc.left, rc.top };
			POINT rb = { rc.right, rc.bottom };
			p2lPonit(NULL, &lt);
			p2lPonit(NULL, &rb);
			rc = { lt.x, lt.y, rb.x, rb.y };
		}
	}

	m_pRequestHandler->UpdateInputPosition(rc, lParam);
	return 0;
}

DWORD ServerImpl::OnStartMaintenance(WEASEL_IPC_COMMAND uMsg, DWORD wParam, DWORD lParam, BOOL& hasResp)
{
	if (m_pRequestHandler)
		m_pRequestHandler->StartMaintenance();
	return 0;
}

DWORD ServerImpl::OnEndMaintenance(WEASEL_IPC_COMMAND uMsg, DWORD wParam, DWORD lParam, BOOL& hasResp)
{
	if (m_pRequestHandler)
		m_pRequestHandler->EndMaintenance();
	return 0;
}

DWORD ServerImpl::OnCommitComposition(WEASEL_IPC_COMMAND uMsg, DWORD wParam, DWORD lParam, BOOL& hasResp)
{
	if (m_pRequestHandler)
		m_pRequestHandler->CommitComposition(lParam);
	return 0;
}

DWORD ServerImpl::OnClearComposition(WEASEL_IPC_COMMAND uMsg, DWORD wParam, DWORD lParam, BOOL& hasResp)
{
	if (m_pRequestHandler)
		m_pRequestHandler->ClearComposition(lParam);
	return 0;
}

#define MAP_PIPE_MSG_HANDLE(__msg, __wParam, __lParam, __handled) {\
auto lParam = __lParam;\
auto wParam = __wParam;\
LRESULT _result;\
BOOL& _has_resp = __handled;\
switch (__msg) {\

#define PIPE_MSG_HANDLE(__msg, __func) \
case __msg:\
	_result = __func(__msg, wParam, lParam, _has_resp);\
	break;\

#define END_MAP_PIPE_MSG_HANDLE(__result) }__result = _result; }

void ServerImpl::HandlePipeMessage(HANDLE pipe)
{
	PipeMessage pipe_msg;
	LRESULT result = 0;
	DWORD bytes_read, bytes_written;
	DWORD write_len;

	for (;;) {
		auto success = ReadFile(pipe, (LPVOID)&pipe_msg, sizeof(PipeMessage), &bytes_read, NULL);
		if (!success)
		{
			if ((GetLastError()) != ERROR_MORE_DATA) {
				break;
			}
			memset(buffer.get(), 0, WEASEL_IPC_BUFFER_SIZE);
			success = ReadFile(pipe, buffer.get(), WEASEL_IPC_BUFFER_SIZE, &bytes_read, NULL);
			if (!success) {
				break;
			}
		}
		BOOL has_resp = false;
		MAP_PIPE_MSG_HANDLE(pipe_msg.Msg, pipe_msg.wParam, pipe_msg.lParam, has_resp)
			PIPE_MSG_HANDLE(WEASEL_IPC_ECHO, OnEcho)
			PIPE_MSG_HANDLE(WEASEL_IPC_START_SESSION, OnStartSession)
			PIPE_MSG_HANDLE(WEASEL_IPC_END_SESSION, OnEndSession)
			PIPE_MSG_HANDLE(WEASEL_IPC_PROCESS_KEY_EVENT, OnKeyEvent)
			PIPE_MSG_HANDLE(WEASEL_IPC_SHUTDOWN_SERVER, OnShutdownServer)
			PIPE_MSG_HANDLE(WEASEL_IPC_FOCUS_IN, OnFocusIn)
			PIPE_MSG_HANDLE(WEASEL_IPC_FOCUS_OUT, OnFocusOut)
			PIPE_MSG_HANDLE(WEASEL_IPC_UPDATE_INPUT_POS, OnUpdateInputPosition)
			PIPE_MSG_HANDLE(WEASEL_IPC_START_MAINTENANCE, OnStartMaintenance)
			PIPE_MSG_HANDLE(WEASEL_IPC_END_MAINTENANCE, OnEndMaintenance)
			PIPE_MSG_HANDLE(WEASEL_IPC_COMMIT_COMPOSITION, OnCommitComposition)
			PIPE_MSG_HANDLE(WEASEL_IPC_CLEAR_COMPOSITION, OnClearComposition);
		END_MAP_PIPE_MSG_HANDLE(result);

		// 这里本来是用 sizeof(LRESULT)，但是 64 位和 32 位的这个长度不统一。
		write_len = has_resp ? WEASEL_IPC_BUFFER_SIZE + sizeof(DWORD) : sizeof(DWORD);
		*(DWORD *)buffer.get() = result;
		if (!WriteFile(pipe, buffer.get(), write_len, &bytes_written, NULL)) {
			break;
		}
		FlushFileBuffers(pipe);
	}
	DisconnectNamedPipe(pipe);
	CloseHandle(pipe);
}

void ServerImpl::ListenPipe()
{
	DWORD err;
	for (;;) {
		HANDLE pipe = InitPipe();
		BOOL connected = ConnectNamedPipe(pipe, NULL);
		if (!connected) {
			err = GetLastError();
			CloseHandle(pipe);
			continue;
		}

		// 前端的消息是串行的，这里使用线程是为了消息循环不中断
		boost::thread pipe_t([pipe, this]() {
			HandlePipeMessage(pipe);
		});

		// 允许线程在此处中断
		boost::this_thread::interruption_point();
	}
}

HANDLE ServerImpl::InitPipe()
{
	std::wstring pipe_name = GetPipeName();
	HANDLE pipe = CreateNamedPipe(
		pipe_name.c_str(),
		PIPE_ACCESS_DUPLEX,
		PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
		PIPE_UNLIMITED_INSTANCES,
		WEASEL_IPC_SHARED_MEMORY_SIZE,
		WEASEL_IPC_SHARED_MEMORY_SIZE,
		0,
		&sa);

	return pipe;
}

// 为了在 winrt 应用中访问到 pipe 而进行权限设置

void weasel::ServerImpl::InitSecurityAttr()
{
	memset(&ea, 0, sizeof(ea));

	// 对一般 desktop APP 的权限设置

	SID_IDENTIFIER_AUTHORITY worldSidAuthority = SECURITY_WORLD_SID_AUTHORITY;
	AllocateAndInitializeSid(&worldSidAuthority, 1,
		SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &sid_everyone);

	ea[0].grfAccessPermissions = GENERIC_ALL;
	ea[0].grfAccessMode = SET_ACCESS;
	ea[0].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
	ea[0].Trustee.pMultipleTrustee = NULL;
	ea[0].Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
	ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
	ea[0].Trustee.ptstrName = (LPTSTR)sid_everyone;

	// 对 winrt (UWP) APP 的权限设置
	//
	// Application Package Authority.
	//

#define SECURITY_APP_PACKAGE_AUTHORITY              {0,0,0,0,0,15}
#define SECURITY_APP_PACKAGE_BASE_RID               (0x00000002L)
#define SECURITY_BUILTIN_APP_PACKAGE_RID_COUNT      (2L)
#define SECURITY_APP_PACKAGE_RID_COUNT              (8L)
#define SECURITY_CAPABILITY_BASE_RID                (0x00000003L)
#define SECURITY_BUILTIN_CAPABILITY_RID_COUNT       (2L)
#define SECURITY_CAPABILITY_RID_COUNT               (5L)
#define SECURITY_PARENT_PACKAGE_RID_COUNT           (SECURITY_APP_PACKAGE_RID_COUNT)
#define SECURITY_CHILD_PACKAGE_RID_COUNT            (12L)
#define SECURITY_BUILTIN_PACKAGE_ANY_PACKAGE        (0x00000001L)

	SID_IDENTIFIER_AUTHORITY appPackageAuthority = SECURITY_APP_PACKAGE_AUTHORITY;
	AllocateAndInitializeSid(&appPackageAuthority,
		SECURITY_BUILTIN_APP_PACKAGE_RID_COUNT,
		SECURITY_APP_PACKAGE_BASE_RID,
		SECURITY_BUILTIN_PACKAGE_ANY_PACKAGE,
		0, 0, 0, 0, 0, 0, &sid_all_apps);

	ea[1].grfAccessPermissions = GENERIC_ALL;
	ea[1].grfAccessMode = SET_ACCESS;
	ea[1].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
	ea[1].Trustee.pMultipleTrustee = NULL;
	ea[1].Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
	ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[1].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
	ea[1].Trustee.ptstrName = (LPTSTR)sid_all_apps;

	// create DACL
	DWORD err = SetEntriesInAcl(2, ea, NULL, &pacl);
	if (0 == err) {
		// security descriptor
		pd = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
		InitializeSecurityDescriptor(pd, SECURITY_DESCRIPTOR_REVISION);

		// Add the ACL to the security descriptor. 
		SetSecurityDescriptorDacl(pd, TRUE, pacl, FALSE);
	}

	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = pd;
	sa.bInheritHandle = TRUE;
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
