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
#define BSZI2(x) (64-__builtin_clzl((x)-1ull))
#define BSZ(x) (1ull<<(x))
#define BSZI BSZI2
#define MMI(x) x

u64 mm_round(usz sz) {
  return BSZ(BSZI(sz));
}

void mm_visit(B x) {
  
}

#include "mm_buddyTemplate.c"

void* mm_allocN(usz sz, u8 type) {
  assert(sz>8);
  onAlloc(sz, type);
  return mm_allocL(BSZI2(sz), type);
}
