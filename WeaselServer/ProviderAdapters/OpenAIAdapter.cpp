#include "../AssistantGateway.h"

namespace {
std::wstring EscapeJson(std::wstring const& value) {
  std::wstring out;
  out.reserve(value.size());
  for (wchar_t ch : value) {
    switch (ch) {
      case L'\\':
        out.append(L"\\\\");
        break;
      case L'"':
        out.append(L"\\\"");
        break;
      case L'\n':
        out.append(L"\\n");
        break;
      default:
        out.push_back(ch);
        break;
    }
  }
  return out;
}

std::wstring ExtractJsonStringAfterToken(std::wstring const& text,
                                         std::wstring const& begin_token) {
  size_t begin = text.find(begin_token);
  if (begin == std::wstring::npos) {
    return L"";
  }
  begin += begin_token.size();
  std::wstring out;
  bool escaped = false;
  for (size_t i = begin; i < text.size(); ++i) {
    wchar_t ch = text[i];
    if (escaped) {
      if (ch == L'n') {
        out.push_back(L'\n');
      } else {
        out.push_back(ch);
      }
      escaped = false;
      continue;
    }
    if (ch == L'\\') {
      escaped = true;
      continue;
    }
    if (ch == L'"') {
      return out;
    }
    out.push_back(ch);
  }
  return L"";
}
}  // namespace

std::wstring BuildOpenAIRequestBody(weasel::AiAnalyzeRequest const& request) {
  std::wstring prompt;
  prompt.reserve(request.text.size() + request.context.size() + request.scene.size() + 64);
  prompt.append(L"TEXT:\n").append(request.text);
  if (!request.context.empty()) {
    prompt.append(L"\n\nCONTEXT:\n").append(request.context);
  }
  if (!request.scene.empty()) {
    prompt.append(L"\n\nSCENE:\n").append(request.scene);
  }

  std::wstring body = L"{\"model\":\"gpt-4o-mini\",\"timeout_ms\":";
  body.append(std::to_wstring(request.timeout_ms));
  body.append(L",\"messages\":[{\"role\":\"user\",\"content\":\"");
  body.append(EscapeJson(prompt));
  body.append(L"\"}]}" );
  return body;
}

bool ExtractOpenAIText(std::wstring const& raw, std::wstring* text) {
  if (!text) {
    return false;
  }
  *text = ExtractJsonStringAfterToken(raw, L"\"content\":\"");
  return !text->empty();
}
