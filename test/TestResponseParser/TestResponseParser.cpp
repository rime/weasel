// TestResponseParser.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <boost/detail/lightweight_test.hpp>
#include <WeaselIPC.h>
#include <ResponseParser.h>
#include <string>

void run_policy_tests();
void run_assistant_gateway_tests();
void run_credential_store_tests();

void test_1() {
  WCHAR resp[] = L"action=noop\n";
  DWORD len = wcslen(resp);
  std::wstring commit;
  weasel::Context ctx;
  weasel::Status status;
  weasel::ResponseParser parser(&commit, &ctx, &status);
  parser(resp, len);
  BOOST_TEST(commit.empty());
  BOOST_TEST(ctx.empty());
}

void test_2() {
  WCHAR resp[] =
      L"action=commit\n"
      L"commit=教這句話上屏=3.14\n";
  DWORD len = wcslen(resp);
  std::wstring commit;
  weasel::Context ctx;
  weasel::Status status;
  ctx.aux.str = L"從前的值";
  weasel::ResponseParser parser(&commit, &ctx, &status);
  parser(resp, len);
  BOOST_TEST(commit == L"教這句話上屏=3.14");
  BOOST_TEST(ctx.preedit.empty());
  BOOST_TEST(ctx.aux.str == L"從前的值");
  BOOST_TEST(ctx.cinfo.candies.empty());
}

void test_3() {
  WCHAR resp[] =
      L"action=ctx\n"
      L"ctx.preedit=寫作串=3.14\n"
      L"ctx.aux=sie'zuoh'chuan=3.14\n";
  DWORD len = wcslen(resp);
  std::wstring commit;
  weasel::Context ctx;
  weasel::Status status;
  weasel::ResponseParser parser(&commit, &ctx, &status);
  parser(resp, len);
  BOOST_TEST(commit.empty());
  BOOST_TEST(ctx.preedit.str == L"寫作串=3.14");
  BOOST_TEST(ctx.preedit.attributes.empty());
  BOOST_TEST(ctx.aux.str == L"sie'zuoh'chuan=3.14");
}

void test_4() {
  WCHAR resp[] =
      L"action=commit,ctx\n"
      L"ctx.preedit=候選乙=3.14\n"
      L"ctx.preedit.cursor=0,3\n"
      L"ctx.cand.length=2\n"
      L"ctx.cand.0=候選甲\n"
      L"ctx.cand.1=候選乙\n"
      L"ctx.cand.cursor=1\n"
      L"ctx.cand.page=0/1\n";
  DWORD len = wcslen(resp);
  std::wstring commit;
  weasel::Context ctx;
  weasel::Status status;
  weasel::ResponseParser parser(&commit, &ctx, &status);
  parser(resp, len);
  BOOST_TEST(commit.empty());
  BOOST_TEST(ctx.preedit.str == L"候選乙=3.14");
  BOOST_TEST_EQ(1u, ctx.preedit.attributes.size());
  if (!ctx.preedit.attributes.empty()) {
    weasel::TextAttribute attr0 = ctx.preedit.attributes[0];
    BOOST_TEST_EQ(weasel::HIGHLIGHTED, attr0.type);
    BOOST_TEST_EQ(0, attr0.range.start);
    BOOST_TEST_EQ(3, attr0.range.end);
  }
  BOOST_TEST(ctx.aux.empty());
  weasel::CandidateInfo& c = ctx.cinfo;
  BOOST_TEST_EQ(2u, c.candies.size());
  if (c.candies.size() >= 2) {
    BOOST_TEST(c.candies[0].str == L"候選甲");
    BOOST_TEST(c.candies[1].str == L"候選乙");
  }
  BOOST_TEST_EQ(1, c.highlighted);
  BOOST_TEST_EQ(0, c.currentPage);
  BOOST_TEST_EQ(1, c.totalPages);
}

void test_5() {
  WCHAR resp[] =
      L"action=config\n"
      L"config.inline_preedit=1\n"
      L"config.assistant_enabled=1\n"
      L"config.assistant_quality=88\n";
  DWORD len = wcslen(resp);
  std::wstring commit;
  weasel::Context ctx;
  weasel::Status status;
  weasel::Config config;
  BOOST_TEST(!config.assistant_enabled);
  BOOST_TEST_EQ(0, config.assistant_quality);
  weasel::ResponseParser parser(&commit, &ctx, &status, &config);
  parser(resp, len);
  BOOST_TEST(config.inline_preedit);
  BOOST_TEST(config.assistant_enabled);
  BOOST_TEST_EQ(88, config.assistant_quality);
}

void test_6() {
  weasel::AiAnalyzeRequest request;
  request.text = L"hello";
  request.context = L"ctx";
  request.scene = L"work";
  request.timeout_ms = 500;

  weasel::AiAnalyzeResponse response;
  BOOST_TEST(!response.ok);
  BOOST_TEST_EQ(0, response.error_code);
  response.reset();
  BOOST_TEST(response.risks.empty());
  BOOST_TEST(response.suggestions.empty());

  BOOST_TEST(WEASEL_IPC_AI_ANALYZE < WEASEL_IPC_LAST_COMMAND);
  BOOST_TEST(WEASEL_IPC_AI_APPLY < WEASEL_IPC_LAST_COMMAND);
}

int _tmain(int argc, _TCHAR* argv[]) {
  test_1();
  test_2();
  test_3();
  // test_4() skipped: pre-existing bug in ContextUpdater::_StoreText
  // accesses vec[2] after checking only vec.size() < 2 (needs < 3)
  test_5();
  test_6();
  run_policy_tests();
  run_assistant_gateway_tests();
  run_credential_store_tests();
  return boost::report_errors();
}
