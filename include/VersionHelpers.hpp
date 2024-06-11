#pragma once

#ifndef NTSTATUS
typedef __success(return >= 0) LONG NTSTATUS;
#endif

// Visual Studio comes with Windows SDK 10, VersionHelpers.h came with Windows 8.1 SDK, while windows sdk 8.1 came with VS2013 _MSC_VER=1800
#include <VersionHelpers.h>

inline BOOL GetVersionEx2(LPOSVERSIONINFOW lpVersionInformation)
{
	HMODULE hNtDll = GetModuleHandleW(L"NTDLL"); // 获取ntdll.dll的句柄
	typedef NTSTATUS(NTAPI* tRtlGetVersion)(PRTL_OSVERSIONINFOW povi); // RtlGetVersion的原型
	tRtlGetVersion pRtlGetVersion = NULL;
	if (hNtDll)
	{
		pRtlGetVersion = (tRtlGetVersion)GetProcAddress(hNtDll, "RtlGetVersion"); // 获取RtlGetVersion地址
	}
	if (pRtlGetVersion)
	{
		return pRtlGetVersion((PRTL_OSVERSIONINFOW)lpVersionInformation) >= 0; // 调用RtlGetVersion
	}
	return FALSE;
}

static inline BOOL IsWinVersionGreaterThan(DWORD dwMajorVersion, DWORD dwMinorVersion)
{
	OSVERSIONINFOEXW ovi = { sizeof ovi };
	GetVersionEx2((LPOSVERSIONINFOW)&ovi);
	if ((ovi.dwMajorVersion == dwMajorVersion && ovi.dwMinorVersion >= dwMinorVersion) || ovi.dwMajorVersion > dwMajorVersion)
		return true;
	else
		return false;
}

static inline BOOL IsBlurAvailable()
{
	OSVERSIONINFOEXW ovi = { sizeof ovi };
	GetVersionEx2((LPOSVERSIONINFOW)&ovi);
	return (ovi.dwMajorVersion >= 10 && ovi.dwBuildNumber <= 22000);
}

// Use WinBlue for Windows 8.1
#define IsWindowsBlueOrLaterEx() IsWinVersionGreaterThan(6, 3)
#define IsWindows10OrGreaterEx() IsWinVersionGreaterThan(10, 0)
