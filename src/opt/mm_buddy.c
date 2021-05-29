#include "gc.c"

#ifdef OBJ_COUNTER
u64 currObjCounter;
#endif

#define  BSZ(X) (1ull<<(X))
#define BSZI(X) ((u8)(64-__builtin_clzl((X)-1ull)))
#define  MMI(X) X
#define   BN(X) mm_##X

#include "mm_buddyTemplate.c"
