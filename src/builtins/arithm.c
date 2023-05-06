#include "../core.h"
#include "../utils/each.h"
#include "../builtins.h"
#include "../nfns.h"
#include "../ns.h"
#include <math.h>

static inline B arith_recm(FC1 f, B x) {
  B fx = getFillR(x);
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

B add_c1(B t, B x) {
  if (isF64(x)) return x;
  if (!isArr(x)) thrM("+: Argument must consist of numbers");
  if (elNum(TI(x,elType))) return x;
  decG(eachm_fn(m_f64(0), incG(x), add_c1));
  return x;
}
#if SINGELI_SIMD
  #define SINGELI_FILE monarith
  #include "../utils/includeSingeli.h"
#endif

#define GC1i(SYMB,NAME,FEXPR,TMIN,RMIN,MAIN) B NAME##_c1(B t, B x) { \
  if (isF64(x)) { f64 v = x.f; return m_f64(FEXPR); } \
  if (RARE(!isArr(x))) thrM(SYMB ": Argument contained non-number"); \
  u8 xe = TI(x,elType);                               \
  if (elNum(xe)) {                                    \
    if (xe<=TMIN) return RMIN;                        \
    MAIN(FEXPR)                                       \
  }                                                   \
  SLOW1(SYMB"𝕩", x); return arith_recm(NAME##_c1, x); \
}

#define LOOP_BODY(INIT, EXPR, POST) { \
  i64 ia = IA(x); INIT;               \
  void* xp = tyany_ptr(x);            \
  switch(xe) { default: UD;           \
    case el_i8:  for(usz i=0; i<ia; i++) { i8  c = ((i8* )xp)[i]; EXPR(i8,  c==I8_MIN)  } break; \
    case el_i16: for(usz i=0; i<ia; i++) { i16 c = ((i16*)xp)[i]; EXPR(i16, c==I16_MIN) } break; \
    case el_i32: for(usz i=0; i<ia; i++) { i32 c = ((i32*)xp)[i]; EXPR(i32, c==I32_MIN) } break; \
    case el_f64: for(usz i=0; i<ia; i++) { f64 c = ((f64*)xp)[i]; EXPR(f64, 0)          } break; \
  } \
  decG(x); return r; POST \
}

#define SIGN_EXPR(T, C) rp[i] = c>0? 1 : c==0? 0 : -1;
#define SIGN_MAIN(FEXPR) LOOP_BODY(i8* rp; B r=m_i8arrc(&rp,x);, SIGN_EXPR,)

#if SINGELI_SIMD
  #define STILE_BODY(FEXPR) { usz ia = IA(x); B r; retry:; \
    void* rp = m_tyarrlc(&r, elWidth(xe), x, el2t(xe));    \
    u64 got = simd_abs[xe-el_i8](rp, tyany_ptr(x), ia);    \
    if (LIKELY(got==ia)) { decG(x); return r; }            \
    tyarr_freeF(v(r));                                     \
    xe++;if (xe==el_i16) x=taga(cpyI16Arr(x));             \
    else if (xe==el_i32) x=taga(cpyI32Arr(x));             \
    else                 x=taga(cpyF64Arr(x));             \
    goto retry;                                            \
  }
#else
  #define STILE_EXPR(T, C) if(C) goto bad;  ((T*)rp)[i] = c>=0? c : -c;
  #define STILE_BODY(FEXPR) LOOP_BODY(B r; void* rp = m_tyarrlc(&r, elWidth(xe), x, el2t(xe));, STILE_EXPR, bad: tyarr_freeF(v(r));)
#endif

#define FLOAT_BODY(FEXPR) { i64 ia = IA(x);                  \
  assert(xe==el_f64); f64* xp = f64any_ptr(x);               \
  f64* rp; B r = m_f64arrc(&rp, x);                          \
  for (usz i = 0; i < ia; i++) { f64 v=xp[i]; rp[i]=FEXPR; } \
  decG(x); return num_squeeze(r);                            \
}
B sub_c2(B,B,B);
#define SUB_BODY(FEXPR) return sub_c2(t, m_f64(0), x);
#define NOT_BODY(FEXPR) x = num_squeezeChk(x); return TI(x,elType)==el_bit? bit_negate(x) : C2(sub, m_f64(1), x);

