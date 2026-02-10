#include "AssistantGateway.h"
#include "CredentialStore.h"

#include <algorithm>
#include <cwctype>

namespace {
std::wstring ToLower(std::wstring value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](wchar_t ch) { return static_cast<wchar_t>(std::towlower(ch)); });
  return value;
}

std::vector<std::wstring> SplitLines(std::wstring const& text) {
  std::vector<std::wstring> lines;
  size_t begin = 0;
  while (begin <= text.size()) {
    size_t end = text.find(L'\n', begin);
    if (end == std::wstring::npos) {
      lines.push_back(text.substr(begin));
      break;
    }
    lines.push_back(text.substr(begin, end - begin));
    begin = end + 1;
  }
  return lines;
}

std::vector<std::wstring> SplitByPipe(std::wstring const& line) {
  std::vector<std::wstring> fields;
  size_t begin = 0;
  while (begin <= line.size()) {
    size_t end = line.find(L'|', begin);
    if (end == std::wstring::npos) {
      fields.push_back(line.substr(begin));
      break;
    }
    fields.push_back(line.substr(begin, end - begin));
    begin = end + 1;
  }
  return fields;
}

std::wstring ProviderCredentialTarget(AssistantProvider provider) {
  switch (provider) {
    case AssistantProvider::OpenAI:
      return L"assistant/openai";
    case AssistantProvider::Anthropic:
      return L"assistant/anthropic";
    case AssistantProvider::DeepSeek:
      return L"assistant/deepseek";
    default:
      return L"";
  }
}
}  // namespace

AssistantProvider ParseAssistantProvider(std::wstring const& provider_name) {
  std::wstring name = ToLower(provider_name);
  if (name == L"openai") {
    return AssistantProvider::OpenAI;
  }
  if (name == L"anthropic") {
    return AssistantProvider::Anthropic;
  }
  if (name == L"deepseek") {
    return AssistantProvider::DeepSeek;
  }
  return AssistantProvider::Unknown;
}

weasel::AiAnalyzeResponse AssistantGateway::Analyze(
    AssistantProvider provider,
    weasel::AiAnalyzeRequest const& request,
    std::wstring const& raw_response) const {
  (void)request;
  std::wstring content;
  bool ok = false;
  CredentialStore credential_store;
  auto target = ProviderCredentialTarget(provider);
  if (!target.empty() && !credential_store.Load(target).has_value()) {
    weasel::AiAnalyzeResponse no_credential;
    no_credential.ok = false;
    no_credential.error_code = 401;
    no_credential.explanation = L"missing provider credential";
    return no_credential;
  }

  // Strict budget: degrade early when timeout window is too tight
  if (request.timeout_ms > 0 && request.timeout_ms < 100) {
    weasel::AiAnalyzeResponse degraded;
    degraded.ok = true;
    degraded.error_code = 408;
    degraded.explanation = L"degraded: timeout";
    if (!request.text.empty()) {
      weasel::AiRisk risk;
      risk.text = request.text;
      risk.reason = L"timeout budget exceeded, fallback mode";
      risk.severity = 1;
      degraded.risks.push_back(risk);
    }
    return degraded;
  }

  switch (provider) {
    case AssistantProvider::OpenAI:
      ok = ExtractOpenAIText(raw_response, &content);
      break;
    case AssistantProvider::Anthropic:
      ok = ExtractAnthropicText(raw_response, &content);
      break;
    case AssistantProvider::DeepSeek:
      ok = ExtractDeepSeekText(raw_response, &content);
      break;
    default:
      break;
  }

  if (!ok) {
    weasel::AiAnalyzeResponse degraded;
    degraded.ok = true;
    degraded.error_code = 408;
    degraded.explanation = L"degraded: parse failure";
    if (!request.text.empty()) {
      weasel::AiRisk risk;
      risk.text = request.text;
      risk.reason = L"provider unavailable, fallback mode";
      risk.severity = 1;
      degraded.risks.push_back(risk);
    }
    return degraded;
  }
  return NormalizeContent(content);
}

weasel::AiAnalyzeResponse AssistantGateway::NormalizeContent(
    std::wstring const& content) const {
  weasel::AiAnalyzeResponse response;
  response.ok = true;
  response.error_code = 0;

  auto lines = SplitLines(content);
  for (auto const& line : lines) {
    if (line.empty()) {
      continue;
    }
    auto fields = SplitByPipe(line);
    if (fields.empty()) {
      continue;
    }

    if (fields[0] == L"EXPLAIN" && fields.size() >= 2) {
      response.explanation = fields[1];
      continue;
    }
    if (fields[0] == L"RISK" && fields.size() >= 4) {
      weasel::AiRisk risk;
      risk.text = fields[1];
      risk.reason = fields[2];
      try {
        risk.severity = std::stoi(fields[3]);
      } catch (...) {
        risk.severity = 1;
      }
      response.risks.push_back(risk);
      continue;
    }
    if (fields[0] == L"SUGGEST" && fields.size() >= 3) {
      weasel::AiSuggestion suggestion;
      suggestion.text = fields[1];
      suggestion.reason = fields[2];
      response.suggestions.push_back(suggestion);
    }
  }

  return response;
}
