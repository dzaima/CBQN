#include "h.h"
#include <math.h>

#define ffnx(name, expr, extra) B name(B t, B w, B x) { \
  if (isF64(w) & isF64(x)) return m_f64(expr); \
  extra \
  return err(#name ": invalid arithmetic"); \
}
#define ffn(name, op, extra) ffnx(name, w.f op x.f, extra)

ffn(add_c2, +, {
  if (isC32(w) & isF64(x)) return m_c32((u32)w.u + o2i(x));
  if (isF64(w) & isC32(x)) return m_c32((u32)x.u + o2i(w));
})
ffn(sub_c2, -, {
  if (isC32(w) & isF64(x)) return m_c32((u32)w.u - o2i(x));
  if (isC32(w) & isC32(x)) return m_f64((u32)w.u - (i64)(u32)x.u);
})
ffn(mul_c2, *, {})
ffn(div_c2, /, {})
ffn(le_c2, <=, {
  if (isC32(w) & isC32(x)) return m_f64(w.u<=x.u);
  if (isF64(w) & isC32(x)) return m_f64(1);
  if (isC32(w) & isF64(x)) return m_f64(0);
})
ffnx(pow_c2, pow(w.f,x.f), {})
ffnx(log_c2, log(x.f)/log(w.f), {})

#undef ffn
#undef ffnx

B decp_c1(B t, B x);
B eq_c2(B t, B w, B x) {
  if(isF64(w)&isF64(x)) return m_i32(w.f==x.f);
  if (w.u==x.u) return m_i32(1);
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
  for (i32 i = 0; i<wia; i++) if(!o2i(eq_c2(t,inc(wp[i]),inc(xp[i]))))
                              { dec(w);dec(x); return m_i32(0); }
                                dec(w);dec(x); return m_i32(1);
}


B   add_c1(B t, B x) { return x; }
B   sub_c1(B t, B x) { if (isF64(x)) return m_f64(     -x.f ); return err("negating non-number"); }
B   mul_c1(B t, B x) { if (isF64(x)) return m_f64(x.f?x.f>0?1:-1:0); return err("getting sign of non-number"); }
B   div_c1(B t, B x) { if (isF64(x)) return m_f64(    1/x.f ); return err("getting reciprocal of non-number"); }
B   pow_c1(B t, B x) { if (isF64(x)) return m_f64(  exp(x.f)); return err("getting exp of non-number"); }
B floor_c1(B t, B x) { if (isF64(x)) return m_f64(floor(x.f)); return err("getting floor of non-number"); }
B   log_c1(B t, B x) { if (isF64(x)) return m_f64(  log(x.f)); return err("getting log of non-number"); }
B    eq_c1(B t, B x) { B r = m_i32(isArr(x)? rnk(x) : 0); decR(x); return r; }



#define ba(NAME) bi_##NAME = mm_alloc(sizeof(Fun), t_fun_def, ftag(FUN_TAG)); c(Fun,bi_##NAME)->c2 = NAME##_c2; c(Fun,bi_##NAME)->c1 = NAME##_c1 ; c(Fun,bi_##NAME)->extra=pf_##NAME;
#define bd(NAME) bi_##NAME = mm_alloc(sizeof(Fun), t_fun_def, ftag(FUN_TAG)); c(Fun,bi_##NAME)->c2 = NAME##_c2; c(Fun,bi_##NAME)->c1 = c1_invalid; c(Fun,bi_##NAME)->extra=pf_##NAME;
#define bm(NAME) bi_##NAME = mm_alloc(sizeof(Fun), t_fun_def, ftag(FUN_TAG)); c(Fun,bi_##NAME)->c2 = c2_invalid;c(Fun,bi_##NAME)->c1 = NAME##_c1 ; c(Fun,bi_##NAME)->extra=pf_##NAME;

B                   bi_add, bi_sub, bi_mul, bi_div, bi_pow, bi_floor, bi_eq, bi_le, bi_log;
void arith_init() { ba(add) ba(sub) ba(mul) ba(div) ba(pow) bm(floor) ba(eq) bd(le) ba(log) }

#undef ba
#undef bd
#undef bm
