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
	ExpandEnvironmentStringsA("%AppData%\\Rime\\weasel.log", path, _countof(path));
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

RimeWithWeaselHandler::RimeWithWeaselHandler()
	: active_session(0)
{
}

RimeWithWeaselHandler::~RimeWithWeaselHandler()
{
}

void RimeWithWeaselHandler::Initialize()
{
	EZDBGONLYLOGGERPRINT("Initializing la rime.");
	RimeTraits weasel_traits;
	weasel_traits.shared_data_dir = weasel_shared_data_dir();
	weasel_traits.user_data_dir = weasel_user_data_dir();
	RimeInitialize(&weasel_traits);
	m_ui.Create(NULL);
}

void RimeWithWeaselHandler::Finalize()
{
	EZDBGONLYLOGGERPRINT("Finalizing la rime.");
	m_ui.Destroy();
	RimeFinalize();
}

UINT RimeWithWeaselHandler::FindSession(UINT session_id)
{
	bool found = RimeFindSession(session_id);
	EZDBGONLYLOGGERPRINT("Find session: session_id = 0x%x, found = %d", session_id, found);
	return found ? session_id : 0;
}

UINT RimeWithWeaselHandler::AddSession(LPWSTR buffer)
{
	UINT session_id = RimeCreateSession();
	EZDBGONLYLOGGERPRINT("Add session: created session_id = 0x%x", session_id);
	// show session's welcome message :-) if any
	_UpdateUI(session_id);
	active_session = session_id;
	return session_id;
}

UINT RimeWithWeaselHandler::RemoveSession(UINT session_id)
{
	EZDBGONLYLOGGERPRINT("Remove session: session_id = 0x%x", session_id);
	// TODO: force committing? otherwise current composition would be lost
	RimeDestroySession(session_id);
	m_ui.Hide();
	active_session = 0;
	return 0;
}

BOOL RimeWithWeaselHandler::ProcessKeyEvent(weasel::KeyEvent keyEvent, UINT session_id, LPWSTR buffer)
{
	EZDBGONLYLOGGERPRINT("Process key event: keycode = 0x%x, mask = 0x%x, session_id = 0x%x", 
		keyEvent.keycode, keyEvent.mask, session_id);
	bool taken = RimeProcessKey(session_id, keyEvent.keycode, expand_ibus_modifier(keyEvent.mask)); 
	_UpdateUI(session_id);
	_Respond(session_id, buffer);
	active_session = session_id;
	return (BOOL)taken;
}

void RimeWithWeaselHandler::FocusIn(UINT session_id)
{
	EZDBGONLYLOGGERPRINT("Focus in: session_id = 0x%x", session_id);
	_UpdateUI(session_id);
	active_session = session_id;
}

void RimeWithWeaselHandler::FocusOut(UINT session_id)
{
	EZDBGONLYLOGGERPRINT("Focus out: session_id = 0x%x", session_id);
	m_ui.Hide();
	active_session = 0;
}

void RimeWithWeaselHandler::UpdateInputPosition(RECT const& rc, UINT session_id)
{
	EZDBGONLYLOGGERPRINT("Update input position: (%d, %d), session_id = 0x%x, active_session = 0x%x", 
		rc.left, rc.top, session_id, active_session);
	m_ui.UpdateInputPosition(rc);
	if (active_session != session_id)
	{
		_UpdateUI(session_id);
		active_session = session_id;
	}
}

void RimeWithWeaselHandler::_UpdateUI(UINT session_id)
{
	weasel::Context weasel_context;
	RimeContext ctx;
	if (RimeGetContext(session_id, &ctx))
	{
		if (ctx.composition.is_composing)
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

	RimeStatus status;
	if (RimeGetStatus(session_id, &status))
	{
		// not interesting for now...
	}

	if (!weasel_context.empty())
	{
		m_ui.UpdateContext(weasel_context);
		m_ui.Show();
	}
	else
	{
		m_ui.Hide();
		m_ui.UpdateContext(weasel_context);
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
