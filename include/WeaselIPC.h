#pragma once
#include <WeaselCommon.h>
#include <windows.h>
#include <boost/function.hpp>

#define WEASEL_IPC_WINDOW L"WeaselIPCWindow_0.9"
#define WEASEL_IPC_READY_EVENT L"Local\\WeaselIPCReadyEvent_0.9"
#define WEASEL_IPC_SHARED_MEMORY "WeaselIPCSharedMemory_0.9"

#define WEASEL_IPC_METADATA_SIZE 1024
#define WEASEL_IPC_BUFFER_SIZE (4 * 1024)
#define WEASEL_IPC_BUFFER_LENGTH (WEASEL_IPC_BUFFER_SIZE / sizeof(WCHAR))
#define WEASEL_IPC_SHARED_MEMORY_SIZE (WEASEL_IPC_METADATA_SIZE + WEASEL_IPC_BUFFER_SIZE)

enum WEASEL_IPC_COMMAND
{	
	WEASEL_IPC_ECHO = (WM_APP + 1),
	WEASEL_IPC_START_SESSION,
	WEASEL_IPC_END_SESSION,
	WEASEL_IPC_PROCESS_KEY_EVENT,
	WEASEL_IPC_SHUTDOWN_SERVER,
	WEASEL_IPC_FOCUS_IN,
	WEASEL_IPC_FOCUS_OUT,
	WEASEL_IPC_UPDATE_INPUT_POS,
};

namespace weasel
{

	struct IPCMetadata
	{
		enum { WINDOW_CLASS_LENGTH = 64 };
		UINT32 server_hwnd;
		WCHAR server_window_class[WINDOW_CLASS_LENGTH];
	};

	struct KeyEvent
	{
		UINT keycode : 16;
		UINT mask : 16;
		KeyEvent() : keycode(0), mask(0) {}
		KeyEvent(UINT _keycode, UINT _mask) : keycode(_keycode), mask(_mask) {}
		KeyEvent(UINT x)
		{
			*reinterpret_cast<UINT*>(this) = x;
		}
		operator UINT32 const() const
		{
			return *reinterpret_cast<UINT32 const*>(this);
		}
	};

	// 處理請求之物件
	struct RequestHandler
	{
		RequestHandler() {}
		virtual ~RequestHandler() {}
		virtual void Initialize() {}
		virtual void Finalize() {}
		virtual UINT FindSession(UINT session_id) { return 0; }
		virtual UINT AddSession(LPWSTR buffer) { return 0; }
		virtual UINT RemoveSession(UINT session_id) { return 0; }
		virtual BOOL ProcessKeyEvent(KeyEvent keyEvent, UINT session_id, LPWSTR buffer) { return FALSE; }
		virtual void FocusIn(UINT session_id) {}
		virtual void FocusOut(UINT session_id) {}
		virtual void UpdateInputPosition(RECT const& rc, UINT session_id) {}
	};
	
	// 處理server端回應之物件
	typedef boost::function<bool (LPWSTR buffer, UINT length)> ResponseHandler;
	
	// 啟動服務進程之物件
	typedef boost::function<bool ()> ServerLauncher;

	// IPC實現類聲明

	class ClientImpl;
	class ServerImpl;

	// IPC接口類

	class Client
	{
	public:

		Client();
		virtual ~Client();
		
		// 连接到服务，必要时启动服务进程
		bool Connect(ServerLauncher launcher = 0);
		// 断开连接
		void Disconnect();
		// 终止服务
		void ShutdownServer();
		// 發起會話
		void StartSession();
		// 結束會話
		void EndSession();
		// 测试连接
		bool Echo();
		// 请求服务处理按键消息
		bool ProcessKeyEvent(KeyEvent const& keyEvent);
		// 更新输入位置
		void UpdateInputPosition(RECT const& rc);
		// 输入窗口获得焦点
		void FocusIn();
		// 输入窗口失去焦点
		void FocusOut();
		// 读取server返回的数据
		bool GetResponseData(ResponseHandler handler);

	private:
		ClientImpl* m_pImpl;
	};

	class Server
	{
	public:
		Server(RequestHandler* pHandler = 0);
		virtual ~Server();

		// 初始化服务
		int Start();
		// 结束服务
		int Stop();
		// 消息循环
		int Run();

	private:
		ServerImpl* m_pImpl;
	};

}