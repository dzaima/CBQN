#include "h.h"
#include<stdlib.h>

#ifdef ALLOC_STAT
u64* ctr_a = 0;
u64* ctr_f = 0;
u64 actrc = 21000;
u32** actrs;
#endif

void* aalloc(usz sz) { // actual allocate
  void* p = malloc(sz);
  return p;
}

void* mm_allocN(usz sz, u8 type) {
  Value* x = aalloc(sz);
  #ifdef ALLOC_STAT
    if (!actrs) {
      actrs = malloc(sizeof(u32*)*actrc);
      ctr_a = calloc(Type_MAX, sizeof(u64));
      ctr_f = calloc(Type_MAX, sizeof(u64));
      for (i32 i = 0; i < actrc; i++) actrs[i] = calloc(Type_MAX, sizeof(u32));
    }
    assert(type<Type_MAX);
    actrs[(sz+3)/4>=actrc? actrc-1 : (sz+3)/4][type]++;
    ctr_a[type]++;
  #endif
  #ifdef DEBUG
    memset(x, 'a', sz);
  #endif
  x->refc = 1;
  x->flags = 0;
  x->type = type;
  return x;
}
B mm_alloc(usz sz, u8 type, u64 tag) {
  assert(tag>1LL<<16); // make sure it's `ftag`ged :|
  return b((u64)mm_allocN(sz,type) | tag);
}
void mm_free(Value* x) {
  #ifdef ALLOC_STAT
    ctr_f[x->type]++;
    x->refc = 0x61616161;
  #endif
  free(x);
}
void mm_visit(B x) {
  
}