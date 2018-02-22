#pragma once
#include <WeaselIPC.h>
#include <PipeChannel.h>

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
		void _InitializeClientInfo();
		bool _WriteClientInfo();

		LRESULT _SendMessage(WEASEL_IPC_COMMAND Msg, DWORD wParam, DWORD lParam);

		bool _Connected() const { return channel.Connected(); }
		bool _Active() const { return channel.Connected() && session_id != 0; }

		//inline WCHAR* _GetSendBuffer() const {
		//	return reinterpret_cast<WCHAR *>(channel.GetBuffer());
		//}

	private:
		UINT session_id;
		//HANDLE pipe;
		//bool has_cnt;

		//std::unique_ptr<char[]> buffer;
		std::wstring app_name;
		bool is_ime;

		PipeChannel<PipeMessage> channel;
	};

}