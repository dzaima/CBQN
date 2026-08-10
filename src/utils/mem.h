#pragma once
#include "core.h"

ux getPageSize(void);
#define ALLOC_PADDING 1024 // number of accessible padding bytes always required around an object

typedef struct AllocInfo {
  Value* p;
  u64 sz;
} AllocInfo;

#if __has_include(<sys/mman.h>) && __has_include(<unistd.h>) && !WASM && !NO_MMAP
  #include <sys/mman.h>
  static void* mmap_anon(void* addr, ux size, ux prot, ux flags) {
    return mmap(addr, size, prot, MAP_PRIVATE|MAP_ANONYMOUS|flags, -1, 0);
  }
  #define HAS_MMAP 1
#endif
