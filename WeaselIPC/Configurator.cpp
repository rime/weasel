#include "stdafx.h"
#include "Deserializer.h"
#include "Configurator.h"

using namespace weasel;

Deserializer::Ptr Configurator::Create(ResponseParser* pTarget)
{
	return Deserializer::Ptr(new Configurator(pTarget));
}

Configurator::Configurator(ResponseParser* pTarget)
	: Deserializer(pTarget)
{
}

Configurator::~Configurator()
{
}

void Configurator::Store(Deserializer::KeyType const& key, std::wstring const& value)
{
    if (key.size() < 2) return;
    bool bool_value = (!value.empty() && value != L"0");
    if (m_pTarget->p_context) {
        if (key[1] == L"inline_preedit") {
            m_pTarget->p_config->inline_preedit = bool_value;
        }
    } else if (key[1] == L"global_ascii_mode") {
        m_pTarget->p_config->global_ascii_mode = bool_value;
    } else if (key[1] == L"macos_capslock") {
        m_pTarget->p_config->macos_capslock = bool_value;
    }
}
