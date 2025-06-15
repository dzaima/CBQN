#define MM_C 1
#include "../core.h"
#include "../utils/mem.h"

static u64 prepAllocSize(u64 sz) {
  u64 psz = getPageSize();
  u64 minTotPad = ALLOC_PADDING*2 + 128;
  if (psz < minTotPad) psz = minTotPad;
  return sz + psz;
}
#define MMAP(SZ) mmap(NULL, SZ, PROT_READ|PROT_WRITE, MAP_NORESERVE|MAP_PRIVATE|MAP_ANONYMOUS, -1, 0)

GLOBAL bool mem_log_enabled;

#if MM==0
  #include "../opt/mm_malloc.c"
#elif MM==1
  #include "../opt/mm_buddy.c"
#elif MM==2
  #include "../opt/mm_2buddy.c"
#else
  #error "bad MM value"
#endif
#ifndef CLANGD
#undef MMAP
#endif
