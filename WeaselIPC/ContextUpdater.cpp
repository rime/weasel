#include "stdafx.h"
#include "Deserializer.h"
#include "ContextUpdater.h"

using namespace weasel;

Deserializer::Ptr ContextUpdater::Create(ResponseParser* pTarget)
{
	return Deserializer::Ptr(new ContextUpdater(pTarget));
}


ContextUpdater::ContextUpdater(ResponseParser* pTarget)
: Deserializer(pTarget)
{
}

ContextUpdater::~ContextUpdater()
{
}

void ContextUpdater::Store(Deserializer::KeyType const& k, wstring const& value)
{
	if(k.size() < 2)
		return;

	if (k[1] == L"preedit")
	{
		_StoreText(m_pTarget->r_context.preedit, k, value);
		return;
	}

	if (k[1] == L"aux")
	{
		_StoreText(m_pTarget->r_context.aux, k, value);
		return;
	}
	if (k[1] == L"cand")
	{
		_StoreCand(k, value);
		return;
	}
}

void ContextUpdater::_StoreText(Text& target, Deserializer::KeyType k, wstring const& value)
{
	if(k.size() == 2)
	{
		target.clear();
		target.str = value;
		return;
	}
	if(k.size() == 3)
	{
		if (k[2] == L"cursor")
		{
			vector<wstring> vec;
			split(vec, value, is_any_of(L","));
			if (vec.size() < 2)
				return;

			weasel::TextAttribute attr;
			attr.type = HIGHLIGHTED;
			attr.range.start = _wtoi(vec[0].c_str());
			attr.range.end = _wtoi(vec[1].c_str());
			
			target.attributes.push_back(attr);
			return;
		}
	}
}

void ContextUpdater::_StoreCand(Deserializer::KeyType k, wstring const& value)
{
	CandidateInfo& cinfo = m_pTarget->r_context.cinfo;
	if(k.size() < 3)
		return;
	if (k[2] == L"length")
	{
		cinfo.clear();
		int size = _wtoi(value.c_str());
		cinfo.candies.resize(size);
		return;
	}
	if (k[2] == L"cursor")
	{
		int cur = _wtoi(value.c_str());
		cinfo.highlighted = cur;
		return;
	}
	if (k[2] == L"page")
	{
		vector<wstring> vec;
		split(vec, value, is_any_of(L"/"));
		if (vec.size() == 0)
			return;
		int i = _wtoi(vec[0].c_str());
		cinfo.currentPage = i;
		int n = (vec.size() >= 2) ? _wtoi(vec[1].c_str()) : 0;
		cinfo.totalPages = n;
		return;
	}

	size_t idx = _wtoi(k[2].c_str());
	if (idx >= cinfo.candies.size())
		return;
	cinfo.candies[idx].str = value;
}