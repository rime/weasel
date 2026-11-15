// TestResponseParser.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <boost/detail/lightweight_test.hpp>
#include <ResponseParser.h>
#include <string>

// P2 regression tests: malformed IPC payloads must not throw across the
// TSF callback boundary, and must not publish a half-deserialized object.
#include <sstream>
#include <stdexcept>
#include <boost/archive/text_wiarchive.hpp>
#include <boost/archive/text_woarchive.hpp>
#include <PipeChannel.h>
#include "../../WeaselIPC/Deserializer.h"

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
  // ctx.cand carries one whole serialized CandidateInfo; the per-field
  // ctx.cand.length/.0/.1 keys belong to the old protocol and are not read.
  weasel::CandidateInfo expected;
  weasel::Text cand0;
  cand0.str = L"候選甲";
  weasel::Text cand1;
  cand1.str = L"候選乙";
  expected.candies.push_back(cand0);
  expected.candies.push_back(cand1);
  expected.highlighted = 1;
  expected.currentPage = 0;
  expected.totalPages = 1;

  std::wstringstream ss;
  {
    boost::archive::text_woarchive oa(ss);
    oa << expected;
  }

  std::wstring commit;
  weasel::Context ctx;
  weasel::Status status;
  weasel::ResponseParser parser(&commit, &ctx, &status);
  parser.Feed(L"action=commit,ctx");
  parser.Feed(L"ctx.preedit=候選乙=3.14");
  parser.Feed(L"ctx.preedit.cursor=0,3");
  parser.Feed(L"ctx.cand=" + ss.str());
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

// TryDeserialize reports failure and leaves the caller's object untouched when
// the archive body is malformed. no_header keeps archive construction out of
// the test: only the `ia >> t` step is under test here.
void test_5() {
  weasel::CandidateInfo original;
  original.currentPage = 3;
  original.totalPages = 7;
  original.highlighted = 2;
  weasel::Text keep;
  keep.str = L"keep me";
  original.candies.push_back(keep);

  weasel::CandidateInfo target = original;
  std::wstringstream ss(L"not a serialized CandidateInfo");
  boost::archive::text_wiarchive ia(ss, boost::archive::no_header);

  BOOST_TEST(!weasel::TryDeserialize(ia, target));
  BOOST_TEST(target == original);
}

// A malformed ctx.cand value goes through the real ResponseParser dispatch.
// _StoreCand must swallow the exception and leave the previous cinfo intact.
void test_6() {
  std::wstring commit;
  weasel::Context ctx;
  weasel::Status status;
  weasel::ResponseParser parser(&commit, &ctx, &status);
  parser.Feed(L"action=ctx");

  weasel::Text keep;
  keep.str = L"keep me";
  ctx.cinfo.candies.push_back(keep);
  ctx.cinfo.currentPage = 5;
  const weasel::CandidateInfo before = ctx.cinfo;

  parser.Feed(L"ctx.cand=not-a-boost-archive");

  BOOST_TEST(ctx.cinfo == before);
  BOOST_TEST(commit.empty());
}

// Same guarantee for the style deserializer.
void test_7() {
  std::wstring commit;
  weasel::Context ctx;
  weasel::Status status;
  weasel::UIStyle style;
  style.font_point = 42;
  style.font_face = L"original-face";
  const int font_point_before = style.font_point;
  const std::wstring font_face_before = style.font_face;

  weasel::ResponseParser parser(&commit, &ctx, &status, NULL, &style);
  parser.Feed(L"action=style");
  parser.Feed(L"style=not-a-boost-archive");

  BOOST_TEST_EQ(font_point_before, style.font_point);
  BOOST_TEST(font_face_before == style.font_face);
}

// The transactional change must still publish a well-formed archive.
void test_8() {
  weasel::CandidateInfo expected;
  weasel::Text cand;
  cand.str = L"candidate-one";
  expected.candies.push_back(cand);
  expected.highlighted = 0;
  expected.currentPage = 1;
  expected.totalPages = 2;

  std::wstringstream ss;
  {
    boost::archive::text_woarchive oa(ss);
    oa << expected;
  }
  const std::wstring payload = ss.str();
  BOOST_ASSERT(payload.size() > 20);

  std::wstring commit;
  weasel::Context ctx;
  weasel::Status status;
  weasel::ResponseParser parser(&commit, &ctx, &status);
  parser.Feed(L"action=ctx");
  parser.Feed(L"ctx.cand=" + payload);

  BOOST_ASSERT(1 == ctx.cinfo.candies.size());
  BOOST_TEST(ctx.cinfo.candies[0].str == L"candidate-one");
  BOOST_TEST_EQ(1, ctx.cinfo.currentPage);
  BOOST_TEST_EQ(2, ctx.cinfo.totalPages);
}

// The outermost IPC barrier converts a throwing handler into `false`
// instead of letting the exception reach the caller / TSF callback.
void test_9() {
  weasel::PipeChannel<DWORD> channel(
      std::wstring(L"\\\\.\\pipe\\weasel-p2-unused"));
  bool result = channel.HandleResponseData([](LPWSTR, UINT) -> bool {
    throw std::runtime_error("simulated IPC version mismatch");
  });
  BOOST_TEST(!result);
}

int _tmain(int argc, _TCHAR* argv[]) {
  test_1();
  test_2();
  test_3();
  test_4();
  test_5();
  test_6();
  test_7();
  test_8();
  test_9();

  system("pause");
  return boost::report_errors();
}
