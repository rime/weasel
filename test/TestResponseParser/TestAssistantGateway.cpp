#include "stdafx.h"

#include <boost/detail/lightweight_test.hpp>

#include "../../WeaselServer/AssistantGateway.h"
#include "../../WeaselServer/CredentialStore.h"

namespace {
void EnsureCredential(std::wstring const& target) {
  CredentialStore store;
  store.Save(target, L"sk-test");
}
}

void test_gateway_openai_normalization() {
  AssistantGateway gateway;
  weasel::AiAnalyzeRequest request;
  request.text = L"原文";
  request.context = L"上下文";
  EnsureCredential(L"assistant/openai");

  std::wstring raw =
      L"{\"choices\":[{\"message\":{\"content\":\"EXPLAIN|表达不清\\nRISK|今天完成了|缺少量化结果|2\\nSUGGEST|今天完成接口联调，回归测试剩余2项|补充量化\"}}]}";

  auto response = gateway.Analyze(AssistantProvider::OpenAI, request, raw);
  BOOST_TEST(response.ok);
  BOOST_TEST_EQ(0, response.error_code);
  BOOST_TEST(response.explanation == L"表达不清");
  BOOST_TEST_EQ(1u, response.risks.size());
  BOOST_TEST_EQ(1u, response.suggestions.size());
  if (!response.risks.empty()) {
    BOOST_TEST(response.risks[0].text == L"今天完成了");
    BOOST_TEST(response.risks[0].reason == L"缺少量化结果");
    BOOST_TEST_EQ(2, response.risks[0].severity);
  }
  if (!response.suggestions.empty()) {
    BOOST_TEST(response.suggestions[0].text == L"今天完成接口联调，回归测试剩余2项");
    BOOST_TEST(response.suggestions[0].reason == L"补充量化");
  }
}

void test_gateway_anthropic_normalization() {
  AssistantGateway gateway;
  weasel::AiAnalyzeRequest request;
  EnsureCredential(L"assistant/anthropic");
  std::wstring raw =
      L"{\"content\":[{\"type\":\"text\",\"text\":\"EXPLAIN|结论先行\\nSUGGEST|先说结论，再说下一步|结构优化\"}]}";

  auto response = gateway.Analyze(AssistantProvider::Anthropic, request, raw);
  BOOST_TEST(response.ok);
  BOOST_TEST_EQ(0u, response.risks.size());
  BOOST_TEST_EQ(1u, response.suggestions.size());
  BOOST_TEST(response.explanation == L"结论先行");
  if (!response.suggestions.empty()) {
    BOOST_TEST(response.suggestions[0].text == L"先说结论，再说下一步");
    BOOST_TEST(response.suggestions[0].reason == L"结构优化");
  }
}

void test_gateway_deepseek_normalization() {
  AssistantGateway gateway;
  weasel::AiAnalyzeRequest request;
  EnsureCredential(L"assistant/deepseek");
  std::wstring raw =
      L"{\"choices\":[{\"message\":{\"content\":\"RISK|你看下|语气不明确|1\"}}]}";

  auto response = gateway.Analyze(AssistantProvider::DeepSeek, request, raw);
  BOOST_TEST(response.ok);
  BOOST_TEST_EQ(1u, response.risks.size());
  if (!response.risks.empty()) {
    BOOST_TEST(response.risks[0].text == L"你看下");
    BOOST_TEST(response.risks[0].reason == L"语气不明确");
    BOOST_TEST_EQ(1, response.risks[0].severity);
  }
}

void test_gateway_request_body_escapes() {
  weasel::AiAnalyzeRequest request;
  request.text = L"A\"B\\C\nD";

  auto openai = BuildOpenAIRequestBody(request);
  auto anthropic = BuildAnthropicRequestBody(request);
  auto deepseek = BuildDeepSeekRequestBody(request);

  BOOST_TEST(openai.find(L"A\\\"B\\\\C\\nD") != std::wstring::npos);
  BOOST_TEST(anthropic.find(L"A\\\"B\\\\C\\nD") != std::wstring::npos);
  BOOST_TEST(deepseek.find(L"A\\\"B\\\\C\\nD") != std::wstring::npos);
}

void run_assistant_gateway_tests() {
  test_gateway_openai_normalization();
  test_gateway_anthropic_normalization();
  test_gateway_deepseek_normalization();
  test_gateway_request_body_escapes();
}
