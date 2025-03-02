#include "gc.h"

#if ENABLE_GC
  static void mm_freeFreedAndMerge(void);
  #include "../utils/time.h"
  #if GC_LOG_DETAILED
    GLOBAL bool gc_log_enabled = true;
  #else
    GLOBAL bool gc_log_enabled;
  #endif
#endif


GLOBAL u64 gc_depth = 1;
#ifdef GC_EVERY_NTH_ALLOC
  GLOBAL u64 nth_alloc = 1;
#endif


GLOBAL vfn gc_roots[8];
GLOBAL u32 gc_rootSz;
void gc_addFn(vfn f) {
  if (gc_rootSz>=8) fatal("Too many GC root functions");
  gc_roots[gc_rootSz++] = f;
}

GLOBAL Value* gc_rootObjs[512];
GLOBAL u32 gc_rootObjSz;
void gc_add(B x) {
  assert(isVal(x));
  if (gc_rootObjSz>=512) fatal("Too many GC root objects");
  gc_rootObjs[gc_rootObjSz++] = v(x);
}

GLOBAL B* gc_rootBRefs[64]; GLOBAL u32 gc_rootBRefsSz;
void gc_add_ref(B* x) {
  if (gc_rootBRefsSz>=64) fatal("Too many GC root B refs");
  gc_rootBRefs[gc_rootBRefsSz++] = x;
}





static void gc_resetTag(Value* x) {
  x->mmInfo&= 0x7F;
}

NOINLINE void gc_visitRoots() {
  for (u32 i = 0; i < gc_rootSz; i++) gc_roots[i]();
  for (u32 i = 0; i < gc_rootObjSz; i++) mm_visitP(gc_rootObjs[i]);
  for (u32 i = 0; i < gc_rootBRefsSz; i++) mm_visit(*gc_rootBRefs[i]);
}

static void gc_tryFree(Value* v) {
  u8 t = v->type;
  #if DEBUG && !USE_SETJMP
    if (t==t_freed) fatal("GC found t_freed\n");
  #endif
  if (t!=t_empty && !(v->mmInfo&0x80)) {
    if (t==t_shape || t==t_temp || t==t_talloc) return;
    #if DONT_FREE
      v->flags = t;
    #else
      #if USE_SETJMP
        if (t==t_freed) { mm_free(v); return; }
      #endif
    #endif
    #if GC_LOG_DETAILED
      gcs_freedBytes+= mm_size(v); gcs_freedCount++;
    #endif
    v->type = t_freed;
    ptr_inc(v); // required as otherwise the object may free itself while not done reading its own fields
    TIi(t,freeO)(v);
    tptr_dec(v, mm_free);
    // Object may not be immediately freed if it's not a part of a cycle, but instead a descendant of one.
    // It will be freed when the cycle is freed, and the t_freed type ensures it doesn't double-free itself
  }
}

#if GC_LOG_DETAILED
  GLOBAL u64 gcs_visitBytes, gcs_visitCount, gcs_freedBytes, gcs_freedCount, gcs_unkRefsBytes, gcs_unkRefsCount; // GC stat counters
#endif

GLOBAL i32 visit_mode;
enum {
  GC_DEC_REFC, // decrement refcount
  GC_INC_REFC, // increment refcount
  GC_MARK,     // if unmarked, mark & visit
  #if HEAP_VERIFY
  GC_DEC_REFC_HV, // decrement refcount without checking for 0
  #endif
};

void gc_onVisit(Value* x) {
  switch (visit_mode) { default: UD;
    case GC_DEC_REFC:
      #if DEBUG
        if(x->refc==0) fatal("decrementing refc 0");
      #endif
      x->refc--;
      return;
    
    #if HEAP_VERIFY
    case GC_DEC_REFC_HV:
      x->refc--;
      return;
    #endif
    
    case GC_INC_REFC: x->refc++; return;
    case GC_MARK: {
      if (x->mmInfo&0x80) return;
      x->mmInfo|= 0x80;
      #if GC_LOG_DETAILED
        gcs_visitBytes+= mm_size(x); gcs_visitCount++;
      #endif
      TIv(x,visit)(x);
      return;
    }
  }
}

