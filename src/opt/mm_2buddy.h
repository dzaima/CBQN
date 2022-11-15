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

extern u64 mm_ctrs[128];
extern EmptyValue* mm_buckets[128];
#define BSZ(X) (((X)&64? 3ull : 1ull)<<(X))
#define  BN(X) mm_##X
#include "mm_buddyTemplate.h"


#define LOG2(X) ((u8)(64-CLZ((X)-1ull)))

#if !ALLOC_NOINLINE || ALLOC_IMPL || ALLOC_IMPL_MMX
static void* mm_alloc(u64 sz, u8 type) {
  assert(sz>=16);
  u32 log = LOG2(sz);
  u32 logm2 = log-2;
  bool b2 = sz <= (3ull<<logm2);
  return mm_allocL(b2? logm2|64 : log, type);
}
#endif

static u64 mm_round(usz x) {
  u8 log = LOG2(x);
  u64 s3 = 3ull<<(log-2);
  if (x<=s3) return s3;
  return 1ull<<log;
}
static u64 mm_size(Value* x) {
  u8 m = x->mmInfo;
  if (m&64) return 3ull<<(x->mmInfo&63);
  else      return 1ull<<(x->mmInfo&63);
}
void mm_forHeap(V2v f);
void mm_dumpHeap(FILE* f);

#undef BN
#undef BSZ
#undef LOG2