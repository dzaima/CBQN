#include "h.h"

typedef struct C32Arr {
  struct Arr;
  u32 a[];
} C32Arr;


B m_c32arrv(u32** p, usz ia) {
  C32Arr* r = mm_allocN(fsizeof(C32Arr,a,u32,ia), t_c32arr); B rb = tag(r, ARR_TAG);
  *p = r->a;
  arr_shVec(rb, ia);
  return rb;
}
B m_c32arrc(u32** p, B x) { assert(isArr(x));
  C32Arr* r = mm_allocN(fsizeof(C32Arr,a,u32,a(x)->ia), t_c32arr); B rb = tag(r, ARR_TAG);
  *p = r->a;
  arr_shCopy(rb, x);
  return rb;
}
B m_c32arrp(u32** p, usz ia) { // doesn't write shape/rank
  C32Arr* r = mm_allocN(fsizeof(C32Arr,a,u32,ia), t_c32arr);
  *p = r->a;
  r->ia = ia;
  return tag(r, ARR_TAG);
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


u32* c32arr_ptr(B x) { VT(x, t_c32arr); return c(C32Arr,x)->a; }
u32* c32any_ptr(B x) { assert(isArr(x)); u8 t=v(x)->type; if(t==t_c32arr) return c(C32Arr,x)->a; assert(t==t_c32slice); return c(C32Slice,x)->a; }

B m_str8(usz sz, char* s) {
  u32* rp; B r = m_c32arrv(&rp, sz);
  for (u64 i = 0; i < sz; i++) rp[i] = (u32)s[i];
  return r;
}
NOINLINE B m_str32(u32* s) {
  usz sz = 0; while(s[sz]) sz++;
  u32* rp; B r = m_c32arrv(&rp, sz);
  for (usz i = 0; i < sz; i++) rp[i] = s[i];
  return r;
}
C32Arr* toC32Arr(B x) {
  if (v(x)->type==t_c32arr) return c(C32Arr,x);
  u32* rp; B r = m_c32arrc(&rp, x);
  usz ia = a(r)->ia;
  BS2B xgetU = TI(x).getU;
  for (usz i = 0; i < ia; i++) rp[i] = o2c(xgetU(x,i));
  dec(x);
  return c(C32Arr,r);
}
bool eqStr(B w, u32* x) {
  if (isAtm(w) || rnk(w)!=1) return false;
  BS2B wgetU = TI(w).getU;
  u64 i = 0;
  while (x[i]) {
    B c = wgetU(w, i);
    if (!isC32(c) || x[i]!=(u32)c.u) return false;
    i++;
  }
  return i==a(w)->ia;
}


B c32arr_slice  (B x, usz s) {return m_c32slice(x                 , c(C32Arr  ,x)->a+s); }
B c32slice_slice(B x, usz s) { B r = m_c32slice(inc(c(Slice,x)->p), c(C32Slice,x)->a+s); dec(x); return r; }

B c32arr_get  (B x, usz n) { VT(x,t_c32arr  ); return m_c32(c(C32Arr  ,x)->a[n]); }
B c32slice_get(B x, usz n) { VT(x,t_c32slice); return m_c32(c(C32Slice,x)->a[n]); }
void c32arr_free(Value* x) { decSh(x); }
bool c32arr_canStore(B x) { return isC32(x); }

static inline void c32arr_init() {
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
}
