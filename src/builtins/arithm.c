#include "../core.h"
#include "../utils/each.h"
#include <math.h>

static inline B arith_recm(BB2B f, B x) {
  B fx = getFillQ(x);
  B r = eachm_fn(f, bi_N, x);
  return withFill(r, fx);
}

#define P1(N) { if(isArr(x)) return arith_recm(N##_c1, x); }
B   add_c1(B t, B x) { return x; }
B   sub_c1(B t, B x) { if (isF64(x)) return m_f64(     -x.f ); P1(  sub); thrM("-: Negating non-number"); }
B   not_c1(B t, B x) { if (isF64(x)) return m_f64(    1-x.f ); P1(  not); thrM("¬: Argument was not a number"); }
B   mul_c1(B t, B x) { if (isF64(x)) return m_f64(x.f==0?0:x.f>0?1:-1); P1(mul); thrM("×: Getting sign of non-number"); }
B   div_c1(B t, B x) { if (isF64(x)) return m_f64(    1/x.f ); P1(  div); thrM("÷: Getting reciprocal of non-number"); }
B   pow_c1(B t, B x) { if (isF64(x)) return m_f64(  exp(x.f)); P1(  pow); thrM("⋆: Getting exp of non-number"); }
B floor_c1(B t, B x) { if (isF64(x)) return m_f64(floor(x.f)); P1(floor); thrM("⌊: Argument was not a number"); }
B  ceil_c1(B t, B x) { if (isF64(x)) return m_f64( ceil(x.f)); P1( ceil); thrM("⌈: Argument was not a number"); }
B stile_c1(B t, B x) { if (isF64(x)) return m_f64( fabs(x.f)); P1(stile); thrM("|: Argument was not a number"); }
B   log_c1(B t, B x) { if (isF64(x)) return m_f64(  log(x.f)); P1(  log); thrM("⋆⁼: Getting log of non-number"); }
#undef P1

B lt_c1(B t, B x) { return m_unit(x); }
B eq_c1(B t, B x) { B r = m_i32(isArr(x)? rnk(x) : 0); decR(x); return r; }
B ne_c1(B t, B x) { B r = m_f64(isArr(x)&&rnk(x)? *a(x)->sh : 1); decR(x); return r; }

extern B rt_sortDsc;
B or_c1(B t, B x) { return c1(rt_sortDsc, x); }
B and_c1(B t, B x); // defined in sort.c

void arith_init() {
  c(BFn,bi_add)->ident = c(BFn,bi_sub)->ident = c(BFn,bi_or )->ident = c(BFn,bi_ne)->ident = c(BFn,bi_gt)->ident = m_i32(0);
  c(BFn,bi_mul)->ident = c(BFn,bi_div)->ident = c(BFn,bi_and)->ident = c(BFn,bi_eq)->ident = c(BFn,bi_ge)->ident = c(BFn,bi_pow)->ident = c(BFn,bi_not)->ident = m_i32(1);
  c(BFn,bi_floor)->ident = m_f64(1.0/0.0);
  c(BFn,bi_ceil )->ident = m_f64(-1.0/0.0);
}
