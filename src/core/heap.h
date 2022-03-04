u64 mm_heapUsed(void);

#ifdef HEAP_VERIFY

extern u32 heapVerify_mode;
extern Value* heap_observed;
extern Value* heap_curr;
static bool heapVerify_visit(B x) {
  if (heapVerify_mode==-1) return false;
  if (isVal(x)) mm_visitP(v(x));
  return true;
}
static bool heapVerify_visitP(void* x) {
  if(heapVerify_mode==-1) return false;
  Value* v = x;
       if(heapVerify_mode==0) v->refc--;
  else if(heapVerify_mode==1) v->refc++;
  else if(heapVerify_mode==2) if (x==heap_observed) { printf("referee: %p ", heap_curr); print(tag(heap_curr,OBJ_TAG)); putchar('\n'); }
  return true;
}
void heapVerify(void);

#endif

void heap_printInfo(bool sizes, bool types, bool freed);
