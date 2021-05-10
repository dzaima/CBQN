#include "h.h"

typedef struct F64Arr {
  struct Arr;
  f64 a[];
} F64Arr;


B m_f64arrv(usz ia) {
  B r = m_arr(fsizeof(F64Arr,a,f64,ia), t_f64arr);
  arr_shVec(r, ia);
  return r;
}
B m_f64arrc(B x) { assert(isArr(x));
  B r = m_arr(fsizeof(F64Arr,a,f64,a(x)->ia), t_f64arr);
  arr_shCopy(r, x);
  return r;
}
B m_f64arrp(usz ia) { // doesn't write shape/rank
  B r = m_arr(fsizeof(F64Arr,a,f64,ia), t_f64arr);
  a(r)->ia = ia;
  return r;
}


typedef struct F64Slice {
  struct Slice;
  f64* a;
} F64Slice;
B m_f64slice(B p, f64* ptr) {
  F64Slice* r = mm_allocN(sizeof(F64Slice), t_f64slice);
  r->p = p;
  r->a = ptr;
  return tag(r, ARR_TAG);
}


f64* f64arr_ptr(B x) { VT(x, t_f64arr); return c(F64Arr,x)->a; }
f64* f64any_ptr(B x) { assert(isArr(x)); u8 t=v(x)->type; if(t==t_f64arr) return c(F64Arr,x)->a; assert(t==t_f64slice); return c(F64Slice,x)->a; }

NOINLINE B m_caf64(usz sz, f64* a) {
  B r = m_f64arrv(sz); f64* rp = f64arr_ptr(r);
  for (usz i = 0; i < sz; i++) rp[i] = a[i];
  return r;
}

F64Arr* toF64Arr(B x) {
  if (v(x)->type==t_f64arr) return c(F64Arr,x);
  B r = m_f64arrc(x);
  f64* rp = f64arr_ptr(r);
  usz ia = a(r)->ia;
  BS2B xgetU = TI(x).getU;
  for (usz i = 0; i < ia; i++) rp[i] = o2f(xgetU(x,i));
  dec(x);
  return c(F64Arr,r);
}


B f64arr_slice  (B x, usz s) {return m_f64slice(x                 , c(F64Arr  ,x)->a+s); }
B f64slice_slice(B x, usz s) { B r = m_f64slice(inc(c(Slice,x)->p), c(F64Slice,x)->a+s); dec(x); return r; }

B f64arr_get  (B x, usz n) { VT(x,t_f64arr  ); return m_f64(c(F64Arr  ,x)->a[n]); }
B f64slice_get(B x, usz n) { VT(x,t_f64slice); return m_f64(c(F64Slice,x)->a[n]); }
void f64arr_free(B x) { decSh(x); }
bool f64arr_canStore(B x) { return q_f64(x); }

static inline void f64arr_init() {
  ti[t_f64arr].get   = f64arr_get;   ti[t_f64slice].get   = f64slice_get;
  ti[t_f64arr].getU  = f64arr_get;   ti[t_f64slice].getU  = f64slice_get;
  ti[t_f64arr].slice = f64arr_slice; ti[t_f64slice].slice = f64slice_slice;
  ti[t_f64arr].free  = f64arr_free;  ti[t_f64slice].free  =    slice_free;
  ti[t_f64arr].visit = do_nothing;   ti[t_f64slice].visit =    slice_visit;
  ti[t_f64arr].print =    arr_print; ti[t_f64slice].print = arr_print;
  ti[t_f64arr].isArr = true;         ti[t_f64slice].isArr = true;
  ti[t_f64arr].arrD1 = true;         ti[t_f64slice].arrD1 = true;
  ti[t_f64arr].elType = el_f64;      ti[t_f64slice].elType = el_f64;
  ti[t_f64arr].canStore = f64arr_canStore;
}
