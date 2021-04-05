#include "h.h"
#include <sys/mman.h>

#ifndef MAP_NORESERVE
 #define MAP_NORESERVE 0 // apparently needed for freebsd or something
#endif

typedef struct EmptyValue EmptyValue;
struct EmptyValue { // needs set: mmInfo; type=t_empty; next; everything else can be garbage
  struct Value;
  EmptyValue* next;
};

#define BSZ(x) (1ull<<(x))
#define BSZI(x) (64-__builtin_clzl((x)-1ull))
#define MMI(x) x
#define buckets      b1_buckets
#define mm_free      b1_free
#define mm_makeEmpty b1_makeEmpty
#define mm_allocL    b1_allocL
#include "mm_buddyTemplate.c"
#undef buckets
#undef mm_free
#undef mm_makeEmpty
#undef mm_allocL

#define BSZ(x) ((1ull<<(x))*3)
#define BSZI(x) (64-__builtin_clzl((x/3)-1ull))
#define MMI(x) ((x)|64)
#define buckets      b3_buckets
#define mm_free      b3_free
#define mm_makeEmpty b3_makeEmpty
#define mm_allocL    b3_allocL
#include "mm_buddyTemplate.c"
#undef buckets
#undef mm_free
#undef mm_makeEmpty
#undef mm_allocL

void* mm_allocN(usz sz, u8 type) {
  assert(sz>12);
  onAlloc(sz, type);
  u8 b1 = 64-__builtin_clzl(sz-1ull);
  if (sz <= (1ull<<(b1-2)) * 3) return b3_allocL(b1-2, type);
  return b1_allocL(b1, type);
}
void mm_free(Value* x) {
  if (x->mmInfo&64) b3_free(x);
  else b1_free(x);
}

u64 mm_round(usz sz) {
  u8 b1 = 64-__builtin_clzl(sz-1ull);
  u64 s3 = (1ull<<(b1-2)) * 3;
  if (sz<=s3) return s3;
  return 1ull<<b1;
}