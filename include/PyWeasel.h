#pragma once
#include <WeaselIPC.h>
#include <boost/python.hpp>

class PyWeaselHandler :
	public weasel::RequestHandler
{
public:
	PyWeaselHandler();
	virtual ~PyWeaselHandler();
	virtual void Initialize();
	virtual void Finalize();
	virtual UINT FindSession(UINT session_id);
	virtual UINT AddSession(LPWSTR buffer);
	virtual UINT RemoveSession(UINT session_id);
	virtual BOOL ProcessKeyEvent(weasel::KeyEvent keyEvent, UINT session_id, LPWSTR buffer);

private:
	bool _Respond(LPWSTR buffer, std::wstring const& msg);
	boost::python::object m_service;
};
