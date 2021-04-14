u32 heapVerify_mode = -1;


u64 heapUsed_ctr;
void heapUsedFn(Value* p) { heapUsed_ctr+= mm_size(p); }
u64 mm_heapUsed() {
  heapUsed_ctr = 0;
  mm_forHeap(heapUsedFn);
  return heapUsed_ctr;
}


void heap_checkFn(Value* v) {
  if (v->refc!=0) {
    #ifdef OBJ_COUNTER
      printf("delta %d for %s, uid %ld: ", v->refc, format_type(v->type), v->uid);
    #else
      printf("delta %d for %s: ", (i32)v->refc, format_type(v->type));
    #endif
    print(tag(v,OBJ_TAG)); puts("");
  }
}
void heap_callVisit(Value* v) {
  ti[v->type].visit(tag(v,OBJ_TAG));
}
void heapVerify() {
  #ifndef HEAP_VERIFY
  err("heapVerify() HEAP_VERIFY to be defined");
  #endif
  heapVerify_mode=0; mm_forHeap(heap_callVisit); gc_visitRoots();
  mm_forHeap(heap_checkFn);
  heapVerify_mode=1; mm_forHeap(heap_callVisit); gc_visitRoots();
  heapVerify_mode=-1;
}

#ifdef HEAP_VERIFY
bool heapVerify_visit(B x) {
  if (heapVerify_mode==-1) return false;
  if (isVal(x)) mm_visitP(v(x));
  return true;
}
bool heapVerify_visitP(void* x) {
  if(heapVerify_mode==-1) return false;
  Value* v = x;
  if(heapVerify_mode==0) { v->refc--; if(ti[v->type].isArr && rnk(tag(v,OBJ_TAG))>1)heapVerify_visitP(shObj(tag(v,OBJ_TAG))); }
  if(heapVerify_mode==1) { v->refc++; if(ti[v->type].isArr && rnk(tag(v,OBJ_TAG))>1)heapVerify_visitP(shObj(tag(v,OBJ_TAG))); }
  return true;
}
#endif