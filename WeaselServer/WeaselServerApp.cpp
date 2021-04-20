#include "stdafx.h"
#include "WeaselServerApp.h"
#include <strsafe.h>

WeaselServerApp::WeaselServerApp()
	: m_handler(std::make_unique<RimeWithWeaselHandler>(&m_ui))
	, tray_icon(m_ui)
{
	//m_handler.reset(new RimeWithWeaselHandler(&m_ui));
	m_server.SetRequestHandler(m_handler.get());
	SetupMenuHandlers();
}

WeaselServerApp::~WeaselServerApp()
{
}

//
void WeaselServerApp::LoadIMEIndicator(bool bLoad) {
	auto hant = MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL);
	auto simp = MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED);
	auto _GetLocaleID = [&](int langID) {
		WCHAR key[9] = { 0 };
		HKEY hKey;
		std::wstring _localeID;
		HKL hKL = nullptr;
		LSTATUS ret = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts", 0, KEY_READ, &hKey);
		if (ret == ERROR_SUCCESS)
		{
			for (DWORD id = (0xE0200000 | langID); hKL == nullptr && id <= (0xE0FF0000 | langID); id += 0x10000)
			{
				StringCchPrintfW(key, _countof(key), L"%08X", id);
				HKEY hSubKey;
				ret = RegOpenKeyExW(hKey, key, 0, KEY_READ, &hSubKey);
				if (ret == ERROR_SUCCESS)
				{
					WCHAR data[32] = { 0 };
					DWORD type;
					DWORD size = sizeof data;
					ret = RegQueryValueExW(hSubKey, L"Ime File", NULL, &type, (LPBYTE)data, &size);
					if (ret == ERROR_SUCCESS && type == REG_SZ && _wcsicmp(data, L"weasel.ime") == 0) {
						hKL = (HKL)id;
						_localeID = std::wstring(key);
					}
				}
				RegCloseKey(hSubKey);
			}
		}
		RegCloseKey(hKey);
		return _localeID;
	};
	auto _LoadKeyBoadrdLayout = [](LPCWSTR id,bool bLoad) {
		HKL _hKL = ::LoadKeyboardLayout(id, KLF_ACTIVATE | KLF_NOTELLSHELL);
		if (_hKL)
		{
			BOOL b = ::UnloadKeyboardLayout(_hKL);
			if (bLoad) {
				_hKL = ::LoadKeyboardLayout(id, KLF_ACTIVATE | KLF_NOTELLSHELL);
			}
		}
	};
	LCID _lcid = ::GetSystemDefaultLCID();
	std::wstring id;
	if ( 0x804 == _lcid ) {
		id=_GetLocaleID(simp);
	}
	if ( 0x404 == _lcid ) {
		id=_GetLocaleID(hant);
	}
	//if (IsWindows8OrGreater()) {
	if (!id.empty())
			_LoadKeyBoadrdLayout(id.c_str(),  bLoad);
	//}
}
//

int WeaselServerApp::Run()
{
	if (!m_server.Start())
		return -1;

	//win_sparkle_set_appcast_url("http://localhost:8000/weasel/update/appcast.xml");
	win_sparkle_set_registry_path("Software\\Rime\\Weasel\\Updates");
	win_sparkle_init();
	m_ui.Create(m_server.GetHWnd());

	LoadIMEIndicator(true);  //加载键盘

	tray_icon.Create(m_server.GetHWnd());
	tray_icon.Refresh(0);

	m_handler->Initialize();
	m_handler->OnUpdateUI([this](int state){
		tray_icon.Refresh(state);
	});

	int ret = m_server.Run();

	//OutputDebugString(L"m_handler->Finalize");
	m_handler->Finalize();

	LoadIMEIndicator(false);  //卸载键盘,如果设置成默认输入法可能卸载不了,功能没影响，只是输入法列表能看到

	m_ui.Destroy();
	tray_icon.RemoveIcon();
	tray_icon.Refresh(0);
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
	m_server.AddMenuHandler(ID_WEASELTRAY_WIKI, std::bind(open, L"https://rime.im/docs/"));
	m_server.AddMenuHandler(ID_WEASELTRAY_HOMEPAGE, std::bind(open, L"https://rime.im/"));
	m_server.AddMenuHandler(ID_WEASELTRAY_FORUM, std::bind(open, L"https://rime.im/discuss/"));
	m_server.AddMenuHandler(ID_WEASELTRAY_CHECKUPDATE, check_update);
	m_server.AddMenuHandler(ID_WEASELTRAY_INSTALLDIR, std::bind(explore, dir));
	m_server.AddMenuHandler(ID_WEASELTRAY_USERCONFIG, std::bind(explore, WeaselUserDataPath()));
}
