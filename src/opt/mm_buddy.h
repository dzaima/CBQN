#include "gc.h"

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
#define BSZ(X) (1ull<<(X))
#define  BN(X) mm_##X
#include "mm_buddyTemplate.h"


#define LOG2(X) ((u8)(64-CLZ((X)-1ull)))

#if !ALLOC_NOINLINE || ALLOC_IMPL || ALLOC_IMPL_ALWAYS
ALLOC_FN void* mm_alloc(u64 sz, u8 type) {
  assert(sz>=16);
  onAlloc(sz, type);
  return mm_allocL(LOG2(sz), type);
}
#endif

static u64 mm_round(usz sz) {
  return BSZ(LOG2(sz));
}
static u64 mm_size(Value* x) {
  return BSZ(x->mmInfo&63);
}
void mm_forHeap(V2v f);
void mm_dumpHeap(FILE* f);

#undef LOG2
#undef BN
#undef BSZ
