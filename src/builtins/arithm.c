#include "../core.h"
#include "../utils/each.h"
#include "../builtins.h"
#include "../nfns.h"
#include "../ns.h"
#include <math.h>

static inline B arith_recm(BB2B f, B x) {
  B fx = getFillQ(x);
  B r = eachm_fn(bi_N, x, f);
  return withFill(r, fx);
}

void bit_negatePtr(u64* rp, u64* xp, usz count) {
  for (usz i = 0; i < count; i++) rp[i] = ~xp[i];
}
B bit_negate(B x) { // consumes
  u64* xp = bitarr_ptr(x);
  u64* rp; B r = m_bitarrc(&rp, x);
  bit_negatePtr(rp, xp, BIT_N(IA(x)));
  decG(x);
  return r;
}

#define GC1i(SYMB,NAME,FEXPR,IBAD,IEXPR,SQF,TMIN,RMIN) B NAME##_c1(B t, B x) { \
  if (isF64(x)) { f64 v = x.f; return m_f64(FEXPR); }                   \
  if (RARE(!isArr(x))) thrM(SYMB ": Expected argument to be a number"); \
  u8 xe = TI(x,elType);                                                 \
  if (xe<=TMIN) return RMIN;                                            \
  i64 sz = IA(x);                                                       \
  if (xe==el_i8) { i8 MAX=I8_MAX; i8 MIN=I8_MIN; i8* xp=i8any_ptr(x); i8* rp; B r=m_i8arrc(&rp,x);        \
    for (i64 i = 0; i < sz; i++) { i8 v = xp[i]; if (RARE(IBAD)) { decG(r); goto base; } rp[i] = IEXPR; } \
    decG(x); (void)MIN;(void)MAX; return r;                             \
  }                                                                     \
  if (xe==el_i16) { i16 MAX=I16_MAX; i16 MIN=I16_MIN; i16* xp=i16any_ptr(x); i16* rp; B r=m_i16arrc(&rp,x); \
    for (i64 i = 0; i < sz; i++) { i16 v = xp[i]; if (RARE(IBAD)) { decG(r); goto base; } rp[i] = IEXPR; }  \
    decG(x); (void)MIN;(void)MAX; return r;                             \
  }                                                                     \
  if (xe==el_i32) { i32 MAX=I32_MAX; i32 MIN=I32_MIN; i32* xp=i32any_ptr(x); i32* rp; B r=m_i32arrc(&rp,x); \
    for (i64 i = 0; i < sz; i++) { i32 v = xp[i]; if (RARE(IBAD)) { decG(r); goto base; } rp[i] = IEXPR; }  \
    decG(x); (void)MIN;(void)MAX; return r;                             \
  }                                                                     \
  if (xe==el_f64) { f64* xp = f64any_ptr(x);                            \
    f64* rp; B r = m_f64arrc(&rp, x);                                   \
    for (i64 i = 0; i < sz; i++) { f64 v = xp[i]; rp[i] = FEXPR; }      \
    decG(x); return SQF? num_squeeze(r) : r;                            \
  }                                                                     \
  base: SLOW1(SYMB"𝕩", x); return arith_recm(NAME##_c1, x);             \
}


B add_c1(B t, B x) {
  if (isF64(x)) return x;
  if (!isArr(x)) thrM("+: Argument must consist of numbers");
  if (elNum(TI(x,elType))) return x;
  dec(eachm_fn(m_f64(0), inc(x), add_c1));
  return x;
}

GC1i("-", sub,   -v,              v== MIN, -v,      0, el_bit, bit_sel(x,m_f64(0),m_f64(-1))) // change icond to v==-v to support ¯0 (TODO that won't work for i8/i16)
GC1i("|", stile, fabs(v),         v== MIN, v<0?-v:v,0, el_bit, x)
GC1i("⌊", floor, floor(v),        0,       v,       1, el_i32, x)
GC1i("⌈", ceil,  ceil(v),         0,       v,       1, el_i32, x)
GC1i("×", mul,   v==0?0:v>0?1:-1, 0,v==0?0:v>0?1:-1,1, el_bit, x)
GC1i("¬", not,   1-v,             v<=-MAX, 1-v,     0, el_bit, bit_negate(x))

