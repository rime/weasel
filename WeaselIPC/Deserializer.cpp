#include "stdafx.h"
#include "Deserializer.h"
#include "ActionLoader.h"
#include "Committer.h"
#include "ContextUpdater.h"

using namespace weasel;


map<wstring, Deserializer::Factory> Deserializer::s_factories;


void Deserializer::Initialize(ResponseParser* pTarget)
{
	if (s_factories.empty())
	{
		// register factory methods
		// TODO: extend the parser's functionality in the future by defining more actions here
		Define(L"action", ActionLoader::Create);
		Define(L"commit", Committer::Create);
		Define(L"ctx", ContextUpdater::Create);
	}

	// loaded by default
	Require(L"action", pTarget);
}

void Deserializer::Define(std::wstring const& action, Factory factory)
{
	s_factories[action] = factory;
	//s_factories.insert(make_pair(action, factory));
}

bool Deserializer::Require(std::wstring const& action, ResponseParser* pTarget)
{
	if (!pTarget)
		return false;

	map<wstring, Factory>::iterator i = s_factories.find(action);
	if (i == s_factories.end())
	{
		// unknown action type
		return false;
	}

	Factory& factory = i->second;

	pTarget->deserializers[action] = factory(pTarget);
	//pTarget->deserializers.insert(make_pair(action, factory(pTarget)));
	return true;
}