#pragma once
#include <WeaselIPCData.h>
#include <windows.h>
#include <map>
#include <memory>
#include <string>

namespace weasel {
class Deserializer;

// How the server advertised its protocol version in the current response.
// Kept separate from the numeric field: an invalid version line and a missing
// one both leave that field at -1, but they mean different things
// diagnostically.
enum class ProtocolVersionState {
  Missing,
  Valid,
  Invalid,
};

// 解析server回應文本
struct ResponseParser {
  std::map<std::wstring, std::shared_ptr<Deserializer> > deserializers;

  std::wstring* p_commit;
  Context* p_context;
  Status* p_status;
  Config* p_config;
  UIStyle* p_style;

  // Protocol version advertised by the server in this response, or -1 when
  // the line was absent or invalid. Populated while parsing; the check happens
  // in operator(). For the reason behind -1, see `protocol_version_state`.
  int server_protocol_version = -1;
  ProtocolVersionState protocol_version_state = ProtocolVersionState::Missing;

  ResponseParser(std::wstring* commit,
                 Context* context = 0,
                 Status* status = 0,
                 Config* config = 0,
                 UIStyle* style = 0);

  // 重載函數調用運算符, 以扮做ResponseHandler
  bool operator()(LPWSTR buffer, UINT length);

  // 處理一行回應文本
  void Feed(const std::wstring& line);

 private:
  // Parse the response buffer. Returns whether it terminated cleanly with the
  // "." line. Deliberately separate from the diagnostics so a truncated
  // response is still reported.
  bool _Parse(LPWSTR buffer, UINT length);
  // Report the handshake outcome. `parsed_ok` disambiguates an absent version
  // line: a clean response means a pre-handshake server, a failed one means the
  // header never arrived.
  void _ReportProtocolVersion(bool parsed_ok);
};

}  // namespace weasel
