#pragma once
#include "h.h"
#include "core/stuff.h"
#include "core/heap.h"

#if MM==0
  #include "opt/mm_malloc.h"
#elif MM==1
  #include "opt/mm_buddy.h"
#elif MM==2
  #include "opt/mm_2buddy.h"
#else
  #error bad MM value
#endif

#include "core/gstack.h"
#include "core/harr.h"
#include "core/f64arr.h"
#include "core/i32arr.h"
#include "core/c32arr.h"
#include "core/fillarr.h"
#include "core/derv.h"

#ifdef RT_VERIFY
  extern B r1Objs[rtLen];
#endif

typedef struct BFn {
  struct Fun;
  B ident;
   BBB2B uc1;
  BBBB2B ucw;
} BFn;
typedef struct BMd2 {
  struct Md1;
   BBBBB2B uc1;
  BBBBBB2B ucw;
} BMd2;

static i64 isum(B x) { // doesn't consume; may error; TODO error on overflow
  assert(isArr(x));
  i64 r = 0;
  usz xia = a(x)->ia;
  u8 xe = TI(x,elType);
  if (xe==el_i32) {
    i32* p = i32any_ptr(x);
    for (usz i = 0; i < xia; i++) r+= p[i];
  } else if (xe==el_f64) {
    f64* p = f64any_ptr(x);
    for (usz i = 0; i < xia; i++) { if(p[i]!=(f64)p[i]) thrM("Expected integer"); r+= p[i]; }
  } else {
    BS2B xgetU = TI(x,getU);
    for (usz i = 0; i < xia; i++) r+= o2i64(xgetU(x,i));
  }
  return r;
}
