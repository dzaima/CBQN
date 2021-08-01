#include "../core.h"

NOINLINE B m_cai32(usz ia, i32* a) {
  i32* rp; B r = m_i32arrv(&rp, ia);
  for (usz i = 0; i < ia; i++) rp[i] = a[i];
  return r;
}

static Arr* m_i32slice(Arr* p, i32* ptr) {
  I32Slice* r = mm_alloc(sizeof(I32Slice), t_i32slice);
  r->p = p;
  r->a = ptr;
  return (Arr*)r;
}
static Arr* i32arr_slice  (B x, usz s) { return m_i32slice(a(x), c(I32Arr,x)->a+s); }
static Arr* i32slice_slice(B x, usz s) { Arr* p=c(Slice,x)->p; ptr_inc(p); Arr* r = m_i32slice(p, c(I32Slice,x)->a+s); dec(x); return r; }

static B i32arr_get  (B x, usz n) { VTY(x,t_i32arr  ); return m_i32(c(I32Arr  ,x)->a[n]); }
static B i32slice_get(B x, usz n) { VTY(x,t_i32slice); return m_i32(c(I32Slice,x)->a[n]); }
static bool i32arr_canStore(B x) { return q_i32(x); }

void i32arr_init() {
  TIi(t_i32arr,get)   = i32arr_get;   TIi(t_i32slice,get)   = i32slice_get;
  TIi(t_i32arr,getU)  = i32arr_get;   TIi(t_i32slice,getU)  = i32slice_get;
  TIi(t_i32arr,slice) = i32arr_slice; TIi(t_i32slice,slice) = i32slice_slice;
  TIi(t_i32arr,freeO) = tyarr_freeO;  TIi(t_i32slice,freeO) =    slice_freeO;
  TIi(t_i32arr,freeF) = tyarr_freeF;  TIi(t_i32slice,freeF) =    slice_freeF;
  TIi(t_i32arr,visit) = noop_visit;   TIi(t_i32slice,visit) =    slice_visit;
  TIi(t_i32arr,print) =    arr_print; TIi(t_i32slice,print) = arr_print;
  TIi(t_i32arr,isArr) = true;         TIi(t_i32slice,isArr) = true;
  TIi(t_i32arr,arrD1) = true;         TIi(t_i32slice,arrD1) = true;
  TIi(t_i32arr,elType) = el_i32;      TIi(t_i32slice,elType) = el_i32;
  TIi(t_i32arr,canStore) = i32arr_canStore;
  i32* tmp; bi_emptyIVec = m_i32arrv(&tmp, 0); gc_add(bi_emptyIVec);
}
