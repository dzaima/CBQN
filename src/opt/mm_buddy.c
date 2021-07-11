#include "gc.c"

#ifdef OBJ_COUNTER
  u64 currObjCounter;
#endif

EmptyValue* mm_buckets[64];
#define  ALSZ   20
#define  BSZ(X) (1ull<<(X))
#define  MMI(X) X
#define   BN(X) mm_##X
#include "mm_buddyTemplate.c"
#undef BN
#undef BSZ
