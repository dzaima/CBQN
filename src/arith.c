#include "h.h"
#include <math.h>

#define P1(N) { if(         isArr(x)) return eachm_fn(N##_c1, bi_nothing,    x); }
#define P2(N) { if(isArr(w)|isArr(x)) return eachd_fn(N##_c2, bi_nothing, w, x); }
#define ffnx(name, expr, extra) B name##_c2(B t, B w, B x) { \
  if (isF64(w) & isF64(x)) return m_f64(expr); \
  extra \
  P2(name) \
  thrM(#name ": invalid arithmetic"); \
}
#define ffn(name, op, extra) ffnx(name, w.f op x.f, extra)

ffn(add, +, {
  if (isC32(w) & isF64(x)) { u64 r = (u64)(u32)w.u + o2i64(x); if(r>CHR_MAX)thrM("+: Invalid character"); return m_c32(r); }
  if (isF64(w) & isC32(x)) { u64 r = (u64)(u32)x.u + o2i64(w); if(r>CHR_MAX)thrM("+: Invalid character"); return m_c32(r); }
})
ffn(sub, -, {
  if (isC32(w) & isF64(x)) { u64 r = (u64)(u32)w.u - o2i64(x); if(r>CHR_MAX)thrM("-: Invalid character"); return m_c32(r); }
  if (isC32(w) & isC32(x)) return m_f64((u32)w.u - (i64)(u32)x.u);
})
ffn(mul, *, {})
ffn(and, *, {})
ffn(div, /, {})
ffnx(pow, pow(w.f,x.f), {})
ffnx(floor, fmin(w.f, x.f), {})
ffnx(ceil, fmax(w.f, x.f), {})
f64 pfmod(f64 a, f64 b) {
  f64 r = fmod(a, b);
  if (a<0 != b<0 && r) r+= b;
  return r;
}
ffnx(stile, pfmod(x.f, w.f), {})
ffnx(log, log(x.f)/log(w.f), {})
ffnx(or, (w.f+x.f)-(w.f*x.f), {})
ffnx(not, 1+w.f-x.f, {})

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
  if (w.u==x.u)               { dec(w);dec(x); return m_i32(1); }
  // doesn't handle int=float
  if (!isVal(w) | !isVal(x))  { dec(w);dec(x); return m_i32(0); }
  if (v(w)->type!=v(x)->type) { dec(w);dec(x); return m_i32(0); }
  B2B dcf = TI(w).decompose;
  if (dcf == def_decompose)   { dec(w);dec(x); return m_i32(0); }
  w=dcf(w); B* wp = harr_ptr(w);
  x=dcf(x); B* xp = harr_ptr(x);
  if (o2i(wp[0])<=1)          { dec(w);dec(x); return m_i32(0); }
  i32 wia = a(w)->ia;
  i32 xia = a(x)->ia;
  if (wia != xia)             { dec(w);dec(x); return m_i32(0); }
  for (i32 i = 0; i<wia; i++) if(!equal(wp[i], xp[i]))
                              { dec(w);dec(x); return m_i32(0); }
                                dec(w);dec(x); return m_i32(1);
}
B ne_c2(B t, B w, B x) {
  if(isF64(w)&isF64(x)) return m_i32(w.f!=x.f);
  P2(ne);
  return m_i32(1-o2i(eq_c2(t,w,x)));
}


