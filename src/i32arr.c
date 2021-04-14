#include "h.h"

typedef struct I32Arr {
  struct Arr;
  i32 a[];
} I32Arr;


B m_i32arrv(usz ia) {
  B r = m_arr(fsizeof(I32Arr,a,i32,ia), t_i32arr);
  arr_shVec(r, ia);
  return r;
}
B m_i32arrc(B x) { assert(isArr(x));
  B r = m_arr(fsizeof(I32Arr,a,B,a(x)->ia), t_i32arr);
  arr_shCopy(r, x);
  return r;
}
B m_i32arrp(usz ia) { // doesn't write any shape/size info! be careful!
  return m_arr(fsizeof(I32Arr,a,i32,ia), t_i32arr);
}


i32* i32arr_ptr(B x) { VT(x, t_i32arr); return c(I32Arr,x)->a; }


NOINLINE B m_cai32(usz ia, i32* a) {
  B r = m_i32arrv(ia);
  i32* rp = i32arr_ptr(r);
  for (usz i = 0; i < ia; i++) rp[i] = a[i];
  return r;
}

I32Arr* toI32Arr(B x) {
  if (v(x)->type==t_i32arr) return c(I32Arr,x);
  B r = m_i32arrc(x);
  i32* rp = i32arr_ptr(r);
  usz ia = a(r)->ia;
  BS2B xget = TI(x).get;
  for (usz i = 0; i < ia; i++) rp[i] = o2i(xget(x,i));
  dec(x);
  return c(I32Arr,r);
}


typedef struct I32Slice {
  struct Slice;
  i32* a;
} I32Slice;
B m_i32slice(B p, i32* ptr) {
  I32Slice* r = mm_allocN(sizeof(I32Slice), t_i32slice);
  r->p = p;
  r->a = ptr;
  return tag(r, ARR_TAG);
}

B i32arr_slice  (B x, usz s) {return m_i32slice(x                    , c(I32Arr  ,x)->a+s); }
B i32slice_slice(B x, usz s) { B r = m_i32slice(inc(c(I32Slice,x)->p), c(I32Slice,x)->a+s); dec(x); return r; }


B i32arr_get  (B x, usz n) { VT(x,t_i32arr  ); return m_i32(c(I32Arr  ,x)->a[n]); }
B i32slice_get(B x, usz n) { VT(x,t_i32slice); return m_i32(c(I32Slice,x)->a[n]); }
void i32arr_free(B x) { decSh(x); }
bool i32arr_canStore(B x) { return q_i32(x); }

void i32arr_init() {
  ti[t_i32arr].get   = i32arr_get;   ti[t_i32slice].get   = i32slice_get;
  ti[t_i32arr].getU  = i32arr_get;   ti[t_i32slice].getU  = i32slice_get;
  ti[t_i32arr].slice = i32arr_slice; ti[t_i32slice].slice = i32slice_slice;
  ti[t_i32arr].free  = i32arr_free;  ti[t_i32slice].free  =    slice_free;
  ti[t_i32arr].visit = do_nothing;   ti[t_i32slice].visit =    slice_visit;
  ti[t_i32arr].print =    arr_print; ti[t_i32slice].print = arr_print;
  ti[t_i32arr].isArr = true;         ti[t_i32slice].isArr = true;
  ti[t_i32arr].canStore = i32arr_canStore;
}
