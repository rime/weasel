#include "stdafx.h"
#include <RimingWeasel.h>
#include <rime_api.h>
#include <list>
#include <set>
#include <string>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>

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

RimingWeaselHandler::RimingWeaselHandler()
{
}

RimingWeaselHandler::~RimingWeaselHandler()
{
}

void RimingWeaselHandler::Initialize()
{
	RimeInitialize();
}

void RimingWeaselHandler::Finalize()
{
	RimeFinalize();
}

UINT RimingWeaselHandler::FindSession(UINT sessionID)
{
	bool found = RimeFindSession(sessionID);
	return found ? sessionID : 0;
}

UINT RimingWeaselHandler::AddSession(LPWSTR buffer)
{
	UINT id = RimeCreateSession();
	return id;
}

UINT RimingWeaselHandler::RemoveSession(UINT sessionID)
{
	RimeDestroySession(sessionID);
	return 0;
}

BOOL RimingWeaselHandler::ProcessKeyEvent(weasel::KeyEvent keyEvent, UINT sessionID, LPWSTR buffer)
{
	bool taken = RimeProcessKey(sessionID, keyEvent.keycode, expand_ibus_modifier(keyEvent.mask)); 
	_Respond(sessionID, buffer);	
	return (BOOL)taken;
}

bool RimingWeaselHandler::_Respond(UINT sessionID, LPWSTR buffer)
{
	std::set<std::string> actions;
	std::list<std::string> messages;

	// extract information

	RimeCommit commit;
	if (RimeGetCommit(sessionID, &commit))
	{
		actions.insert("commit");
		messages.push_back(boost::str(boost::format("commit=%s\n") % commit.text));
	}

	RimeContext ctx;
	if (RimeGetContext(sessionID, &ctx))
	{
		if (ctx.composition.is_composing)
		{
			actions.insert("ctx");
			messages.push_back(boost::str(boost::format("ctx.preedit=%s\n") % ctx.composition.preedit));
			if (ctx.composition.sel_start < ctx.composition.sel_end)
			{
				messages.push_back(boost::str(boost::format("ctx.preedit.cursor=%d,%d\n") % 
					utf8towcslen(ctx.composition.preedit, ctx.composition.sel_start) % 
					utf8towcslen(ctx.composition.preedit, ctx.composition.sel_end)));
			}
		}
		if (ctx.menu.num_candidates)
		{
			actions.insert("ctx");
			messages.push_back(boost::str(boost::format("ctx.cand.length=%d\n") % ctx.menu.num_candidates));
			for (int i = 0; i < ctx.menu.num_candidates; ++i)
			{
				messages.push_back(boost::str(boost::format("ctx.cand.%d=%s\n") % i % ctx.menu.candidates[i]));
			}
			messages.push_back(boost::str(boost::format("ctx.cand.cursor=%d\n") % ctx.menu.highlighted_candidate_index));
			messages.push_back(boost::str(boost::format("ctx.cand.page=%d\n") % ctx.menu.page_no));
		}
	}

	RimeStatus status;
	if (RimeGetStatus(sessionID, &status))
	{
		// not useful for now...
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
