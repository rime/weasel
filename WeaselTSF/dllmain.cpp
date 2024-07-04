// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include "Globals.h"
#include <stdio.h>
#include <Windows.h>
#include <DbgHelp.h>
#include <tchar.h>
#include <time.h>
#pragma comment(lib, "DbgHelp.lib")

LONG WINAPI _UnhandledExceptionFilter(EXCEPTION_POINTERS* pExceptionPointers) {
  TCHAR szFileName[MAX_PATH];
  TCHAR szAppName[MAX_PATH];
  TCHAR szDllName[MAX_PATH];
  DWORD dwProcessId = GetCurrentProcessId();
  SYSTEMTIME st;
  TCHAR szDateTime[20];  // 用于存储日期时间字符串
  // 获取当前系统时间
  GetLocalTime(&st);
  // 格式化日期时间字符串
  _stprintf_s(szDateTime, 20, _T("%04d%02d%02d-%02d%02d%02d"), st.wYear,
              st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
  // 获取应用程序名称
  GetModuleFileName(NULL, szAppName, MAX_PATH);
  // 从完整路径中提取应用程序名称
  TCHAR* pszBaseName = _tcsrchr(szAppName, '\\');
  if (pszBaseName) {
    pszBaseName++;  // 移过 '\\'
  } else {
    pszBaseName = szAppName;
  }
  // 获取当前dll模块的文件名
  GetModuleFileName(g_hInst, szDllName, MAX_PATH);
  TCHAR* pszDllBaseName = _tcsrchr(szDllName, '\\');
  if (pszDllBaseName) {
    pszDllBaseName++;  // 移过 '\\'
  } else {
    pszDllBaseName = szDllName;
  }
  WCHAR _path[MAX_PATH] = {0};
  // default location
  ExpandEnvironmentStringsW(L"%TEMP%\\rime.weasel", _path, _countof(_path));
  // 构造 dump 文件名：应用程序名称-DLL名称-时间.进程号.dmp
  _stprintf_s(szFileName, MAX_PATH, _T("%s\\%s-%s-%s.%lu.dmp"), _path,
              pszBaseName, pszDllBaseName, szDateTime, dwProcessId);
  HANDLE hDumpFile = CreateFile(szFileName, GENERIC_WRITE, 0, NULL,
                                CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (hDumpFile != INVALID_HANDLE_VALUE) {
    MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
    dumpInfo.ExceptionPointers = pExceptionPointers;
    dumpInfo.ThreadId = GetCurrentThreadId();
    dumpInfo.ClientPointers = TRUE;
    MiniDumpWriteDump(GetCurrentProcess(), dwProcessId, hDumpFile,
                      MiniDumpNormal, &dumpInfo, NULL, NULL);
    CloseHandle(hDumpFile);
  }
  return EXCEPTION_EXECUTE_HANDLER;
}

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID pvReserved) {
  switch (dwReason) {
    case DLL_PROCESS_ATTACH:
      g_hInst = hInstance;
      SetUnhandledExceptionFilter(_UnhandledExceptionFilter);
      if (!InitializeCriticalSectionAndSpinCount(&g_cs, 0))
        return FALSE;
      break;
    case DLL_PROCESS_DETACH:
      DeleteCriticalSection(&g_cs);
      break;
  }
  return TRUE;
}
