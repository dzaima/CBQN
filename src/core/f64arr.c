#include "../core.h"

NOINLINE B m_caf64(usz sz, f64* a) {
  f64* rp; B r = m_f64arrv(&rp, sz);
  for (usz i = 0; i < sz; i++) rp[i] = a[i];
  return r;
}

static B m_f64slice(B p, f64* ptr) {
  F64Slice* r = mm_allocN(sizeof(F64Slice), t_f64slice);
  r->p = p;
  r->a = ptr;
  return tag(r, ARR_TAG);
}
static B f64arr_slice  (B x, usz s) {return m_f64slice(x                 , c(F64Arr  ,x)->a+s); }
static B f64slice_slice(B x, usz s) { B r = m_f64slice(inc(c(Slice,x)->p), c(F64Slice,x)->a+s); dec(x); return r; }

static B f64arr_get  (B x, usz n) { VTY(x,t_f64arr  ); return m_f64(c(F64Arr  ,x)->a[n]); }
static B f64slice_get(B x, usz n) { VTY(x,t_f64slice); return m_f64(c(F64Slice,x)->a[n]); }
static void f64arr_free(Value* x) { decSh(x); }
static bool f64arr_canStore(B x) { return q_f64(x); }

void f64arr_init() {
  ti[t_f64arr].get   = f64arr_get;   ti[t_f64slice].get   = f64slice_get;
  ti[t_f64arr].getU  = f64arr_get;   ti[t_f64slice].getU  = f64slice_get;
  ti[t_f64arr].slice = f64arr_slice; ti[t_f64slice].slice = f64slice_slice;
  ti[t_f64arr].free  = f64arr_free;  ti[t_f64slice].free  =    slice_free;
  ti[t_f64arr].visit = noop_visit;   ti[t_f64slice].visit =    slice_visit;
  ti[t_f64arr].print =    arr_print; ti[t_f64slice].print = arr_print;
  ti[t_f64arr].isArr = true;         ti[t_f64slice].isArr = true;
  ti[t_f64arr].arrD1 = true;         ti[t_f64slice].arrD1 = true;
  ti[t_f64arr].elType = el_f64;      ti[t_f64slice].elType = el_f64;
  ti[t_f64arr].canStore = f64arr_canStore;
}
