#include "../core.h"
#include "../vm.h"

#if JIT_START!=-1
  #include "nvm_x86_64.c"
#else
  #include "nvm_placeholder.c"
#endif
