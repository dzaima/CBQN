#include "gc.c"

#ifdef OBJ_COUNTER
  u64 currObjCounter;
#endif

EmptyValue* b1_buckets[64];
#define  ALSZ   20
#define  BSZ(X) (1ull<<(X))
#define  MMI(X) X
#define   BN(X) b1_##X
#include "mm_buddyTemplate.c"
#undef BN
#undef BSZ

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
