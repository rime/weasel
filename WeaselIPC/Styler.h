#pragma once

namespace weasel {
class Deserializr;
}

class Styler : public weasel::Deserializer {
 public:
  Styler(weasel::ResponseParser* pTarget);
  virtual ~Styler();
  // store data
  virtual void Store(weasel::Deserializer::KeyType const& key,
                     std::wstring const& value);
  // factory method
  static weasel::Deserializer::Ptr Create(weasel::ResponseParser* pTarget);
};
