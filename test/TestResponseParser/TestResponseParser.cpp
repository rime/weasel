// TestResponseParser.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <boost/detail/lightweight_test.hpp>
#include <ResponseParser.h>
#include <string>

using namespace std;

void test_1()
{
	WCHAR resp[] = 
		L"action=noop\n"
		;
	DWORD len = wcslen(resp);
	wstring commit;
	weasel::Context ctx;
	weasel::Status status;
	weasel::ResponseParser parser(&commit, &ctx, &status);
	parser(resp, len);
	BOOST_TEST(commit.empty());
	BOOST_TEST(ctx.empty());
}

void test_2()
{
	WCHAR resp[] = 
		L"action=commit\n"
		L"commit=教這句話上屏=3.14\n"
		;
	DWORD len = wcslen(resp);
	wstring commit;
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

void test_3()
{
	WCHAR resp[] = 
		L"action=ctx\n"
		L"ctx.preedit=寫作串=3.14\n"
		L"ctx.aux=sie'zuoh'chuan=3.14\n"
		;
	DWORD len = wcslen(resp);
	wstring commit;
	weasel::Context ctx;
	weasel::Status status;
	weasel::ResponseParser parser(&commit, &ctx, &status);
	parser(resp, len);
	BOOST_TEST(commit.empty());
	BOOST_TEST(ctx.preedit.str == L"寫作串=3.14");
	BOOST_TEST(ctx.preedit.attributes.empty());
	BOOST_TEST(ctx.aux.str == L"sie'zuoh'chuan=3.14");
}

void test_4()
{
	WCHAR resp[] = 
		L"action=commit,ctx\n"
		L"ctx.preedit=候選乙=3.14\n"
		L"ctx.preedit.cursor=0,3\n"
		L"ctx.cand.length=2\n"
		L"ctx.cand.0=候選甲\n"
		L"ctx.cand.1=候選乙\n"
		L"ctx.cand.cursor=1\n"
		L"ctx.cand.page=0/1\n"
		;
	DWORD len = wcslen(resp);
	wstring commit;
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

int _tmain(int argc, _TCHAR* argv[])
{
	test_1();
	test_2();
	test_3();
	test_4();

	system("pause");
	return boost::report_errors();
}

