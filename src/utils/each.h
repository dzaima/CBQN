#pragma once
#include "mut.h"

B eachd_fn(B fo, B w, B x, BBB2B f); // consumes w,x; assumes at least one is array
B eachm_fn(B fo, B x, BB2B f); // consumes x; x must be array

static B eachm(B f, B x) { // complete F¨ x without fills; consumes x
  if (isAtm(x)) return m_hunit(c1(f, x));
  if (isFun(f)) return eachm_fn(f, x, c(Fun,f)->c1);
  if (isMd(f)) if (isAtm(x) || a(x)->ia) { decR(x); thrM("Calling a modifier"); }
  
  usz ia = a(x)->ia;
  MAKE_MUT(r, ia);
  mut_fill(r, 0, f, ia);
  return mut_fcd(r, x);
}

static B eachd(B f, B w, B x) { // complete w F¨ x without fills; consumes w,x
  if (isAtm(w) & isAtm(x)) return m_hunit(c2(f, w, x));
  return eachd_fn(f, w, x, c2fn(f));
}


#if CATCH_ERRORS
static inline B arith_recd(BBB2B f, B w, B x) {
  B fx = getFillQ(x);
  if (noFill(fx)) return eachd_fn(bi_N, w, x, f);
  B fw = getFillQ(w);
  B r = eachd_fn(bi_N, w, x, f);
  if (noFill(fw)) { dec(fx); return r; }
  if (CATCH) { freeThrown(); return r; }
  B fr = f(bi_N, fw, fx);
  popCatch();
  return withFill(r, asFill(fr));
}
#else
static inline B arith_recd(BBB2B f, B w, B x) {
  return eachd_fn(bi_N, w, x, f);
}
#endif