static void gcv2_visit(Value* x) {
  TIv(x,visit)(x);
}
static void gcv2_unmark_visit(Value* x) {
  gc_resetTag(x);
  TIv(x,visit)(x);
}

#if HEAP_VERIFY
  void gcv2_runHeapverify(i32 mode) {
    visit_mode = mode==0? GC_DEC_REFC_HV : GC_INC_REFC;
    mm_forHeap(gcv2_visit);
    gc_visitRoots();
  }
#endif

#if ENABLE_GC
  static void gc_visitRefcNonzero(Value* x) {
    if (x->refc == 0) return;
    #if GC_LOG_DETAILED
      gcs_unkRefsBytes+= mm_size(x); gcs_unkRefsCount++;
    #endif
    mm_visitP(x);
  }
  
  static NOINLINE void gc_run_toplevel_1() {
    mm_forHeap(gc_resetTag);
  }
  static NOINLINE void gc_run_nontoplevel_1() {
    visit_mode = GC_DEC_REFC;
    mm_forHeap(gcv2_unmark_visit);
  }
  static NOINLINE void gc_run_nontoplevel_2() {
    visit_mode = GC_MARK;
    mm_forHeap(gc_visitRefcNonzero);
  }
  static NOINLINE void gc_run_nontoplevel_3() {
    visit_mode = GC_INC_REFC;
    mm_forHeap(gcv2_visit);
  }
  static NOINLINE void gc_run_tryFree() {
    mm_forHeap(gc_tryFree);
  }
  
  static void gc_run(bool toplevel) {
    if (toplevel) {
      gc_run_toplevel_1();
    } else {
      gc_run_nontoplevel_1();
      gc_run_nontoplevel_2();
      gc_run_nontoplevel_3();
    }
    
    visit_mode = GC_MARK;
    gc_visitRoots();
    
    gc_run_tryFree();
    mm_freeFreedAndMerge();
  }
#endif

GLOBAL u64 gc_lastAlloc;
GLOBAL bool gc_running;
void gc_forceGC(bool toplevel) {
  #if ENABLE_GC
    if (gc_running) fatal("starting GC while GC is in the middle of running");
    gc_running = 1;
    u64 startTime=0, startSize=0;
    if (gc_log_enabled) {
      startTime = nsTime();
      startSize = tot_heapUsed();
      #if GC_LOG_DETAILED
        gcs_visitBytes = 0; gcs_freedBytes = 0;
        gcs_visitCount = 0; gcs_freedCount = 0;
        gcs_unkRefsBytes = 0; gcs_unkRefsCount = 0;
      #endif
    }
    gc_run(toplevel);
    u64 endSize = tot_heapUsed();
    if (gc_log_enabled) {
      fprintf(stderr, "GC: before: "N64d"B/"N64d"B", startSize, mm_heapAlloc);
      #if GC_LOG_DETAILED
        fprintf(stderr, "; kept "N64d"B="N64d" objs, freed "N64d"B, incl. directly "N64d"B="N64d" objs", gcs_visitBytes, gcs_visitCount, startSize-endSize, gcs_freedBytes, gcs_freedCount);
        fprintf(stderr, "; unknown refs: "N64d"B="N64d" objs", gcs_unkRefsBytes, gcs_unkRefsCount);
      #else
        fprintf(stderr, "; freed "N64d"B, left "N64d"B", startSize-endSize, endSize);
      #endif
      fprintf(stderr, "; took %.3fms\n", (nsTime()-startTime)/1e6);
    }
    gc_lastAlloc = endSize;
    gc_running = 0;
  #endif
}

STATIC_GLOBAL bool gc_wantTopLevelGC;
NOINLINE bool gc_maybeGC(bool toplevel) {
  if (gc_depth) return false;
  u64 used = tot_heapUsed();
  if (used > gc_lastAlloc*2 || (toplevel && gc_wantTopLevelGC)) {
    if (toplevel) gc_wantTopLevelGC = false;
    gc_forceGC(toplevel);
    return true;
  }
  return false;
}
