#include "gc.c"

#ifdef OBJ_COUNTER
  u64 currObjCounter;
#endif

u64 b1_ctrs[64];
EmptyValue* b1_buckets[64];
#define  ALSZ   20
#define  BSZ(X) (1ull<<(X))
#define  MMI(X) X
#define   BN(X) b1_##X
#include "mm_buddyTemplate.c"
#undef BN
#undef BSZ

u64 b3_ctrs[64];
EmptyValue* b3_buckets[64];
#define  ALSZ   20
#define  BSZ(X) (3ull<<(X))
#define  MMI(X) ((X)|64)
#define   BN(X) b3_##X
#include "mm_buddyTemplate.c"
#undef BN
#undef BSZ

void mm_forHeap(V2v f) {
  b1_forHeap(f);
  b3_forHeap(f);
}

u64 mm_heapUsed() {
  u64 r = 0;
  for (i32 i = 0; i < 64; i++) r+= b1_ctrs[i] * (1ull<<i);
  for (i32 i = 0; i < 64; i++) r+= b3_ctrs[i] * (3ull<<i);
  return r;
}
