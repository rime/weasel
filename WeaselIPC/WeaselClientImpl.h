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
		bool ProcessKeyEvent(KeyEvent keyEvent);
		void StartSession();
		void EndSession();
		bool Echo();
		bool GetResponseData(ResponseHandler const& handler);

	protected:
		HWND _GetServerWindow(LPCWSTR windowClass);
		bool _Connected() const { return serverWnd != NULL; }
		bool _Active() const { return _Connected() && sessionID != 0; }

	private:
		UINT sessionID;
		HWND serverWnd;
	};

}