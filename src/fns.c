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
    i32* rp; B r = m_i32arrv(&rp, xu);
    for (usz i = 0; i < xu; i++) rp[i] = i;
    return r;
  }
  f64* rp; B r = m_f64arrv(&rp, xu);
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
  if (fl==0 || fl>rtLen) {
    u8 ty = v(x)->type;
    if (ty==t_funBI) { B r = fromUTF8l(format_pf (c(Fun,x)->extra)); dec(x); return r; }
    if (ty==t_md1BI) { B r = fromUTF8l(format_pm1(c(Md1,x)->extra)); dec(x); return r; }
    if (ty==t_md2BI) { B r = fromUTF8l(format_pm2(c(Md2,x)->extra)); dec(x); return r; }
    return m_str32(U"(fmtF: not given a runtime primitive)");
  }
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
      f64* rp; B r = m_f64arrv(&rp, xr);
      for (i32 j = 0; j < xr; j++) rp[j] = sh[j];
      dec(x);
      return r;
    }
    i32* rp;
    B r = m_i32arrv(&rp, xr);
    for (i32 i = 0; i < xr; i++) rp[i] = sh[i];
    dec(x);
    return r;
  } else {
    dec(x);
    return inc(bi_emptyIVec);
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


#define BI_A(N) { B t=bi_##N = mm_alloc(sizeof(BFn), t_funBI, ftag(FUN_TAG)); BFn*f=c(BFn,t); f->c2=N##_c2    ; f->c1=N##_c1    ; f->extra=pf_##N; f->ident=bi_N; gc_add(t); }
#define BI_D(N) { B t=bi_##N = mm_alloc(sizeof(BFn), t_funBI, ftag(FUN_TAG)); BFn*f=c(BFn,t); f->c2=N##_c2    ; f->c1=c1_invalid; f->extra=pf_##N; f->ident=bi_N; gc_add(t); }
#define BI_M(N) { B t=bi_##N = mm_alloc(sizeof(BFn), t_funBI, ftag(FUN_TAG)); BFn*f=c(BFn,t); f->c2=c2_invalid; f->c1=N##_c1    ; f->extra=pf_##N; f->ident=bi_N; gc_add(t); }
#define BI_VAR(N) B bi_##N;
#define BI_FNS0(F) F(BI_VAR,BI_VAR,BI_VAR)
#define BI_FNS1(F) F(BI_A,BI_M,BI_D)


#define F(A,M,D) M(ud) A(fne) A(feq) A(ltack) A(rtack) M(fmtF) M(fmtN)
BI_FNS0(F);
static inline void fns_init() { BI_FNS1(F)
  ti[t_funBI].print = print_funBI;
  ti[t_funBI].identity = funBI_identity;
}
#undef F