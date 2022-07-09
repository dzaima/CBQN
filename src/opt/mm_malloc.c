#include "../core.h"
#include <stdlib.h>
#include <malloc.h>

#if USE_VALGRIND
#warning "USE_VALGRIND=1 and MM=0 don't work well together; CBQN requires the ability to read past the end of allocations, but malloc doesn't provide that."
#endif

void gc_add(B x) { }
void gc_addFn(vfn f) { }
void gc_maybeGC() { }
void gc_forceGC() { }
void gc_visitRoots() { }
void mm_forHeap(V2v f) { }
u64 mm_heapUsed() { return 123; } // idk
void mm_dumpHeap(FILE* f) { }
