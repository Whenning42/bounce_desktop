#ifndef EXCEPTIONS_H_
#define EXCEPTIONS_H_

#include "status_or.h"

void raise_status(const StatusVal& status) {
  if (!status.ok()) {
    if (status.code() == StatusCode::INVALID_ARGUMENT) {
      throw std::invalid_argument(status.to_string());
    } else {
      throw std::runtime_error(status.to_string());
    }
  }
}

#define RAISE_IF_ERROR(expr)            \
  do {                                  \
    auto&& status_or = (expr);          \
    if (!status_or.ok()) {              \
      raise_status(status_or.status()); \
    }                                   \
  } while (0)

#define CAT(a, b) a##b
#define CAT_(a, b) CAT(a, b)
#define UNIQ(v) CAT_(v, __COUNTER__)

#define ASSIGN_OR_RAISE(lhs, rhs) ASSIGN_OR_RAISE_IMPL(UNIQ(v), lhs, rhs)

#define ASSIGN_OR_RAISE_IMPL(var, lhs, rhs)  \
  auto var = rhs;                            \
  if (!var.ok()) raise_status(var.status()); \
  lhs = std::move(var.value());

#endif
