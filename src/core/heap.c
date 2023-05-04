#include "../core.h"

#ifdef HEAP_VERIFY
u32 heapVerify_mode = -1;

Value* heap_observed;
Value* heap_curr;
void heapVerify_checkFn(Value* v) {
  if (v->refc!=0) {
    #ifdef OBJ_COUNTER
      printf("delta %d for %s @ %p, uid "N64d": ", v->refc, type_repr(v->type), v, v->uid);
    #else
      printf("delta %d for %s @ %p: ", (i32)v->refc, type_repr(v->type), v);
    #endif
    heap_observed = v;
    printI(tag(v,OBJ_TAG)); putchar('\n');
  }
}



void gc_visitRoots(void);
void gcv2_runHeapverify(i32);
void cbqn_heapVerify() {
  heap_observed = 0;
  gcv2_runHeapverify(0);
  mm_forHeap(heapVerify_checkFn);
  gcv2_runHeapverify(1);
  if (heap_observed) printf("refc of last: %d\n", heap_observed->refc);
  heapVerify_mode=-1;
}

#endif

static u64 heap_PICounts[t_COUNT];
static u64 heap_PISizes[t_COUNT];

NOINLINE void heap_PIFn(Value* v) {
  heap_PICounts[PTY(v)]++;
  heap_PISizes[PTY(v)]+= mm_size(v);
}

static u64 heap_PIFreed[128];
void heap_PIFreedFn(Value* v) {
  heap_PIFreed[v->mmInfo&127]++;
}

void mm_forFreedHeap(V2v f);
void heap_printInfo(bool sizes, bool types, bool freed, bool chain) {
  u64 total = mm_heapAlloc;
  u64 used = mm_heapUsed();
  fprintf(stderr, "RAM allocated: "N64u"\n", total);
  fprintf(stderr, "heap in use: "N64u"\n", used);
  #if MM!=0
    if (sizes) {
      for (i32 i = 2; i < 64; i++) {
        for (i32 j = 0; j < MM; j++) {
          i32 o = i-j;
          u64 count = mm_ctrs[o + j*64];
          if (count>0) fprintf(stderr, "size %llu: "N64u"\n", (1ULL<<o)*(j?3:1), count);
        }
      }
    }
    if (types) {
      for (i32 i = 0; i < t_COUNT; i++) heap_PICounts[i] = heap_PISizes[i] = 0;
      mm_forHeap(heap_PIFn);
      for (i32 i = 0; i < t_COUNT; i++) {
        u64 count = heap_PICounts[i];
        if (count>0) fprintf(stderr, "type %d/%s: count "N64u", total size "N64u"\n", i, type_repr(i), count, heap_PISizes[i]);
      }
    }
    if (freed || chain) {
      if (freed) {
        for (i32 i = 0; i < MM*64; i++) heap_PIFreed[i] = 0;
        mm_forFreedHeap(heap_PIFreedFn);
      }
      
      for (i32 i = 2; i < 64; i++) {
        for (i32 j = 0; j < MM; j++) {
          i32 o = i-j;
          u64 cf=0, cc=0;
          if (freed) {
            cf = heap_PIFreed[o + j*64];
          }
          
          if (chain) {
            i32 b = j*64 + i;
            if (mm_buckets[b]) {
              EmptyValue* p = mm_buckets[b];
              while (p) { cc++; p = p->next; }
            }
          }
          if (cf!=0 || cc!=0) {
            if (chain && freed) fprintf(stderr, "freed size %llu: count "N64u", chain "N64u"\n", (1ULL<<o)*(j?3:1), cf, cc);
            else fprintf(stderr, "freed size %llu: "N64u"\n", (1ULL<<o)*(j?3:1), freed? cf : cc);
          }
        }
      }
    }
  #endif
  fflush(stderr);
}
