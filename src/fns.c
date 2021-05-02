#include "h.h"

typedef struct BFn {
  struct Fun;
  B ident;
} BFn;

void print_funBI(B x) { printf("%s", format_pf(c(Fun,x)->extra)); }
B funBI_identity(B x) { return inc(c(BFn,x)->ident); }



B ud_c1(B t, B x) {
  usz xu = o2s(x);
  if (xu<I32_MAX) {
    B r = m_i32arrv(xu); i32* rp = i32arr_ptr(r);
    for (usz i = 0; i < xu; i++) rp[i] = i;
    return r;
  }
  B r = m_f64arrv(xu); f64* rp = f64arr_ptr(r);
  for (usz i = 0; i < xu; i++) rp[i] = i;
  return r;
}

B pair_c1(B t,      B x) { return m_v1(   x); }
B pair_c2(B t, B w, B x) { return m_v2(w, x); }
B ltack_c1(B t,      B x) {         return x; }
B ltack_c2(B t, B w, B x) { dec(x); return w; }
B rtack_c1(B t,      B x) {         return x; }
B rtack_c2(B t, B w, B x) { dec(w); return x; }

B fmtN_c1(B t, B x) {
  #define BL 100
  char buf[BL];
  if (isF64(x)) snprintf(buf, BL, "%g", x.f);
  else snprintf(buf, BL, "(fmtN: not given a number?)");
  return m_str8(strlen(buf), buf);
}
B fmtF_c1(B t, B x) {
  if (!isVal(x)) return m_str32(U"(fmtF: not given a function)");
  u8 fl = v(x)->flags;
  if (fl==0 || fl>rtLen) return m_str32(U"(fmtF: not given a runtime primitive)");
  dec(x);
  return m_c32(U"+-×÷⋆√⌊⌈|¬∧∨<>≠=≤≥≡≢⊣⊢⥊∾≍↑↓↕«»⌽⍉/⍋⍒⊏⊑⊐⊒∊⍷⊔!˙˜˘¨⌜⁼´˝`∘○⊸⟜⌾⊘◶⎉⚇⍟⎊"[fl-1]);
}

i64 isum(B x) { // doesn't consume; assumes is array; may error
  BS2B xgetU = TI(x).getU;
  i64 r = 0;
  usz xia = a(x)->ia;
  for (usz i = 0; i < xia; i++) r+= (i64)o2f(xgetU(x,i)); // TODO error on overflow and non-integers or something
  return r;
}


B fne_c1(B t, B x) {
  if (isArr(x)) {
    ur xr = rnk(x);
    usz* sh = a(x)->sh;
    for (i32 i = 0; i < xr; i++) if (sh[i]>I32_MAX) {
      B r = m_f64arrv(xr); f64* rp = f64arr_ptr(r);
      for (i32 j = 0; j < xr; j++) rp[j] = sh[j];
      dec(x);
      return r;
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
u64 depth(B x) { // doesn't consume
  if (isAtm(x)) return 0;
  if (TI(x).arrD1) return 1;
  u64 r = 0;
  usz ia = a(x)->ia;
  BS2B xgetU = TI(x).getU;
  for (usz i = 0; i < ia; i++) {
    u64 n = depth(xgetU(x,i));
    if (n>r) r = n;
  }
  return r+1;
}
B feq_c1(B t, B x) {
  u64 r = depth(x);
  dec(x);
  return m_f64(r);
}


B feq_c2(B t, B w, B x) {
  bool r = equal(w, x);
  dec(w); dec(x);
  return m_i32(r);
}
B fne_c2(B t, B w, B x) {
  bool r = !equal(w, x);
  dec(w); dec(x);
  return m_i32(r);
}


#define ba(N) bi_##N = mm_alloc(sizeof(BFn), t_funBI, ftag(FUN_TAG)); c(Fun,bi_##N)->c2 = N##_c2    ;c(Fun,bi_##N)->c1 = N##_c1    ; c(Fun,bi_##N)->extra=pf_##N; c(BFn,bi_##N)->ident=bi_N; gc_add(bi_##N);
#define bd(N) bi_##N = mm_alloc(sizeof(BFn), t_funBI, ftag(FUN_TAG)); c(Fun,bi_##N)->c2 = N##_c2    ;c(Fun,bi_##N)->c1 = c1_invalid; c(Fun,bi_##N)->extra=pf_##N; c(BFn,bi_##N)->ident=bi_N; gc_add(bi_##N);
#define bm(N) bi_##N = mm_alloc(sizeof(BFn), t_funBI, ftag(FUN_TAG)); c(Fun,bi_##N)->c2 = c2_invalid;c(Fun,bi_##N)->c1 = N##_c1    ; c(Fun,bi_##N)->extra=pf_##N; c(BFn,bi_##N)->ident=bi_N; gc_add(bi_##N);



B                               bi_ud, bi_fne, bi_feq, bi_ltack, bi_rtack, bi_fmtF, bi_fmtN;
static inline void fns_init() { bm(ud) ba(fne) ba(feq) ba(ltack) ba(rtack) bm(fmtF) bm(fmtN)
  ti[t_funBI].print = print_funBI;
  ti[t_funBI].identity = funBI_identity;
}

#undef ba
#undef bd
#undef bm
