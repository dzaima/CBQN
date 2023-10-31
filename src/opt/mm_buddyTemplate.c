#include "../utils/file.h"
#include "../utils/interrupt.h"
#define AllocInfo BN(AllocInfo)
#define buckets   BN(buckets)
#define al        BN(al)
#define alCap     BN(alCap)
#define alSize    BN(alSize)

typedef struct AllocInfo {
  Value* p;
  u64 sz;
} AllocInfo;
AllocInfo* al;
u64 alCap;
u64 alSize;

FORCE_INLINE void BN(splitTo)(EmptyValue* c, i64 from, i64 to, bool notEqual) {
  c->mmInfo = MMI(to);
  PLAINLOOP while (from != to) {
    from--;
    EmptyValue* b = (EmptyValue*) (BSZ(from) + (u8*)c);
    b->type = t_empty;
    b->mmInfo = MMI(from);
    b->next = buckets[from];
    vg_undef_p(b, sizeof(EmptyValue));
    buckets[from] = b;
  }
  c->next = buckets[from];
  vg_undef_p(c, sizeof(EmptyValue));
  buckets[from] = c;
}

#if ALLOC_MODE==0 && ENABLE_GC
  static bool BN(allocMore_rec);
#endif

static NOINLINE void* BN(allocateMore)(i64 bucket, u8 type, i64 from, i64 to) {
  if (from >= ALSZ) from = ALSZ;
  if (from < (bucket&63)) from = bucket&63;
  u64 sz = BSZ(from);
  CHECK_INTERRUPT;
  
  #if ALLOC_MODE==0 && ENABLE_GC
    if (gc_maybeGC(false)) goto alloc_rec;
  #endif
  
  if (mm_heapAlloc+sz >= mm_heapMax) {
    #if ALLOC_MODE==0 && ENABLE_GC
      if (!BN(allocMore_rec)) {
        gc_forceGC(false);
        BN(allocMore_rec) = true;
        alloc_rec:;
        void* r = BN(allocL)(bucket, type);
        BN(allocMore_rec) = false;
        return r;
      }
      BN(allocMore_rec) = false;
      gc_wantTopLevelGC = true;
    #endif
    thrOOM();
  }
  
  if (mem_log_enabled) fprintf(stderr, "requesting "N64u" more " STR1(BN()) " heap (during allocation of "N64u"B t_%s)", sz, (u64)BSZ(bucket), type_repr(type));
  #if NO_MMAP
    EmptyValue* c = calloc(sz+getPageSize(), 1);
    if (mem_log_enabled) fprintf(stderr, "\n");
  #else
    u8* mem = MMAP(sz);
    if (mem_log_enabled) fprintf(stderr, ": %s\n", mem==MAP_FAILED? "failed" : "success");
    if (mem==MAP_FAILED) thrOOM();
    if (ptr2u64(mem)+sz > (1ULL<<48)) fatal("mmap returned address range above 2⋆48");
    #if ALLOC_MODE==0
      mem+= ALLOC_PADDING;
      // ux off = offsetof(TyArr,a);
      // if (off&31) mem+= 32-(off&31); // align heap such that arr->a is 32-byte-aligned
    #endif
    EmptyValue* c = (void*)mem;
  #endif
  mm_heapAlloc+= sz;
  
  if (alSize+1>=alCap) {
    alCap = alCap? alCap*2 : 1024;
    al = realloc(al, sizeof(AllocInfo)*alCap);
  }
  al[BN(alSize)++] = (AllocInfo){.p = (Value*)c, .sz = sz};
  c->type = t_empty;
  c->mmInfo = from;
  c->next = 0;
  vg_undef_p(c, sz);
  BN(splitTo)(c, from, to, false);
  assert(buckets[to]!=NULL);
  return BN(allocL)(bucket, type);
}

NOINLINE void* BN(allocS)(i64 bucket, u8 type) {
  i64 to = bucket&63;
  i64 from = to;
  EmptyValue* c;
  while (true) {
    if (from >= 63) return BN(allocateMore)(bucket, type, from, to);
    from++;
    if (buckets[from]) {
      c = buckets[from];
      assert((vg_def_v(c->mmInfo)&63)==from);
      buckets[from] = vg_def_v(c->next);
      break;
    }
  }
  BN(splitTo)(c, from, to, true);
  assert(buckets[to]!=NULL);
  return BN(allocL)(bucket, type);
}

void BN(forHeap)(V2v f) {
  for (u64 i = 0; i < alSize; i++) {
    AllocInfo ci = al[i];
    Value* s = ci.p;
    Value* e = (Value*)(ci.sz + (u8*)ci.p);
    while (s!=e) {
      if (vg_def_v(s->type)!=t_empty) f(s);
      s = (Value*)(BSZ(vg_def_v(s->mmInfo)&63) + (u8*)s);
    }
  }
}
void BN(forFreedHeap)(V2v f) {
  for (u64 i = 0; i < alSize; i++) {
    AllocInfo ci = al[i];
    Value* s = ci.p;
    Value* e = (Value*)(ci.sz + (u8*)ci.p);
    while (s!=e) {
      if (vg_def_v(s->type)==t_empty) f(s);
      s = (Value*)(BSZ(vg_def_v(s->mmInfo)&63) + (u8*)s);
    }
  }
}
#if ALLOC_MODE==0 && ENABLE_GC
static void BN(freeFreedAndMerge)() {
  for (u64 i = 0; i < 64; i++) buckets[i] = NULL;
  
  for (u64 i = 0; i < alSize; i++) {
    AllocInfo ci = al[i];
    u64 emptySize = 0;
    Value* c = ci.p;
    Value* e = (Value*)(ci.sz + (u8*)ci.p);
    assert(c!=e);
    while (true) {
      if (vg_def_v(c->type)==t_freed) mm_freeLink(c, false);
      
      Value* next = (Value*)(BSZ(vg_def_v(c->mmInfo)&63) + (u8*)c);
      if (vg_def_v(c->type)==t_empty) {
        emptySize+= 1ULL<<(vg_def_v(c->mmInfo)&63);
      } else if (emptySize > 0) {
        emptyTail:;
        u8* emptyStart = ((u8*)c) - emptySize*MUL;
        while(emptySize) {
          u64 left = emptySize & (emptySize-1);
          u64 curr = emptySize ^ left;
          
          EmptyValue* cv = (EmptyValue*)emptyStart;
          u64 b = 63-CLZ(curr);
          *cv = (EmptyValue){
            .type = 0,
            .mmInfo = MMI(b),
            .next = buckets[b]
          };
          vg_undef_p(cv, sizeof(EmptyValue));
          buckets[b] = cv;
          
          emptyStart+= curr*MUL;
          emptySize = left;
        }
        emptySize = 0;
      }
      
      c = next;
      if (c==e) {
        if (emptySize!=0) goto emptyTail;
        break;
      }
    }
  }
}
#endif

void writeNum(FILE* f, u64 v, i32 len);
void BN(dumpHeap)(FILE* f) {
  for (u64 i = 0; i < alSize; i++) {
    AllocInfo ci = al[i];
    u64 addrI = ptr2u64(ci.p);
    writeNum(f, ci.sz, 8);
    writeNum(f, addrI, 8);
    char* prefix = STR1(BN());
    fwrite(prefix, 1, strlen(prefix)+1, f);
    fwrite(ci.p, 1, ci.sz, f);
  }
  fflush(f);
}

#undef AllocInfo
#undef buckets
#undef al
#undef alSize
#undef alCap
#undef MMI
#undef MUL
#undef ALSZ
