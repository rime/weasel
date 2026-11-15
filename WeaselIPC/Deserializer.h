#pragma once
#include <ResponseParser.h>
#include <functional>

namespace weasel {

template <typename T>
void TryDeserialize(boost::archive::text_wiarchive& ia, T& t) {
  // Swallow any deserialization failure. A version/ABI mismatch between
  // WeaselServer.exe and weasel.dll must not let an exception escape into the
  // host's UI/TSF callbacks (that surfaces as 0xc000041d and crashes the host).
  // Do not show a modal here: a blocking MessageBoxA on the host thread is what
  // caused issue #1949.
  try {
    ia >> t;
  } catch (...) {
  }
}
class Deserializer {
 public:
  typedef std::vector<std::wstring> KeyType;
  typedef std::shared_ptr<Deserializer> Ptr;
  typedef std::function<Ptr(ResponseParser* pTarget)> Factory;

  Deserializer(ResponseParser* pTarget) : m_pTarget(pTarget) {}
  virtual ~Deserializer() {}
  virtual void Store(KeyType const& key, std::wstring const& value) {}

  static void Initialize(ResponseParser* pTarget);
  static void Define(std::wstring const& action, Factory factory);
  static bool Require(std::wstring const& action, ResponseParser* pTarget);

 protected:
  ResponseParser* m_pTarget;

 private:
  static std::map<std::wstring, Factory> s_factories;
};

}  // namespace weasel
