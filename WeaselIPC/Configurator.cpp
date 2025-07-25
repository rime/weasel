#include "stdafx.h"
#include "Deserializer.h"
#include "Configurator.h"

using namespace weasel;

Deserializer::Ptr Configurator::Create(ResponseParser* pTarget) {
  return Deserializer::Ptr(new Configurator(pTarget));
}

Configurator::Configurator(ResponseParser* pTarget) : Deserializer(pTarget) {}

Configurator::~Configurator() {}

void Configurator::Store(Deserializer::KeyType const& key,
                         std::wstring const& value) {
  if (!m_pTarget->p_context || key.size() < 2)
    return;
  bool bool_value = (!value.empty() && value != L"0");
  if (key[1] == L"inline_preedit") {
    m_pTarget->p_config->inline_preedit = bool_value;
    return;
  }

  if (key[1] == L"hide_ime_mode_icon") {
    m_pTarget->p_config->hide_ime_mode_icon = bool_value;
    return;
  }
}
