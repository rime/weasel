#pragma once

#include <Windows.h>
#include <memory>

#include "WeaselServerApp.h"

#define WEASEL_SERVICE_NAME L"WeaselInputService"

class WeaselService {
 public:
  WeaselService(BOOL fCanStop, BOOL fCanShutdown, BOOL fCanPauseContinue);
  ~WeaselService();
  static BOOL Run(WeaselService& serv);

  void Stop();
  // Start the service.
  void Start(DWORD dwArgc, PWSTR* pszArgv);

 protected:
  // Entry point for the service. It registers the handler function for the
  // service and starts the service.
  static void WINAPI ServiceMain(DWORD dwArgc, PWSTR* pszArgv);

  // The function is called by the SCM whenever a control code is sent to
  // the service.
  static void WINAPI ServiceCtrlHandler(DWORD dwCtrl);

  void SetServiceStatus(DWORD dwCurrentState,
                        DWORD dwWin32ExitCode = NO_ERROR,
                        DWORD dwWaitHint = 0);

  // Execute when the system is shutting down.
  void Shutdown();

 private:
  static WeaselService* _service;

  // The status of the service
  SERVICE_STATUS _status;

  // The service status handle
  SERVICE_STATUS_HANDLE _statusHandle;

  BOOL _stopping;
  HANDLE _stoppedEvent;

  WeaselServerApp app;
};
