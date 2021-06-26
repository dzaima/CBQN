#if defined(__x86_64) || defined(__amd64__)
  #include "nvm_x86_64.c"
#else
  #include "nvm_placeholder.c"
#endif