#pragma once

extern GLOBAL u64 gc_depth;
extern GLOBAL bool gc_running;
static void gc_disable() { gc_depth++; }
static void gc_enable() { gc_depth--; }

#if GC_LOG_DETAILED
  extern u64 gcs_visitBytes, gcs_visitCount, gcs_freedBytes, gcs_freedCount;
#endif

extern void gc_onVisit(Value*);
static void mm_visit(B x) {
  if (isVal(x)) gc_onVisit(v(x));
}
static void mm_visitP(void* xp) {
  gc_onVisit(xp);
}
