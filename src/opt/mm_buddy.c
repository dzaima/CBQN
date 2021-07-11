#include "gc.c"

#ifdef OBJ_COUNTER
  u64 currObjCounter;
#endif

u64 mm_ctrs[64];
EmptyValue* mm_buckets[64];
#define  ALSZ   20
#define  BSZ(X) (1ull<<(X))
#define  MMI(X) X
#define   BN(X) mm_##X
#include "mm_buddyTemplate.c"

u64 mm_heapUsed() {
  u64 r = 0;
  for (i32 i = 0; i < 64; i++) r+= mm_ctrs[i]*BSZ(i);
  return r;
}
#undef BN
#undef BSZ
