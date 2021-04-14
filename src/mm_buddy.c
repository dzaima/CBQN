#include "h.h"
#include "gc.c"
#include <sys/mman.h>

#ifndef MAP_NORESERVE
 #define MAP_NORESERVE 0 // apparently needed for freebsd or something
#endif

typedef struct EmptyValue EmptyValue;
struct EmptyValue { // needs set: mmInfo; type=t_empty; next; everything else can be garbage
  struct Value;
  EmptyValue* next;
};

#define  BSZ(X) (1ull<<(X))
#define BSZI(X) (64-__builtin_clzl((X)-1ull))
#define  MMI(X) X
#define   BN(X) mm_##X

#include "mm_buddyTemplate.c"

#ifdef OBJ_COUNTER
u64 currObjCounter;
#endif
void* mm_allocN(usz sz, u8 type) {
  assert(sz>=16);
  onAlloc(sz, type);
  Value* r = mm_allocL(BSZI(sz), type);
  #ifdef OBJ_COUNTER
  r->uid = currObjCounter++;
  #endif
  return r;
}

u64 mm_round(usz sz) {
  return BSZ(BSZI(sz));
}
u64 mm_size(Value* x) {
  return BSZ(x->mmInfo&63);
}

#undef BSZ
#undef BSZI