#include "../core.h"
#include "../utils/each.h"
#include <math.h>

#define P2(N) { if(isArr(w)|isArr(x)) return arith_recd(N##_c2, w, x); }

#define CMP_IMPL(CHR,OP,FC,CF) \
  if (isF64(w)&isF64(x)) return m_i32(w.f OP x.f); \
  if (isC32(w)&isC32(x)) return m_i32(w.u OP x.u); \
  if (isF64(w)&isC32(x)) return m_i32(FC);         \
  if (isC32(w)&isF64(x)) return m_i32(CF);         \
  bool wa = isArr(w);                              \
  bool xa = isArr(x);                              \
  if (wa|xa && (!wa|!xa || rnk(w)==rnk(x))) {      \
    if (wa&xa && !eqShape(w, x)) thrF(CHR": Expected equal shape prefix (%H ≡ ≢𝕨, %H ≡ ≢𝕩)", w, x); \
    u8 we = wa? TI(w,elType) : selfElType(w);      \
    u8 xe = xa? TI(x,elType) : selfElType(x);      \
    if (we==el_i32 && xe==el_i32) {                \
      i8* rp; B r = m_i8arrc(&rp, isArr(x)? x : w); usz ria=a(r)->ia;                                      \
      if      (!wa) { i32 wv=o2iu(w); i32* xp=i32any_ptr(x); for(usz i=0;i<ria;i++)rp[i]=wv    OP xp[i]; } \
      else if (!xa) { i32 xv=o2iu(x); i32* wp=i32any_ptr(w); for(usz i=0;i<ria;i++)rp[i]=wp[i] OP xv;    } \
      else { i32* wp=i32any_ptr(w);   i32* xp=i32any_ptr(x); for(usz i=0;i<ria;i++)rp[i]=wp[i] OP xp[i]; } \
      if(wa) dec(w); if(xa) dec(x); return r;      \
    }                                              \
    if ((we==el_f64||isNum(w)) && (xe==el_f64||isNum(x))) { \
      i8* rp; B r = m_i8arrc(&rp, isArr(x)? x : w); usz ria=a(r)->ia;                                      \
      if      (!wa) { f64 wv=o2fu(w); f64* xp=f64any_ptr(x); for(usz i=0;i<ria;i++)rp[i]=wv    OP xp[i]; } \
      else if (!xa) { f64 xv=o2fu(x); f64* wp=f64any_ptr(w); for(usz i=0;i<ria;i++)rp[i]=wp[i] OP xv;    } \
      else { f64* wp=f64any_ptr(w);   f64* xp=f64any_ptr(x); for(usz i=0;i<ria;i++)rp[i]=wp[i] OP xp[i]; } \
      if(wa) dec(w); if(xa) dec(x); return r;      \
    }                                              \
    if (we==el_c32 && xe==el_c32) {                \
      i8* rp; B r = m_i8arrc(&rp, isArr(x)? x : w); usz ria=a(r)->ia;                                      \
      if      (!wa) { u32 wv=o2cu(w); u32* xp=c32any_ptr(x); for(usz i=0;i<ria;i++)rp[i]=wv    OP xp[i]; } \
      else if (!xa) { u32 xv=o2cu(x); u32* wp=c32any_ptr(w); for(usz i=0;i<ria;i++)rp[i]=wp[i] OP xv;    } \
      else { u32* wp=c32any_ptr(w);   u32* xp=c32any_ptr(x); for(usz i=0;i<ria;i++)rp[i]=wp[i] OP xp[i]; } \
      if(wa) dec(w); if(xa) dec(x); return r;      \
    }                                              \
    if (wa&xa) {                                   \
      if (we==el_f64 && xe==el_i32) {              \
        i8* rp; B r = m_i8arrc(&rp, isArr(x)? x : w); usz ria=a(r)->ia;                           \
        f64* wp=f64any_ptr(w); i32* xp=i32any_ptr(x); for(usz i=0;i<ria;i++)rp[i]=wp[i] OP xp[i]; \
        dec(w); dec(x); return r;                  \
      }                                            \
      if (we==el_i32 && xe==el_f64) {              \
        i8* rp; B r = m_i8arrc(&rp, isArr(x)? x : w); usz ria=a(r)->ia;                           \
        i32* wp=i32any_ptr(w); f64* xp=f64any_ptr(x); for(usz i=0;i<ria;i++)rp[i]=wp[i] OP xp[i]; \
        dec(w); dec(x); return r;                  \
      }                                            \
    }                                              \
  }

#define CMP(CHR,NAME,OP,FC,CF)      \
B NAME##_c2(B t, B w, B x) {        \
  CMP_IMPL(CHR, OP, FC, CF);        \
  P2(NAME);                         \
  return m_i32(compare(w, x) OP 0); \
}
CMP("≤", le, <=, 1, 0)
CMP("≥", ge, >=, 0, 1)
CMP("<", lt, < , 1, 0)
CMP(">", gt, > , 0, 1)
#undef CMP

B eq_c2(B t, B w, B x) {
  CMP_IMPL("=", ==, 0, 0);
  P2(eq);
  B r = m_i32(atomEqual(w, x));
  dec(w); dec(x);
  return r;
}
B ne_c2(B t, B w, B x) {
  CMP_IMPL("≠", !=, 1, 1);
  P2(ne);
  B r = m_i32(!atomEqual(w, x));
  dec(w); dec(x);
  return r;
}

extern B rt_merge;
B gt_c1(B t, B x) {
  if (isAtm(x)) return x;
  return bqn_merge(x);
}
