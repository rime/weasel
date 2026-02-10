#include "AssistantPolicy.h"

namespace {
bool contains_any(std::wstring const& text,
                  std::initializer_list<const wchar_t*> terms) {
  for (auto term : terms) {
    if (text.find(term) != std::wstring::npos) {
      return true;
    }
  }
  return false;
}

std::wstring merge_context(AssistantContext const& context) {
  std::wstring merged = context.current_text;
  for (auto const& item : context.recent_context) {
    merged.append(L"\n").append(item);
  }
  return merged;
}
}  // namespace

AssistantPolicy ClassifyPolicy(AssistantContext const& context) {
  AssistantPolicy policy;
  std::wstring content = merge_context(context);

  if (contains_any(content,
                   {L"进展", L"汇报", L"完成", L"风险", L"里程碑", L"状态"})) {
    policy.scene = SceneType::WorkReport;
    policy.timeout_ms = 500;
    policy.diagnosis_budget_ms = 300;
    policy.rewrite_budget_ms = 200;
    return policy;
  }

  if (contains_any(content,
                   {L"问题", L"答复", L"结论", L"下一步", L"原因", L"方案"})) {
    policy.scene = SceneType::QuestionAnswer;
    policy.timeout_ms = 500;
    policy.diagnosis_budget_ms = 320;
    policy.rewrite_budget_ms = 180;
    return policy;
  }

  policy.scene = SceneType::CasualChat;
  policy.timeout_ms = 400;
  policy.diagnosis_budget_ms = 250;
  policy.rewrite_budget_ms = 150;
  return policy;
}
