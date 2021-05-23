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
#define BI_A(N) { B t=bi_##N = mm_alloc(sizeof(BFn), t_funBI, ftag(FUN_TAG)); BFn*f=c(BFn,t); f->c2=N##_c2    ; f->c1=N##_c1    ; f->extra=pf_##N; f->ident=bi_N; f->uc1=def_fn_uc1; f->ucw=def_fn_ucw; gc_add(t); }
#define BI_D(N) { B t=bi_##N = mm_alloc(sizeof(BFn), t_funBI, ftag(FUN_TAG)); BFn*f=c(BFn,t); f->c2=N##_c2    ; f->c1=c1_invalid; f->extra=pf_##N; f->ident=bi_N; f->uc1=def_fn_uc1; f->ucw=def_fn_ucw; gc_add(t); }
#define BI_M(N) { B t=bi_##N = mm_alloc(sizeof(BFn), t_funBI, ftag(FUN_TAG)); BFn*f=c(BFn,t); f->c2=c2_invalid; f->c1=N##_c1    ; f->extra=pf_##N; f->ident=bi_N; f->uc1=def_fn_uc1; f->ucw=def_fn_ucw; gc_add(t); }
#define BI_FNS(F) F(BI_A,BI_M,BI_D)

static i64 isum(B x) { // doesn't consume; may error; TODO error on overflow
  assert(isArr(x));
  i64 r = 0;
  usz xia = a(x)->ia;
  u8 xe = TI(x).elType;
  if (xe==el_i32) {
    i32* p = i32any_ptr(x);
    for (usz i = 0; i < xia; i++) r+= p[i];
  } else if (xe==el_f64) {
    f64* p = f64any_ptr(x);
    for (usz i = 0; i < xia; i++) { if(p[i]!=(f64)p[i]) thrM("Expected integer"); r+= p[i]; }
  } else {
    BS2B xgetU = TI(x).getU;
    for (usz i = 0; i < xia; i++) r+= o2i64(xgetU(x,i));
  }
  return r;
}
