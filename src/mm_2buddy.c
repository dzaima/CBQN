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
#define   BN(X) b1_##X
#define buckets b1_buckets
#include "mm_buddyTemplate.c"
#undef buckets
#undef BN
#undef BSZ
#undef BSZI

#define  BSZ(X) (3ull<<(X))
#define BSZI(X) (64-__builtin_clzl((X)/3-1ull))
#define  MMI(X) ((X)|64)
#define   BN(X) b3_##X
#define buckets b3_buckets
#include "mm_buddyTemplate.c"
#undef buckets
#undef BN
#undef BSZ
#undef BSZI
#ifdef OBJ_COUNTER
u64 currObjCounter;
#endif
void* mm_allocN(usz sz, u8 type) {
  assert(sz>=16);
  onAlloc(sz, type);
  u8 b1 = 64-__builtin_clzl(sz-1ull);
  Value* r;
  if (sz <= (3ull<<(b1-2))) r = b3_allocL(b1-2, type);
  else r = b1_allocL(b1, type);
  #ifdef OBJ_COUNTER
  r->uid = currObjCounter++;
  #endif
  return r;
}
void mm_free(Value* x) {
  if (x->mmInfo&64) b3_free(x);
  else b1_free(x);
}
void mm_forHeap(V2v f) {
  b1_forHeap(f);
  b3_forHeap(f);
}

u64 mm_round(usz x) {
  u8 b1 = 64-__builtin_clzl(x-1ull);
  u64 s3 = 3ull<<(b1-2);
  if (x<=s3) return s3;
  return 1ull<<b1;
}
u64 mm_size(Value* x) {
  u8 m = x->mmInfo;
  if (m&64) return 3ull<<(x->mmInfo&63);
  else      return 1ull<<(x->mmInfo&63);
}
u64 mm_heapAllocated() {
  return b1_heapAllocated() + b3_heapAllocated();
}
