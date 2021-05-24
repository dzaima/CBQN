#include "gc.c"

#ifdef OBJ_COUNTER
u64 currObjCounter;
#endif

EmptyValue* b1_buckets[64];
b1_AllocInfo* b1_al;
u64 b1_alCap;
u64 b1_alSize;
EmptyValue* b3_buckets[64];
b3_AllocInfo* b3_al;
u64 b3_alCap;
u64 b3_alSize;
