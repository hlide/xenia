/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2013 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef ALLOY_BACKEND_IVM_IVM_ASSEMBLER_H_
#define ALLOY_BACKEND_IVM_IVM_ASSEMBLER_H_

#include <alloy/core.h>

#include <alloy/arena.h>
#include <alloy/backend/assembler.h>

namespace alloy {
namespace backend {
namespace ivm {

class IVMAssembler : public Assembler {
 public:
  IVMAssembler(Backend* backend);
  ~IVMAssembler() override;

  int Initialize() override;

  void Reset() override;

  int Assemble(runtime::FunctionInfo* symbol_info, hir::HIRBuilder* builder,
               uint32_t debug_info_flags,
               std::unique_ptr<runtime::DebugInfo> debug_info,
               runtime::Function** out_function) override;

 private:
  Arena intcode_arena_;
  Arena source_map_arena_;
  Arena scratch_arena_;
};

}  // namespace ivm
}  // namespace backend
}  // namespace alloy

#endif  // ALLOY_BACKEND_IVM_IVM_ASSEMBLER_H_
