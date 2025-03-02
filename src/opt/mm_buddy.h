#include "gc.h"

typedef struct EmptyValue EmptyValue;
struct EmptyValue { // needs set: mmInfo; type=t_empty; next; everything else can be garbage
  struct Value;
  EmptyValue* next;
};
#if OBJ_COUNTER
  extern GLOBAL u64 currObjCounter;
#endif
extern GLOBAL u64 mm_heapAlloc;
extern GLOBAL u64 mm_heapMax;

extern GLOBAL u64 mm_ctrs[64];
extern GLOBAL EmptyValue* mm_buckets[64];
#define BSZ(X) (1ull<<(X))
#define  BN(X) mm_##X
#include "mm_buddyTemplate.h"


#define LOG2(X) ((u8)(64-CLZ((X)-1ull)))

#if !ALLOC_NOINLINE || ALLOC_IMPL || ALLOC_IMPL_MMX
ALLOC_FN void* mm_alloc(u64 sz, u8 type) {
  assert(sz>=16);
  NOGC_CHECK("allocating during noalloc");
  preAlloc(sz, type);
  #if VERIFY_TAIL
    ux logAlloc = LOG2(sz + VERIFY_TAIL);
  #else
    ux logAlloc = LOG2(sz);
  #endif
  void* res = mm_allocL(logAlloc, type);
  #if VERIFY_TAIL && !ALLOC_IMPL_MMX
    tailVerifyAlloc(res, sz, logAlloc, type);
  #endif
  return res;
}
#endif

static u64 mm_round(usz sz) {
  return BSZ(LOG2(sz));
}
static u64 mm_size(Value* x) {
  return BSZ(x->mmInfo&63);
}
#if VERIFY_TAIL
static u64 mm_sizeUsable(Value* x) {
  return mm_size(x) - VERIFY_TAIL;
}
#endif
void mm_forHeap(V2v f);
void mm_dumpHeap(FILE* f);

#undef LOG2
#undef BN
#undef BSZ
