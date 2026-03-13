#include "core.h"
#include "vm/vm.h"

u64 mm_heapUsed();
#if JIT_START!=-1
  #include "vm/jit/nvm_x86_64.c"
  u64 tot_heapUsed() {
    return mm_heapUsed() + mmX_heapUsed();
  }
#else
  #include "vm/jit/nvm_placeholder.c"
  u64 tot_heapUsed() {
    return mm_heapUsed();
  }
#endif
