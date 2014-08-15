/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2013 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include <alloy/runtime/thread_state.h>

#include <alloy/runtime/runtime.h>
#include <poly/poly.h>

namespace alloy {
namespace runtime {

thread_local ThreadState* thread_state_ = nullptr;

ThreadState::ThreadState(Runtime* runtime, uint32_t thread_id)
    : runtime_(runtime),
      memory_(runtime->memory()),
      thread_id_(thread_id),
      name_(""),
      backend_data_(0),
      raw_context_(0) {
  if (thread_id_ == UINT_MAX) {
    // System thread. Assign the system thread ID with a high bit
    // set so people know what's up.
    uint32_t system_thread_handle = poly::threading::current_thread_id();
    thread_id_ = 0x80000000 | system_thread_handle;
  }
  backend_data_ = runtime->backend()->AllocThreadData();
}

ThreadState::~ThreadState() {
  if (backend_data_) {
    runtime_->backend()->FreeThreadData(backend_data_);
  }
  if (thread_state_ == this) {
    thread_state_ = nullptr;
  }
}

void ThreadState::Bind(ThreadState* thread_state) {
  thread_state_ = thread_state;
}

ThreadState* ThreadState::Get() { return thread_state_; }

uint32_t ThreadState::GetThreadID() { return thread_state_->thread_id_; }

}  // namespace runtime
}  // namespace alloy
