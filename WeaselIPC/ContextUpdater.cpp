#include "stdafx.h"
#include <StringAlgorithm.hpp>
#include "Deserializer.h"
#include "ContextUpdater.h"

using namespace weasel;

// ContextUpdater

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

void ContextUpdater::Store(Deserializer::KeyType const& k, std::wstring const& value)
{
	if(!m_pTarget->p_context || k.size() < 2)
		return;

	if (k[1] == L"preedit")
	{
		_StoreText(m_pTarget->p_context->preedit, k, value);
		return;
	}

	if (k[1] == L"aux")
	{
		_StoreText(m_pTarget->p_context->aux, k, value);
		return;
	}

	if (k[1] == L"cand")
	{
		_StoreCand(k, value);
		return;
	}
}

void ContextUpdater::_StoreText(Text& target, Deserializer::KeyType k, std::wstring const& value)
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
			std::vector<std::wstring> vec;
			split(vec, value, L",");
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

void ContextUpdater::_StoreCand(Deserializer::KeyType k, std::wstring const& value)
{
	CandidateInfo& cinfo = m_pTarget->p_context->cinfo;
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
		std::vector<std::wstring> vec;
		split(vec, value, L"/");
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

// StatusUpdater

Deserializer::Ptr StatusUpdater::Create(ResponseParser* pTarget)
{
	return Deserializer::Ptr(new StatusUpdater(pTarget));
}

StatusUpdater::StatusUpdater(ResponseParser* pTarget)
: Deserializer(pTarget)
{
}

StatusUpdater::~StatusUpdater()
{
}

void StatusUpdater::Store(Deserializer::KeyType const& k, std::wstring const& value)
{
	if(!m_pTarget->p_status || k.size() < 2)
		return;

	bool bool_value = (!value.empty() && value != L"0");

	if (k[1] == L"ascii_mode")
	{
		m_pTarget->p_status->ascii_mode = bool_value;
		return;
	}

	if (k[1] == L"composing")
	{
		m_pTarget->p_status->composing = bool_value;
		return;
	}

	if (k[1] == L"disabled")
	{
		m_pTarget->p_status->disabled = bool_value;
		return;
	}
}
