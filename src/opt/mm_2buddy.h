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

extern EmptyValue* b1_buckets[64];
#define  BSZ(X) (1ull<<(X))
#define   BN(X) b1_##X
#include "mm_buddyTemplate.h"
#undef BN
#undef BSZ

extern EmptyValue* b3_buckets[64];
#define  BSZ(X) (3ull<<(X))
#define   BN(X) b3_##X
#include "mm_buddyTemplate.h"
#undef BN
#undef BSZ


static void* mm_alloc(usz sz, u8 type) {
  assert(sz>=16);
  onAlloc(sz, type);
  u8 b1 = 64-__builtin_clzl(sz-1ull);
  if (sz <= (3ull<<(b1-2))) return b3_allocL(b1-2, type);
  else return b1_allocL(b1, type);
}
static void mm_free(Value* x) {
  if (x->mmInfo&64) b3_free(x);
  else b1_free(x);
}

static u64 mm_round(usz x) {
  u8 b1 = 64-__builtin_clzl(x-1ull);
  u64 s3 = 3ull<<(b1-2);
  if (x<=s3) return s3;
  return 1ull<<b1;
}
static u64 mm_size(Value* x) {
  u8 m = x->mmInfo;
  if (m&64) return 3ull<<(x->mmInfo&63);
  else      return 1ull<<(x->mmInfo&63);
}
void mm_forHeap(V2v f);
