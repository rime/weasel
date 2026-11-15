#pragma once
#include <ResponseParser.h>
#include <functional>

namespace weasel {

template <typename T>
bool TryDeserialize(boost::archive::text_wiarchive& ia, T& t) {
  // A version/ABI mismatch between WeaselServer.exe and weasel.dll must never
  // let an exception escape into the host's UI/TSF callbacks (that surfaces as
  // 0xc000041d and crashes the host). Report success/failure to the caller
  // instead of swallowing it: the caller deserializes into a temporary and
  // commits it only on success, so a half-updated object is never published.
  // Do not show a modal here: a blocking MessageBoxA on the host thread caused
  // issue #1949.
  try {
    ia >> t;
    return true;
  } catch (...) {
    // Non-modal and non-throwing: a debugger (or log capture) can count
    // these failures without blocking the host UI thread the way the
    // MessageBoxA in issue #1949 did. One line per bad IPC payload.
    OutputDebugStringW(L"weasel: IPC TryDeserialize failed\n");
    return false;
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
