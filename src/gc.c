#pragma once
#include "h.h"


u64 gc_depth = 1;
void gc_disable() { gc_depth++; }
void gc_enable() { gc_depth--; }

vfn gc_roots[16];
u32 gc_rootSz;
void gc_addFn(vfn f) {
  if (gc_rootSz>=16) err("Too many GC roots");
  gc_roots[gc_rootSz++] = f;
}
B gc_rootObjs[256];
u32 gc_rootObjSz;
void gc_add(B x) {
  if (gc_rootObjSz>=256) err("Too many GC root objects");
  gc_rootObjs[gc_rootObjSz++] = x;
}

#ifdef LOG_GC
u64 gc_visitBytes, gc_visitCount, gc_freedBytes, gc_freedCount;
#endif

u8 gc_tagCurr = 0x80; // if no gc is running, this is what all objects will have
u8 gc_tagNew  = 0x00;
void mm_visit(B x) {
  #ifdef HEAP_VERIFY
    if(heapVerify_visit(x)) return;
  #endif
  
  if (!isVal(x)) return;
  u8 p = v(x)->mmInfo;
  if ((p&0x80)==gc_tagNew) return;
  v(x)->mmInfo = p^0x80;
  #ifdef LOG_GC
    gc_visitBytes+= mm_size(v(x)); gc_visitCount++;
  #endif
  TI(x).visit(x);
}
void mm_visitP(void* xp) {
  #ifdef HEAP_VERIFY
    if(heapVerify_visitP(xp)) return;
  #endif
  
  Value* x = (Value*)xp;
  u8 p = x->mmInfo;
  if ((p&0x80)==gc_tagNew) return;
  x->mmInfo = p^0x80;
  #ifdef LOG_GC
    gc_visitBytes+= mm_size(x); gc_visitCount++;
  #endif
  ti[x->type].visit(tag(x, OBJ_TAG));
}
void gc_tryFree(Value* v) {
  u8 t = v->type;
  #ifdef DEBUG
    if (t==t_freed) err("GC found t_freed\n");
  #endif
  if (t!=t_empty & (v->mmInfo&0x80)==gc_tagCurr) {
    if (t==t_shape) return;
    #ifdef DONT_FREE
      v->flags = t;
    #endif
    #ifdef LOG_GC
      gc_freedBytes+= mm_size(v); gc_freedCount++;
    #endif
    v->type = t_freed;
    ti[t].free(tag(v, OBJ_TAG));
    // Object may not be immediately freed if it's not a part of a cycle, but instead a descendant of one.
    // It will be freed when the cycle is freed, and the t_freed type ensures it doesn't double-free itself
  }
}


void gc_visitRoots() {
  for (u32 i = 0; i < gc_rootSz; i++) gc_roots[i]();
  for (u32 i = 0; i < gc_rootObjSz; i++) mm_visit(gc_rootObjs[i]);
}
void gc_forceGC() {
  #ifdef LOG_GC
    u64 start = nsTime();
    gc_visitBytes = 0; gc_freedBytes = 0;
    gc_visitCount = 0; gc_freedCount = 0;
  #endif
  gc_visitRoots();
  mm_forHeap(gc_tryFree);
  gc_tagNew = gc_tagCurr;
  gc_tagCurr^= 0x80;
  #ifdef LOG_GC
    fprintf(stderr, "GC kept %ldB from %ld objects, freed %ldB from %ld objects; took %.3fms\n", gc_visitBytes, gc_visitCount, gc_freedBytes, gc_freedCount, (nsTime()-start)/1e6);
  #endif
}


void gc_maybeGC() {
  if (!gc_depth) gc_forceGC();
}