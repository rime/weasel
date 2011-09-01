#include "stdafx.h"
#include "Deserializer.h"
#include "ActionLoader.h"

using namespace weasel;


Deserializer::Ptr ActionLoader::Create(ResponseParser* pTarget)
{
	return Deserializer::Ptr(new ActionLoader(pTarget));
}

ActionLoader::ActionLoader(ResponseParser* pTarget)
: Deserializer(pTarget)
{
}

ActionLoader::~ActionLoader()
{
}

void ActionLoader::Store(Deserializer::KeyType const& key, wstring const& value)
{
	if (key.size() == 1)  // no extention parts
	{
		// split value by L","
		vector<wstring> vecAction;
		split(vecAction, value, is_any_of(L","));
		
		// require specified action deserializers
		for(vector<wstring>::const_iterator it = vecAction.begin(); it != vecAction.end(); ++it)
		{
			Deserializer::Require(*it, m_pTarget);
		}
	}
}
