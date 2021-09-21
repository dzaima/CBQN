#include "../core.h"
#include "../utils/each.h"
#include "../builtins.h"
#include "../nfns.h"
#include <math.h>

static inline B arith_recm(BB2B f, B x) {
  B fx = getFillQ(x);
  B r = eachm_fn(f, bi_N, x);
  return withFill(r, fx);
}

#define GC1i(SYMB, NAME, FEXPR, IBAD, IEXPR) B NAME##_c1(B t, B x) {    \
  if (isF64(x)) { f64 v = x.f; return m_f64(FEXPR); }                   \
  if (RARE(!isArr(x))) thrM(SYMB ": Expected argument to be a number"); \
  u8 xe = TI(x,elType);                                                 \
  i64 sz = a(x)->ia;                                                    \
  if (xe==el_i8) { i8 MAX=I8_MAX; i8 MIN=I8_MIN; i8* xp=i8any_ptr(x); i8* rp; B r=m_i8arrc(&rp,x); \
    for (i64 i = 0; i < sz; i++) { i8 v = xp[i]; if (RARE(IBAD)) { dec(r); goto base; } rp[i] = IEXPR; } \
    dec(x); (void)MIN;(void)MAX; return r; \
  }                   \
  if (xe==el_i16) { i16 MAX=I16_MAX; i16 MIN=I16_MIN; i16* xp=i16any_ptr(x); i16* rp; B r=m_i16arrc(&rp,x); \
    for (i64 i = 0; i < sz; i++) { i16 v = xp[i]; if (RARE(IBAD)) { dec(r); goto base; } rp[i] = IEXPR; } \
    dec(x); (void)MIN;(void)MAX; return r; \
  }                   \
  if (xe==el_i32) { i32 MAX=I32_MAX; i32 MIN=I32_MIN; i32* xp=i32any_ptr(x); i32* rp; B r=m_i32arrc(&rp,x); \
    for (i64 i = 0; i < sz; i++) { i32 v = xp[i]; if (RARE(IBAD)) { dec(r); goto base; } rp[i] = IEXPR; } \
    dec(x); (void)MIN;(void)MAX; return r; \
  }                   \
  if (xe==el_f64) { f64* xp = f64any_ptr(x);                            \
    f64* rp; B r = m_f64arrc(&rp, x);                                   \
    for (i64 i = 0; i < sz; i++) { f64 v = xp[i]; rp[i] = FEXPR; }      \
    dec(x); return r;                                                   \
  }                                                                     \
  base: return arith_recm(NAME##_c1, x);                                \
}
  

#define P1(N) { if(isArr(x)) return arith_recm(N##_c1, x); }
B   add_c1(B t, B x) { return x; }
GC1i("-", sub,   -v,              v== MIN, -v) // change icond to v==-v to support ¯0 (TODO that won't work for i8/i16)
GC1i("¬", not,   1-v,             v<=-MAX, 1-v)
GC1i("|", stile, fabs(v),         v== MIN, v<0?-v:v)
GC1i("⌊", floor, floor(v),        0,           v)
GC1i("⌈", ceil,  ceil(v),         0,           v)
GC1i("×", mul,   v==0?0:v>0?1:-1, 0,           v==0?0:v>0?1:-1)

B   div_c1(B t, B x) { if (isF64(x)) return m_f64(    1/x.f ); P1(  div); thrM("÷: Getting reciprocal of non-number"); }
B   pow_c1(B t, B x) { if (isF64(x)) return m_f64(  exp(x.f)); P1(  pow); thrM("⋆: Getting exp of non-number"); }
B  root_c1(B t, B x) { if (isF64(x)) return m_f64( sqrt(x.f)); P1( root); thrM("√: Getting root of non-number"); }
B   log_c1(B t, B x) { if (isF64(x)) return m_f64(  log(x.f)); P1(  log); thrM("⋆⁼: Getting log of non-number"); }
B   sin_c1(B t, B x) { if (isF64(x)) return m_f64(  sin(x.f)); P1(  sin); thrM("•math.Sin: Argument contained non-number"); }
B   cos_c1(B t, B x) { if (isF64(x)) return m_f64(  cos(x.f)); P1(  cos); thrM("•math.Cos: Argument contained non-number"); }
B   tan_c1(B t, B x) { if (isF64(x)) return m_f64(  tan(x.f)); P1(  tan); thrM("•math.Tan: Argument contained non-number"); }
B  asin_c1(B t, B x) { if (isF64(x)) return m_f64( asin(x.f)); P1( asin); thrM("•math.Asin: Argument contained non-number"); }
B  acos_c1(B t, B x) { if (isF64(x)) return m_f64( acos(x.f)); P1( acos); thrM("•math.Acos: Argument contained non-number"); }
B  atan_c1(B t, B x) { if (isF64(x)) return m_f64( atan(x.f)); P1( atan); thrM("•math.Atan: Argument contained non-number"); }
#undef P1

B lt_c1(B t, B x) { return m_atomUnit(x); }
B eq_c1(B t, B x) { B r = m_i32(isArr(x)? rnk(x) : 0); dec(x); return r; }
B ne_c1(B t, B x) { B r = m_f64(isArr(x)&&rnk(x)? *a(x)->sh : 1); dec(x); return r; }


static B mathNS;
B getMathNS() {
  if (mathNS.u == 0) {
    #define F(X,N) m_nfn(registerNFn(m_str32(U"•math." N), X##_c1, c2_bad),m_f64(0)),
    B fn = bqn_exec(m_str32(U"{⟨      Sin,         Cos,         Tan,          Asin,          Acos,          Atan ⟩⇐𝕩}"), emptyCVec(), emptySVec());
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
