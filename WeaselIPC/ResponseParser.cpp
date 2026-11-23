#include "stdafx.h"
#include <cerrno>
#include <climits>
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
  //
  // DoEditSession builds a fresh ResponseParser for every response, so a
  // per-instance flag cannot dedupe. Keep one-shot flags at process scope,
  // keyed by the diagnostic, so an old server does not grow the debug log
  // with one identical line per keystroke.
  static bool reported_missing = false;
  static bool reported_truncated = false;
  static bool reported_invalid = false;

  if (protocol_version_state == ProtocolVersionState::Missing) {
    // No version line. Either the peer predates the handshake, or the response
    // was cut before the header could be read -- say which one.
    if (parsed_ok) {
      if (!reported_missing) {
        OutputDebugStringW(
            L"weasel: IPC response had no protocol.version (old server?)\n");
        reported_missing = true;
      }
    } else {
      if (!reported_truncated) {
        OutputDebugStringW(
            L"weasel: IPC response truncated before protocol.version "
            L"(unknown server)\n");
        reported_truncated = true;
      }
    }
  } else if (protocol_version_state == ProtocolVersionState::Invalid) {
    // A version line was present but malformed -- a different diagnosis from
    // an old server that never sends one.
    if (!reported_invalid) {
      OutputDebugStringW(
          L"weasel: IPC response had an invalid "
          L"protocol.version value\n");
      reported_invalid = true;
    }
  } else if (server_protocol_version != WEASEL_IPC_PROTOCOL_VERSION) {
    // Gated: a mismatched pair stays mismatched on every response, so this one
    // sits behind the LogLevel switch instead of spamming the trace.
    DEBUG << "weasel: IPC protocol version mismatch: client="
          << WEASEL_IPC_PROTOCOL_VERSION
          << " server=" << server_protocol_version << "\n";
  }

  if (!parsed_ok) {
    // The body did not parse. Describe the version state actually observed so
    // a truncated response is not misread as an old server.
    const char* version_desc = nullptr;
    switch (protocol_version_state) {
      case ProtocolVersionState::Missing:
        version_desc = "absent";
        break;
      case ProtocolVersionState::Invalid:
        version_desc = "invalid";
        break;
      case ProtocolVersionState::Valid:
        version_desc = "valid";
        break;
    }
    DEBUG << "weasel: IPC response did not parse; server protocol="
          << version_desc;
    if (protocol_version_state == ProtocolVersionState::Valid)
      DEBUG << "=" << server_protocol_version;
    DEBUG << "\n";
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
    // Only a plain, non-negative decimal integer is a valid version. wcstol
    // alone would accept leading whitespace and a sign, letting "-1" collide
    // with the "absent" sentinel used by the diagnostics below.
    bool valid = !value.empty();
    for (wchar_t ch : value) {
      if (ch < L'0' || ch > L'9') {
        valid = false;
        break;
      }
    }
    long parsed = 0;
    if (valid) {
      errno = 0;
      wchar_t* end = nullptr;
      parsed = wcstol(value.c_str(), &end, 10);
      // ERANGE catches values beyond long; the INT_MAX cap also covers
      // platforms where long is wider than the int field.
      valid = errno != ERANGE && end != value.c_str() && *end == L'\0' &&
              parsed <= INT_MAX;
    }
    if (valid) {
      protocol_version_state = ProtocolVersionState::Valid;
      server_protocol_version = static_cast<int>(parsed);
    } else {
      protocol_version_state = ProtocolVersionState::Invalid;
      server_protocol_version = -1;
    }
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
