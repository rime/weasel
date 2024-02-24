#pragma once
#include "Deserializer.h"

class Configurator : public weasel::Deserializer {
 public:
  Configurator(weasel::ResponseParser* pTarget);
  virtual ~Configurator();
  // store data
  virtual void Store(weasel::Deserializer::KeyType const& key,
                     std::wstring const& value);
  // factory method
  static weasel::Deserializer::Ptr Create(weasel::ResponseParser* pTarget);
};
