#pragma once
#include "Deserializer.h"

class ActionLoader : public weasel::Deserializer
{
public:
	ActionLoader(weasel::ResponseParser* pTarget);
	virtual ~ActionLoader();
	// store data
	virtual void Store(weasel::Deserializer::KeyType const& key, std::wstring const& value);
	// factory method
	static weasel::Deserializer::Ptr Create(weasel::ResponseParser* pTarget);
};
