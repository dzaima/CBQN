#pragma once

extern u64 gc_depth;
static void gc_disable() { gc_depth++; }
static void gc_enable() { gc_depth--; }

#ifdef LOG_GC
extern u64 gc_visitBytes, gc_visitCount, gc_freedBytes, gc_freedCount;
#endif

#if GC_VISIT_V2
  extern void gc_onVisit(Value*);
  static void mm_visit(B x) {
    if (isVal(x)) gc_onVisit(v(x));
  }
  static void mm_visitP(void* xp) {
    gc_onVisit(xp);
  }
#else
  static void mm_visit(B x) {
    #ifdef HEAP_VERIFY
      if(heapVerify_visit(x)) return;
    #endif
    
    if (!isVal(x)) return;
    Value* vx = v(x);
    if (vx->mmInfo&0x80) return;
    vx->mmInfo|= 0x80;
    #ifdef LOG_GC
      gc_visitBytes+= mm_size(vx); gc_visitCount++;
    #endif
    TI(x,visit)(vx);
  }
  static void mm_visitP(void* xp) {
    #ifdef HEAP_VERIFY
      if(heapVerify_visitP(xp)) return;
    #endif
    
    Value* x = (Value*)xp;
    if (x->mmInfo&0x80) return;
    x->mmInfo|= 0x80;
    #ifdef LOG_GC
      gc_visitBytes+= mm_size(x); gc_visitCount++;
    #endif
    TIv(x,visit)(x);
  }
#endif