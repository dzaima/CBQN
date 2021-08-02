#include "../core.h"

B m_str8(usz sz, char* s) {
  u32* rp; B r = m_c32arrv(&rp, sz);
  for (u64 i = 0; i < sz; i++) rp[i] = (u32)s[i];
  return r;
}
B m_str8l(char* s) {
  usz sz = strlen(s);
  u32* rp; B r = m_c32arrv(&rp, sz);
  for (u64 i = 0; i < sz; i++) rp[i] = (u32)s[i];
  return r;
}

B m_str32(u32* s) {
  usz sz = 0; while(s[sz]) sz++;
  u32* rp; B r = m_c32arrv(&rp, sz);
  for (usz i = 0; i < sz; i++) rp[i] = s[i];
  return r;
}

static Arr* m_c32slice(Arr* p, u32* ptr, usz ia) {
  C32Slice* r = m_arr(sizeof(C32Slice), t_c32slice, ia);
  r->p = p;
  r->a = ptr;
  return (Arr*)r;
}
static Arr* c32arr_slice  (B x, usz s, usz ia) { return m_c32slice(a(x), c(C32Arr,x)->a+s, ia); }
static Arr* c32slice_slice(B x, usz s, usz ia) { Arr* p=c(Slice,x)->p; ptr_inc(p); Arr* r = m_c32slice(p, c(C32Slice,x)->a+s, ia); dec(x); return r; }

static B c32arr_get  (B x, usz n) { VTY(x,t_c32arr  ); return m_c32(c(C32Arr  ,x)->a[n]); }
static B c32slice_get(B x, usz n) { VTY(x,t_c32slice); return m_c32(c(C32Slice,x)->a[n]); }
static bool c32arr_canStore(B x) { return isC32(x); }



void c32arr_init() {
  TIi(t_c32arr,get)   = c32arr_get;   TIi(t_c32slice,get)   = c32slice_get;
  TIi(t_c32arr,getU)  = c32arr_get;   TIi(t_c32slice,getU)  = c32slice_get;
  TIi(t_c32arr,slice) = c32arr_slice; TIi(t_c32slice,slice) = c32slice_slice;
  TIi(t_c32arr,freeO) = tyarr_freeO;  TIi(t_c32slice,freeO) =    slice_freeO;
  TIi(t_c32arr,freeF) = tyarr_freeF;  TIi(t_c32slice,freeF) =    slice_freeF;
  TIi(t_c32arr,visit) = noop_visit;   TIi(t_c32slice,visit) =    slice_visit;
  TIi(t_c32arr,print) =    arr_print; TIi(t_c32slice,print) = arr_print;
  TIi(t_c32arr,isArr) = true;         TIi(t_c32slice,isArr) = true;
  TIi(t_c32arr,arrD1) = true;         TIi(t_c32slice,arrD1) = true;
  TIi(t_c32arr,elType) = el_c32;      TIi(t_c32slice,elType) = el_c32;
  TIi(t_c32arr,canStore) = c32arr_canStore;
  
  u32* tmp; bi_emptyCVec = m_c32arrv(&tmp, 0); gc_add(bi_emptyCVec);
  
  Arr* emptySVec = m_fillarrp(0); arr_shVec(emptySVec);
  fillarr_setFill(emptySVec, emptyCVec());
  bi_emptySVec = taga(emptySVec); gc_add(bi_emptySVec);
}
