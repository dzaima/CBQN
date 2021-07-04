#define AllocInfo BN(AllocInfo)
#define al     BN(al)
#define alCap  BN(alCap)
#define alSize BN(alSize)
#define str(X) #X
#ifndef PROT
  #define PROT PROT_READ|PROT_WRITE
#endif
#ifndef FLAGS
  #define FLAGS MAP_NORESERVE|MAP_PRIVATE|MAP_ANON
#endif

EmptyValue* buckets[64];
AllocInfo* al;
u64 alCap;
u64 alSize;

static void BN(guaranteeEmpty)(u8 bucket) {
  u8 cb = bucket;
  EmptyValue* c;
  while (true) {
    cb++;
    if (buckets[cb]) {
      c = buckets[cb];
      assert((c->mmInfo&63)==cb);
      buckets[cb] = c->next;
      break;
    }
    if (cb >= ALSZ) {
      u64 sz = BSZ(cb);
      if (mm_heapAlloc+sz >= mm_heapMax) { printf("Heap size limit reached\n"); exit(1); }
      mm_heapAlloc+= sz;
      // gc_maybeGC();
      c = mmap(NULL, sz, PROT, FLAGS, -1, 0);
      #ifdef USE_VALGRIND
        VALGRIND_MAKE_MEM_UNDEFINED(c, sz);
      #endif
      if (alSize+1>=alCap) {
        alCap = alCap? alCap*2 : 1024;
        al = realloc(al, sizeof(AllocInfo)*alCap);
      }
      al[BN(alSize)++] = (AllocInfo){.p = (Value*)c, .sz = sz};
      if (c==MAP_FAILED) { printf("Failed to allocate memory\n"); exit(1); }
      c->type = t_empty;
      c->mmInfo = cb;
      c->next = 0;
      break;
    }
  }
  c->mmInfo = MMI(bucket);
  while (cb != bucket) {
    cb--;
    EmptyValue* b = (EmptyValue*) (BSZ(cb) + (u8*)c);
    b->type = t_empty;
    b->mmInfo = MMI(cb);
    b->next = buckets[cb];
    buckets[cb] = b;
  }
  c->next = buckets[cb];
  buckets[cb] = c;
}
NOINLINE void* BN(allocS)(i64 bucket, u8 type) {
  BN(guaranteeEmpty)(bucket);
  return BN(allocL)(bucket, type);
}

void BN(forHeap)(V2v f) {
  for (u64 i = 0; i < alSize; i++) {
    AllocInfo ci = al[i];
    Value* s = ci.p;
    Value* e = (Value*)(ci.sz + (u8*)ci.p);
    while (s!=e) {
      if (s->type!=t_empty) f(s);
      s = (Value*)(BSZ(s->mmInfo&63) + (u8*)s);
    }
  }
}

#undef FLAGS
#undef PROT
#undef AllocInfo
#undef al
#undef alSize
#undef alCap
#undef MMI
#undef ALSZ
