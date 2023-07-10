#pragma once
#include <WeaselIPC.h>
#include <PipeChannel.h>

#include "weasel/ipc.h"

namespace weasel
{

	class ClientImpl
	{
	public:
		ClientImpl();
		~ClientImpl();

		bool Connect(ServerLauncher const& launcher);
		void Disconnect();
		void ShutdownServer();
		void StartSession();
		void EndSession();
		void StartMaintenance();
		void EndMaintenance();
		bool Echo();
		bool ProcessKeyEvent(KeyEvent const& keyEvent);
		bool CommitComposition();
		bool ClearComposition();
		void UpdateInputPosition(RECT const& rc);
		void FocusIn();
		void FocusOut();
		void TrayCommand(UINT menuId);
		bool GetResponseData(ResponseHandler const& handler);

	protected:
		void _InitializeClientInfo();
		bool _WriteClientInfo();

		LRESULT _SendMessage(ipc::ipc_command Msg, DWORD wParam, DWORD lParam);

		bool _Connected() const { return true; }
		bool _Active() const { return session_id != 0; }

	private:
		UINT session_id;
		std::wstring app_name;
		bool is_ime;

	  ipc::pipe_client pipe;
		// PipeChannel channel;
	};

}
