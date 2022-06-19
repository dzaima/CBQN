#include "gc.h"

#ifdef LOG_GC
#include "../utils/time.h"
#endif

u64 gc_depth = 1;


vfn gc_roots[32];
u32 gc_rootSz;
void gc_addFn(vfn f) {
  if (gc_rootSz>=32) err("Too many GC root functions");
  gc_roots[gc_rootSz++] = f;
}

Value* gc_rootObjs[256];
u32 gc_rootObjSz;
void gc_add(B x) {
  assert(isVal(x));
  if (gc_rootObjSz>=256) err("Too many GC root objects");
  gc_rootObjs[gc_rootObjSz++] = v(x);
}


#ifdef LOG_GC
u64 gc_visitBytes, gc_visitCount, gc_freedBytes, gc_freedCount;
#endif

static void gc_tryFree(Value* v) {
  u8 t = v->type;
  #if defined(DEBUG) && !defined(CATCH_ERRORS)
    if (t==t_freed) err("GC found t_freed\n");
  #endif
  if (t!=t_empty && !(v->mmInfo&0x80)) {
    if (t==t_shape || t==t_temp) return;
    #ifdef DONT_FREE
      v->flags = t;
    #else
      #if CATCH_ERRORS
        if (t==t_freed) { mm_free(v); return; }
      #endif
    #endif
    #ifdef LOG_GC
      gc_freedBytes+= mm_size(v); gc_freedCount++;
    #endif
    v->type = t_freed;
    ptr_inc(v); // required as otherwise the object may free itself while not done reading its own fields
    TIi(t,freeO)(v);
    tptr_dec(v, mm_free);
    // Object may not be immediately freed if it's not a part of a cycle, but instead a descendant of one.
    // It will be freed when the cycle is freed, and the t_freed type ensures it doesn't double-free itself
  }
}

static void gc_freeFreed(Value* v) {
  if (v->type==t_freed) mm_free(v);
}

static void gc_resetTag(Value* x) {
  x->mmInfo&= 0x7F;
}

void gc_visitRoots() {
  for (u32 i = 0; i < gc_rootSz; i++) gc_roots[i]();
  for (u32 i = 0; i < gc_rootObjSz; i++) mm_visitP(gc_rootObjs[i]);
}
u64 gc_lastAlloc;
void gc_forceGC() {
  #if ENABLE_GC
    #ifdef LOG_GC
      u64 start = nsTime();
      gc_visitBytes = 0; gc_freedBytes = 0;
      gc_visitCount = 0; gc_freedCount = 0;
      u64 startSize = mm_heapUsed();
    #endif
    mm_forHeap(gc_resetTag);
    gc_visitRoots();
    mm_forHeap(gc_tryFree);
    mm_forHeap(gc_freeFreed);
    u64 endSize = mm_heapUsed();
    #ifdef LOG_GC
      fprintf(stderr, "GC kept "N64d"B from "N64d" objects, freed "N64d"B, including directly "N64d"B from "N64d" objects; took %.3fms\n", gc_visitBytes, gc_visitCount, startSize-endSize, gc_freedBytes, gc_freedCount, (nsTime()-start)/1e6);
    #endif
    gc_lastAlloc = endSize;
  #endif
}


void gc_maybeGC() {
  if (!gc_depth && mm_heapUsed() > gc_lastAlloc*2) gc_forceGC();
}
