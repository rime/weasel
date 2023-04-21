#pragma once
#include <msctf.h>
#include <RimeWithWeasel.h>
#include <Consts.hpp>

class RimeWithWeaselHandler;

class KeyboardManager {
   public:
    KeyboardManager(RimeWithWeaselHandler *handler);
    ~KeyboardManager();

    void StartHook();
    void StopHook();

    static const int SUPPRESS_FLAG = 0x233;

   private:
    static LRESULT CALLBACK _HookProc(int nCode, WPARAM wParam, LPARAM lParam);

    static bool SetAsciiMode(bool ascii_mode);

    static HHOOK m_hook_handle;

    static RimeWithWeaselHandler *pHandler;
};