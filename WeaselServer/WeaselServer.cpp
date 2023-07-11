// WeaselServer.cpp : main source file for WeaselServer.exe
//
//	WTL MessageLoop 封装了消息循环. 实现了 getmessage/dispatchmessage....

#include "stdafx.h"
#include <thread>
#include <WinUser.h>
#include <VersionHelpers.h>
#include <weasel/version.h>
#include <weasel/log.h>
#include <weasel/util.h>
#include "Util.h"
#include "MessageDispatcher.h"

CAppModule _Module;

#define HAS_FLAG(x) (!wcscmp((x), lpstrCmdLine))

namespace
{
bool debugMode = false;

void quit_old_instance()
{
  weasel::ipc::client c(true);
  c.shutdown_server();
}

void parse_cmdline(LPTSTR lpstrCmdLine)
{
  if (HAS_FLAG(L"/debug"))
  {
    debugMode = true;
  }
  if (HAS_FLAG(L"/userdir"))
  {
    explore(weasel::utils::user_data_dir());
    ExitProcess(0);
  }
  if (HAS_FLAG(L"/weaseldir"))
  {
    explore(weasel::utils::install_dir());
    ExitProcess(0);
  }
  if (HAS_FLAG(L"/q") || HAS_FLAG(L"/quit"))
  {
    quit_old_instance();
    ExitProcess(0);
  }
  if (HAS_FLAG(L"/update"))
  {
    check_update();
  }
}

void print_weasel_version()
{
  LOG(info, "weasel " WEASEL_BUILD_STRING);
}

void check_os_version()
{
  if (!IsWindows8Point1OrGreater())
  {
    MessageBox(NULL, L"僅支持Windows 8.1或更高版本系統", L"系統版本過低", MB_ICONERROR);
    ExitProcess(-1);
  }
}

void check_user()
{
  constexpr size_t len = _countof(L"SYSTEM");
  WCHAR user[ MAX_PATH ] = { 0 };
  DWORD size = len;
  GetUserNameW(user, &size);
  if (!_wcsicmp(user, L"SYSTEM"))
  {
    ExitProcess(-1);
  }
}
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{
  // 防止服务进程开启输入法
  ImmDisableIME(-1);
  parse_cmdline(lpstrCmdLine);
#if defined(_DEBUG) || defined(DEBUG)
  debugMode = true;
#endif

  if (debugMode) {
    AllocConsole();
    weasel::log::init_console();
  }

  print_weasel_version();
  check_os_version();
  check_user();
  quit_old_instance();
  // ensure user data directory exists
  CreateDirectoryW(weasel::utils::user_data_dir().c_str(), NULL);

  HRESULT hRes = ::CoInitialize(NULL);
  ATLASSERT(SUCCEEDED(hRes));
  AtlInitCommonControls(ICC_BAR_CLASSES);	// add flags to support other controls
  hRes = _Module.Init(NULL, hInstance);
  ATLASSERT(SUCCEEDED(hRes));

  int nRet;
  try
  {
    RegisterApplicationRestart(NULL, 0);
    message_dispatcher dispatcher;
    if (nRet = dispatcher.init(); nRet != 0) throw std::exception("failed to init dispatcher");
    nRet = dispatcher.run();
  } catch (...)
  {
    // bad luck...
    nRet = -1;
  }

  _Module.Term();
  ::CoUninitialize();

  return nRet;
}
