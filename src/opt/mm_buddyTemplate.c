#include "../utils/file.h"
#include "../utils/interrupt.h"
#define AllocInfo BN(AllocInfo)
#define buckets   BN(buckets)
#define al        BN(al)
#define alCap     BN(alCap)
#define alSize    BN(alSize)
#ifndef MMAP
  usz getPageSize();
  #define MMAP(SZ) mmap(NULL, (SZ)+getPageSize(), PROT_READ|PROT_WRITE, MAP_NORESERVE|MAP_PRIVATE|MAP_ANONYMOUS, -1, 0)
#endif

typedef struct AllocInfo {
  Value* p;
  u64 sz;
} AllocInfo;
AllocInfo* al;
u64 alCap;
u64 alSize;

FORCE_INLINE void BN(splitTo)(EmptyValue* c, i64 from, i64 to, bool notEqual) {
  c->mmInfo = MMI(to);
  NOUNROLL while (from != to) {
    from--;
    EmptyValue* b = (EmptyValue*) (BSZ(from) + (u8*)c);
    b->type = t_empty;
    b->mmInfo = MMI(from);
    b->next = buckets[from];
    buckets[from] = b;
  }
  c->next = buckets[from];
  buckets[from] = c;
}

static NOINLINE void* BN(allocateMore)(i64 bucket, u8 type, i64 from, i64 to) {
  u64 sz = BSZ(from);
  if (mm_heapAlloc+sz >= mm_heapMax) thrOOM();
  CHECK_INTERRUPT;
  mm_heapAlloc+= sz;
  // gc_maybeGC();
  #if NO_MMAP
    EmptyValue* c = calloc(sz+getPageSize(), 1);
  #else
    EmptyValue* c = MMAP(sz);
    if (c==MAP_FAILED) thrOOM();
  #endif
  #ifdef USE_VALGRIND
    VALGRIND_MAKE_MEM_UNDEFINED(c, sz);
  #endif
  if (alSize+1>=alCap) {
    alCap = alCap? alCap*2 : 1024;
    al = realloc(al, sizeof(AllocInfo)*alCap);
  }
  al[BN(alSize)++] = (AllocInfo){.p = (Value*)c, .sz = sz};
  c->type = t_empty;
  c->mmInfo = from;
  c->next = 0;
  BN(splitTo)(c, from, to, false);
  return BN(allocL)(bucket, type);
}

NOINLINE void* BN(allocS)(i64 bucket, u8 type) {
  i64 to = bucket&63;
  i64 from = to;
  EmptyValue* c;
  while (true) {
    from++;
    if (buckets[from]) {
      c = buckets[from];
      assert((c->mmInfo&63)==from);
      buckets[from] = c->next;
      break;
    }
    if (from >= ALSZ) return BN(allocateMore)(bucket, type, from, to);
  }
  BN(splitTo)(c, from, to, true);
  return BN(allocL)(bucket, type);
}

void BN(forHeap)(V2v f) {
  for (u64 i = 0; i < alSize; i++) {
    AllocInfo ci = al[i];
    Value* s = ci.p;
    Value* e = (Value*)(ci.sz + (u8*)ci.p);
    while (s!=e) {
      if (s->type!=t_empty) f(s);
      s = (Value*)(BSZ(s->mmInfo&63) + (u8*)s);
    }
  }
}
void BN(forFreedHeap)(V2v f) {
  for (u64 i = 0; i < alSize; i++) {
    AllocInfo ci = al[i];
    Value* s = ci.p;
    Value* e = (Value*)(ci.sz + (u8*)ci.p);
    while (s!=e) {
      if (s->type==t_empty) f(s);
      s = (Value*)(BSZ(s->mmInfo&63) + (u8*)s);
    }
  }
}

#define str0(X) #X
#define str1(X) str0(X)
void writeNum(FILE* f, u64 v, i32 len);
void BN(dumpHeap)(FILE* f) {
  for (u64 i = 0; i < alSize; i++) {
    AllocInfo ci = al[i];
    u64 addrI = (u64) ci.p;
    writeNum(f, ci.sz, 8);
    writeNum(f, addrI, 8);
    char* prefix = str1(BN());
    fwrite(prefix, 1, strlen(prefix)+1, f);
    fwrite(ci.p, 1, ci.sz, f);
  }
  fflush(f);
}

#undef MMAP
#undef AllocInfo
#undef buckets
#undef al
#undef alSize
#undef alCap
#undef MMI
#undef ALSZ
