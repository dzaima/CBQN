#include "../core.h"
#include <stdlib.h>
#include <malloc.h>

void gc_add(B x) { }
void gc_addFn(vfn f) { }
void gc_maybeGC() { }
void gc_forceGC() { }
void gc_visitRoots() { }
void mm_forHeap(V2v f) { }
u64 mm_heapUsed() { return 123; } // idk
