EmptyValue* buckets[64];

#define AllocInfo BN(AllocInfo)
#define al     BN(al)
#define alCap  BN(alCap)
#define alSize BN(alSize)
typedef struct AllocInfo {
  Value* p;
  u64 sz;
} AllocInfo;
AllocInfo* al;
u64 alCap;
u64 alSize;

static EmptyValue* BN(makeEmpty)(u8 bucket) { // result->next is garbage
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
    if (cb >= 20) {
      u64 sz = BSZ(cb);
      c = mmap(NULL, sz, PROT_READ|PROT_WRITE, MAP_NORESERVE|MAP_PRIVATE|MAP_ANON, -1, 0);
      if (alSize+1>=alCap) {
        alCap = alCap? alCap*2 : 1024;
        al = realloc(al, sizeof(AllocInfo)*alCap);
      }
      al[BN(alSize)++] = (AllocInfo){.p = (Value*)c, .sz = sz};
      if (c==MAP_FAILED) {
        printf("failed to allocate memory\n");
        exit(1);
      }
      c->type = t_empty;
      c->mmInfo = cb;
      c->next = 0;
      break;
    }
  }
  c->mmInfo = MMI(bucket);
  while (cb != bucket) {
    cb--;
    EmptyValue* b = (EmptyValue*) (BSZ(cb) + (char*)c);
    b->type = t_empty;
    b->mmInfo = MMI(cb);
    b->next = 0; assert(buckets[cb]==0);
    buckets[cb] = b;
  }
  return c;
}

void BN(free)(Value* x) {
  onFree(x);
  EmptyValue* c = (EmptyValue*) x;
  c->type = t_empty;
  u8 b = c->mmInfo&63;
  c->next = buckets[b];
  buckets[b] = c;
}

void* BN(allocL)(u8 bucket, u8 type) {
  EmptyValue* x = buckets[bucket];
  if (x==NULL) x = BN(makeEmpty)(bucket);
  else buckets[bucket] = x->next;
  x->flags = x->extra = x->type = 0;
  x->refc = 1;
  x->type = type;
  return x;
}
void BN(forHeap)(V2v f) {
  for (u64 i = 0; i < alSize; i++) {
    AllocInfo ci = al[i];
    Value* s = ci.p;
    Value* e = ci.sz + (void*)ci.p;
    while (s!=e) {
      if (s->type!=t_empty) f(s);
      s = BSZ(s->mmInfo&63) + (void*)s;
    }
  }
}
u64 BN(totalAllocated)() {
  u64 res = 0;
  for (u64 i = 0; i < alSize; i++) res+= al[i].sz;
  return res;
}

#undef MMI
#undef AllocInfo
#undef al
#undef alSize
#undef alCap