B   add_c1(B t, B x) { return x; }
B   sub_c1(B t, B x) { if (isF64(x)) return m_f64(     -x.f );       P1(  sub); thrM("-: Negating non-number"); }
B   not_c1(B t, B x) { if (isF64(x)) return m_f64(    1-x.f );       P1(  not); thrM("¬: Argument was not a number"); }
B   mul_c1(B t, B x) { if (isF64(x)) return m_f64(x.f?x.f>0?1:-1:0); P1(  mul); thrM("×: Getting sign of non-number"); }
B   div_c1(B t, B x) { if (isF64(x)) return m_f64(    1/x.f );       P1(  div); thrM("÷: Getting reciprocal of non-number"); }
B   pow_c1(B t, B x) { if (isF64(x)) return m_f64(  exp(x.f));       P1(  pow); thrM("⋆: Getting exp of non-number"); }
B floor_c1(B t, B x) { if (isF64(x)) return m_f64(floor(x.f));       P1(floor); thrM("⌊: Argument was not a number"); }
B  ceil_c1(B t, B x) { if (isF64(x)) return m_f64( ceil(x.f));       P1( ceil); thrM("⌈: Argument was not a number"); }
B stile_c1(B t, B x) { if (isF64(x)) return m_f64( fabs(x.f));       P1(stile); thrM("|: Argument was not a number"); }
B   log_c1(B t, B x) { if (isF64(x)) return m_f64(  log(x.f));       P1(  log); thrM("⋆⁼: Getting log of non-number"); }

B lt_c1(B t, B x) { return m_unit(x); }
B eq_c1(B t, B x) { B r = m_i32(isArr(x)? rnk(x) : 0); decR(x); return r; }
B ne_c1(B t, B x) { B r = m_f64(isArr(x)&&rnk(x)? *a(x)->sh : 1); decR(x); return r; }

B rt_sortAsc, rt_sortDsc, rt_merge;
B and_c1(B t, B x) { return c1(rt_sortAsc, x); }
B  or_c1(B t, B x) { return c1(rt_sortDsc, x); }
B  gt_c1(B t, B x) { return c1(rt_merge,   x); }

#undef P1
#undef P2

#define ba(N) bi_##N = mm_alloc(sizeof(BFn), t_funBI, ftag(FUN_TAG)); c(Fun,bi_##N)->c2 = N##_c2    ;c(Fun,bi_##N)->c1 = N##_c1    ; c(Fun,bi_##N)->extra=pf_##N; c(BFn,bi_##N)->ident=bi_nothing; gc_add(bi_##N);
#define bd(N) bi_##N = mm_alloc(sizeof(BFn), t_funBI, ftag(FUN_TAG)); c(Fun,bi_##N)->c2 = N##_c2    ;c(Fun,bi_##N)->c1 = c1_invalid; c(Fun,bi_##N)->extra=pf_##N; c(BFn,bi_##N)->ident=bi_nothing; gc_add(bi_##N);
#define bm(N) bi_##N = mm_alloc(sizeof(BFn), t_funBI, ftag(FUN_TAG)); c(Fun,bi_##N)->c2 = c2_invalid;c(Fun,bi_##N)->c1 = N##_c1    ; c(Fun,bi_##N)->extra=pf_##N; c(BFn,bi_##N)->ident=bi_nothing; gc_add(bi_##N);

B                   bi_add, bi_sub, bi_mul, bi_div, bi_pow, bi_floor, bi_ceil, bi_stile, bi_eq, bi_ne, bi_le, bi_ge, bi_lt, bi_gt, bi_and, bi_or, bi_not, bi_log;
static inline void arith_init() { ba(add) ba(sub) ba(mul) ba(div) ba(pow) ba(floor) ba(ceil) ba(stile) ba(eq) ba(ne) bd(le) bd(ge) ba(lt) ba(gt) ba(and) ba(or) ba(not) ba(log)
  c(BFn,bi_add)->ident = c(BFn,bi_sub)->ident = c(BFn,bi_or )->ident = c(BFn,bi_eq)->ident = c(BFn,bi_ne)->ident = m_i32(0);
  c(BFn,bi_mul)->ident = c(BFn,bi_div)->ident = c(BFn,bi_and)->ident = c(BFn,bi_eq)->ident = c(BFn,bi_ge)->ident = c(BFn,bi_pow)->ident = c(BFn,bi_not)->ident = m_i32(1);
  c(BFn,bi_floor)->ident = m_f64(1.0/0.0);
  c(BFn,bi_ceil )->ident = m_f64(-1.0/0.0);
}

#undef ba
#undef bd
#undef bm
