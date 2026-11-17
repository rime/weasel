#include "stdafx.h"
#include <StringAlgorithm.hpp>
#include <WeaselIPC.h>
#include "Deserializer.h"

using namespace weasel;

ResponseParser::ResponseParser(std::wstring* commit,
                               Context* context,
                               Status* status,
                               Config* config,
                               UIStyle* style)
    : p_commit(commit),
      p_context(context),
      p_status(status),
      p_config(config),
      p_style(style) {
  Deserializer::Initialize(this);
}

bool ResponseParser::operator()(LPWSTR buffer, UINT length) {
  // Keep the parse result and the protocol diagnostics independent. A response
  // that truncates or fails mid-read is *the* case worth diagnosing, so the
  // reporting below runs on every exit path, not only on the happy one. An
  // early `return` here would silently drop exactly those cases.
  const bool parsed_ok = _Parse(buffer, length);
  _ReportProtocolVersion(parsed_ok);
  return parsed_ok;
}

bool ResponseParser::_Parse(LPWSTR buffer, UINT length) {
  wbufferstream bs(buffer, length);
  std::wstring line;
  while (bs.good()) {
    std::getline(bs, line);
    if (!bs.good())
      break;  // truncated or unreadable; diagnosed by _ReportProtocolVersion

    // file ends
    if (line == L".")
      break;

    Feed(line);
  }
  return bs.good();
}

void ResponseParser::_ReportProtocolVersion(bool parsed_ok) {
  // Logging only: never throws, never blocks the host UI thread with a modal
  // dialog (#1949). This is what makes a half-installed server/client pairing
  // visible in DebugView instead of surfacing later as an archive exception
  // inside a TSF callback (#1883).
  if (server_protocol_version < 0) {
    // No version line. Either the peer predates the handshake, or the response
    // was cut before the header could be read -- say which one.
    OutputDebugStringW(
        parsed_ok
            ? L"weasel: IPC response had no protocol.version (old server?)\n"
            : L"weasel: IPC response truncated before protocol.version "
              L"(unknown server)\n");
  } else if (server_protocol_version != WEASEL_IPC_PROTOCOL_VERSION) {
    // Gated: a mismatched pair stays mismatched on every response, so this one
    // sits behind the LogLevel switch instead of spamming the trace.
    DEBUG << "weasel: IPC protocol version mismatch: client="
          << WEASEL_IPC_PROTOCOL_VERSION
          << " server=" << server_protocol_version << "\n";
  }

  if (!parsed_ok) {
    // The body did not parse. Report the version actually observed so a
    // truncated response is not misread as an old server.
    DEBUG << "weasel: IPC response did not parse; server protocol="
          << (server_protocol_version < 0
                  ? std::string("absent")
                  : std::to_string(server_protocol_version))
          << "\n";
  }
}

void ResponseParser::Feed(const std::wstring& line) {
  // ignore blank lines and comments
  if (line.empty() || line.find_first_of(L'#') == 0)
    return;

  // The protocol handshake line is handled inline: it must be tracked for
  // every response, even when no other deserializer is active yet.
  const std::wstring kVersionKey = L"protocol.version=";
  if (starts_with(line, kVersionKey)) {
    const std::wstring value = line.substr(kVersionKey.length());
    wchar_t* end = nullptr;
    const long parsed = wcstol(value.c_str(), &end, 10);
    if (end && *end == L'\0' && end != value.c_str())
      server_protocol_version = static_cast<int>(parsed);
    else
      server_protocol_version = -1;  // malformed value: treat as absent
    return;
  }

  Deserializer::KeyType key;
  std::wstring value;

  // extract key (split by L'.') and value
  std::wstring::size_type sep_pos = line.find_first_of(L'=');
  if (sep_pos == std::wstring::npos)
    return;
  split(key, line.substr(0, sep_pos), L".");
  if (key.empty())
    return;
  value = line.substr(sep_pos + 1);

  // first part of the key serve as action type
  std::wstring const& action = key[0];

  // get required action deserializer instance
  std::map<std::wstring, Deserializer::Ptr>::iterator i =
      deserializers.find(action);
  if (i == deserializers.end()) {
    // line ignored... since corresponding deserializer is not active
    return;
  }

  // dispatch
  Deserializer::Ptr p = i->second;
  p->Store(key, value);
}
