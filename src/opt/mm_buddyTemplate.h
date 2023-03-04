#define buckets BN(buckets)

#if !ALLOC_NOINLINE || ALLOC_IMPL || ALLOC_IMPL_MMX

FORCE_INLINE void BN(freeLink)(Value* x, bool link) {
  #if ALLOC_IMPL_MMX
    preFree(x, true);
  #else
    preFree(x, false);
  #endif
  #ifdef DONT_FREE
    if (x->type!=t_freed) x->flags = x->type;
  #else
    u8 b = x->mmInfo&127;
    BN(ctrs)[b]--;
    if (link) {
      ((EmptyValue*)x)->next = buckets[b];
      buckets[b] = (EmptyValue*)x;
    }
  #endif
  x->type = t_empty;
  vg_undef_p(x, BSZ(x->mmInfo&127));
}

ALLOC_FN void BN(free)(Value* x) {
  BN(freeLink)(x, true);
}

NOINLINE void* BN(allocS)(i64 bucket, u8 type);
static   void* BN(allocL)(i64 bucket, u8 type) {
  EmptyValue* x = buckets[bucket];
  if (RARE(x==NULL)) return BN(allocS)(bucket, type);
  buckets[bucket] = vg_def_v(x->next);
  BN(ctrs)[bucket]++;
  x->flags = x->extra = x->type = x->mmInfo = 0;
  x->refc = 1;
  x->type = type;
  x->mmInfo = bucket;
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
#endif
#undef buckets
