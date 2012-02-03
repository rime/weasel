#include "stdafx.h"
#include <RimeWithWeasel.h>
#include <windows.h>
#include <list>
#include <set>
#include <string>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>

#pragma warning(disable: 4005)
#include <rime_api.h>
#pragma warning(default: 4005)

static const std::string WeaselLogFilePath()
{
	char path[MAX_PATH] = {0};
	ExpandEnvironmentStringsA("%AppData%\\Rime\\rime.log", path, _countof(path));
	return path;
}

#define EZLOGGER_OUTPUT_FILENAME WeaselLogFilePath()
#define EZLOGGER_REPLACE_EXISTING_LOGFILE_

// logging enabled
//#define EZDBGONLYLOGGERPRINT(...)
//#define EZDBGONLYLOGGERFUNCTRACKER

#pragma warning(disable: 4995)
#pragma warning(disable: 4996)
#include <ezlogger/ezlogger_headers.hpp>
#pragma warning(default: 4996)
#pragma warning(default: 4995)

const WCHAR* utf8towcs(const char* utf8_str)
{
	const int buffer_len = 4096;
	static WCHAR buffer[buffer_len];
	memset(buffer, 0, sizeof(buffer));
	MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, buffer, buffer_len - 1);
	return buffer;
}

int utf8towcslen(const char* utf8_str, int utf8_len)
{
	return MultiByteToWideChar(CP_UTF8, 0, utf8_str, utf8_len, NULL, 0);
}

int expand_ibus_modifier(int m)
{
	return (m & 0xff) | ((m & 0xff00) << 16);
}

static const char* weasel_shared_data_dir() {
	static char path[MAX_PATH] = {0};
	GetModuleFileNameA(NULL, path, _countof(path));
	std::string str_path(path);
	size_t k = str_path.find_last_of("/\\");
	strcpy(path + k + 1, "data");
	return path;
}

static const char* weasel_user_data_dir() {
	static char path[MAX_PATH] = {0};
	ExpandEnvironmentStringsA("%AppData%\\Rime", path, _countof(path));
	return path;
}

RimeWithWeaselHandler::RimeWithWeaselHandler(weasel::UI *ui)
	: m_ui(ui), m_active_session(0), m_disabled(true)
{
}

RimeWithWeaselHandler::~RimeWithWeaselHandler()
{
}

void RimeWithWeaselHandler::Initialize()
{
	m_disabled = _IsDeployerRunning();
	if (m_disabled)
	{
		 return;
	}
	EZLOGGERPRINT("Initializing la rime.");
	RimeTraits weasel_traits;
	weasel_traits.shared_data_dir = weasel_shared_data_dir();
	weasel_traits.user_data_dir = weasel_user_data_dir();
	const int len = 20;
	char utf8_str[len];
	memset(utf8_str, 0, sizeof(utf8_str));
	WideCharToMultiByte(CP_UTF8, 0, WEASEL_IME_NAME, -1, utf8_str, len - 1, NULL, NULL);
	weasel_traits.distribution_name = utf8_str;
	weasel_traits.distribution_code_name = WEASEL_CODE_NAME;
	weasel_traits.distribution_version = WEASEL_VERSION;
	RimeInitialize(&weasel_traits);
	if (m_ui)
	{
		_UpdateUIStyle();
	}
}

void RimeWithWeaselHandler::Finalize()
{
	if (m_disabled) return;
	m_active_session = 0;
	m_disabled = true;
	EZLOGGERPRINT("Finalizing la rime.");
	RimeFinalize();
}

UINT RimeWithWeaselHandler::FindSession(UINT session_id)
{
	if (m_disabled) return 0;
	bool found = RimeFindSession(session_id);
	EZDBGONLYLOGGERPRINT("Find session: session_id = 0x%x, found = %d", session_id, found);
	return found ? session_id : 0;
}

UINT RimeWithWeaselHandler::AddSession(LPWSTR buffer)
{
	if (m_disabled)
	{
		EZDBGONLYLOGGERPRINT("Trying to resume service.");
		EndMaintenance();
		if (m_disabled) return 0;
	}
	UINT session_id = RimeCreateSession();
	EZDBGONLYLOGGERPRINT("Add session: created session_id = 0x%x", session_id);
	// show session's welcome message :-) if any
	_UpdateUI(session_id);
	m_active_session = session_id;
	return session_id;
}

