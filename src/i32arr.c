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


i32* i32arr_ptr(B x) {
  assert(v(x)->type==t_i32arr);
  return c(I32Arr,x)->a;
}


B m_vai32(usz sz, ...) {
  va_list vargs;
  va_start(vargs, sz);
  B r = m_i32arrv(sz);
  i32* rp = i32arr_ptr(r);
  for (usz i = 0; i < sz; i++) rp[i] = va_arg(vargs, i32);
  va_end(vargs);
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


void i32arr_free(B x) { decSh(x); }
void i32arr_print(B x) { arr_print(x); }
B i32arr_get(B x, usz n) { assert(v(x)->type==t_i32arr); return m_i32(c(I32Arr,x)->a[n]); }

void i32arr_init() {
  ti[t_i32arr].free = i32arr_free;
  ti[t_i32arr].print = i32arr_print;
  ti[t_i32arr].get = i32arr_get;
}

