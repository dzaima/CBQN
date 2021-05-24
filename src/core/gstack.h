#pragma once
// #define GS_REALLOC // whether to dynamically realloc gStack
#ifndef GS_SIZE
#define GS_SIZE 65536 // if !GS_REALLOC, size in number of B objects of the global object stack
#endif
#ifndef ENV_SIZE
#define ENV_SIZE 4096 // max recursion depth; GS_SIZE and C stack size may limit this
#endif

extern B* gStack; // points to after end
extern B* gStackStart;
extern B* gStackEnd;
void gsPrint();

static void gsReserve(u64 am) {
  #ifdef GS_REALLOC
    if (am>gStackEnd-gStack) {
      u64 n = gStackEnd-gStackStart + am + 500;
      u64 d = gStack-gStackStart;
      gStackStart = realloc(gStackStart, n*sizeof(B));
      gStack    = gStackStart+d;
      gStackEnd = gStackStart+n;
    }
  #elif DEBUG
    if (am>gStackEnd-gStack) thrM("Stack overflow");
  #endif
}

static NOINLINE void gsReserveR(u64 am) { gsReserve(am); }


static inline void gsAdd(B x) {
  #ifdef GS_REALLOC
    if (gStack==gStackEnd) gsReserveR(1);
  #else
    if (gStack==gStackEnd) thrM("Stack overflow");
  #endif
  *(gStack++) = x;
}
static inline B gsPop() {
  return *--gStack;
}
