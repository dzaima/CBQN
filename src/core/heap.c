#include "../core.h"

#ifdef HEAP_VERIFY
u32 heapVerify_mode = -1;

Value* heap_observed;
Value* heap_curr;
void heapVerify_checkFn(Value* v) {
  if (v->refc!=0) {
    #ifdef OBJ_COUNTER
      printf("delta %d for %s, uid "N64d": ", v->refc, type_repr(v->type), v->uid);
    #else
      printf("delta %d for %s: ", (i32)v->refc, type_repr(v->type));
    #endif
    heap_observed = v;
    print(tag(v,OBJ_TAG)); putchar('\n');
  }
}



void heapVerify_callVisit(Value* v) {
  if (TIv(v,isArr) && prnk(v)>1) heapVerify_visitP(shObjP(v));
  TIv(v,visit)(v);
}

void heap_getReferents(Value* v) {
  heap_curr = v;
  if (TIv(v,isArr) && prnk(v)>1) heapVerify_visitP(shObjP(v));
  TIv(v,visit)(v);
}
void heapVerify() {
  heap_observed = 0;
  heapVerify_mode=0; mm_forHeap(heapVerify_callVisit); gc_visitRoots();
  mm_forHeap(heapVerify_checkFn);
  heapVerify_mode=1; mm_forHeap(heapVerify_callVisit); gc_visitRoots();
  if (heap_observed) {
    printf("refc of last: %d\n", heap_observed->refc);
    // heapVerify_mode=2; mm_forHeap(heap_getReferents);
  }
  heapVerify_mode=-1;
}

#endif

static u64 heap_PICounts[t_COUNT];
static u64 heap_PISizes[t_COUNT];

void heap_PIFn(Value* v) {
  heap_PICounts[v->type]++;
  heap_PISizes[v->type]+= mm_size(v);
}

void heap_printInfo(bool sizes, bool types) {
  u64 total = mm_heapAlloc;
  u64 used = mm_heapUsed();
  printf("RAM allocated: "N64u"\n", total);
  printf("heap in use: "N64u"\n", used);
  #if MM!=0
    if (sizes) {
      i32 count = sizeof(mm_ctrs)/8;
      for (i32 i = 0; i < count; i++) {
        u64 count = mm_ctrs[i];
        if (count>0) printf("size %llu: "N64u"\n", i>64? 1ULL<<(i*3) : 1ULL<<i, count);
      }
    }
    if (types) {
      for (i32 i = 0; i < t_COUNT; i++) heap_PICounts[i] = heap_PISizes[i] = 0;
      mm_forHeap(heap_PIFn);
      for (i32 i = 0; i < t_COUNT; i++) {
        u64 count = heap_PICounts[i];
        if (count>0) printf("type %d/%s: count "N64u", total size "N64u"\n", i, type_repr(i), count, heap_PISizes[i]);
      }
    }
  #endif
}
