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

static B m_c32slice(B p, u32* ptr) {
  C32Slice* r = mm_allocN(sizeof(C32Slice), t_c32slice);
  r->p = p;
  r->a = ptr;
  return tag(r, ARR_TAG);
}
static B c32arr_slice  (B x, usz s) {return m_c32slice(x                 , c(C32Arr  ,x)->a+s); }
static B c32slice_slice(B x, usz s) { B r = m_c32slice(inc(c(Slice,x)->p), c(C32Slice,x)->a+s); dec(x); return r; }

static B c32arr_get  (B x, usz n) { VTY(x,t_c32arr  ); return m_c32(c(C32Arr  ,x)->a[n]); }
static B c32slice_get(B x, usz n) { VTY(x,t_c32slice); return m_c32(c(C32Slice,x)->a[n]); }
static void c32arr_free(Value* x) { decSh(x); }
static bool c32arr_canStore(B x) { return isC32(x); }

void c32arr_init() {
  ti[t_c32arr].get   = c32arr_get;   ti[t_c32slice].get   = c32slice_get;
  ti[t_c32arr].getU  = c32arr_get;   ti[t_c32slice].getU  = c32slice_get;
  ti[t_c32arr].slice = c32arr_slice; ti[t_c32slice].slice = c32slice_slice;
  ti[t_c32arr].free  = c32arr_free;  ti[t_c32slice].free  =    slice_free;
  ti[t_c32arr].visit = noop_visit;   ti[t_c32slice].visit =    slice_visit;
  ti[t_c32arr].print =    arr_print; ti[t_c32slice].print = arr_print;
  ti[t_c32arr].isArr = true;         ti[t_c32slice].isArr = true;
  ti[t_i32arr].arrD1 = true;         ti[t_i32slice].arrD1 = true;
  ti[t_c32arr].elType = el_c32;      ti[t_c32slice].elType = el_c32;
  ti[t_c32arr].canStore = c32arr_canStore;
  u32* tmp; bi_emptyCVec = m_c32arrv(&tmp, 0); gc_add(bi_emptyCVec);
  bi_emptySVec = m_fillarrp(0); gc_add(bi_emptySVec);
  arr_shVec(bi_emptySVec, 0);
  fillarr_setFill(bi_emptySVec, inc(bi_emptyCVec));
}
