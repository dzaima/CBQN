#include "../core.h"
#include "../utils/each.h"
#include "../utils/builtins.h"
#include "../nfns.h"
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
B  root_c1(B t, B x) { if (isF64(x)) return m_f64( sqrt(x.f)); P1( root); thrM("√: Getting root of non-number"); }
B floor_c1(B t, B x) { if (isF64(x)) return m_f64(floor(x.f)); P1(floor); thrM("⌊: Argument was not a number"); }
B  ceil_c1(B t, B x) { if (isF64(x)) return m_f64( ceil(x.f)); P1( ceil); thrM("⌈: Argument was not a number"); }
B stile_c1(B t, B x) { if (isF64(x)) return m_f64( fabs(x.f)); P1(stile); thrM("|: Argument was not a number"); }
B   log_c1(B t, B x) { if (isF64(x)) return m_f64(  log(x.f)); P1(  log); thrM("⋆⁼: Getting log of non-number"); }
B   sin_c1(B t, B x) { if (isF64(x)) return m_f64(  sin(x.f)); P1(  sin); thrM("•math.Sin: Argument contained non-number"); }
B   cos_c1(B t, B x) { if (isF64(x)) return m_f64(  cos(x.f)); P1(  cos); thrM("•math.Cos: Argument contained non-number"); }
B   tan_c1(B t, B x) { if (isF64(x)) return m_f64(  tan(x.f)); P1(  tan); thrM("•math.Tan: Argument contained non-number"); }
B  asin_c1(B t, B x) { if (isF64(x)) return m_f64( asin(x.f)); P1( asin); thrM("•math.Asin: Argument contained non-number"); }
B  acos_c1(B t, B x) { if (isF64(x)) return m_f64( acos(x.f)); P1( acos); thrM("•math.Acos: Argument contained non-number"); }
B  atan_c1(B t, B x) { if (isF64(x)) return m_f64( atan(x.f)); P1( atan); thrM("•math.Atan: Argument contained non-number"); }
#undef P1

B lt_c1(B t, B x) { return m_unit(x); }
B eq_c1(B t, B x) { B r = m_i32(isArr(x)? rnk(x) : 0); decR(x); return r; }
B ne_c1(B t, B x) { B r = m_f64(isArr(x)&&rnk(x)? *a(x)->sh : 1); decR(x); return r; }


static B mathNS;
B getMathNS() {
  if (mathNS.u == 0) {
    #define F(X,N) m_nfn(registerNFn(m_str32(U"•math." N), X##_c1, c2_invalid),m_f64(0)),
    B fn = bqn_exec(m_str32(U"{⟨      Sin,         Cos,         Tan,          Asin,          Acos,          Atan ⟩⇐𝕩}"), inc(bi_emptyCVec), inc(bi_emptySVec));
    B arg =    m_caB(6, (B[]){F(sin,U"Sin")F(cos,U"Cos")F(tan,U"Tan")F(asin,U"Asin")F(acos,U"Acos")F(atan,U"Atan")});
    #undef F
    mathNS = c1(fn,arg);
    gc_add(mathNS);
    dec(fn);
  }
  return inc(mathNS);
}

void arith_init() {
  c(BFn,bi_add)->ident = c(BFn,bi_sub)->ident = c(BFn,bi_or )->ident = c(BFn,bi_ne)->ident = c(BFn,bi_gt)->ident = m_i32(0);
  c(BFn,bi_mul)->ident = c(BFn,bi_div)->ident = c(BFn,bi_and)->ident = c(BFn,bi_eq)->ident = c(BFn,bi_ge)->ident = c(BFn,bi_pow)->ident = c(BFn,bi_not)->ident = m_i32(1);
  c(BFn,bi_floor)->ident = m_f64(1.0/0.0);
  c(BFn,bi_ceil )->ident = m_f64(-1.0/0.0);
}