GC1i("-", sub,    -v,              el_bit, bit_sel(x,m_f64(0),m_f64(-1)), SUB_BODY)
GC1i("|", stile,  fabs(v),         el_bit, x, STILE_BODY)
GC1i("⌊", floor,  floor(v),        el_i32, x, FLOAT_BODY)
GC1i("⌈", ceil,   ceil(v),         el_i32, x, FLOAT_BODY)
GC1i("×", mul,    v==0?0:v>0?1:-1, el_bit, x, SIGN_MAIN)
GC1i("¬", not,    1-v,             el_bit, bit_negate(x), NOT_BODY)

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

GC1f( div, 1/(xv+0), "÷: Argument contained non-number")
GC1f(root, sqrt(xv), "√: Argument contained non-number")
#undef GC1i
#undef LOOP_BODY
#undef SIGN_EXPR
#undef SIGN_MAIN
#undef STILE_BODY
#undef STILE_EXPR
#undef STILE_BODY
#undef FLOAT_BODY
#undef SUB_BODY
#undef NOT_BODY
#undef GC1f

f64    fact(f64 x) { return tgamma(x+1); }
f64 logfact(f64 x) { return lgamma(x+1); }
NOINLINE f64 logfact_inv(f64 y) {
  if (!(y >= -0.12)) thrM("⁼: required factorial result too small");
  if (y == INFINITY) return y;
  f64 x = 4;
  PLAINLOOP for (usz i = 0; i < 20; i++) {
    f64 x0 = x;
    x += (y - logfact(x)) / log(0.52 + x);
    if (x == x0) break;
  }
  return x;
}
f64 fact_inv(f64 y) { return logfact_inv(log(y)); }

#define P1(N) { if(isArr(x)) { SLOW1("arithm " #N, x); return arith_recm(N##_c1, x); } }
B   pow_c1(B t, B x) { if (isF64(x)) return m_f64(  exp(x.f)); P1(  pow); thrM("⋆: Argument contained non-number"); }
B   log_c1(B t, B x) { if (isF64(x)) return m_f64(  log(x.f)); P1(  log); thrM("⋆⁼: Argument contained non-number"); }
#undef P1
static NOINLINE B arith_recm_slow(f64 (*fn)(f64), FC1 rec, B x, char* s) {
  if (isF64(x)) return m_f64(fn(x.f));
  if(isArr(x)) return arith_recm(rec, x);
  thrF("•math.%S: Argument contained non-number", s);
}
#define MATH(n,N) B n##_c1(B t, B x) { return arith_recm_slow(n, n##_c1, x, #N); }
MATH(cbrt,Cbrt) MATH(log2,Log2) MATH(log10,Log10) MATH(log1p,Log1p) MATH(expm1,Expm1)
MATH(fact,Fact) MATH(logfact,LogFact) MATH(logfact_inv,LogFact⁼) MATH(fact_inv,Fact⁼) MATH(erf,Erf) MATH(erfc,ErfC)
#define TRIG(n,N) MATH(n,N) MATH(a##n,A##n) MATH(n##h,N##h) MATH(a##n##h,A##n##h)
TRIG(sin,Sin) TRIG(cos,Cos) TRIG(tan,Tan)
#undef TRIG
#undef MATH

B lt_c1(B t, B x) { return m_unit(x); }
B eq_c1(B t, B x) { if (isAtm(x)) { decA(x); return m_i32(0); } B r = m_i32(RNK(x)); decG(x); return r; }
B ne_c1(B t, B x) { if (isAtm(x)) { decA(x); return m_i32(1); } B r = m_f64(*SH(x)); decG(x); return r; }


static B mathNS;
B getMathNS(void) {
  if (mathNS.u == 0) {
    #define F(X) incG(bi_##X),
    Body* d = m_nnsDesc("sin","cos","tan","asin","acos","atan","atan2","sinh","cosh","tanh","asinh","acosh","atanh","cbrt","log2","log10","log1p","expm1","hypot","fact","logfact","erf","erfc","comb","gcd","lcm","sum");
    mathNS = m_nns(d,  F(sin)F(cos)F(tan)F(asin)F(acos)F(atan)F(atan2)F(sinh)F(cosh)F(tanh)F(asinh)F(acosh)F(atanh)F(cbrt)F(log2)F(log10)F(log1p)F(expm1)F(hypot)F(fact)F(logfact)F(erf)F(erfc)F(comb)F(gcd)F(lcm)F(sum));
    #undef F
    gc_add(mathNS);
  }
  return incG(mathNS);
}

void arithm_init(void) {
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
  c(BFn,bi_fact)->im = fact_inv_c1;
  c(BFn,bi_logfact)->im = logfact_inv_c1;
  #undef INVERSE_PAIR
}
