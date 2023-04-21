#include "stdafx.h"
#include <KeyboardManager.h>
#include <logging.h>

HHOOK KeyboardManager::m_hook_handle;
RimeWithWeaselHandler *KeyboardManager::pHandler;

KeyboardManager::KeyboardManager(RimeWithWeaselHandler *handler) { pHandler = handler; }

KeyboardManager::~KeyboardManager() { StopHook(); }

void KeyboardManager::StartHook() {
    if (!m_hook_handle) {
        m_hook_handle = SetWindowsHookEx(WH_KEYBOARD_LL, _HookProc, GetModuleHandle(NULL), NULL);
        if (!m_hook_handle) {
            DWORD errorCode = GetLastError();
            LOG(ERROR) << "StartLowlevelKeyboardHook failed, error code: " << errorCode;
        }
    }
}

void KeyboardManager::StopHook() {
    if (m_hook_handle) {
        UnhookWindowsHookEx(m_hook_handle);
        m_hook_handle = nullptr;
    }
}

LRESULT CALLBACK KeyboardManager::_HookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT *kb = reinterpret_cast<KBDLLHOOKSTRUCT *>(lParam);
        if (kb->dwExtraInfo == SUPPRESS_FLAG) {
            return CallNextHookEx(m_hook_handle, nCode, wParam, lParam);
        }

        if (kb->vkCode == VK_CAPITAL) {
            if (wParam == WM_KEYUP && pHandler) {
                bool ascii_mode = pHandler->ToggleAllAsciiMode();
                SetAsciiMode(ascii_mode);
            }
            return 1;
        }
        return CallNextHookEx(m_hook_handle, nCode, wParam, lParam);
    }
}

bool KeyboardManager::SetAsciiMode(bool ascii_mode) {
    ITfThreadMgr *pThreadMgr;
    DWORD hr = CoCreateInstance(CLSID_TF_ThreadMgr, NULL, CLSCTX_INPROC_SERVER, IID_ITfThreadMgr, (void **)&pThreadMgr);

    if (FAILED(hr)) {
        return false;
    }

    ITfCompartmentMgr *pCompMgr;
    hr = pThreadMgr->GetGlobalCompartment(&pCompMgr);
    if (FAILED(hr)) {
        pThreadMgr->Release();
        return false;
    }

    ITfCompartment *pCompartment;
    hr = pCompMgr->GetCompartment(c_guidStatus, &pCompartment);

    if (FAILED(hr)) {
        pThreadMgr->Release();
        pCompMgr->Release();
        return false;
    }

    VARIANT var;
    VariantInit(&var);
    var.vt = VT_I4;
    var.intVal = (int)ascii_mode;
    hr = pCompartment->SetValue(0, &var);
    VariantClear(&var);

    pThreadMgr->Release();
    pCompMgr->Release();
    pCompartment->Release();

    return true;
}