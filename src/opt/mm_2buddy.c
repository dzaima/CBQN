#include "gc.c"

#ifdef OBJ_COUNTER
u64 currObjCounter;
#endif

#define  ALSZ   20
#define  BSZ(X) (1ull<<(X))
#define  MMI(X) X
#define   BN(X) b1_##X
#define buckets b1_buckets
#include "mm_buddyTemplate.c"
#undef buckets
#undef BN
#undef BSZ

#define  ALSZ   20
#define  BSZ(X) (3ull<<(X))
#define  MMI(X) ((X)|64)
#define   BN(X) b3_##X
#define buckets b3_buckets
#include "mm_buddyTemplate.c"
#undef buckets
#undef BN
#undef BSZ

void mm_forHeap(V2v f) {
  b1_forHeap(f);
  b3_forHeap(f);
}
