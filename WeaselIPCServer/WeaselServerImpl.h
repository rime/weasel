#pragma once
#include <WeaselIPC.h>
#include <boost/scoped_ptr.hpp>
#include <map>

namespace weasel
{

	class SharedMemory
	{
	public:
		SharedMemory();
		~SharedMemory();
		IPCMetadata* GetMetadata();
		LPWSTR GetBuffer();

	private:
		boost::shared_ptr<windows_shared_memory> m_pShm;
		boost::shared_ptr<mapped_region> m_pRegion;
	};

	typedef CWinTraits<WS_DISABLED, WS_EX_TRANSPARENT> ServerWinTraits;

	class ServerImpl :
		public CWindowImpl<ServerImpl, CWindow, ServerWinTraits>
	{
	public:
		DECLARE_WND_CLASS (WEASEL_IPC_WINDOW)

		BEGIN_MSG_MAP(WEASEL_IPC_WINDOW)
			MESSAGE_HANDLER(WM_CREATE, OnCreate)
			MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
			MESSAGE_HANDLER(WM_CLOSE, OnClose)
			MESSAGE_HANDLER(WM_COMMAND, OnCommand)
			MESSAGE_HANDLER(WEASEL_IPC_ECHO, OnEcho)
			MESSAGE_HANDLER(WEASEL_IPC_START_SESSION, OnStartSession)
			MESSAGE_HANDLER(WEASEL_IPC_END_SESSION, OnEndSession)
			MESSAGE_HANDLER(WEASEL_IPC_PROCESS_KEY_EVENT, OnKeyEvent)
			MESSAGE_HANDLER(WEASEL_IPC_SHUTDOWN_SERVER, OnShutdownServer)
			MESSAGE_HANDLER(WEASEL_IPC_FOCUS_IN, OnFocusIn)
			MESSAGE_HANDLER(WEASEL_IPC_FOCUS_OUT, OnFocusOut)
			MESSAGE_HANDLER(WEASEL_IPC_UPDATE_INPUT_POS, OnUpdateInputPosition)
			MESSAGE_HANDLER(WEASEL_IPC_START_MAINTENANCE, OnStartMaintenance)
			MESSAGE_HANDLER(WEASEL_IPC_END_MAINTENANCE, OnEndMaintenance)
		END_MSG_MAP()

		LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
		LRESULT OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
		LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
		LRESULT OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
		LRESULT OnEcho(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
		LRESULT OnStartSession(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
		LRESULT OnEndSession(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
		LRESULT OnKeyEvent(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
		LRESULT OnShutdownServer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
		LRESULT OnFocusIn(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
		LRESULT OnFocusOut(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
		LRESULT OnUpdateInputPosition(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
		LRESULT OnStartMaintenance(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
		LRESULT OnEndMaintenance(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

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
		RequestHandler *m_pRequestHandler;  // reference
		std::map<UINT, CommandHandler> m_MenuHandlers;
		boost::scoped_ptr<SharedMemory> m_pSharedMemory;
	};

}
