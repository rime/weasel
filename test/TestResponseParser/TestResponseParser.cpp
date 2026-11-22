// TestResponseParser.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <boost/detail/lightweight_test.hpp>
#include <ResponseParser.h>
#include <WeaselIPC.h>
#include <string>

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
  BOOST_ASSERT(1 == ctx.preedit.attributes.size());
  weasel::TextAttribute attr0 = ctx.preedit.attributes[0];
  BOOST_TEST_EQ(weasel::HIGHLIGHTED, attr0.type);
  BOOST_TEST_EQ(0, attr0.range.start);
  BOOST_TEST_EQ(3, attr0.range.end);
  BOOST_TEST(ctx.aux.empty());
  weasel::CandidateInfo& c = ctx.cinfo;
  BOOST_ASSERT(2 == c.candies.size());
  BOOST_TEST(c.candies[0].str == L"候選甲");
  BOOST_TEST(c.candies[1].str == L"候選乙");
  BOOST_TEST_EQ(1, c.highlighted);
  BOOST_TEST_EQ(0, c.currentPage);
  BOOST_TEST_EQ(1, c.totalPages);
}

// ---- Protocol version handshake (fix/ipc-version-handshake) ----

// The server's protocol.version line is tracked even when no other
// deserializer is active.
void protocol_version_line_is_parsed() {
  std::wstring commit;
  weasel::Context ctx;
  weasel::Status status;
  weasel::ResponseParser parser(&commit, &ctx, &status);
  parser.Feed(L"protocol.version=1");
  BOOST_TEST_EQ(WEASEL_IPC_PROTOCOL_VERSION, parser.server_protocol_version);
}

// Whatever the compiled-in constant is, the parser reports verbatim what the
// server sent; the comparison against the client constant happens elsewhere.
void protocol_version_reported_verbatim() {
  std::wstring commit;
  weasel::Context ctx;
  weasel::Status status;
  weasel::ResponseParser parser(&commit, &ctx, &status);
  parser.Feed(L"protocol.version=42");
  BOOST_TEST_EQ(42, parser.server_protocol_version);
}

// An old server without the handshake leaves the field at -1.
void absent_protocol_version_is_minus_one() {
  std::wstring commit;
  weasel::Context ctx;
  weasel::Status status;
  weasel::ResponseParser parser(&commit, &ctx, &status);
  parser.Feed(L"action=noop");
  BOOST_TEST_EQ(-1, parser.server_protocol_version);
}

// A malformed version value is treated as absent instead of guessing.
void malformed_protocol_version_is_absent() {
  std::wstring commit;
  weasel::Context ctx;
  weasel::Status status;
  weasel::ResponseParser parser(&commit, &ctx, &status);
  parser.Feed(L"protocol.version=not-a-number");
  BOOST_TEST_EQ(-1, parser.server_protocol_version);

  weasel::ResponseParser parser2(&commit, &ctx, &status);
  parser2.Feed(L"protocol.version=1garbage");
  BOOST_TEST_EQ(-1, parser2.server_protocol_version);
}

// The handshake line flows through the full operator() path and does not
// disturb parsing of the action lines that follow it.
void handshake_line_does_not_disturb_action_parsing() {
  WCHAR resp[] =
      L"protocol.version=1\n"
      L"action=commit\n"
      L"commit=\x4f60\x597d\n"
      L".\n";
  DWORD len = wcslen(resp);
  std::wstring commit;
  weasel::Context ctx;
  weasel::Status status;
  weasel::ResponseParser parser(&commit, &ctx, &status);
  const bool ok = parser(resp, len);
  BOOST_TEST(ok);
  BOOST_TEST_EQ(WEASEL_IPC_PROTOCOL_VERSION, parser.server_protocol_version);
  BOOST_TEST(commit == L"\x4f60\x597d");
}

// A response that ends right after a newline, with no "." terminator, makes
// getline fail inside the loop -- the early-return path in the original code.
// The version observed so far must still be available, and the parse must
// still report failure. Diagnostics used to be skipped entirely here.
void truncated_response_keeps_protocol_version() {
  WCHAR resp[] =
      L"protocol.version=1\n"
      L"action=commit\n"
      L"commit=abc\n";
  DWORD len = wcslen(resp);
  std::wstring commit;
  weasel::Context ctx;
  weasel::Status status;
  weasel::ResponseParser parser(&commit, &ctx, &status);

  const bool ok = parser(resp, len);
  BOOST_TEST(!ok);
  BOOST_TEST_EQ(1, parser.server_protocol_version);
}

// Same early-return path, but the response was cut before the version line:
// the field stays at -1 so the report can say "truncated before
// protocol.version" rather than blaming an old server.
void truncation_before_version_line_is_distinguished() {
  WCHAR resp[] =
      L"action=commit\n"
      L"commit=abc\n";
  DWORD len = wcslen(resp);
  std::wstring commit;
  weasel::Context ctx;
  weasel::Status status;
  weasel::ResponseParser parser(&commit, &ctx, &status);

  const bool ok = parser(resp, len);
  BOOST_TEST(!ok);
  BOOST_TEST_EQ(-1, parser.server_protocol_version);
}

int _tmain(int argc, _TCHAR* argv[]) {
  // Selector for validating the handshake subset alone, independent of the
  // other response-parser cases:  TestResponseParser.exe /handshake
  const bool handshake_only = (argc > 1 && !wcscmp(argv[1], L"/handshake"));

  if (handshake_only) {
    protocol_version_line_is_parsed();
    protocol_version_reported_verbatim();
    absent_protocol_version_is_minus_one();
    malformed_protocol_version_is_absent();
    handshake_line_does_not_disturb_action_parsing();
    truncated_response_keeps_protocol_version();
    truncation_before_version_line_is_distinguished();
  } else {
    test_1();
    test_2();
    test_3();
    test_4();
    protocol_version_line_is_parsed();
    protocol_version_reported_verbatim();
    absent_protocol_version_is_minus_one();
    malformed_protocol_version_is_absent();
    handshake_line_does_not_disturb_action_parsing();
    truncated_response_keeps_protocol_version();
    truncation_before_version_line_is_distinguished();
  }

  system("pause");
  return boost::report_errors();
}
