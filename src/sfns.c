#include "h.h"

B shape_c1(B t, B x) {
  if (isArr(x)) {
    usz ia = a(x)->ia;
    if (reusable(x)) {
      decSh(x);
      arr_shVec(x, ia);
      return x;
    }
    HArr_p r = m_harrv(ia);
    BS2B xget = TI(x).get;
    for (i32 i = 0; i < ia; i++) r.a[i] = xget(x,i);
    dec(x);
    return r.b;
  } else return err("reshaping non-array");
}
B shape_c2(B t, B w, B x) {
  if (isArr(x)) {
    if (!isArr(w)) return shape_c1(t, x);
    BS2B wget = TI(w).get;
    ur nr = a(w)->ia;
    usz nia = a(x)->ia;
    B r;
    bool reuse = reusable(x);
    if (reuse) {
      r = x;
      decSh(x);
    } else {
      HArr_p rg = m_harrp(nia);
      BS2B xget = TI(x).get;
      for (usz i = 0; i < nia; i++) rg.a[i] = xget(x,i);
      r = rg.b;
    }
    usz* sh = arr_shAlloc(r, nia, nr);
    if (sh) for (i32 i = 0; i < nr; i++) sh[i] = o2i(wget(w,i));
    if (!reuse) dec(x);
    dec(w);
    return r;
  } else return err("reshaping non-array");
}

B pick_c1(B t, B x) {
  if (!isArr(x)) return x;
  // if (a(x)->ia==0) return err("⊑: called on empty array"); // no bounds check for now
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
  HArr_p r = m_harrv(xu);
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

#define ba(NAME) bi_##NAME = mm_alloc(sizeof(Fun), t_fun_def, ftag(FUN_TAG)); c(Fun,bi_##NAME)->c2 = NAME##_c2; c(Fun,bi_##NAME)->c1 = NAME##_c1 ; c(Fun,bi_##NAME)->extra=pf_##NAME;
#define bd(NAME) bi_##NAME = mm_alloc(sizeof(Fun), t_fun_def, ftag(FUN_TAG)); c(Fun,bi_##NAME)->c2 = NAME##_c2; c(Fun,bi_##NAME)->c1 = c1_invalid; c(Fun,bi_##NAME)->extra=pf_##NAME;
#define bm(NAME) bi_##NAME = mm_alloc(sizeof(Fun), t_fun_def, ftag(FUN_TAG)); c(Fun,bi_##NAME)->c2 = c2_invalid;c(Fun,bi_##NAME)->c1 = NAME##_c1 ; c(Fun,bi_##NAME)->extra=pf_##NAME;

void print_fun_def(B x) { printf("%s", format_pf(c(Fun,x)->extra)); }

B                  bi_shape, bi_pick, bi_ud, bi_pair, bi_fne, bi_lt, bi_rt;
void sfns_init() { ba(shape) ba(pick) bm(ud) ba(pair) bm(fne) ba(lt) ba(rt)
  ti[t_fun_def].print = print_fun_def;
}

#undef ba
#undef bd
#undef bm