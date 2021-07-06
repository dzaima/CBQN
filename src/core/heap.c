#include "../core.h"

u64 heapUsed_ctr;
static void heapUsedFn(Value* p) { heapUsed_ctr+= mm_size(p); }
u64 mm_heapUsed() {
  heapUsed_ctr = 0;
  mm_forHeap(heapUsedFn);
  return heapUsed_ctr;
}



#ifdef HEAP_VERIFY
u32 heapVerify_mode = -1;

Value* heap_observed;
Value* heap_curr;
void heapVerify_checkFn(Value* v) {
  if (v->refc!=0) {
    #ifdef OBJ_COUNTER
      printf("delta %d for %s, uid "N64d": ", v->refc, format_type(v->type), v->uid);
    #else
      printf("delta %d for %s: ", (i32)v->refc, format_type(v->type));
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
    heapVerify_mode=2; mm_forHeap(heap_getReferents);
  }
  heapVerify_mode=-1;
}

#endif
