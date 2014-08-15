/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2013 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef ALLOY_MEMORY_H_
#define ALLOY_MEMORY_H_

#include <alloy/core.h>

namespace alloy {

enum {
  MEMORY_FLAG_64KB_PAGES = (1 << 1),
  MEMORY_FLAG_ZERO = (1 << 2),
  MEMORY_FLAG_PHYSICAL = (1 << 3),
};

class Memory {
 public:
  Memory();
  virtual ~Memory();

  inline uint8_t* membase() const { return membase_; }
  inline uint8_t* Translate(uint64_t guest_address) const {
    return membase_ + guest_address;
  };
  inline uint32_t* reserve_address() { return &reserve_address_; }

  virtual uint64_t page_table() const = 0;

  virtual int Initialize();

  void Zero(uint64_t address, size_t size);
  void Fill(uint64_t address, size_t size, uint8_t value);
  void Copy(uint64_t dest, uint64_t src, size_t size);

  uint64_t SearchAligned(uint64_t start, uint64_t end, const uint32_t* values,
                         size_t value_count);

  virtual uint8_t LoadI8(uint64_t address) = 0;
  virtual uint16_t LoadI16(uint64_t address) = 0;
  virtual uint32_t LoadI32(uint64_t address) = 0;
  virtual uint64_t LoadI64(uint64_t address) = 0;
  virtual void StoreI8(uint64_t address, uint8_t value) = 0;
  virtual void StoreI16(uint64_t address, uint16_t value) = 0;
  virtual void StoreI32(uint64_t address, uint32_t value) = 0;
  virtual void StoreI64(uint64_t address, uint64_t value) = 0;

  virtual uint64_t HeapAlloc(uint64_t base_address, size_t size, uint32_t flags,
                             uint32_t alignment = 0x20) = 0;
  virtual int HeapFree(uint64_t address, size_t size) = 0;

  virtual size_t QueryInformation(uint64_t base_address, MEMORY_BASIC_INFORMATION mem_info) = 0;
  virtual size_t QuerySize(uint64_t base_address) = 0;

  virtual int Protect(uint64_t address, size_t size, uint32_t access) = 0;
  virtual uint32_t QueryProtect(uint64_t address) = 0;

 protected:
  size_t system_page_size_;
  uint8_t* membase_;
  uint32_t reserve_address_;
};

}  // namespace alloy

#endif  // ALLOY_MEMORY_H_
