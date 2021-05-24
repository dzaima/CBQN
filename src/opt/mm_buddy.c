#include "gc.c"

#ifdef OBJ_COUNTER
u64 currObjCounter;
#endif

EmptyValue* buckets[64];
mm_AllocInfo* mm_al;
u64 mm_alCap;
u64 mm_alSize;
