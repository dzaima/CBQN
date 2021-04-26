#include "h.h"

typedef struct C32Arr {
  struct Arr;
  u32 a[];
} C32Arr;


B m_c32arrv(usz ia) {
  B r = m_arr(fsizeof(C32Arr,a,u32,ia), t_c32arr);
  arr_shVec(r, ia);
  return r;
}
B m_c32arrc(B x) { assert(isArr(x));
  B r = m_arr(fsizeof(C32Arr,a,B,a(x)->ia), t_c32arr);
  arr_shCopy(r, x);
  return r;
}
B m_c32arrp(usz ia) { // doesn't write any shape/size info! be careful!
  return m_arr(fsizeof(C32Arr,a,u32,ia), t_c32arr);
}


u32* c32arr_ptr(B x) { VT(x, t_c32arr); return c(C32Arr,x)->a; }

B m_str8(usz sz, char* s) {
  B r = m_c32arrv(sz); u32* p = c32arr_ptr(r);
  for (u64 i = 0; i < sz; i++) p[i] = s[i];
  return r;
}
NOINLINE B m_str32(u32* s) {
  u64 sz = 0; while(s[sz]) sz++;
  B r = m_c32arrv(sz); u32* p = c32arr_ptr(r);
  for (u64 i = 0; i < sz; i++) p[i] = s[i];
  return r;
}

typedef struct C32Slice {
  struct Slice;
  u32* a;
} C32Slice;
B m_c32slice(B p, u32* ptr) {
  C32Slice* r = mm_allocN(sizeof(C32Slice), t_c32slice);
  r->p = p;
  r->a = ptr;
  return tag(r, ARR_TAG);
}

B c32arr_slice  (B x, usz s) {return m_c32slice(x                 , c(C32Arr  ,x)->a+s); }
B c32slice_slice(B x, usz s) { B r = m_c32slice(inc(c(Slice,x)->p), c(C32Slice,x)->a+s); dec(x); return r; }


B c32arr_get  (B x, usz n) { VT(x,t_c32arr  ); return m_c32(c(C32Arr  ,x)->a[n]); }
B c32slice_get(B x, usz n) { VT(x,t_c32slice); return m_c32(c(C32Slice,x)->a[n]); }
void c32arr_free(B x) { decSh(x); }
bool c32arr_canStore(B x) { return isC32(x); }


bool eqStr(B w, u32* x) {
  if (!isArr(w) || rnk(w)!=1) return false;
  BS2B wget = TI(w).get;
  u64 i = 0;
  while (x[i]) {
    B c = wget(w, i);
    if (!isC32(c) || x[i]!=(u32)c.u) return false;
    i++;
  }
  return i==a(w)->ia;
}


static inline void c32arr_init() {
  ti[t_c32arr].get   = c32arr_get;   ti[t_c32slice].get   = c32slice_get;
  ti[t_c32arr].getU  = c32arr_get;   ti[t_c32slice].getU  = c32slice_get;
  ti[t_c32arr].slice = c32arr_slice; ti[t_c32slice].slice = c32slice_slice;
  ti[t_c32arr].free  = c32arr_free;  ti[t_c32slice].free  =    slice_free;
  ti[t_c32arr].visit = do_nothing;   ti[t_c32slice].visit =    slice_visit;
  ti[t_c32arr].print =    arr_print; ti[t_c32slice].print = arr_print;
  ti[t_c32arr].isArr = true;         ti[t_c32slice].isArr = true;
  ti[t_i32arr].arrD1 = true;         ti[t_i32slice].arrD1 = true;
  ti[t_c32arr].canStore = c32arr_canStore;
}
