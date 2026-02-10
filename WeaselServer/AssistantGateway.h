#pragma once

#include <string>
#include <vector>

#include <WeaselIPCData.h>

enum class AssistantProvider {
  OpenAI = 0,
  Anthropic,
  DeepSeek,
  Unknown,
};

AssistantProvider ParseAssistantProvider(std::wstring const& provider_name);

std::wstring BuildOpenAIRequestBody(weasel::AiAnalyzeRequest const& request);
std::wstring BuildAnthropicRequestBody(weasel::AiAnalyzeRequest const& request);
std::wstring BuildDeepSeekRequestBody(weasel::AiAnalyzeRequest const& request);

bool ExtractOpenAIText(std::wstring const& raw, std::wstring* text);
bool ExtractAnthropicText(std::wstring const& raw, std::wstring* text);
bool ExtractDeepSeekText(std::wstring const& raw, std::wstring* text);

class AssistantGateway {
 public:
  weasel::AiAnalyzeResponse Analyze(AssistantProvider provider,
                                    weasel::AiAnalyzeRequest const& request,
                                    std::wstring const& raw_response) const;

 private:
  weasel::AiAnalyzeResponse NormalizeContent(std::wstring const& content) const;
};
