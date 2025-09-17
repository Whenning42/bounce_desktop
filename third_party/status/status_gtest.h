#ifndef STATUS_GTEST_H_
#define STATUS_GTEST_H_

#include <gmock/gmock.h>
#include <gtest/gtest.h>

MATCHER_P(StatusIs, code, "") { return get_status_(arg).code() == code; }

#define ASSERT_OK(a) ASSERT_THAT(a, StatusIs(StatusCode::OK));

#define EXPECT_OK(a) EXPECT_THAT(a, StatusIs(StatusCode::OK));

#define CAT_VAR(a, b) a##b
#define CAT_VAR_(a, b) CAT_VAR(a, b)
#define UNIQ_VAR(v) CAT_VAR_(v, __COUNTER__)

#define ASSERT_OK_AND_ASSIGN(lhs, rhs) \
  ASSERT_OK_AND_ASSIGN_IMPL(UNIQ_VAR(v), lhs, rhs)

#define ASSERT_OK_AND_ASSIGN_IMPL(var, lhs, rhs) \
  auto var = rhs;                                \
  ASSERT_OK(var);                                \
  lhs = std::move(var.value());

#endif
