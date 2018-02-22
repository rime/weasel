#pragma once
#include <WeaselIPC.h>
#include <map>
#include <Winnt.h> // for security attributes constants
#include <aclapi.h> // for ACL
#include <boost/thread.hpp>
#include <PipeChannel.h>

#include "SecurityAttribute.h"

namespace weasel
{

	#define WEASEL_MSG_HANDLER(__name) DWORD __name (WEASEL_IPC_COMMAND, DWORD, LPARAM, BOOL&);

	class PipeServer : public PipeChannel<DWORD, PipeMessage>
	{
	public:
		using ServerRunner = std::function<void()>;
		using Respond = std::function<void(Msg)>;
		using ServerHandler = std::function<void(PipeMessage, Respond)>;

		PipeServer(std::wstring &pn_cmd, SECURITY_ATTRIBUTES *s);

	public:
		void Listen(ServerHandler const &handler);
		/* Get a server runner */
		ServerRunner GetServerRunner(ServerHandler const &handler);
	private:
		void _ProcessPipeThread(HANDLE pipe, ServerHandler const &handler);
	};

	typedef CWinTraits<WS_DISABLED, WS_EX_TRANSPARENT> ServerWinTraits;

	class ServerImpl :
		public CWindowImpl<ServerImpl, CWindow, ServerWinTraits>
	//class ServerImpl
	{
	public:
		DECLARE_WND_CLASS (WEASEL_IPC_WINDOW)

		BEGIN_MSG_MAP(WEASEL_IPC_WINDOW)
			MESSAGE_HANDLER(WM_CREATE, OnCreate)
			MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
			MESSAGE_HANDLER(WM_CLOSE, OnClose)
			MESSAGE_HANDLER(WM_QUERYENDSESSION, OnQueryEndSystemSession)
			MESSAGE_HANDLER(WM_ENDSESSION, OnEndSystemSession)
			MESSAGE_HANDLER(WM_COMMAND, OnCommand)
		END_MSG_MAP()

		LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
		LRESULT OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
		LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
		LRESULT OnQueryEndSystemSession(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
		LRESULT OnEndSystemSession(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
		LRESULT OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
		DWORD OnEcho(WEASEL_IPC_COMMAND uMsg, DWORD wParam, DWORD lParam);
		DWORD OnStartSession(WEASEL_IPC_COMMAND uMsg, DWORD wParam, DWORD lParam);
		DWORD OnEndSession(WEASEL_IPC_COMMAND uMsg, DWORD wParam, DWORD lParam);
		DWORD OnKeyEvent(WEASEL_IPC_COMMAND uMsg, DWORD wParam, DWORD lParam);
		DWORD OnShutdownServer(WEASEL_IPC_COMMAND uMsg, DWORD wParam, DWORD lParam);
		DWORD OnFocusIn(WEASEL_IPC_COMMAND uMsg, DWORD wParam, DWORD lParam);
		DWORD OnFocusOut(WEASEL_IPC_COMMAND uMsg, DWORD wParam, DWORD lParam);
		DWORD OnUpdateInputPosition(WEASEL_IPC_COMMAND uMsg, DWORD wParam, DWORD lParam);
		DWORD OnStartMaintenance(WEASEL_IPC_COMMAND uMsg, DWORD wParam, DWORD lParam);
		DWORD OnEndMaintenance(WEASEL_IPC_COMMAND uMsg, DWORD wParam, DWORD lParam);
		DWORD OnCommitComposition(WEASEL_IPC_COMMAND uMsg, DWORD wParam, DWORD lParam);
		DWORD OnClearComposition(WEASEL_IPC_COMMAND uMsg, DWORD wParam, DWORD lParam);

	public:
		ServerImpl();
		~ServerImpl();

		int Start();
		int Stop();
		int Run();

		void SetRequestHandler(RequestHandler* pHandler)
		{
			m_pRequestHandler = pHandler;
		}
		void AddMenuHandler(UINT uID, CommandHandler &handler)
		{
			m_MenuHandlers[uID] = handler;
		}

	private:
		void HandlePipeMessage(PipeMessage pipe_msg, PipeServer::Respond resp);

		std::unique_ptr<PipeServer> channel;
		std::unique_ptr<boost::thread> pipeThread;
		RequestHandler *m_pRequestHandler;  // reference
		std::map<UINT, CommandHandler> m_MenuHandlers;
		HMODULE m_hUser32Module;
		SecurityAttribute sa;
	};


}
