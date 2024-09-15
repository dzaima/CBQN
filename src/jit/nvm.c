#include "../core.h"
#include "../vm.h"

u64 mm_heapUsed();
#if JIT_START!=-1
  #include "nvm_x86_64.c"
  u64 tot_heapUsed() {
    return mm_heapUsed() + mmX_heapUsed();
  }
#else
  #include "nvm_placeholder.c"
  u64 tot_heapUsed() {
    return mm_heapUsed();
  }
#endif
