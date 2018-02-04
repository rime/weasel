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
		void _ConnectPipe(const wchar_t* pipeName);
		void _InitializeClientInfo();
		bool _WriteClientInfo();

		LRESULT _SendMessage(WEASEL_IPC_COMMAND Msg, DWORD wParam, DWORD lParam);

		bool _Connected() const { return pipe != INVALID_HANDLE_VALUE; }
		bool _Active() const { return _Connected() && session_id != 0; }

		inline WCHAR* _GetSendBuffer() const {
			return reinterpret_cast<WCHAR*>((char *)buffer.get() + sizeof(PipeMessage));
		}

	private:
		UINT session_id;
		HANDLE pipe;
		bool has_cnt;

		std::unique_ptr<char[]> buffer;
		std::wstring app_name;
		bool is_ime;
	};

}