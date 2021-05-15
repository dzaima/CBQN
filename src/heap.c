u64 heapUsed_ctr;
void heapUsedFn(Value* p) { heapUsed_ctr+= mm_size(p); }
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
      printf("delta %d for %s, uid %ld: ", v->refc, format_type(v->type), v->uid);
    #else
      printf("delta %d for %s: ", (i32)v->refc, format_type(v->type));
    #endif
    heap_observed = v;
    print(tag(v,OBJ_TAG)); puts("");
  }
}


bool heapVerify_visit(B x) {
  if (heapVerify_mode==-1) return false;
  if (isVal(x)) mm_visitP(v(x));
  return true;
}
bool heapVerify_visitP(void* x) {
  if(heapVerify_mode==-1) return false;
  Value* v = x;
       if(heapVerify_mode==0) v->refc--;
  else if(heapVerify_mode==1) v->refc++;
  else if(heapVerify_mode==2) if (x==heap_observed) { printf("referee: %p ", heap_curr); print(tag(heap_curr,OBJ_TAG)); puts(""); }
  return true;
}

void heapVerify_callVisit(Value* v) {
  if (ti[v->type].isArr && prnk(v)>1) heapVerify_visitP(shObjP(v));
  ti[v->type].visit(tag(v,OBJ_TAG));
}

void heap_getReferents(Value* v) {
  heap_curr = v;
  if (ti[v->type].isArr && prnk(v)>1) heapVerify_visitP(shObjP(v));
  ti[v->type].visit(tag(v,OBJ_TAG));
}
#ifdef CATCH_ERRORS
bool heapVerify_msg;
#endif
void heapVerify() {
  #ifdef CATCH_ERRORS
  if(!heapVerify_msg) { printf("note: heapVerify() will give garbage in combination with CATCH_ERRORS\n"); heapVerify_msg=true; }
  #endif
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
