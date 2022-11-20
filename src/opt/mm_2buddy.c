#include "../core.h"
#if !NO_MMAP
#include <sys/mman.h>
#endif
#include "gc.c"

#ifdef OBJ_COUNTER
  u64 currObjCounter;
#endif
#if VERIFY_TAIL
  #error MM=2 doesn't support VERIFY_TAIL
#endif

u64 mm_ctrs[128];
EmptyValue* mm_buckets[128];
#define b1_buckets mm_buckets
#define b1_allocL mm_allocL
#define  ALSZ   20
#define  BSZ(X) (1ull<<(X))
#define  MMI(X) X
#define   BN(X) b1_##X
#include "mm_buddyTemplate.c"
#undef BN
#undef BSZ

#define b3_buckets (mm_buckets+64)
#define b3_allocL mm_allocL
#define  ALSZ   20
#define  BSZ(X) (3ull<<(X))
#define  MMI(X) ((X)|64)
#define   BN(X) b3_##X
#include "mm_buddyTemplate.c"
#undef BN
#undef BSZ

NOINLINE void* mm_allocS(i64 bucket, u8 type) {
  return bucket&64? b3_allocS(bucket, type) : b1_allocS(bucket, type);
}

void mm_forHeap(V2v f) {
  b1_forHeap(f);
  b3_forHeap(f);
}
void mm_forFreedHeap(V2v f) {
  b1_forFreedHeap(f);
  b3_forFreedHeap(f);
}
void mm_dumpHeap(FILE* f) {
  b1_dumpHeap(f);
  b3_dumpHeap(f);
}

u64 mm_heapUsed() {
  u64 r = 0;
  for (i32 i = 0; i < 64; i++) r+= mm_ctrs[i   ] * (1ull<<i);
  for (i32 i = 0; i < 64; i++) r+= mm_ctrs[i+64] * (3ull<<i);
  return r;
}
