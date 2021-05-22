#include "sort.h"

B gradeUp_c1(B t, B x) {
  if (isAtm(x) || rnk(x)==0) thrM("⍋: Argument cannot be a unit");
  if (rnk(x)>1) x = toCells(x);
  usz ia = a(x)->ia;
  if (ia>I32_MAX) thrM("⍋: Argument too large");
  if (ia==0) return inc(bi_emptyIVec);
  
  i32* rp; B r = m_i32arrv(&rp, ia);
  if (TI(x).elType==el_i32) {
    i32* xp = i32any_ptr(x);
    i32 min=I32_MAX, max=I32_MIN;
    for (usz i = 0; i < ia; i++) {
      i32 c = xp[i];
      if (c<min) min=c;
      if (c>max) max=c;
    }
    i64 range = max - (i64)min + 1;
    if (range/2 < ia) {
      TALLOC(usz, tmp, range+1);
      for (i64 i = 0; i < range; i++) tmp[i] = 0;
      for (usz i = 0; i < ia; i++) (tmp-min+1)[xp[i]]++;
      for (i64 i = 1; i < range; i++) tmp[i]+= tmp[i-1];
      for (usz i = 0; i < ia; i++) rp[(tmp-min)[xp[i]]++] = i;
      
      TFREE(tmp); dec(x);
      return r;
    }
    
    TALLOC(I32I32p, tmp, ia);
    for (usz i = 0; i < ia; i++) {
      tmp[i].v = i;
      tmp[i].k = xp[i];
    }
    ip_tim_sort(tmp, ia);
    for (usz i = 0; i < ia; i++) rp[i] = tmp[i].v;
    TFREE(tmp); dec(x);
    return r;
  }
  
  TALLOC(BI32p, tmp, ia);
  BS2B xgetU = TI(x).getU;
  for (usz i = 0; i < ia; i++) {
    tmp[i].v = i;
    tmp[i].k = xgetU(x,i);
  }
  bp_tim_sort(tmp, ia);
  for (usz i = 0; i < ia; i++) rp[i] = tmp[i].v;
  TFREE(tmp); dec(x);
  return r;
}
B gradeUp_c2(B t, B w, B x) {
  if (isAtm(w) || rnk(w)==0) thrM("⍋: 𝕨 must have rank≥1");
  if (isAtm(x)) x = m_atomUnit(x);
  ur wr = rnk(w);
  ur xr = rnk(x);
  
  if (wr > 1) {
    if (wr > xr+1) thrM("⍋: =𝕨 cannot be greater than =𝕩");
    i32 nxr = xr-wr+1;
    x = toKCells(x, nxr); xr = nxr;
    w = toCells(w);       xr = 1;
  }
  
  u8 we = TI(w).elType; usz wia = a(w)->ia;
  u8 xe = TI(x).elType; usz xia = a(x)->ia;
  
  if (wia>I32_MAX-10) thrM("⍋: 𝕨 too big");
  i32* rp; B r = m_i32arrc(&rp, x);
  
  if (we==el_i32 & xe==el_i32) {
    i32* wi = i32any_ptr(w);
    i32* xi = i32any_ptr(x);
    if (CHECK_VALID) for (usz i = 0; i < (i64)wia-1; i++) if (wi[i] > wi[i+1]) thrM("⍋: 𝕨 must be sorted");
    
    for (usz i = 0; i < xia; i++) {
      i32 c = xi[i];
      usz s = 0, e = wia+1;
      while (e-s > 1) {
        usz m = (s+(i64)e)/2;
        if (c < wi[m-1]) e = m;
        else s = m;
      }
      rp[i] = s;
    }
  } else {
    BS2B wgetU = TI(w).getU;
    BS2B xgetU = TI(x).getU;
    if (CHECK_VALID) for (usz i = 0; i < wia-1; i++) if (compare(wgetU(w,i), wgetU(w,i+1)) > 0) thrM("⍋: 𝕨 must be sorted");
    
    for (usz i = 0; i < xia; i++) {
      B c = xgetU(x,i);
      usz s = 0, e = wia+1;
      while (e-s > 1) {
        usz m = (s+e) / 2;
        if (compare(c, wgetU(w,m-1)) < 0) e = m;
        else s = m;
      }
      rp[i] = s;
    }
  }
  dec(w);dec(x);
  return r;
}

int sort_icmp(const void* w, const void* x) { return *(int*)w - *(int*)x; }
int sort_bcmp(const void* w, const void* x) { return compare(*(B*)w, *(B*)x); }
B rt_sortAsc;
B and_c1(B t, B x) {
  if (isAtm(x) || rnk(x)==0) thrM("∧: Argument cannot have rank 0");
  if (rnk(x)!=1) return bqn_merge(and_c1(t, toCells(x)));
  usz xia = a(x)->ia;
  if (TI(x).elType==el_i32) {
    i32* xp = i32any_ptr(x);
    i32* rp; B r = m_i32arrv(&rp, xia);
    memcpy(rp, xp, xia*4);
    i_tim_sort(rp, xia);
    dec(x);
    return r;
  }
  B xf = getFillQ(x);
  HArr_p r = m_harrUv(xia);
  BS2B xget = TI(x).get;
  for (usz i = 0; i < xia; i++) r.a[i] = xget(x,i);
  b_tim_sort(r.a, xia);
  dec(x);
  return withFill(r.b,xf);
}

#define F(A,M,D) A(gradeUp)
BI_FNS0(F);
static inline void sort_init() { BI_FNS1(F) }
#undef F
