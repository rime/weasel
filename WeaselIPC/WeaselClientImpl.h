#pragma once
#include <WeaselIPC.h>

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
		bool GetResponseData(ResponseHandler const& handler);

	protected:
		HWND _GetServerWindow(LPCWSTR windowClass);
		void _InitializeClientInfo();
		bool _WriteClientInfo();

		bool _Connected() const { return serverWnd != NULL; }
		bool _Active() const { return _Connected() && session_id != 0; }

	private:
		UINT session_id;
		HWND serverWnd;
		std::wstring app_name;
		bool is_ime;
	};

}