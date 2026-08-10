#pragma once
#include "core.h"

ux getPageSize(void);
#define ALLOC_PADDING 1024 // number of accessible padding bytes always required around an object

typedef struct AllocInfo {
  Value* p;
  u64 sz;
} AllocInfo;
typedef void (*AllocRangeConsumer)(void* start, ux size, const char* name, void* extra, void* data);
void forAllMemoryRanges(AllocRangeConsumer f, void* data);

typedef void (*AllocRangeProducer)(AllocRangeConsumer f, void* data);
void addMemoryRangeSource(AllocRangeProducer f);

#if __has_include(<sys/mman.h>) && __has_include(<unistd.h>) && !WASM && !NO_MMAP
  #include <sys/mman.h>
  static void* mmap_anon(void* addr, ux size, ux prot, ux flags) {
    return mmap(addr, size, prot, MAP_PRIVATE|MAP_ANONYMOUS|flags, -1, 0);
  }
  #define HAS_MMAP 1
#endif
