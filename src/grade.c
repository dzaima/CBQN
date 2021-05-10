#include "h.h"

B rt_gradeUp;
B gradeUp_c1(B t, B x) {
  return c1(rt_gradeUp, x);
}
B gradeUp_c2(B t, B w, B x) {
  if (isAtm(w) || rnk(w)==0) thrM("⍋: 𝕨 must have rank≥1");
  if (isAtm(x)) x = m_atomUnit(x);
  ur wr = rnk(w);
  ur xr = rnk(x);
  
  if (wr > 1) {
    if (wr > xr+1) thrM("⍋: =𝕨 cannot be greater than =𝕩");
    i32 nxr = xr-wr+1;
    x = toKCells(x, nxr); xr = nxr;
    w = toCells(w);       xr = 1;
  }
  
  u8 we = TI(w).elType; usz wia = a(w)->ia;
  u8 xe = TI(x).elType; usz xia = a(x)->ia;
  
  if (wia>I32_MAX-10) thrM("⍋: 𝕨 too big");
  B r = m_i32arrc(x);
  i32* ri = i32arr_ptr(r);
  
  if (we==el_i32 & xe==el_i32) {
    i32* wi = i32any_ptr(w);
    i32* xi = i32any_ptr(x);
    if (CHECK_VALID) for (usz i = 0; i < (i64)wia-1; i++) if (wi[i] > wi[i+1]) thrM("⍋: 𝕨 must be sorted");
    
    for (usz i = 0; i < xia; i++) {
      i32 c = xi[i];
      usz s = 0, e = wia+1;
      while (e-s > 1) {
        usz m = (s+(i64)e)/2;
        if (c < wi[m-1]) e = m;
        else s = m;
      }
      ri[i] = s;
    }
  } else {
    BS2B wgetU = TI(w).getU;
    BS2B xgetU = TI(x).getU;
    if (CHECK_VALID) for (usz i = 0; i < wia-1; i++) if (compare(wgetU(w,i), wgetU(w,i+1)) > 0) thrM("⍋: 𝕨 must be sorted");
    
    for (usz i = 0; i < xia; i++) {
      B c = xgetU(x,i);
      usz s = 0, e = wia+1;
      while (e-s > 1) {
        usz m = (s+e) / 2;
        if (compare(c, wgetU(w,m-1)) < 0) e = m;
        else s = m;
      }
      ri[i] = s;
    }
  }
  dec(w);dec(x);
  return r;
}

#define F(A,M,D) A(gradeUp)
BI_FNS0(F);
static inline void grade_init() { BI_FNS1(F) }
#undef F
