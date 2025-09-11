// Copyright 2020 The Abseil Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Modifications:
// Copyright 2025 William Henning
//   This is a re-implementation of a subset of the full Abseil status APIs.
//   It's not clear to me whether this should be considered a derived or
//   original work, so I've retained Abseil's Apache license.
// If you want to reference the original StatusOr implementation. See:
//   https://source.chromium.org/chromium/chromium/src/+/main:third_party/abseil-cpp/absl/status/statusor.h;l=192?ss=chromium%2Fchromium%2Fsrc:third_party%2Fabseil-cpp%2F

#ifndef STATUS_OR_H_
#define STATUS_OR_H_

#include <cstdlib>
#include <type_traits>

#include "logger.h"

enum class StatusCode {
  OK = 0,
  UNKNOWN = 2,
  INVALID_ARGUMENT = 3,
  DEADLINE_EXCEEDED = 4,
  NOT_FOUND = 5,
  // We use INTERNAL errors for cases where we catch a C api error, but don't
  // parse its errno into a specific error type.
  INTERNAL = 13,
  UNAVAILABLE = 14,
};

class StatusVal {
 public:
  explicit StatusVal(StatusCode code, std::string msg = "") : code_(code), msg_(msg) {}

  bool ok() const { return code_ == StatusCode::OK; }
  StatusCode code() const { return code_; }

  // Returns this. Exposed to enable RETURN_IF_ERROR(status_val) to work.
  StatusVal status() { return *this; }

  std::string to_string() const {
    std::string str = "UNKNOWN STATUS CODE";
    switch (code()) {
      case StatusCode::OK:
        str = "OK"; break;
      case StatusCode::UNKNOWN:
        str = "UNKNOWN"; break;
      case StatusCode::INVALID_ARGUMENT:
        str = "INVALID_ARGUMENT"; break;
      case StatusCode::DEADLINE_EXCEEDED:
        str = "DEADLINE_EXCEEDED"; break;
      case StatusCode::NOT_FOUND:
        str = "NOT_FOUND"; break;
      case StatusCode::INTERNAL:
        str = "INTERNAL"; break;
      case StatusCode::UNAVAILABLE:
        str = "UNAVAILABLE"; break;
    }
    if (msg_.size()) {
      str += ":" + msg_;
    }
    return str;
  }

 private:
  StatusCode code_ = StatusCode::OK;
  std::string msg_ = "";
};

inline StatusVal OkStatus(std::string msg="") { return StatusVal(StatusCode::OK, msg); }
inline StatusVal UnknownError(std::string msg="") { return StatusVal(StatusCode::UNKNOWN, msg); }
inline StatusVal InvalidArgumentError(std::string msg="") { return StatusVal(StatusCode::INVALID_ARGUMENT, msg); }
inline StatusVal DeadlineExceededError(std::string msg="") { return StatusVal(StatusCode::DEADLINE_EXCEEDED, msg); }
inline StatusVal NotFoundError(std::string msg="") { return StatusVal(StatusCode::NOT_FOUND, msg); }
inline StatusVal InternalError(std::string msg="") { return StatusVal(StatusCode::INTERNAL, msg); }
inline StatusVal UnavailableError(std::string msg="") { return StatusVal(StatusCode::UNAVAILABLE, msg); }

template <typename T>
class StatusOr {
 public:
  // Construct StatusOr with T values.
  StatusOr(const T& value) : status_(StatusVal(StatusCode::OK)) {
    new (&value_) T(value);
  }

  StatusOr(T&& value) : status_(StatusVal(StatusCode::OK)) {
    new (&value_) T(std::move(value));
  }

  // Construct StatusOr with a StatusVal.
  StatusOr(const StatusVal& status) : status_(status) {}
  StatusOr(StatusVal&& status) : status_(std::move(status)) {}

  // Construct StatusOr with a StatusCode.
  explicit StatusOr(StatusCode code) : status_(StatusVal(code)) {}

  StatusOr(const StatusOr& other) : status_(other.status_) {
    if (other.has_value()) {
      new (&value_) T(other.value_);
    }
  }

  StatusOr(StatusOr&& other) : status_(std::move(other.status_)) {
    if (other.has_value()) {
      new (&value_) T(std::move(other.value_));
    }
  }

  StatusOr& operator=(const StatusOr& other) {
    if (this != &other) {
      if (has_value()) {
        value_.~T();
      }
      status_ = other.status_;
      if (other.has_value()) {
        new (&value_) T(other.value_);
      }
    }
    return *this;
  }

  StatusOr& operator=(StatusOr&& other) {
    if (this != &other) {
      if (has_value()) {
        value_.~T();
      }
      status_ = std::move(other.status_);
      if (other.has_value()) {
        new (&value_) T(std::move(other.value_));
      }
    }
    return *this;
  }

  ~StatusOr() {
    if (has_value()) {
      value_.~T();
    }
  }

  bool ok() const { return status_.ok(); }
  const StatusVal& status() const { return status_; }

  // value() is undefined if this StatusOr doesn't hold a value.
  T& value() { return value_; }
  const T& value() const { return value_; }

  // Dereference results are undefined if this StatusOr doesn't hold a va/lue.
  T& operator*() { return value_; }
  const T& operator*() const { return value_; }
  T* operator->() { return &value_; }
  const T* operator->() const { return &value_; }

  // Returns value if present, otherwise ERROR() logs and exit(1)
  T& value_or_die() {
    if (!has_value()) {
      std::string status_str = status().to_string();
      ERROR("value_or_die() called on error status: %s", status_str.c_str());
      exit(1);
    }
    return value_;
  }

 private:
  bool has_value() const { return ok(); }

  StatusVal status_ = StatusVal(StatusCode::UNKNOWN);
  union {
    T value_;
  };
};

#define RETURN_IF_ERROR(expr)    \
  do {                           \
    auto&& status_or = (expr);   \
    if (!status_or.ok()) {       \
      return status_or.status(); \
    }                            \
  } while (0)


#define ASSIGN_OR_RETURN(lhs, rhs) \
  auto v = rhs;                    \
  if (!v.ok()) return v.status();  \
  lhs = *v;

#endif  // STATUS_OR_H_
