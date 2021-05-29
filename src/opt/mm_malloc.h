#include <sys/mman.h>
#include <stdlib.h>
#include <malloc.h>
extern u64 mm_heapAlloc;
extern u64 mm_heapMax;

static void mm_free(Value* x) {
  onFree(x);
  free(x);
}

static void* mm_allocN(usz sz, u8 type) {
  Value* x = malloc(sz);
  onAlloc(sz, type);
  x->flags = x->extra = x->mmInfo = x->type = 0;
  x->refc = 1;
  x->type = type;
  return x;
}


static void gc_disable() { }
static void gc_enable() { }
static void mm_visit(B x) { }
static void mm_visitP(void* x) { }

void gc_add(B x);
void gc_addFn(vfn f);
void gc_maybeGC();
void gc_forceGC();
void gc_visitRoots();
void mm_forHeap(V2v f);

static u64  mm_round(usz x) { return x; }
static u64  mm_size(Value* x) { return malloc_usable_size(x); }
static u64  mm_totalAllocated() { return -1; }
