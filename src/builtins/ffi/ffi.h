#pragma once
#include "core.h"

#if (!defined(FFI_CHECKS) && FFI) || FFI_CHECKS // enable or disable correctness checks of •FFI
  #define FFI_CHECKS 1
  // mode:
  //   2: non-reused memory blocks (massive overhead, 3-4 syscalls per buffer per call)
  //   (no other modes are implemented)
  // alignment:
  //   1: align to end (i.e. detect accesses past the end)
  //   2: align to start (i.e. detect accesses before the start)
  //   3: random
  bool set_ffi_check(u8 mode, u8 alignment);
  extern GLOBAL u16 ffi_extra_checks; // mode<<8 | alignment
#else
  static bool set_ffi_check(u8 mode, u8 alignment) { return false; }
  static const u16 ffi_extra_checks = 0;
#endif
