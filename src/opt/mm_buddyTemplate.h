#define buckets BN(buckets)
static void BN(free)(Value* x) {
  onFree(x);
  #ifdef USE_VALGRIND
    VALGRIND_MAKE_MEM_UNDEFINED(x, BSZ(x->mmInfo&127));
    VALGRIND_MAKE_MEM_DEFINED(&x->mmInfo, 1);
    VALGRIND_MAKE_MEM_DEFINED(&x->type, 1);
  #endif
  #ifdef DONT_FREE
    if (x->type!=t_freed) x->flags = x->type;
  #else
    u8 b = x->mmInfo&127;
    BN(ctrs)[b]--;
    ((EmptyValue*)x)->next = buckets[b];
    buckets[b] = (EmptyValue*)x;
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
  BN(ctrs)[bucket]++;
  x->flags = x->extra = x->type = x->mmInfo = 0;
  x->refc = 1;
  x->type = type;
  x->mmInfo = bucket;
  #if defined(DEBUG) && !defined(DONT_FREE)
    u64* p = (u64*)x;
    u64* s = p + sizeof(Value)/8;
    u64* e = p + BSZ(bucket)/8;
    while(s<e) *s++ = tag(NULL,OBJ_TAG).u;
  #endif
  #ifdef OBJ_COUNTER
    x->uid = currObjCounter++;
    #ifdef OBJ_TRACK
    if (x->uid == OBJ_TRACK) {
      printf("Tracked object "N64u" created at:\n", (u64)OBJ_TRACK);
      vm_pstLive();
    }
    #endif
  #endif
  return x;
}
#undef buckets
