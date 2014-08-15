/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2014 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef ALLOY_RESET_SCOPE_H_
#define ALLOY_RESET_SCOPE_H_

#include <mutex>

#include <alloy/core.h>

namespace alloy {

template <typename T>
class ResetScope {
 public:
  ResetScope(T* value) : value_(value) {}
  ~ResetScope() {
    if (value_) {
      value_->Reset();
    }
  }

 private:
  T* value_;
};

template <typename T>
inline ResetScope<T> make_reset_scope(T* value) {
  return ResetScope<T>(value);
}

template <typename T>
inline ResetScope<T> make_reset_scope(const std::unique_ptr<T>& value) {
  return ResetScope<T>(value.get());
}

}  // namespace alloy

#endif  // ALLOY_RESET_SCOPE_H_
