#define MM_C 1
#include "core.h"
#include "utils/mem.h"

static u64 prepAllocSize(u64 sz) {
  u64 psz = getPageSize();
  u64 minTotPad = ALLOC_PADDING*2 + 128;
  if (psz < minTotPad) psz = minTotPad;
  return sz + psz;
}
#define MMAP(SZ) mmap_anon(NULL, SZ, PROT_READ|PROT_WRITE, MAP_NORESERVE)

GLOBAL bool mem_log_enabled;

#if MM==0
  #include "opt/mm_malloc.c"
#elif MM==1
  #include "opt/mm_buddy.c"
#elif MM==2
  #include "opt/mm_2buddy.c"
#else
  #error "bad MM value"
#endif
#ifndef CLANGD
#undef MMAP
#endif


typedef struct AllocProducerChain AllocProducerChain;
struct AllocProducerChain {
  AllocRangeProducer f;
  AllocProducerChain* next;
};
STATIC_GLOBAL AllocProducerChain* allocProducers;
void addMemoryRangeSource(AllocRangeProducer f) {
  AllocProducerChain* prev = allocProducers;
  allocProducers = malloc(sizeof(AllocProducerChain));
  allocProducers->next = prev;
  allocProducers->f = f;
}

void forAllMemoryRanges(AllocRangeConsumer f, void* data) {
  AllocProducerChain* c = allocProducers;
  while (c) {
    c->f(f, data);
    c = c->next;
  }
}
