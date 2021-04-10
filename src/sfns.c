#include "h.h"

B shape_c1(B t, B x) {
  if (isArr(x)) {
    usz ia = a(x)->ia;
    if (reusable(x)) {
      decSh(x);
      arr_shVec(x, ia);
      return x;
    }
    B r = TI(x).slice(x, 0);
    arr_shVec(r, ia);
    return r;
  } else return err("reshaping non-array");
}
B shape_c2(B t, B w, B x) {
  if (isArr(x)) {
    if (!isArr(w)) return shape_c1(t, x);
    BS2B wget = TI(w).get;
    ur nr = a(w)->ia;
    usz nia = a(x)->ia;
    B r;
    if (reusable(x)) { r = x; decSh(x); }
    else r = TI(x).slice(x, 0);
    usz* sh = arr_shAlloc(r, nia, nr);
    if (sh) for (i32 i = 0; i < nr; i++) sh[i] = o2i(wget(w,i));
    dec(w);
    return r;
  } else return err("reshaping non-array");
}

B pick_c1(B t, B x) {
  if (!isArr(x)) return x;
  if (a(x)->ia==0) {
    B r = getFill(x);
    if (r.u==bi_noFill.u) return err("⊑: called on empty array without fill");
    return r;
  }
  B r = TI(x).get(x, 0);
  dec(x);
  return r;
}
B pick_c2(B t, B w, B x) {
  usz wu = o2s(w);
  if (isArr(x)) {
    // if (wu >= a(x)->ia) err("⊑: 𝕨 is greater than length of 𝕩"); // no bounds check for now
    B r = TI(x).get(x, wu);
    dec(x);
    return r;
  }
  dec(x); return err("n⊑atom");
}

B ud_c1(B t, B x) {
  usz xu = o2s(x);
  if (xu<I32_MAX) {
    B r = m_i32arrv(xu);
    i32* pr = i32arr_ptr(r);
    for (usz i = 0; i < xu; i++) pr[i] = i;
    return r;
  }
  HArr_p r = m_harrv(xu); // TODO f64arr
  for (usz i = 0; i < xu; i++) r.a[i] = m_f64(i);
  return r.b;
}

B pair_c1(B t,      B x) { return m_v1(   x); }
B pair_c2(B t, B w, B x) { return m_v2(w, x); }

B fne_c1(B t, B x) {
  if (isArr(x)) {
    ur xr = rnk(x);
    usz* sh = a(x)->sh;
    for (i32 i = 0; i < xr; i++) if (sh[i]>I32_MAX) {
      HArr_p r = m_harrv(xr);
      for (i32 j = 0; j < xr; j++) r.a[j] = m_f64(sh[j]);
      dec(x);
      return r.b;
    }
    B r = m_i32arrv(xr); i32* rp = i32arr_ptr(r);
    for (i32 i = 0; i < xr; i++) rp[i] = sh[i];
    dec(x);
    return r;
  } else {
    dec(x);
    return m_i32arrv(0);
  }
}


B lt_c1(B t,      B x) {         return x; }
B lt_c2(B t, B w, B x) { dec(x); return w; }
B rt_c1(B t,      B x) {         return x; }
B rt_c2(B t, B w, B x) { dec(w); return x; }

B fmtN_c1(B t, B x) {
  const u64 BL = 100;
  char buf[BL];
  if (isF64(x)) snprintf(buf, BL, "%g", x.f);
  else snprintf(buf, BL, "(fmtN: not given a number?)");
  return m_str8(strlen(buf), buf);
}
B fmtF_c1(B t, B x) {
  if (!isVal(x)) return m_str32(U"(fmtF: not given a function)");
  u8 fl = v(x)->flags;
  if (fl==0 || fl>=62) return m_str32(U"(fmtF: not given a runtime primitive)");
  dec(x);
  return m_c32(U"+-×÷⋆√⌊⌈|¬∧∨<>≠=≤≥≡≢⊣⊢⥊∾≍↑↓↕«»⌽⍉/⍋⍒⊏⊑⊐⊒∊⍷⊔!˙˜˘¨⌜⁼´˝`∘○⊸⟜⌾⊘◶⎉⚇⍟"[fl-1]);
}

B feq_c2(B t, B w, B x) {
  bool r = equal(w, x);
  dec(w); dec(x);
  return m_i32(r);
}

#define ba(NAME) bi_##NAME = mm_alloc(sizeof(Fun), t_fun_def, ftag(FUN_TAG)); c(Fun,bi_##NAME)->c2 = NAME##_c2; c(Fun,bi_##NAME)->c1 = NAME##_c1 ; c(Fun,bi_##NAME)->extra=pf_##NAME;
#define bd(NAME) bi_##NAME = mm_alloc(sizeof(Fun), t_fun_def, ftag(FUN_TAG)); c(Fun,bi_##NAME)->c2 = NAME##_c2; c(Fun,bi_##NAME)->c1 = c1_invalid; c(Fun,bi_##NAME)->extra=pf_##NAME;
#define bm(NAME) bi_##NAME = mm_alloc(sizeof(Fun), t_fun_def, ftag(FUN_TAG)); c(Fun,bi_##NAME)->c2 = c2_invalid;c(Fun,bi_##NAME)->c1 = NAME##_c1 ; c(Fun,bi_##NAME)->extra=pf_##NAME;

void print_fun_def(B x) { printf("%s", format_pf(c(Fun,x)->extra)); }

B                  bi_shape, bi_pick, bi_ud, bi_pair, bi_fne, bi_feq, bi_lt, bi_rt, bi_fmtF, bi_fmtN;
void sfns_init() { ba(shape) ba(pick) bm(ud) ba(pair) bm(fne) bd(feq) ba(lt) ba(rt) bm(fmtF) bm(fmtN)
  ti[t_fun_def].print = print_fun_def;
}

#undef ba
#undef bd
#undef bm
