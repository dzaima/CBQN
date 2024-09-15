#include "../core.h"
#if !NO_MMAP
#include <sys/mman.h>
#endif
#include "gc.c"

#if OBJ_COUNTER
  GLOBAL u64 currObjCounter;
#endif
#if VERIFY_TAIL
  #error MM=2 doesn't support VERIFY_TAIL
#endif

#define ALLOC_MODE 0
GLOBAL u64 mm_ctrs[128];
GLOBAL EmptyValue* mm_buckets[128];
#define b1_buckets mm_buckets
#define b1_allocL mm_allocL
#define b1_ctrs mm_ctrs
#define  ALSZ   20
#define  BSZ(X) (1ull<<(X))
#define  MUL 1
#define  MMI(X) X
#define   BN(X) b1_##X
#include "mm_buddyTemplate.c"
#undef BN
#undef BSZ

#define b3_buckets (mm_buckets+64)
#define b3_allocL mm_allocL
#define b3_ctrs (mm_ctrs+64)
#define  ALSZ   20
#define  BSZ(X) (3ull<<(X))
#define  MUL 3
#define  MMI(X) ((X)|64)
#define   BN(X) b3_##X
#include "mm_buddyTemplate.c"
#undef BN
#undef BSZ
#undef ALLOC_MODE

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
static void mm_freeFreedAndMerge() {
  b1_freeFreedAndMerge();
  b3_freeFreedAndMerge();
}
void mm_dumpHeap(FILE* f) {
  b1_dumpHeap(f);
  b3_dumpHeap(f);
}

u64 mm_heapUsed() {
  return b1_heapUsed() + b3_heapUsed();
}
