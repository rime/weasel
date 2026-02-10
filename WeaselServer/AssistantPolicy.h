#pragma once

#include <string>
#include <vector>

enum class SceneType {
  CasualChat = 0,
  WorkReport,
  QuestionAnswer,
};

struct AssistantContext {
  std::wstring current_text;
  std::vector<std::wstring> recent_context;
};

struct AssistantPolicy {
  SceneType scene = SceneType::CasualChat;
  int timeout_ms = 500;
  int diagnosis_budget_ms = 300;
  int rewrite_budget_ms = 200;
};

AssistantPolicy ClassifyPolicy(AssistantContext const& context);
