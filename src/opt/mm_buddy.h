#include "gc.h"
#include <sys/mman.h>

typedef struct EmptyValue EmptyValue;
struct EmptyValue { // needs set: mmInfo; type=t_empty; next; everything else can be garbage
  struct Value;
  EmptyValue* next;
};
#ifdef OBJ_COUNTER
  extern u64 currObjCounter;
#endif
extern u64 mm_heapAlloc;
extern u64 mm_heapMax;

extern u64 mm_ctrs[64];
extern EmptyValue* mm_buckets[64];
#define  BSZ(X) (1ull<<(X))
#define   BN(X) mm_##X
#include "mm_buddyTemplate.h"


#define BSZI(X) ((u8)(64-__builtin_clzl((X)-1ull)))
static void* mm_alloc(usz sz, u8 type) {
  assert(sz>=16);
  onAlloc(sz, type);
  Value* r = mm_allocL(BSZI(sz), type);
  return r;
}

static u64 mm_round(usz sz) {
  return BSZ(BSZI(sz));
}
static u64 mm_size(Value* x) {
  return BSZ(x->mmInfo&63);
}
void mm_forHeap(V2v f);

#undef BSZI
#undef BN
#undef BSZ
