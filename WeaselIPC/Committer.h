#pragma once
#include "Deserializer.h"

class Committer : public weasel::Deserializer
{
public:
	Committer(weasel::ResponseParser* pTarget);
	virtual ~Committer();
	// store data
	virtual void Store(weasel::Deserializer::KeyType const& key, std::wstring const& value);
	// factory method
	static weasel::Deserializer::Ptr Create(weasel::ResponseParser* pTarget);
};
