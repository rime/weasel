#include "stdafx.h"
#include "../../WeaselServer/AssistantPolicy.h"
#include <boost/core/lightweight_test.hpp>

void test_policy_work_report() {
  AssistantContext c;
  c.current_text = L"今天项目进展：接口联调完成，剩余回归测试";
  c.recent_context.push_back(L"领导问：今天进展如何？");

  auto p = ClassifyPolicy(c);
  BOOST_TEST(p.scene == SceneType::WorkReport);
  BOOST_TEST(p.timeout_ms <= 500);
}

void test_policy_question_answer() {
  AssistantContext c;
  c.current_text = L"结论先说：先回滚，下一步补充日志定位原因";

  auto p = ClassifyPolicy(c);
  BOOST_TEST(p.scene == SceneType::QuestionAnswer);
  BOOST_TEST_EQ(500, p.timeout_ms);
}

void run_policy_tests() {
  test_policy_work_report();
  test_policy_question_answer();
}
