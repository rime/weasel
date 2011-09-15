#pragma once
#include <WeaselIPC.h>
#include <WeaselUI.h>

class RimingWeaselHandler :
	public weasel::RequestHandler
{
public:
	RimingWeaselHandler();
	virtual ~RimingWeaselHandler();
	virtual void Initialize();
	virtual void Finalize();
	virtual UINT FindSession(UINT session_id);
	virtual UINT AddSession(LPWSTR buffer);
	virtual UINT RemoveSession(UINT session_id);
	virtual BOOL ProcessKeyEvent(weasel::KeyEvent keyEvent, UINT session_id, LPWSTR buffer);
	virtual void FocusIn(UINT session_id);
	virtual void FocusOut(UINT session_id);
	virtual void UpdateInputPosition(RECT const& rc, UINT session_id);

private:
	void _UpdateUI(UINT session_id);
	bool _Respond(UINT session_id, LPWSTR buffer);
	
	weasel::UI m_ui;
	UINT active_session;
};
