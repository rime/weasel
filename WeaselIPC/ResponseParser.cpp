#include "stdafx.h"
#include <WeaselIPC.h>
#include "Deserializer.h"

using namespace weasel;

ResponseParser::ResponseParser(std::wstring* commit, Context* context, Status* status)
 : p_commit(commit), p_context(context), p_status(status)
{
	Deserializer::Initialize(this);
}

bool ResponseParser::operator() (LPWSTR buffer, UINT length)
{
	wbufferstream bs(buffer, length);
	wstring line;
	while (bs.good())
	{
		getline(bs, line);
		if (!bs.good())
			return false;

		// file ends
		if (line == L".")
			break;
		
		Feed(line);
	}
	return bs.good();
}

void ResponseParser::Feed(const wstring& line)
{
	// ignore blank lines and comments
	if (line.empty() || line.find_first_of(L'#') == 0)
		return;

	Deserializer::KeyType key;
	wstring value;

	// extract key (split by L'.') and value
	wstring::size_type sep_pos = line.find_first_of(L'=');
	if (sep_pos == wstring::npos)
		return;
	split(key, line.substr(0, sep_pos), is_any_of(L"."));
	if (key.empty())
		return;
	value = line.substr(sep_pos + 1);

	// first part of the key serve as action type
	wstring const& action = key[0];
	
	// get required action deserializer instance
	map<wstring, Deserializer::Ptr>::iterator i = deserializers.find(action);
	if (i == deserializers.end())
	{
		// line ignored... since corresponding deserializer is not active
		return;
	}

	// dispatch
	Deserializer::Ptr p = i->second;
	p->Store(key, value);
}