UINT RimeWithWeaselHandler::RemoveSession(UINT session_id)
{
	if (m_disabled) return 0;
	EZDBGONLYLOGGERPRINT("Remove session: session_id = 0x%x", session_id);
	// TODO: force committing? otherwise current composition would be lost
	RimeDestroySession(session_id);
	if (m_ui) m_ui->Hide();
	m_active_session = 0;
	return 0;
}

BOOL RimeWithWeaselHandler::ProcessKeyEvent(weasel::KeyEvent keyEvent, UINT session_id, LPWSTR buffer)
{
	EZDBGONLYLOGGERPRINT("Process key event: keycode = 0x%x, mask = 0x%x, session_id = 0x%x", 
		keyEvent.keycode, keyEvent.mask, session_id);
	if (m_disabled) return FALSE;
	bool taken = RimeProcessKey(session_id, keyEvent.keycode, expand_ibus_modifier(keyEvent.mask)); 
	_UpdateUI(session_id);
	_Respond(session_id, buffer);
	m_active_session = session_id;
	return (BOOL)taken;
}

void RimeWithWeaselHandler::FocusIn(UINT session_id)
{
	EZDBGONLYLOGGERPRINT("Focus in: session_id = 0x%x", session_id);
	if (m_disabled) return;
	_UpdateUI(session_id);
	m_active_session = session_id;
}

void RimeWithWeaselHandler::FocusOut(UINT session_id)
{
	EZDBGONLYLOGGERPRINT("Focus out: session_id = 0x%x", session_id);
	if (m_disabled) return;
	if (m_ui) m_ui->Hide();
	m_active_session = 0;
}

void RimeWithWeaselHandler::UpdateInputPosition(RECT const& rc, UINT session_id)
{
	EZDBGONLYLOGGERPRINT("Update input position: (%d, %d), session_id = 0x%x, m_active_session = 0x%x", 
		rc.left, rc.top, session_id, m_active_session);
	if (m_disabled) return;
	if (m_ui) m_ui->UpdateInputPosition(rc);
	if (m_active_session != session_id)
	{
		_UpdateUI(session_id);
		m_active_session = session_id;
	}
}

void RimeWithWeaselHandler::StartMaintenance()
{
	Finalize();
	_UpdateUI(0);
}

void RimeWithWeaselHandler::EndMaintenance()
{
	if (m_disabled)
	{
		Initialize();
		_UpdateUI(0);
	}
}

bool RimeWithWeaselHandler::_IsDeployerRunning()
{
	HANDLE hMutex = CreateMutex(NULL, TRUE, L"WeaselDeployerMutex");
	bool deployer_detected = hMutex && GetLastError() == ERROR_ALREADY_EXISTS;
	if (hMutex)
	{
		CloseHandle(hMutex);
	}
	return deployer_detected;
}

void RimeWithWeaselHandler::_UpdateUI(UINT session_id)
{
	weasel::Status weasel_status;
	weasel::Context weasel_context;
	if (session_id == 0)
		weasel_status.disabled = m_disabled;

	RimeStatus status;
	if (RimeGetStatus(session_id, &status))
	{
		weasel_status.ascii_mode = status.is_ascii_mode;
		weasel_status.composing = status.is_composing;
		weasel_status.disabled = status.is_disabled;
	}

	RimeContext ctx;
	if (RimeGetContext(session_id, &ctx))
	{
		if (ctx.composition.length > 0)
		{
			weasel_context.preedit.str = utf8towcs(ctx.composition.preedit);
			if (ctx.composition.sel_start < ctx.composition.sel_end)
			{
				weasel::TextAttribute attr;
				attr.type = weasel::HIGHLIGHTED;
				attr.range.start = utf8towcslen(ctx.composition.preedit, ctx.composition.sel_start);
				attr.range.end = utf8towcslen(ctx.composition.preedit, ctx.composition.sel_end);
			
				weasel_context.preedit.attributes.push_back(attr);
			}
		}
		if (ctx.menu.num_candidates)
		{
			weasel::CandidateInfo &cinfo(weasel_context.cinfo);
			cinfo.candies.resize(ctx.menu.num_candidates);
			for (int i = 0; i < ctx.menu.num_candidates; ++i)
			{
				cinfo.candies[i].str = utf8towcs(ctx.menu.candidates[i]);
			}
			cinfo.highlighted = ctx.menu.highlighted_candidate_index;
			cinfo.currentPage = ctx.menu.page_no;
		}
	}

	if (!m_ui) return;
	if (weasel_status.composing || weasel_status.ascii_mode)
	{
		m_ui->Update(weasel_context, weasel_status);
		m_ui->Show();
	}
	else
	{
		m_ui->Hide();
		m_ui->Update(weasel_context, weasel_status);
	}
}

