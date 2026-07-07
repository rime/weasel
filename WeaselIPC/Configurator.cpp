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
  if (!m_pTarget->p_config || key.size() < 2)
    return;
  if (key[1] == L"commit_langid") {
    m_pTarget->p_config->commit_langid = _wtoi(value.c_str());
  }
}
