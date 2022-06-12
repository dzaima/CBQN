#pragma once

#include "h.h"
#include "core/stuff.h"
#include "core/heap.h"

#if MM==0
  #include "opt/mm_malloc.h"
  #undef ENABLE_GC
  #define ENABLE_GC 0
#elif MM==1
  #include "opt/mm_buddy.h"
#elif MM==2
  #include "opt/mm_2buddy.h"
#else
  #error "bad MM value"
#endif

#include "core/gstack.h"
#include "core/harr.h"

#include "core/numarr.h"
#include "core/chrarr.h"
#include "core/fillarr.h"

#include "core/derv.h"
#include "core/arrFns.h"

#ifdef RT_VERIFY
  extern B r1Objs[rtLen];
#endif


typedef struct BFn {
  struct Fun;
  B ident;
   BBB2B uc1;
  BBBB2B ucw;
  BB2B im;
  B rtInvReg;
} BFn;
typedef struct BMd1 {
  struct Md1;
  D1C1 im;
  D1C2 iw;
  D1C2 ix;
} BMd1;

typedef struct BMd2 {
  struct Md2;
  D2C1 im;
  D2C2 iw;
  D2C2 ix;
  M2C4 uc1;
  M2C5 ucw;
} BMd2;
