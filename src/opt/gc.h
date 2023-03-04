#pragma once

extern u64 gc_depth;
static void gc_disable() { gc_depth++; }
static void gc_enable() { gc_depth--; }

#ifdef LOG_GC
extern u64 gc_visitBytes, gc_visitCount, gc_freedBytes, gc_freedCount;
#endif

extern void gc_onVisit(Value*);
static void mm_visit(B x) {
  if (isVal(x)) gc_onVisit(v(x));
}
static void mm_visitP(void* xp) {
  gc_onVisit(xp);
}