#define GC1f(N, F, MSG) B N##_c1(B t, B x) {         \
  if (isF64(x)) { f64 xv=o2fG(x); return m_f64(F); } \
  if (isArr(x)) {                           \
    u8 xe = TI(x,elType);                   \
    if (elNum(xe)) {                        \
      if (xe!=el_f64) x=taga(cpyF64Arr(x)); \
      u64 ia = IA(x);                       \
      f64* xp = f64any_ptr(x);              \
      f64* rp; B r = m_f64arrc(&rp, x);     \
      for (i64 i = 0; i < ia; i++) {        \
        f64 xv=xp[i]; rp[i] = (F);          \
      }                                     \
      decG(x); return r;                    \
    }                                       \
    SLOW1("arithm " #N, x);                 \
    return arith_recm(N##_c1, x);           \
  }                                         \
  thrM(MSG);                                \
}

GC1f( div, 1/xv,     "÷: Getting reciprocal of non-number")
GC1f(root, sqrt(xv), "√: Getting square root of non-number")
#undef GC1f

f64    fact(f64 x) { return tgamma(x+1); }
f64 logfact(f64 x) { return lgamma(x+1); }

#define P1(N) { if(isArr(x)) { SLOW1("arithm " #N, x); return arith_recm(N##_c1, x); } }
B   pow_c1(B t, B x) { if (isF64(x)) return m_f64(  exp(x.f)); P1(  pow); thrM("⋆: Getting exp of non-number"); }
B   log_c1(B t, B x) { if (isF64(x)) return m_f64(  log(x.f)); P1(  log); thrM("⋆⁼: Getting log of non-number"); }
#undef P1
static NOINLINE B arith_recm_slow(f64 (*fn)(f64), BB2B rec, B x, char* s) {
  if (isF64(x)) return m_f64(fn(x.f));
  if(isArr(x)) return arith_recm(rec, x);
  thrF("•math.%S: Argument contained non-number", s);
}
#define MATH(n,N) B n##_c1(B t, B x) { return arith_recm_slow(n, n##_c1, x, #N); }
MATH(cbrt,Cbrt) MATH(log2,Log2) MATH(log10,Log10) MATH(log1p,Log1p) MATH(expm1,Expm1)
MATH(fact,Fact) MATH(logfact,LogFact) MATH(erf,Erf) MATH(erfc,ErfC)
#define TRIG(n,N) MATH(n,N) MATH(a##n,A##n) MATH(n##h,N##h) MATH(a##n##h,A##n##h)
TRIG(sin,Sin) TRIG(cos,Cos) TRIG(tan,Tan)
#undef TRIG
#undef MATH

B lt_c1(B t, B x) { return m_atomUnit(x); }
B eq_c1(B t, B x) { if (isAtm(x)) { decA(x); return m_i32(0); } B r = m_i32(RNK(x)); decG(x); return r; }
B ne_c1(B t, B x) { if (isAtm(x)) { decA(x); return m_i32(1); } B r = m_f64(*SH(x)); decG(x); return r; }


static B mathNS;
B getMathNS() {
  if (mathNS.u == 0) {
    #define F(X) inc(bi_##X),
    Body* d = m_nnsDesc("sin","cos","tan","asin","acos","atan","atan2","sinh","cosh","tanh","asinh","acosh","atanh","cbrt","log2","log10","log1p","expm1","hypot","fact","logfact","erf","erfc","comb","gcd","lcm","sum");
    mathNS = m_nns(d,  F(sin)F(cos)F(tan)F(asin)F(acos)F(atan)F(atan2)F(sinh)F(cosh)F(tanh)F(asinh)F(acosh)F(atanh)F(cbrt)F(log2)F(log10)F(log1p)F(expm1)F(hypot)F(fact)F(logfact)F(erf)F(erfc)F(comb)F(gcd)F(lcm)F(sum));
    #undef F
    gc_add(mathNS);
  }
  return inc(mathNS);
}

void arith_init() {
  c(BFn,bi_add)->ident = c(BFn,bi_sub)->ident = c(BFn,bi_or )->ident = c(BFn,bi_ne)->ident = c(BFn,bi_gt)->ident = m_i32(0);
  c(BFn,bi_mul)->ident = c(BFn,bi_div)->ident = c(BFn,bi_and)->ident = c(BFn,bi_eq)->ident = c(BFn,bi_ge)->ident = c(BFn,bi_pow)->ident = c(BFn,bi_not)->ident = m_i32(1);
  c(BFn,bi_floor)->ident = m_f64(1.0/0.0);
  c(BFn,bi_ceil )->ident = m_f64(-1.0/0.0);

  #define INVERSE_PAIR(F,G) \
    c(BFn,bi_##F)->im = G##_c1; \
    c(BFn,bi_##G)->im = F##_c1;
  c(BFn,bi_sub)->im = sub_c1;
  INVERSE_PAIR(sin, asin)
  INVERSE_PAIR(cos, acos)
  INVERSE_PAIR(tan, atan)
  INVERSE_PAIR(sinh, asinh)
  INVERSE_PAIR(cosh, acosh)
  INVERSE_PAIR(tanh, atanh)
  INVERSE_PAIR(expm1, log1p)
  #undef INVERSE_PAIR
}
