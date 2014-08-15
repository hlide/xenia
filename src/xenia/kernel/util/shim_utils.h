/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2013 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_KERNEL_UTIL_SHIM_UTILS_H_
#define XENIA_KERNEL_UTIL_SHIM_UTILS_H_

#include <xenia/common.h>
#include <xenia/core.h>

#include <alloy/frontend/ppc/ppc_context.h>
#include <xenia/export_resolver.h>


namespace xe {
namespace kernel {

using PPCContext = alloy::frontend::ppc::PPCContext;


#define SHIM_CALL             void _cdecl
#define SHIM_SET_MAPPING(library_name, export_name, shim_data) \
  export_resolver->SetFunctionMapping( \
      library_name, ordinals::##export_name, \
      shim_data, \
      (xe_kernel_export_shim_fn)export_name##_shim, \
      nullptr);

#define SHIM_MEM_BASE         ppc_state->membase
#define SHIM_MEM_ADDR(a)      (a ? (ppc_state->membase + a) : nullptr)

#define SHIM_MEM_8(a)         poly::load_and_swap<uint8_t>(SHIM_MEM_ADDR(a))
#define SHIM_MEM_16(a)        poly::load_and_swap<uint16_t>(SHIM_MEM_ADDR(a))
#define SHIM_MEM_32(a)        poly::load_and_swap<uint32_t>(SHIM_MEM_ADDR(a))
#define SHIM_MEM_64(a)        poly::load_and_swap<uint64_t>(SHIM_MEM_ADDR(a))
#define SHIM_SET_MEM_8(a, v)  poly::store_and_swap<uint8_t>(SHIM_MEM_ADDR(a), v)
#define SHIM_SET_MEM_16(a, v) poly::store_and_swap<uint16_t>(SHIM_MEM_ADDR(a), v)
#define SHIM_SET_MEM_32(a, v) poly::store_and_swap<uint32_t>(SHIM_MEM_ADDR(a), v)
#define SHIM_SET_MEM_64(a, v) poly::store_and_swap<uint64_t>(SHIM_MEM_ADDR(a), v)
#define SHIM_SET_MEM_F32(a, v) poly::store_and_swap<float>(SHIM_MEM_ADDR(a), v)
#define SHIM_SET_MEM_F64(a, v) poly::store_and_swap<double>(SHIM_MEM_ADDR(a), v)

#define SHIM_GPR_8(n)         (uint8_t)(ppc_state->r[n])
#define SHIM_GPR_16(n)        (uint16_t)(ppc_state->r[n])
#define SHIM_GPR_32(n)        (uint32_t)(ppc_state->r[n])
#define SHIM_SET_GPR_32(n, v) ppc_state->r[n] = (uint64_t)((v) & UINT32_MAX)
#define SHIM_GPR_64(n)        ppc_state->r[n]
#define SHIM_SET_GPR_64(n, v) ppc_state->r[n] = (uint64_t)(v)

#define SHIM_GET_ARG_8(n)     SHIM_GPR_8(3 + n)
#define SHIM_GET_ARG_16(n)    SHIM_GPR_16(3 + n)
#define SHIM_GET_ARG_32(n)    SHIM_GPR_32(3 + n)
#define SHIM_GET_ARG_64(n)    SHIM_GPR_64(3 + n)
#define SHIM_SET_RETURN_32(v) SHIM_SET_GPR_64(3, (uint64_t)(int32_t)v)
#define SHIM_SET_RETURN_64(v) SHIM_SET_GPR_64(3, v)


#define IMPL_MEM_ADDR(a)      (a ? state->memory()->Translate(a) : nullptr)

#define IMPL_MEM_8(a)         poly::load_and_swap<uint8_t>(IMPL_MEM_ADDR(a))
#define IMPL_MEM_16(a)        poly::load_and_swap<uint16_t>(IMPL_MEM_ADDR(a))
#define IMPL_MEM_32(a)        poly::load_and_swap<uint32_t>(IMPL_MEM_ADDR(a))
#define IMPL_SET_MEM_8(a, v)  poly::store_and_swap<uint8_t>(IMPL_MEM_ADDR(a), v)
#define IMPL_SET_MEM_16(a, v) poly::store_and_swap<uint16_t>(IMPL_MEM_ADDR(a), v)
#define IMPL_SET_MEM_32(a, v) poly::store_and_swap<uint32_t>(IMPL_MEM_ADDR(a), v)


}  // namespace kernel
}  // namespace xe


#endif  // XENIA_KERNEL_UTIL_SHIM_UTILS_H_
