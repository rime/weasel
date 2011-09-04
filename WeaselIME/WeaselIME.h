#pragma once
#include <WeaselIPC.h>
#include <WeaselUI.h>
#include "KeyEvent.h"

#define MAX_COMPOSITION_SIZE 256

struct CompositionInfo
{
	COMPOSITIONSTRING cs;	
	WCHAR szCompStr[MAX_COMPOSITION_SIZE];
	WCHAR szResultStr[MAX_COMPOSITION_SIZE];
	void Reset()
	{
		memset(this, 0, sizeof(*this));
		cs.dwSize = sizeof(*this);
		cs.dwCompStrOffset = (char*)&szCompStr - (char*)this;
		cs.dwResultStrOffset = (char*)&szResultStr - (char*)this;
	}
};

typedef struct _tagTRANSMSG {
	UINT message;
	WPARAM wParam;
	LPARAM lParam;
} TRANSMSG, *LPTRANSMSG;

class WeaselIME;

class HIMCMap : public std::map<HIMC, boost::shared_ptr<WeaselIME> >
{
public:
    HIMCMap() : m_valid(true) {}
    ~HIMCMap() { m_valid = false; }
    boost::mutex& get_mutex() { return m_mutex; }
    bool is_valid() const { return m_valid; }
private:
    bool m_valid;
	boost::mutex m_mutex;
};

class WeaselIME
{
public:
	static LPCWSTR GetIMEName();
	static LPCWSTR GetIMEFileName();
	static LPCWSTR GetRegKey();
	static LPCWSTR GetRegValueName();
	static HINSTANCE GetModuleInstance();
	static void SetModuleInstance(HINSTANCE hModule);
	static HRESULT RegisterUIClass();
	static HRESULT UnregisterUIClass();
	static LPCWSTR GetUIClassName();
	static LRESULT WINAPI UIWndProc(HWND hWnd, UINT uMsg, WPARAM wp, LPARAM lp);
	static BOOL IsIMEMessage(UINT uMsg);
	static boost::shared_ptr<WeaselIME> GetInstance(HIMC hIMC);
	static void Cleanup();

	WeaselIME(HIMC hIMC);
	LRESULT OnIMESelect(BOOL fSelect);
	LRESULT OnIMEFocus(BOOL fFocus);
	LRESULT OnUIMessage(HWND hWnd, UINT uMsg, WPARAM wp, LPARAM lp);
	BOOL ProcessKeyEvent(UINT vKey, KeyInfo kinfo, const LPBYTE lpbKeyState);

private:
	HRESULT _Initialize();
	HRESULT _Finalize();
	LRESULT _OnIMENotify(LPINPUTCONTEXT lpIMC, WPARAM wp, LPARAM lp);
	HRESULT _StartComposition();
	HRESULT _EndComposition(LPCWSTR composition);
	HRESULT _AddIMEMessage(UINT msg, WPARAM wp, LPARAM lp);
	void _SetCandidatePos(LPINPUTCONTEXT lpIMC);
	void _SetCompositionWindow(LPINPUTCONTEXT lpIMC);
	void _UpdateInputPosition(LPINPUTCONTEXT lpIMC, POINT pt);
	void _UpdateContext(weasel::Context const& ctx);
	weasel::UIStyle const GetUIStyleSettings();

private:
	static HINSTANCE s_hModule;
	static HIMCMap s_instances;
	HIMC m_hIMC;
	bool m_preferCandidatePos;
	weasel::UI m_ui;
	weasel::Client m_client;
	weasel::Context m_ctx;
	weasel::Status m_status;
};
