#pragma once
#include <WeaselCommon.h>
#include <WeaselUtility.h>
#include <windows.h>
#include <functional>
#include <memory>

#define WEASEL_IPC_WINDOW L"WeaselIPCWindow_1.0"
#define WEASEL_IPC_PIPE_NAME L"WeaselNamedPipe"

#define WEASEL_IPC_METADATA_SIZE 1024
#define WEASEL_IPC_BUFFER_SIZE (4 * 1024)
#define WEASEL_IPC_BUFFER_LENGTH (WEASEL_IPC_BUFFER_SIZE / sizeof(WCHAR))
#define WEASEL_IPC_SHARED_MEMORY_SIZE \
  (sizeof(PipeMessage) + WEASEL_IPC_BUFFER_SIZE)

enum WEASEL_IPC_COMMAND {
  WEASEL_IPC_ECHO = (WM_APP + 1),
  WEASEL_IPC_START_SESSION,
  WEASEL_IPC_END_SESSION,
  WEASEL_IPC_PROCESS_KEY_EVENT,
  WEASEL_IPC_SHUTDOWN_SERVER,
  WEASEL_IPC_FOCUS_IN,
  WEASEL_IPC_FOCUS_OUT,
  WEASEL_IPC_UPDATE_INPUT_POS,
  WEASEL_IPC_START_MAINTENANCE,
  WEASEL_IPC_END_MAINTENANCE,
  WEASEL_IPC_COMMIT_COMPOSITION,
  WEASEL_IPC_CLEAR_COMPOSITION,
  WEASEL_IPC_TRAY_COMMAND,
  WEASEL_IPC_SELECT_CANDIDATE_ON_CURRENT_PAGE,
  WEASEL_IPC_HIGHLIGHT_CANDIDATE_ON_CURRENT_PAGE,
  WEASEL_IPC_CHANGE_PAGE,
  WEASEL_IPC_LAST_COMMAND
};

namespace weasel {
struct PipeMessage {
  WEASEL_IPC_COMMAND Msg;
  DWORD wParam;
  DWORD lParam;
};

struct IPCMetadata {
  enum { WINDOW_CLASS_LENGTH = 64 };
  UINT32 server_hwnd;
  WCHAR server_window_class[WINDOW_CLASS_LENGTH];
};

struct KeyEvent {
  UINT keycode : 16;
  UINT mask : 16;
  KeyEvent() : keycode(0), mask(0) {}
  KeyEvent(UINT _keycode, UINT _mask) : keycode(_keycode), mask(_mask) {}
  KeyEvent(UINT x) { *reinterpret_cast<UINT*>(this) = x; }
  operator UINT32 const() const {
    return *reinterpret_cast<UINT32 const*>(this);
  }
};

// 處理請求之物件
struct RequestHandler {
  using EatLine = std::function<bool(std::wstring&)>;
  RequestHandler() {}
  virtual ~RequestHandler() {}
  virtual void Initialize() {}
  virtual void Finalize() {}
  virtual DWORD FindSession(DWORD session_id) { return 0; }
  virtual DWORD AddSession(LPWSTR buffer, EatLine eat = 0) { return 0; }
  virtual DWORD RemoveSession(DWORD session_id) { return 0; }
  virtual BOOL ProcessKeyEvent(KeyEvent keyEvent,
                               DWORD session_id,
                               EatLine eat) {
    return FALSE;
  }
  virtual void CommitComposition(DWORD session_id) {}
  virtual void ClearComposition(DWORD session_id) {}
  virtual void SelectCandidateOnCurrentPage(size_t index, DWORD session_id) {}
  virtual bool HighlightCandidateOnCurrentPage(size_t index,
                                               DWORD session_id,
                                               EatLine eat) {
    return false;
  }
  virtual bool ChangePage(bool backward, DWORD session_id, EatLine eat) {
    return false;
  }
  virtual void FocusIn(DWORD param, DWORD session_id) {}
  virtual void FocusOut(DWORD param, DWORD session_id) {}
  virtual void UpdateInputPosition(RECT const& rc, DWORD session_id) {}
  virtual void StartMaintenance() {}
  virtual void EndMaintenance() {}
  virtual void SetOption(DWORD session_id, const std::string& opt, bool val) {}
  virtual void UpdateColorTheme(BOOL darkMode) {}
};

// 處理server端回應之物件
typedef std::function<bool(LPWSTR buffer, DWORD length)> ResponseHandler;

// 事件處理函數
typedef std::function<bool()> CommandHandler;

// 啟動服務進程之物件
typedef CommandHandler ServerLauncher;

// IPC實現類聲明

class ClientImpl;
class ServerImpl;

// IPC接口類

class Client {
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
  // 進入維護模式
  void StartMaintenance();
  // 退出維護模式
  void EndMaintenance();
  // 测试连接
  bool Echo();
  // 请求服务处理按键消息
  bool ProcessKeyEvent(KeyEvent const& keyEvent);
  // 上屏正在編輯的文字
  bool CommitComposition();
  // 清除正在編輯的文字
  bool ClearComposition();
  // 选择当前页面编号为index的候选
  bool SelectCandidateOnCurrentPage(size_t index);
  // 高亮当前页面编号为index的候选
  bool HighlightCandidateOnCurrentPage(size_t index);
  // 翻页，backward = true 向前翻，false向后翻
  bool ChangePage(bool backward);
  // 更新输入位置
  void UpdateInputPosition(RECT const& rc);
  // 输入窗口获得焦点
  void FocusIn();
  // 输入窗口失去焦点
  void FocusOut();
  // 托盤菜單
  void TrayCommand(UINT menuId);
  // 读取server返回的数据
  bool GetResponseData(ResponseHandler handler);

 private:
  ClientImpl* m_pImpl;
};

class Server {
 public:
  Server();
  virtual ~Server();

  // 初始化服务
  int Start();
  // 结束服务
  int Stop();
  // 消息循环
  int Run();

  void SetRequestHandler(RequestHandler* pHandler);
  void AddMenuHandler(UINT uID, CommandHandler handler);
  HWND GetHWnd();

 private:
  ServerImpl* m_pImpl;
};

inline std::wstring GetPipeName() {
  std::wstring pipe_name;
  pipe_name += L"\\\\.\\pipe\\";
  pipe_name += getUsername();
  pipe_name += L"\\";
  pipe_name += WEASEL_IPC_PIPE_NAME;
  return pipe_name;
}
}  // namespace weasel
