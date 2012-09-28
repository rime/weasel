#pragma once
#include <WeaselIPC.h>
#include <WeaselUI.h>
#include <map>
#include <string>

typedef std::map<std::string, bool> AppOptions;
typedef std::map<std::string, AppOptions> AppOptionsByAppName;

class RimeWithWeaselHandler :
	public weasel::RequestHandler
{
public:
	RimeWithWeaselHandler(weasel::UI *ui);
	virtual ~RimeWithWeaselHandler();
	virtual void Initialize();
	virtual void Finalize();
	virtual UINT FindSession(UINT session_id);
	virtual UINT AddSession(LPWSTR buffer);
	virtual UINT RemoveSession(UINT session_id);
	virtual BOOL ProcessKeyEvent(weasel::KeyEvent keyEvent, UINT session_id, LPWSTR buffer);
	virtual void FocusIn(DWORD param, UINT session_id);
	virtual void FocusOut(DWORD param, UINT session_id);
	virtual void UpdateInputPosition(RECT const& rc, UINT session_id);
	virtual void StartMaintenance();
	virtual void EndMaintenance();

private:
	bool _IsDeployerRunning();
	void _UpdateUI(UINT session_id);
	bool _Respond(UINT session_id, LPWSTR buffer);

	AppOptionsByAppName m_app_options;
	weasel::UI* m_ui;  // reference
	UINT m_active_session;
    DWORD m_client_caps;
	bool m_disabled;
};
