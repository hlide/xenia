/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2014 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef ALLOY_BACKEND_MACHINE_INFO_H_
#define ALLOY_BACKEND_MACHINE_INFO_H_

#include <alloy/core.h>

namespace alloy {
namespace backend {

struct MachineInfo {
  struct RegisterSet {
    enum Types {
      INT_TYPES = (1 << 1),
      FLOAT_TYPES = (1 << 2),
      VEC_TYPES = (1 << 3),
    };
    uint8_t id;
    char name[4];
    uint32_t types;
    uint32_t count;
  } register_sets[8];
};

}  // namespace backend
}  // namespace alloy

#endif  // ALLOY_BACKEND_MACHINE_INFO_H_
