EmptyValue* buckets[64];

static EmptyValue* mm_makeEmpty(u8 bucket) { // result->next is garbage
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

void mm_free(Value* x) {
  onFree(x);
  EmptyValue* c = (EmptyValue*) x;
  c->type = t_empty;
  u8 b = c->mmInfo&63;
  c->next = buckets[b];
  buckets[b] = c;
}

void* mm_allocL(u8 bucket, u8 type) {
  EmptyValue* x = buckets[bucket];
  if (x==NULL) x = mm_makeEmpty(bucket);
  else buckets[bucket] = x->next;
  x->flags = x->extra = x->type = 0;
  x->refc = 1;
  x->type = type;
  return x;
}

#undef BSZ
#undef BSZI
#undef MMI
