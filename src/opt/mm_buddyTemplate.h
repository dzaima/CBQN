#define buckets BN(buckets)
static void BN(free)(Value* x) {
  onFree(x);
  #ifdef USE_VALGRIND
    VALGRIND_MAKE_MEM_UNDEFINED(x, BSZ(x->mmInfo&63));
    VALGRIND_MAKE_MEM_DEFINED(&x->mmInfo, 1);
    VALGRIND_MAKE_MEM_DEFINED(&x->type, 1);
  #endif
  #ifdef DONT_FREE
    if (x->type!=t_freed) x->flags = x->type;
  #else
    u8 b = x->mmInfo&63;
    ((EmptyValue*)x)->next = buckets[b];
    buckets[b] = (EmptyValue*)x;
    allocB-= BSZ(b);
  #endif
  x->type = t_empty;
}

NOINLINE void* BN(allocS)(i64 bucket, u8 type);
static   void* BN(allocL)(i64 bucket, u8 type) {
  EmptyValue* x = buckets[bucket];
  if (RARE(x==NULL)) return BN(allocS)(bucket, type);
  buckets[bucket] = x->next;
  #ifdef USE_VALGRIND
    VALGRIND_MAKE_MEM_UNDEFINED(x, BSZ(bucket));
    VALGRIND_MAKE_MEM_DEFINED(&x->mmInfo, 1);
  #endif
  allocB+= BSZ(bucket);
  u8 mmInfo = x->mmInfo;
  x->flags = x->extra = x->type = x->mmInfo = 0;
  x->refc = 1;
  x->mmInfo = (mmInfo&0x7f) | gc_tagCurr;
  x->type = type;
  #if defined(DEBUG) && !defined(DONT_FREE)
    u64* p = (u64*)x;
    u64* s = p + sizeof(Value)/8;
    u64* e = p + BSZ(bucket)/8;
    while(s<e) *s++ = tag(NULL, OBJ_TAG).u;
  #endif
  #ifdef OBJ_COUNTER
  x->uid = currObjCounter++;
  #endif
  return x;
}
#undef buckets
