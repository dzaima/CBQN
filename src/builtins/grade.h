#define GRADE_CAT(N) CAT(GRADE_UD(gradeUp,gradeDown),N)

#define SORT_CMP(W, X) GRADE_NEG compare((W).k, (X).k)
#define SORT_NAME GRADE_CAT(BP)
#define SORT_TYPE BI32p
#include "sortTemplate.h"

#define SORT_CMP(W, X) (GRADE_NEG ((W).k - (i64)(X).k))
#define SORT_NAME GRADE_CAT(IP)
#define SORT_TYPE I32I32p
#include "sortTemplate.h"

B GRADE_CAT(c1)(B t, B x) {
  if (isAtm(x) || rnk(x)==0) thrM(GRADE_CHR": Argument cannot be a unit");
  if (rnk(x)>1) x = toCells(x);
  usz ia = IA(x);
  if (ia>I32_MAX) thrM(GRADE_CHR": Argument too large");
  if (ia==0) { decG(x); return emptyIVec(); }
  
  u8 xe = TI(x,elType);
  i32* rp; B r = m_i32arrv(&rp, ia);
  if (xe==el_bit) {
    u64* xp = bitarr_ptr(x);
    u64 sum = bit_sum(xp, ia);
    u64 r0 = 0;
    u64 r1 = GRADE_UD(ia-sum, sum);
    for (usz i = 0; i < ia; i++) {
      if (bitp_get(xp,i)^GRADE_UD(0,1)) rp[r1++] = i;
      else                              rp[r0++] = i;
    }
    decG(x); return r;
  } else if (xe==el_i8) {
    i8* xp = i8any_ptr(x);
    i32 min=-128, range=256;
    TALLOC(usz, tmp, range+1);
    for (i64 i = 0; i < range+1; i++) tmp[i] = 0;
    GRADE_UD( // i8 range-based
      for (usz i = 0; i < ia; i++) (tmp-min+1)[xp[i]]++;
      for (i64 i = 1; i < range; i++) tmp[i]+= tmp[i-1];
      for (usz i = 0; i < ia; i++) rp[(tmp-min)[xp[i]]++] = i;
    ,
      for (usz i = 0; i < ia; i++) (tmp-min)[xp[i]]++;
      for (i64 i = range-2; i >= 0; i--) tmp[i]+= tmp[i+1];
      for (usz i = 0; i < ia; i++) rp[(tmp-min+1)[xp[i]]++] = i;
    )
    TFREE(tmp); decG(x);
    return r;
  }
  if (xe==el_i32 || xe==el_c32) { // safe to use the same comparison for i32 & c32 as c32 is 0≤x≤1114111
    i32* xp = tyany_ptr(x);
    i32 min=I32_MAX, max=I32_MIN;
    for (usz i = 0; i < ia; i++) {
      i32 c = xp[i];
      if (c<min) min=c;
      if (c>max) max=c;
    }
    i64 range = max - (i64)min + 1;
    if (range/2 < ia) {
      TALLOC(usz, tmp, range+1);
      for (i64 i = 0; i < range+1; i++) tmp[i] = 0;
      GRADE_UD( // i32 range-based
        for (usz i = 0; i < ia; i++) (tmp-min+1)[xp[i]]++;
        for (i64 i = 1; i < range; i++) tmp[i]+= tmp[i-1];
        for (usz i = 0; i < ia; i++) rp[(tmp-min)[xp[i]]++] = i;
      ,
        for (usz i = 0; i < ia; i++) (tmp-min)[xp[i]]++;
        for (i64 i = range-2; i >= 0; i--) tmp[i]+= tmp[i+1];
        for (usz i = 0; i < ia; i++) rp[(tmp-min+1)[xp[i]]++] = i;
      )
      TFREE(tmp); decG(x);
      return r;
    }
    
    TALLOC(I32I32p, tmp, ia);
    for (usz i = 0; i < ia; i++) {
      tmp[i].v = i;
      tmp[i].k = xp[i];
    }
    CAT(GRADE_CAT(IP),tim_sort)(tmp, ia);
    for (usz i = 0; i < ia; i++) rp[i] = tmp[i].v;
    TFREE(tmp); decG(x);
    return r;
  }
  
  SLOW1(GRADE_UD("⍋","⍒")"𝕩", x);
  TALLOC(BI32p, tmp, ia);
  SGetU(x)
  for (usz i = 0; i < ia; i++) {
    tmp[i].v = i;
    tmp[i].k = GetU(x,i);
  }
  CAT(GRADE_CAT(BP),tim_sort)(tmp, ia);
  for (usz i = 0; i < ia; i++) rp[i] = tmp[i].v;
  TFREE(tmp); decG(x);
  return r;
}
B GRADE_CAT(c2)(B t, B w, B x) {
  if (isAtm(w) || rnk(w)==0) thrM(GRADE_CHR": 𝕨 must have rank≥1");
  if (isAtm(x)) x = m_atomUnit(x);
  ur wr = rnk(w);
  ur xr = rnk(x);
  
  if (wr > 1) {
    if (wr > xr+1) thrM(GRADE_CHR": =𝕨 cannot be greater than =𝕩");
    i32 nxr = xr-wr+1;
    x = toKCells(x, nxr); xr = nxr;
    w = toCells(w);       xr = 1;
  }
  
  u8 we = TI(w,elType); usz wia = IA(w);
  u8 xe = TI(x,elType); usz xia = IA(x);
  
  if (wia>I32_MAX-10) thrM(GRADE_CHR": 𝕨 too big");
  i32* rp; B r = m_i32arrc(&rp, x);
  
  u8 fl = GRADE_UD(fl_asc,fl_dsc);
  
  if (we<=el_i32 & xe<=el_i32) {
    w = toI32Any(w); i32* wi = i32any_ptr(w);
    x = toI32Any(x); i32* xi = i32any_ptr(x);
    if (CHECK_VALID && !FL_HAS(w,fl)) {
      for (i64 i = 0; i < (i64)wia-1; i++) if ((wi[i]-wi[i+1]) GRADE_UD(>,<) 0) thrM(GRADE_CHR": 𝕨 must be sorted"GRADE_UD(," in descending order"));
      FL_SET(w, fl);
    }
    
    for (usz i = 0; i < xia; i++) {
      i32 c = xi[i];
      usz s = 0, e = wia+1;
      while (e-s > 1) {
        usz m = (s+(i64)e)/2;
        if (c GRADE_UD(<,>) wi[m-1]) e = m;
        else s = m;
      }
      rp[i] = s;
    }
  } else {
    SGetU(x)
    SLOW2("𝕨"GRADE_UD("⍋","⍒")"𝕩", w, x);
    B* wp = arr_bptr(w);
    if (wp==NULL) {
      HArr* a = toHArr(w);
      wp = a->a;
      w = taga(a);
    }
    if (CHECK_VALID && !FL_HAS(w,fl)) {
      for (i64 i = 0; i < (i64)wia-1; i++) if (compare(wp[i], wp[i+1]) GRADE_UD(>,<) 0) thrM(GRADE_CHR": 𝕨 must be sorted"GRADE_UD(," in descending order"));
      FL_SET(w, fl);
    }
    
    for (usz i = 0; i < xia; i++) {
      B c = GetU(x,i);
      usz s = 0, e = wia+1;
      while (e-s > 1) {
        usz m = (s+e) / 2;
        if (compare(c, wp[m-1]) GRADE_UD(<,>) 0) e = m;
        else s = m;
      }
      rp[i] = s;
    }
  }
  decG(w);decG(x);
  return r;
}
#undef GRADE_CAT
#undef GRADE_CHR
#undef GRADE_NEG
#undef GRADE_UD
