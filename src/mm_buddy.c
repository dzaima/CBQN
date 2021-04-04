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

EmptyValue* buckets[64];

#define BSZ(x) (1ull<<(x))

EmptyValue* makeEmpty(u8 bucket) { // result->next is garbage
  u8 cb = bucket;
  EmptyValue* c;
  while (true) {
    cb++;
    if (buckets[cb]) {
      c = buckets[cb];
      assert((c->mmInfo&63)==cb);
      buckets[cb] = c->next;
      break;
    }
    if (cb >= 20) {
      c = mmap(NULL, BSZ(cb), PROT_READ|PROT_WRITE, MAP_NORESERVE|MAP_PRIVATE|MAP_ANON, -1, 0);
      if (c==MAP_FAILED) {
        printf("failed to allocate memory\n");
        exit(1);
      }
      c->type = t_empty;
      c->mmInfo = cb;
      c->next = 0;
      break;
    }
  }
  while (cb != bucket) {
    cb--;
    EmptyValue* b = (EmptyValue*) (((char*)c)+BSZ(cb));
    b->type = t_empty;
    b->mmInfo = cb;
    b->next = 0; assert(buckets[cb]==0);
    buckets[cb] = b;
  }
  c->mmInfo = bucket;
  return c;
}

void mm_free(Value* x) {
  onFree(x);
  EmptyValue* c = (EmptyValue*) x;
  c->type = t_empty;
  u8 b = c->mmInfo&63;
  c->next = buckets[b];
  buckets[b] = c;
}

void* mm_allocN(usz sz, u8 type) {
  assert(sz>8);
  u8 bucket = 64-__builtin_clzl(sz-1ull); // inverse of BSZ
  EmptyValue* x = buckets[bucket];
  if (x==NULL) x = makeEmpty(bucket);
  else buckets[bucket] = x->next;
  onAlloc(sz, type);
  x->flags = x->extra = x->type = 0;
  x->refc = 1;
  x->type = type;
  return x;
}

void mm_visit(B x) {
  
}
