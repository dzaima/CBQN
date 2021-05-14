#include "h.h"
#include <math.h>

static inline B arith_recm(BB2B f, B x) {
  B fx = getFillQ(x);
  B r = eachm_fn(f, bi_N, x);
  return withFill(r, fx);
}
#ifdef CATCH_ERRORS
static inline B arith_recd(BBB2B f, B w, B x) {
  B fx = getFillQ(x);
  if (noFill(fx)) return eachd_fn(f, bi_N, w, x);
  B fw = getFillQ(w);
  B r = eachd_fn(f, bi_N, w, x);
  if (noFill(fw)) return r;
  if (CATCH) { dec(catchMessage); return r; }
  B fr = f(bi_N, fw, fx);
  popCatch();
  return withFill(r, asFill(fr));
}
#else
static inline B arith_recd(BBB2B f, B w, B x) {
  return eachd_fn(f, bi_N, w, x);
}
#endif


#define P1(N) { if(         isArr(x)) return arith_recm(N##_c1,    x); }
#define P2(N) { if(isArr(w)|isArr(x)) return arith_recd(N##_c2, w, x); }
#define ffnx(name, expr, extra) B name##_c2(B t, B w, B x) { \
  if (isF64(w) & isF64(x)) return m_f64(expr); \
  extra \
  P2(name) \
  thrM(#name ": invalid arithmetic"); \
}
#define ffn(name, op, extra) ffnx(name, w.f op x.f, extra)

f64 pfmod(f64 a, f64 b) {
  f64 r = fmod(a, b);
  if (a<0 != b<0 && r!=0) r+= b;
  return r;
}

ffn(add, +, {
  if (isC32(w) & isF64(x)) { u64 r = (u64)(o2cu(w)+o2i64(x)); if(r>CHR_MAX)thrM("+: Invalid character"); return m_c32((u32)r); }
  if (isF64(w) & isC32(x)) { u64 r = (u64)(o2cu(x)+o2i64(w)); if(r>CHR_MAX)thrM("+: Invalid character"); return m_c32((u32)r); }
})
ffn(sub, -, {
  if (isC32(w) & isF64(x)) { u64 r = (u64)(o2cu(w)-o2u64(x)); if(r>CHR_MAX)thrM("-: Invalid character"); return m_c32((u32)r); }
  if (isC32(w) & isC32(x)) return m_f64((i32)(u32)w.u - (i32)(u32)x.u);
})
ffn(mul, *, {})
ffn(and, *, {})
ffn(div, /, {})
ffnx(pow  ,       pow(w.f, x.f), {})
ffnx(floor,      fmin(w.f, x.f), {})
ffnx(ceil ,      fmax(w.f, x.f), {})
ffnx(stile,     pfmod(x.f, w.f), {})
ffnx(log  ,   log(x.f)/log(w.f), {})
ffnx(or   , (w.f+x.f)-(w.f*x.f), {})
ffnx(not  , 1+w.f-x.f          , {})

#define CMP(X, N, G) \
  ffn(N, X, { \
    if (isC32(w) & isC32(x)) return m_f64(w.u X x.u); \
    if (isF64(w) & isC32(x)) return m_f64(1-G); \
    if (isC32(w) & isF64(x)) return m_f64(G); \
  })
CMP(<=, le, 0)
CMP(>=, ge, 1)
CMP(< , lt, 0)
CMP(> , gt, 1)
#undef CMP

#undef ffn
#undef ffnx

B decp_c1(B t, B x);
B eq_c2(B t, B w, B x) {
  if(isF64(w)&isF64(x)) return m_i32(w.f==x.f);
  P2(eq);
  B r = m_i32(atomEqual(w, x));
  dec(w); dec(x);
  return r;
}
B ne_c2(B t, B w, B x) {
  if(isF64(w)&isF64(x)) return m_i32(w.f!=x.f);
  P2(ne);
  B r = m_i32(!atomEqual(w, x));
  dec(w); dec(x);
  return r;
}


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

B lt_c1(B t, B x) { return m_unit(x); }
B eq_c1(B t, B x) { B r = m_i32(isArr(x)? rnk(x) : 0); decR(x); return r; }
B ne_c1(B t, B x) { B r = m_f64(isArr(x)&&rnk(x)? *a(x)->sh : 1); decR(x); return r; }

B rt_sortAsc, rt_sortDsc, rt_merge;
B and_c1(B t, B x) { return c1(rt_sortAsc, x); }
B  or_c1(B t, B x) { return c1(rt_sortDsc, x); }
B  gt_c1(B t, B x) { return c1(rt_merge,   x); }

#undef P1
#undef P2

#define F(A,M,D) A(add) A(sub) A(mul) A(div) A(pow) A(floor) A(ceil) A(stile) A(eq) A(ne) D(le) D(ge) A(lt) A(gt) A(and) A(or) A(not) A(log)
BI_FNS0(F);
static inline void arith_init() { BI_FNS1(F)
  c(BFn,bi_add)->ident = c(BFn,bi_sub)->ident = c(BFn,bi_or )->ident = c(BFn,bi_ne)->ident = c(BFn,bi_gt)->ident = m_i32(0);
  c(BFn,bi_mul)->ident = c(BFn,bi_div)->ident = c(BFn,bi_and)->ident = c(BFn,bi_eq)->ident = c(BFn,bi_ge)->ident = c(BFn,bi_pow)->ident = c(BFn,bi_not)->ident = m_i32(1);
  c(BFn,bi_floor)->ident = m_f64(1.0/0.0);
  c(BFn,bi_ceil )->ident = m_f64(-1.0/0.0);
}
#undef F