#include "stdafx.h"
#include "WeaselService.h"
#include "WeaselServerApp.h"
#include <boost/thread.hpp>

WeaselService* WeaselService::_service = NULL;

WeaselService::WeaselService(BOOL fCanStop = TRUE,
                             BOOL fCanShutdown = TRUE,
                             BOOL fCanPauseContinue = FALSE) {
  _statusHandle = NULL;

  // The service runs in its own process.
  _status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;

  // The service is starting.
  _status.dwCurrentState = SERVICE_START_PENDING;

  // The accepted commands of the service.
  DWORD dwControlsAccepted = 0;
  if (fCanStop)
    dwControlsAccepted |= SERVICE_ACCEPT_STOP;
  if (fCanShutdown)
    dwControlsAccepted |= SERVICE_ACCEPT_SHUTDOWN;
  if (fCanPauseContinue)
    dwControlsAccepted |= SERVICE_ACCEPT_PAUSE_CONTINUE;
  _status.dwControlsAccepted = dwControlsAccepted;

  _status.dwWin32ExitCode = NO_ERROR;
  _status.dwServiceSpecificExitCode = 0;
  _status.dwCheckPoint = 0;
  _status.dwWaitHint = 0;

  _stopping = FALSE;

  // Create a manual-reset event that is not signaled at first to indicate
  // the stopped signal of the service.
  _stoppedEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
  if (_stoppedEvent == NULL) {
    throw GetLastError();
  }
}

WeaselService::~WeaselService() {
  if (_stoppedEvent != NULL) {
    CloseHandle(_stoppedEvent);
  }
}

BOOL WeaselService::Run(WeaselService& serv) {
  _service = &serv;
  SERVICE_TABLE_ENTRY serviceTable[] = {{WEASEL_SERVICE_NAME, ServiceMain},
                                        {NULL, NULL}};

  // Connects the main thread of a service process to the service control
  // manager, which causes the thread to be the service control dispatcher
  // thread for the calling process. This call returns when the service has
  // stopped. The process should simply terminate when the call returns.
  return StartServiceCtrlDispatcher(serviceTable);
}

void WeaselService::Start(DWORD dwArgc = 0, PWSTR* pszArgv = NULL) {
  try {
    // Tell SCM that the service is starting.
    SetServiceStatus(SERVICE_START_PENDING);

    // Perform service-specific initialization.
    // if (IsWindowsVistaOrGreater())
    { RegisterApplicationRestart(NULL, 0); }
    boost::thread{[this] { app.Run(); }};
    // Tell SCM that the service is started.
    SetServiceStatus(SERVICE_RUNNING);

  } catch (DWORD dwError) {
    // Set the service status to be stopped.
    SetServiceStatus(SERVICE_STOPPED, dwError);
  } catch (...) {
    // Set the service status to be stopped.
    SetServiceStatus(SERVICE_STOPPED);
  }
}

void WeaselService::Stop() {
  DWORD dwOriginalState = _status.dwCurrentState;
  try {
    // Tell SCM that the service is stopping.
    SetServiceStatus(SERVICE_STOP_PENDING);

    // Perform service-specific stop operations.
    weasel::Client client;
    if (client.Connect())  // try to connect to running server
    {
      client.ShutdownServer();
    }

    // Tell SCM that the service is stopped.
    SetServiceStatus(SERVICE_STOPPED);
  } catch (DWORD /* dwError */) {
    // Set the orginal service status.
    SetServiceStatus(dwOriginalState);
  } catch (...) {
    // Set the orginal service status.
    SetServiceStatus(dwOriginalState);
  }
}

void WeaselService::ServiceMain(DWORD dwArgc, PWSTR* pszArgv) {
  _service->_statusHandle =
      RegisterServiceCtrlHandler(WEASEL_SERVICE_NAME, ServiceCtrlHandler);
  if (_service->_statusHandle == NULL) {
    throw GetLastError();
  }

  // Start the service.
  _service->Start(dwArgc, pszArgv);
}

void WeaselService::ServiceCtrlHandler(DWORD dwCtrl) {
  switch (dwCtrl) {
    case SERVICE_CONTROL_STOP:
      _service->Stop();
      break;
    case SERVICE_CONTROL_SHUTDOWN:
      _service->Shutdown();
      break;
    case SERVICE_CONTROL_INTERROGATE:
      break;
    default:
      break;
  }
}

void WeaselService::SetServiceStatus(DWORD dwCurrentState,
                                     DWORD dwWin32ExitCode,
                                     DWORD dwWaitHint) {
  static DWORD dwCheckPoint = 1;

  // Fill in the SERVICE_STATUS structure of the service.

  _status.dwCurrentState = dwCurrentState;
  _status.dwWin32ExitCode = dwWin32ExitCode;
  _status.dwWaitHint = dwWaitHint;

  _status.dwCheckPoint = ((dwCurrentState == SERVICE_RUNNING) ||
                          (dwCurrentState == SERVICE_STOPPED))
                             ? 0
                             : dwCheckPoint++;

  // Report the status of the service to the SCM.
  ::SetServiceStatus(_statusHandle, &_status);
}

void WeaselService::Shutdown() {
  SetServiceStatus(SERVICE_STOPPED);
}
