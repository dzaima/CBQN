#include "h.h"

typedef struct HArr {
  struct Arr;
  B a[];
} HArr;

typedef struct HArr_p {
  B b;
  B* a;
  HArr* c;
} HArr_p;
HArr_p harr_parts(B b) {
  HArr* p = c(HArr,b);
  return (HArr_p){.b = b, .a = p->a, .c = p};
}


HArr_p m_harrv(usz ia) {
  B r = m_arr(fsizeof(HArr,a,B,ia), t_harr);
  arr_shVec(r, ia);
  return harr_parts(r);
}

HArr_p m_harrc(B x) { assert(isArr(x));
  B r = m_arr(fsizeof(HArr,a,B,a(x)->ia), t_harr);
  arr_shCopy(r, x);
  return harr_parts(r);
}

HArr_p m_harrp(usz ia) { // doesn't write any shape/size info! be careful!
  return harr_parts(m_arr(fsizeof(HArr,a,B,ia), t_harr));
}



B* harr_ptr(B x) {
  assert(v(x)->type==t_harr);
  return c(HArr,x)->a;
}

HArr* toHArr(B x) {
  if (v(x)->type==t_harr) return c(HArr,x);
  HArr_p r = m_harrc(x);
  usz ia = r.c->ia;
  BS2B xget = TI(x).get;
  for (usz i = 0; i < ia; i++) r.a[i] = xget(x,i);
  dec(x);
  return r.c;
}



B m_vaf64(usz sz, ...) {
  va_list vargs;
  va_start(vargs, sz);
  HArr_p r = m_harrv(sz);
  for (usz i = 0; i < sz; i++) r.a[i] = m_f64(va_arg(vargs, f64));
  va_end(vargs);
  return r.b;
}

B m_vaB(usz sz, ...) {
  va_list vargs;
  va_start(vargs, sz);
  HArr_p r = m_harrv(sz);
  for (usz i = 0; i < sz; i++) r.a[i] = va_arg(vargs, B);
  va_end(vargs);
  return r.b;
}

B m_v1(B a               ) { HArr_p r = m_harrv(1); r.a[0] = a;                                     return r.b; }
B m_v2(B a, B b          ) { HArr_p r = m_harrv(2); r.a[0] = a; r.a[1] = b;                         return r.b; }
B m_v3(B a, B b, B c     ) { HArr_p r = m_harrv(3); r.a[0] = a; r.a[1] = b; r.a[2] = c;             return r.b; }
B m_v4(B a, B b, B c, B d) { HArr_p r = m_harrv(4); r.a[0] = a; r.a[1] = b; r.a[2] = c; r.a[3] = d; return r.b; }



void harr_print(B x) {
  arr_print(x);
}
B harr_get(B x, usz n) {
  return inci(c(HArr,x)->a[n]);
}
void harr_free(B x) {
  decSh(x);
  B* p = harr_ptr(x);
  usz ia = a(x)->ia;
  for (usz i = 0; i < ia; i++) dec(p[i]);
}

void harr_init() {
  ti[t_harr].free = harr_free;
  ti[t_harr].print = harr_print;
  ti[t_harr].get = harr_get;
}
