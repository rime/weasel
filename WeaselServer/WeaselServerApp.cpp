#include "stdafx.h"
#include "WeaselServerApp.h"

WeaselServerApp::WeaselServerApp()
	: m_handler(std::make_unique<RimeWithWeaselHandler>(&m_ui))
{
	//m_handler.reset(new RimeWithWeaselHandler(&m_ui));
	m_server.SetRequestHandler(m_handler.get());
	SetupMenuHandlers();
}

WeaselServerApp::~WeaselServerApp()
{
}

int WeaselServerApp::Run()
{
	if (!m_server.Start())
		return -1;

	//win_sparkle_set_appcast_url("http://localhost:8000/weasel/update/appcast.xml");
	win_sparkle_set_registry_path("Software\\Rime\\Weasel\\Updates");
	win_sparkle_init();
	m_ui.Create(m_server.GetHWnd());
	m_handler->Initialize();

	int ret = m_server.Run();

	m_handler->Finalize();
	m_ui.Destroy();
	win_sparkle_cleanup();

	return ret;
}

void WeaselServerApp::SetupMenuHandlers()
{
	std::wstring dir(install_dir());
	m_server.AddMenuHandler(ID_WEASELTRAY_QUIT, [this] { return m_server.Stop() == 0; });
	m_server.AddMenuHandler(ID_WEASELTRAY_DEPLOY, std::bind(execute, dir + L"\\WeaselDeployer.exe", std::wstring(L"/deploy")));
	m_server.AddMenuHandler(ID_WEASELTRAY_SETTINGS, std::bind(execute, dir + L"\\WeaselDeployer.exe", std::wstring()));
	m_server.AddMenuHandler(ID_WEASELTRAY_DICT_MANAGEMENT, std::bind(execute, dir + L"\\WeaselDeployer.exe", std::wstring(L"/dict")));
	m_server.AddMenuHandler(ID_WEASELTRAY_SYNC, std::bind(execute, dir + L"\\WeaselDeployer.exe", std::wstring(L"/sync")));
	m_server.AddMenuHandler(ID_WEASELTRAY_WIKI, std::bind(open, L"https://github.com/rime/home/wiki/UserGuide"));
	m_server.AddMenuHandler(ID_WEASELTRAY_HOMEPAGE, std::bind(open, L"http://rime.im/"));
	m_server.AddMenuHandler(ID_WEASELTRAY_FORUM, std::bind(open, L"http://tieba.baidu.com/f?kw=rime"));
	m_server.AddMenuHandler(ID_WEASELTRAY_CHECKUPDATE, check_update);
	m_server.AddMenuHandler(ID_WEASELTRAY_INSTALLDIR, std::bind(explore, dir));
	m_server.AddMenuHandler(ID_WEASELTRAY_USERCONFIG, std::bind(explore, WeaselUserDataPath()));
}
