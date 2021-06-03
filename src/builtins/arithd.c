#include "../core.h"
#include "../utils/each.h"
#include <math.h>

#define P2(N) { if(isArr(w)|isArr(x)) return arith_recd(N##_c2, w, x); }
#if TYPED_ARITH
  static B f64_maybe_i32(B x) {
    f64* xp = f64arr_ptr(x);
    usz ia = a(x)->ia;
    if (ia==0) {
      if (rnk(x)>1) return x;
      dec(x); return inc(bi_emptyIVec);
    }
    if (xp[0] != (i32)xp[0]) return x;
    i32* rp; B r = m_i32arrc(&rp, x);
    for (usz i = 0; i < ia; i++) {
      f64 cf = xp[i];
      i32 c = (i32)cf;
      if (cf!=c) { dec(r); return x; }
      rp[i] = c;
    }
    dec(x);
    return r;
  }
  #define GC2f(NAME, EXPR, EXTRA) B NAME##_c2(B t, B w, B x) {               \
    if (isF64(w) & isF64(x)) return m_f64(EXPR);                             \
    EXTRA                                                                    \
    if (isArr(w)|isArr(x)) { B ow=w; B ox=x;                                 \
      if (isArr(w)&isArr(x) && rnk(w)==rnk(x)) {                             \
        if (memcmp(a(w)->sh, a(x)->sh, rnk(w)*sizeof(usz))) thrF(#NAME ": Expected equal shape prefix (%H ≡ ≢𝕨, %H ≡ ≢𝕩)", w, x); \
        usz ia = a(x)->ia;                                                   \
        u8 we = TI(w).elType;                                                \
        u8 xe = TI(x).elType;                                                \
        if (isNumEl(we)&isNumEl(xe)) {                                       \
          f64* rp; B r = m_f64arrc(&rp, x);                                  \
          if (we==el_i32) { B w,x/*shadow*/; i32* wp = i32any_ptr(ow);       \
            if (xe==el_i32) { i32* xp = i32any_ptr(ox); for (usz i = 0; i < ia; i++) {w.f=wp[i];x.f=xp[i];rp[i]=EXPR;} } \
            else            { f64* xp = f64any_ptr(ox); for (usz i = 0; i < ia; i++) {w.f=wp[i];x.f=xp[i];rp[i]=EXPR;} } \
          } else {          B w,x/*shadow*/; f64* wp = f64any_ptr(ow);       \
            if (xe==el_i32) { i32* xp = i32any_ptr(ox); for (usz i = 0; i < ia; i++) {w.f=wp[i];x.f=xp[i];rp[i]=EXPR;} } \
            else            { f64* xp = f64any_ptr(ox); for (usz i = 0; i < ia; i++) {w.f=wp[i];x.f=xp[i];rp[i]=EXPR;} } \
          }                                                                  \
          dec(w); dec(x); return f64_maybe_i32(r);                           \
        }                                                                    \
      } else if (isF64(w)&isArr(x)) { usz ia = a(x)->ia;                     \
        u8 xe = TI(x).elType; f64*rp;                                        \
        if (xe==el_i32) { B r=m_f64arrc(&rp, x); i32*xp=i32any_ptr(x);       \
          for (usz i = 0; i < ia; i++) {B x/*shadow*/;x.f=xp[i];rp[i]=EXPR;} \
          dec(x); return f64_maybe_i32(r);                                   \
        }                                                                    \
        if (xe==el_f64) { B r=m_f64arrc(&rp, x); f64*xp=f64any_ptr(x);       \
          for (usz i = 0; i < ia; i++) {B x/*shadow*/;x.f=xp[i];rp[i]=EXPR;} \
          dec(x); return f64_maybe_i32(r);                                   \
        }                                                                    \
      } else if (isF64(x)&isArr(w)) { usz ia = a(w)->ia;                     \
        u8 we = TI(w).elType; f64*rp;                                        \
        if (we==el_i32) { B r=m_f64arrc(&rp, w); i32*wp=i32any_ptr(w);       \
          for (usz i = 0; i < ia; i++) {B w/*shadow*/;w.f=wp[i];rp[i]=EXPR;} \
          dec(w); return f64_maybe_i32(r);                                   \
        }                                                                    \
        if (we==el_f64) { B r=m_f64arrc(&rp, w); f64*wp=f64any_ptr(w);       \
          for (usz i = 0; i < ia; i++) {B w/*shadow*/;w.f=wp[i];rp[i]=EXPR;} \
          dec(w); return f64_maybe_i32(r);                                   \
        }                                                                    \
      }                                                                      \
      P2(NAME)                                                               \
    }                                                                        \
    thrM(#NAME ": invalid arithmetic");                                      \
  }
  
  #define PF(N) f64* N##p = f64any_ptr(N);
  #define PI(N) i32* N##p = i32any_ptr(N);
  #define RI(A) i32* rp; B r=m_i32arrc(&rp, A);
  #define RF(A) f64* rp; B r=m_f64arrc(&rp, A);
  
  #define DOF(EXPR,A,W,X) {        \
    for (usz i = 0; i < ia; i++) { \
      f64 wv = W; f64 xv = X;      \
      rp[i] = EXPR;                \
    }                              \
  }
  #define DOI(EXPR,A,W,X,BASE) {   \
    RI(A)                          \
    for (usz i = 0; i < ia; i++) { \
      i64 wv = W; i64 xv = X;      \
      i64 rv = EXPR;               \
      if (RARE(rv!=(i32)rv)) {     \
        dec(r);                    \
        goto BASE;                 \
      }                            \
      rp[i] = rv;                  \
    }                              \
    dec(w); dec(x);                \
    return r;                      \
  }
  #define GC2i(NAME, EXPR, EXTRA) B NAME##_c2(B t, B w, B x) {       \
    if (isF64(w) & isF64(x)) {f64 wv=w.f,xv=x.f;return m_f64(EXPR);} \
    EXTRA                                                            \
    if (isArr(w)|isArr(x)) {                                         \
      if (isArr(w)&isArr(x) && rnk(w)==rnk(x)) {                     \
        if (memcmp(a(w)->sh, a(x)->sh, rnk(w)*sizeof(usz))) thrF(#NAME ": Expected equal shape prefix (%H ≡ ≢𝕨, %H ≡ ≢𝕩)", w, x); \
        usz ia = a(x)->ia;                                           \
        u8 we = TI(w).elType;                                        \
        u8 xe = TI(x).elType;                                        \
        if (isNumEl(we)&isNumEl(xe)) {                               \
          bool wei = we==el_i32; bool xei = xe==el_i32;              \
          if (wei&xei) { PI(w) PI(x) DOI(EXPR,w,wp[i],xp[i],aaB); }  \
          aaB:; RF(x)                                                \
          if (wei) { PI(w)                                           \
            if (xei) { PI(x) DOF(EXPR,w,wp[i],xp[i]) }               \
            else     { PF(x) DOF(EXPR,w,wp[i],xp[i]) }               \
          } else { PF(w)                                             \
            if (xei) { PI(x) DOF(EXPR,w,wp[i],xp[i]) }               \
            else     { PF(x) DOF(EXPR,w,wp[i],xp[i]) }               \
          }                                                          \
          dec(w); dec(x); return f64_maybe_i32(r);                   \
        }                                                            \
      } else if (isF64(w)&isArr(x)) { usz ia = a(x)->ia; u8 xe = TI(x).elType;                 \
        if (xe==el_i32 && q_i32(w)) { PI(x) i32 wc=o2iu(w); DOI(EXPR,x,wc,xp[i],naB) } naB:;   \
        if (xe==el_i32) { RF(x) PI(x) DOF(EXPR,w,w.f,xp[i]) dec(x); return f64_maybe_i32(r); } \
        if (xe==el_f64) { RF(x) PF(x) DOF(EXPR,w,w.f,xp[i]) dec(x); return f64_maybe_i32(r); } \
      } else if (isF64(x)&isArr(w)) { usz ia = a(w)->ia; u8 we = TI(w).elType;                 \
        if (we==el_i32 && q_i32(x)) { PI(w) i32 xc=o2iu(x); DOI(EXPR,w,wp[i],xc,anB) } anB:;   \
        if (we==el_i32) { RF(w) PI(w) DOF(EXPR,x,wp[i],x.f) dec(w); return f64_maybe_i32(r); } \
        if (we==el_f64) { RF(w) PF(w) DOF(EXPR,x,wp[i],x.f) dec(w); return f64_maybe_i32(r); } \
      }                                                              \
      P2(NAME)                                                       \
    }                                                                \
    thrM(#NAME ": invalid arithmetic");                              \
  }
#else // if !TYPED_ARITH
  #define GC2i(NAME, EXPR, EXTRA) B NAME##_c2(B t, B w, B x) { \
    if (isF64(w) & isF64(x)) { f64 wv=w.f; f64 xv=x.f; return m_f64(EXPR); } \
    EXTRA \
    P2(NAME) \
    thrM(#NAME ": invalid arithmetic"); \
  }
  #define GC2f(NAME, EXPR, EXTRA) B NAME##_c2(B t, B w, B x) { \
    if (isF64(w) & isF64(x)) return m_f64(EXPR); \
    EXTRA \
    P2(NAME) \
    thrM(#NAME ": invalid arithmetic"); \
  }
#endif // TYPED_ARITH

static f64 pfmod(f64 a, f64 b) {
  f64 r = fmod(a, b);
  if (a<0 != b<0 && r!=0) r+= b;
  return r;
}

GC2i(add, wv+xv, {
  if (isC32(w) & isF64(x)) { u64 r = (u64)(o2cu(w)+o2i64(x)); if(r>CHR_MAX)thrM("+: Invalid character"); return m_c32((u32)r); }
  if (isF64(w) & isC32(x)) { u64 r = (u64)(o2cu(x)+o2i64(w)); if(r>CHR_MAX)thrM("+: Invalid character"); return m_c32((u32)r); }
  if (isArr(w)&isC32(x) || isC32(w)&isArr(x)) { if (isArr(w)) { B t=w;w=x;x=t; }
    if (TI(x).elType == el_i32) {
      u32 wv = o2cu(w);
      i32* xp = i32any_ptr(x); usz xia = a(x)->ia;
      u32* rp; B r = m_c32arrc(&rp, x);
      for (usz i = 0; i < xia; i++) {
        rp[i] = (u32)(xp[i]+(i32)wv);
        if (rp[i]>CHR_MAX) thrM("+: Invalid character"); // safe to only check this as wv already must be below CHR_MAX, which is less than U32_MAX/2
      }
      dec(x);
      return r;
    }
  }
})
GC2i(sub, wv-xv, {
  if (isC32(w) & isF64(x)) { u64 r = (u64)((i32)o2cu(w)-o2i64(x)); if(r>CHR_MAX)thrM("-: Invalid character"); return m_c32((u32)r); }
  if (isC32(w) & isC32(x)) return m_f64((i32)(u32)w.u - (i32)(u32)x.u);
  if (isArr(w) && TI(w).elType==el_c32) {
    if (isC32(x)) {
      i32 xv = (i32)o2cu(x);
      u32* wp = c32any_ptr(w); usz wia = a(w)->ia;
      i32* rp; B r = m_i32arrc(&rp, w);
      for (usz i = 0; i < wia; i++) rp[i] = (i32)wp[i] - xv;
      dec(w);
      return r;
    }
    if (isArr(x) && eqShape(w, x)) {
      u32* wp = c32any_ptr(w); usz wia = a(w)->ia;
      if (TI(x).elType==el_i32) {
        u32* rp; B r = m_c32arrc(&rp, w);
        i32* xp = i32any_ptr(x);
        for (usz i = 0; i < wia; i++) {
          rp[i] = (u32)((i32)wp[i] - (i32)xp[i]);
          if (rp[i]>CHR_MAX) thrM("-: Invalid character"); // safe - see add
        }
        dec(w); dec(x);
        return r;
      }
    }
  }
})
GC2i(mul, wv*xv, {})
GC2i(and, wv*xv, {})
GC2i(or , (wv+xv)-(wv*xv), {})
GC2i(not, 1+wv-xv, {})

GC2f(div  ,           w.f/x.f, {})
GC2f(pow  ,     pow(w.f, x.f), {})
GC2f(floor,    fmin(w.f, x.f), {})
GC2f(ceil ,    fmax(w.f, x.f), {})
GC2f(stile,   pfmod(x.f, w.f), {})
GC2f(log  , log(x.f)/log(w.f), {})

#undef GC2i
#undef GC2f

#undef P2
