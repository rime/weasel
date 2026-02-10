#include "stdafx.h"

#include <boost/detail/lightweight_test.hpp>

#include "../../WeaselServer/AssistantPolicy.h"
#include "../../WeaselServer/AssistantGateway.h"
#include "../../WeaselServer/CredentialStore.h"
#include <WeaselIPCData.h>

namespace {
// Mirror the _IsChatApp logic from RimeWithWeasel.cpp for testing
bool IsChatApp(std::string const& app_name) {
  if (app_name.empty())
    return false;
  return app_name.find("wechat") != std::string::npos ||
         app_name.find("weixin") != std::string::npos ||
         app_name.find("qq.exe") != std::string::npos ||
         app_name.find("tim.exe") != std::string::npos ||
         app_name.find("dingtalk") != std::string::npos ||
         app_name.find("feishu") != std::string::npos ||
         app_name.find("lark") != std::string::npos ||
         app_name.find("slack") != std::string::npos ||
         app_name.find("telegram") != std::string::npos;
}
}  // namespace

void test_chat_app_detection() {
  BOOST_TEST(IsChatApp("wechat.exe"));
  BOOST_TEST(IsChatApp("c:\\program files\\tencent\\wechat\\wechat.exe"));
  BOOST_TEST(IsChatApp("qq.exe"));
  BOOST_TEST(IsChatApp("tim.exe"));
  BOOST_TEST(IsChatApp("dingtalk.exe"));
  BOOST_TEST(IsChatApp("feishu.exe"));
  BOOST_TEST(IsChatApp("lark.exe"));
  BOOST_TEST(IsChatApp("slack.exe"));
  BOOST_TEST(IsChatApp("telegram.exe"));
  // Negative cases
  BOOST_TEST(!IsChatApp("notepad.exe"));
  BOOST_TEST(!IsChatApp("chrome.exe"));
  BOOST_TEST(!IsChatApp("code.exe"));
  BOOST_TEST(!IsChatApp(""));
}

void test_send_intent_pipeline() {
  // Simulate the full pipeline: text -> policy -> gateway
  // This is what the server-side AnalyzeText handler does

  // Step 1: Policy classification
  AssistantContext policy_ctx;
  policy_ctx.current_text = L"今天项目进展：接口联调完成";
  auto policy = ClassifyPolicy(policy_ctx);
  BOOST_TEST(policy.scene == SceneType::WorkReport);

  // Step 2: Gateway analysis with mock response
  CredentialStore store;
  store.Save(L"assistant/openai", L"sk-test");

  AssistantGateway gateway;
  weasel::AiAnalyzeRequest request;
  request.text = L"今天项目进展：接口联调完成";
  request.timeout_ms = 500;

  // Simulate a provider response
  std::wstring raw =
      L"{\"choices\":[{\"message\":{\"content\":\"EXPLAIN|汇报内容清晰\\n"
      L"RISK|接口联调完成|缺少量化数据|1\\n"
      L"SUGGEST|今天完成接口联调3项，剩余回归测试2项|补充量化\"}}]}";

  auto response =
      gateway.Analyze(AssistantProvider::OpenAI, request, raw);
  BOOST_TEST(response.ok);
  BOOST_TEST(response.explanation == L"汇报内容清晰");
  BOOST_TEST_EQ(1u, response.risks.size());
  BOOST_TEST_EQ(1u, response.suggestions.size());
  if (!response.risks.empty()) {
    BOOST_TEST(response.risks[0].text == L"接口联调完成");
    BOOST_TEST_EQ(1, response.risks[0].severity);
  }
  if (!response.suggestions.empty()) {
    BOOST_TEST(response.suggestions[0].text ==
               L"今天完成接口联调3项，剩余回归测试2项");
  }

  // Cleanup
  store.Remove(L"assistant/openai");
}

void test_analyze_response_serialization() {
  // Test that the response format produced by the server handler
  // can be parsed by a client. The format is key=value lines.
  std::wstring resp = L"ai_analyze.ok=0\n"
                      L"ai_analyze.error_code=501\n"
                      L"ai_analyze.explanation=HTTP transport not yet implemented\n"
                      L"ai_analyze.scene=WorkReport\n"
                      L"ai_analyze.timeout_ms=500\n"
                      L".\n";

  // Verify format is well-formed (each line has key=value or is ".")
  size_t begin = 0;
  int line_count = 0;
  while (begin < resp.size()) {
    size_t end = resp.find(L'\n', begin);
    if (end == std::wstring::npos)
      break;
    std::wstring line = resp.substr(begin, end - begin);
    if (line == L".") {
      line_count++;
      break;
    }
    // Each non-terminal line should contain "="
    BOOST_TEST(line.find(L'=') != std::wstring::npos);
    line_count++;
    begin = end + 1;
  }
  BOOST_TEST_EQ(6, line_count);  // 5 data lines + 1 terminator
}

void run_send_intent_tests() {
  test_chat_app_detection();
  test_send_intent_pipeline();
  test_analyze_response_serialization();
}