bool RimeWithWeaselHandler::_Respond(UINT session_id, LPWSTR buffer)
{
	std::set<std::string> actions;
	std::list<std::string> messages;

	// extract information

	RimeCommit commit;
	if (RimeGetCommit(session_id, &commit))
	{
		actions.insert("commit");
		messages.push_back(boost::str(boost::format("commit=%s\n") % commit.text));
	}

	// summarize

	if (actions.empty())
	{
		messages.insert(messages.begin(), std::string("action=noop\n"));
	}
	else
	{
		std::string actionList(boost::join(actions, ","));
		messages.insert(messages.begin(), boost::str(boost::format("action=%s\n") % actionList));
	}

	messages.push_back(std::string(".\n"));

	// printing to stream

	memset(buffer, 0, WEASEL_IPC_BUFFER_SIZE);
	wbufferstream bs(buffer, WEASEL_IPC_BUFFER_LENGTH);

	BOOST_FOREACH(const std::string &msg, messages)
	{
		bs << utf8towcs(msg.c_str());
		if (!bs.good())
		{
			// response text toooo long!
			return false;
		}
	}

	return true;
}

void RimeWithWeaselHandler::_UpdateUIStyle()
{
	if (!m_ui) return;

	RimeConfig config = { NULL };
	if (!RimeConfigOpen("weasel", &config))
		return;

	weasel::UIStyle &style(m_ui->style());

	const int BUF_SIZE = 99;
	char buffer[BUF_SIZE + 1];
	memset(buffer, '\0', sizeof(buffer));
	if (RimeConfigGetString(&config, "style/font_face", buffer, BUF_SIZE))
	{
		style.font_face = utf8towcs(buffer);
	}
	RimeConfigGetInt(&config, "style/font_point", &style.font_point);
	// layout
	RimeConfigGetInt(&config, "style/layout/min_width", &style.min_width);
	RimeConfigGetInt(&config, "style/layout/min_height", &style.min_height);
	RimeConfigGetInt(&config, "style/layout/border", &style.border);
	RimeConfigGetInt(&config, "style/layout/margin_x", &style.margin_x);
	RimeConfigGetInt(&config, "style/layout/margin_y", &style.margin_y);
	RimeConfigGetInt(&config, "style/layout/spacing", &style.spacing);
	RimeConfigGetInt(&config, "style/layout/candidate_spacing", &style.candidate_spacing);
	RimeConfigGetInt(&config, "style/layout/hilite_spacing", &style.hilite_spacing);
	RimeConfigGetInt(&config, "style/layout/hilite_padding", &style.hilite_padding);
	RimeConfigGetInt(&config, "style/layout/round_corner", &style.round_corner);
	// color scheme
	if (RimeConfigGetString(&config, "style/color_scheme", buffer, BUF_SIZE))
	{
		std::string prefix("preset_color_schemes/");
		prefix += buffer;
		RimeConfigGetInt(&config, (prefix + "/text_color").c_str(), &style.text_color);
		if (!RimeConfigGetInt(&config, (prefix + "/candidate_text_color").c_str(), &style.candidate_text_color))
		{
			style.candidate_text_color = style.text_color;
		}
		RimeConfigGetInt(&config, (prefix + "/back_color").c_str(), &style.back_color);
		if (!RimeConfigGetInt(&config, (prefix + "/border_color").c_str(), &style.border_color))
		{
			style.border_color = style.text_color;
		}
		if (!RimeConfigGetInt(&config, (prefix + "/hilited_text_color").c_str(), &style.hilited_text_color))
		{
			style.hilited_text_color = style.text_color;
		}
		if (!RimeConfigGetInt(&config, (prefix + "/hilited_back_color").c_str(), &style.hilited_back_color))
		{
			style.hilited_back_color = style.back_color;
		}
		if (!RimeConfigGetInt(&config, (prefix + "/hilited_candidate_text_color").c_str(), &style.hilited_candidate_text_color))
		{
			style.hilited_candidate_text_color = style.hilited_text_color;
		}
		if (!RimeConfigGetInt(&config, (prefix + "/hilited_candidate_back_color").c_str(), &style.hilited_candidate_back_color))
		{
			style.hilited_candidate_back_color = style.hilited_back_color;
		}
	}
	RimeConfigClose(&config);
}
