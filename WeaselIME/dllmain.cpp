// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include "WeaselIME.h"

BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD ul_reason_for_call,
                      LPVOID lpReserved) {
  WeaselIME::SetModuleInstance(hModule);

  switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH: {
      HRESULT hr = WeaselIME::RegisterUIClass();
      if (FAILED(hr))
        return FALSE;
    } break;
    case DLL_THREAD_ATTACH:
      break;
    case DLL_THREAD_DETACH:
      break;
    case DLL_PROCESS_DETACH:
      WeaselIME::Cleanup();
      WeaselIME::UnregisterUIClass();
      break;
  }
  return TRUE;
}
