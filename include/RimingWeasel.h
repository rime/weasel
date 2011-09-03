#pragma once
#include <WeaselIPC.h>

class RimingWeaselHandler :
	public weasel::RequestHandler
{
public:
	RimingWeaselHandler();
	virtual ~RimingWeaselHandler();
	virtual void Initialize();
	virtual void Finalize();
	virtual UINT FindSession(UINT sessionID);
	virtual UINT AddSession(LPWSTR buffer);
	virtual UINT RemoveSession(UINT sessionID);
	virtual BOOL ProcessKeyEvent(weasel::KeyEvent keyEvent, UINT sessionID, LPWSTR buffer);

private:
	bool _Respond(UINT sessionID, LPWSTR buffer);

};
