/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2014 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef ALLOY_BACKEND_X64_X64_SEQUENCES_H_
#define ALLOY_BACKEND_X64_X64_SEQUENCES_H_

#include <alloy/core.h>

XEDECLARECLASS2(alloy, hir, Instr);

namespace alloy {
namespace backend {
namespace x64 {

class X64Emitter;

void RegisterSequences();
bool SelectSequence(X64Emitter& e, const hir::Instr* i,
                    const hir::Instr** new_tail);

}  // namespace x64
}  // namespace backend
}  // namespace alloy

#endif  // ALLOY_BACKEND_X64_X64_SEQUENCES_H_
