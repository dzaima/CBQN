#include "../core.h"
#include "../utils/each.h"
#include <math.h>

#define P2(N) { if(isArr(w)|isArr(x)) { \
  SLOWIF((!isArr(w) || TI(w,elType)!=el_B)  &&  (!isArr(x) || TI(x,elType)!=el_B)) SLOW2("arithd " #N, w, x); \
  return arith_recd(N##_c2, w, x); \
}}
#if TYPED_ARITH
  #define GC2f(SYMB, NAME, EXPR, EXTRA) B NAME##_c2(B t, B w, B x) {         \
    if (isF64(w) & isF64(x)) return m_f64(EXPR);                             \
    EXTRA                                                                    \
    if (isArr(w)|isArr(x)) { B ow=w; B ox=x;                                 \
      if (isArr(w)&isArr(x) && rnk(w)==rnk(x)) {                             \
        if (memcmp(a(w)->sh, a(x)->sh, rnk(w)*sizeof(usz))) thrF(SYMB ": Expected equal shape prefix (%H ≡ ≢𝕨, %H ≡ ≢𝕩)", w, x); \
        usz ia = a(x)->ia;                                                   \
        u8 we = TI(w,elType);                                                \
        u8 xe = TI(x,elType);                                                \
        if ((we==el_i32|we==el_f64)&(xe==el_i32|xe==el_f64)) {               \
          f64* rp; B r = m_f64arrc(&rp, x);                                  \
          if (we==el_i32) { B w,x/*shadow*/; i32* wp = i32any_ptr(ow);       \
            if (xe==el_i32) { i32* xp = i32any_ptr(ox); for (usz i = 0; i < ia; i++) {w.f=wp[i];x.f=xp[i];rp[i]=EXPR;} } \
            else            { f64* xp = f64any_ptr(ox); for (usz i = 0; i < ia; i++) {w.f=wp[i];x.f=xp[i];rp[i]=EXPR;} } \
          } else {          B w,x/*shadow*/; f64* wp = f64any_ptr(ow);       \
            if (xe==el_i32) { i32* xp = i32any_ptr(ox); for (usz i = 0; i < ia; i++) {w.f=wp[i];x.f=xp[i];rp[i]=EXPR;} } \
            else            { f64* xp = f64any_ptr(ox); for (usz i = 0; i < ia; i++) {w.f=wp[i];x.f=xp[i];rp[i]=EXPR;} } \
          }                                                                  \
          dec(w); dec(x); return num_squeeze(r);                             \
        }                                                                    \
      } else if (isF64(w)&isArr(x)) { usz ia = a(x)->ia;                     \
        u8 xe = TI(x,elType); f64*rp;                                        \
        if (xe==el_i32) { B r=m_f64arrc(&rp, x); i32*xp=i32any_ptr(x);       \
          for (usz i = 0; i < ia; i++) {B x/*shadow*/;x.f=xp[i];rp[i]=EXPR;} \
          dec(x); return num_squeeze(r);                                     \
        }                                                                    \
        if (xe==el_f64) { B r=m_f64arrc(&rp, x); f64*xp=f64any_ptr(x);       \
          for (usz i = 0; i < ia; i++) {B x/*shadow*/;x.f=xp[i];rp[i]=EXPR;} \
          dec(x); return num_squeeze(r);                                     \
        }                                                                    \
      } else if (isF64(x)&isArr(w)) { usz ia = a(w)->ia;                     \
        u8 we = TI(w,elType); f64*rp;                                        \
        if (we==el_i32) { B r=m_f64arrc(&rp, w); i32*wp=i32any_ptr(w);       \
          for (usz i = 0; i < ia; i++) {B w/*shadow*/;w.f=wp[i];rp[i]=EXPR;} \
          dec(w); return num_squeeze(r);                                     \
        }                                                                    \
        if (we==el_f64) { B r=m_f64arrc(&rp, w); f64*wp=f64any_ptr(w);       \
          for (usz i = 0; i < ia; i++) {B w/*shadow*/;w.f=wp[i];rp[i]=EXPR;} \
          dec(w); return num_squeeze(r);                                     \
        }                                                                    \
      }                                                                      \
      P2(NAME)                                                               \
    }                                                                        \
    thrM(SYMB ": Unexpected argument types");                                \
  }
  
  #define PF(N) f64* N##p = f64any_ptr(N);
  #define PI8(N)  i8*  N##p = i8any_ptr (N);
  #define PI16(N) i16* N##p = i16any_ptr(N);
  #define PI32(N) i32* N##p = i32any_ptr(N);
  #define RI8(A)  i8*  rp; B r=m_i8arrc (&rp, A);
  #define RI16(A) i16* rp; B r=m_i16arrc(&rp, A);
  #define RI32(A) i32* rp; B r=m_i32arrc(&rp, A);
  #define RF(A) f64* rp; B r=m_f64arrc(&rp, A);
  
  static NOINLINE u8 iMakeEq(B* w, B* x, u8 we, u8 xe) {
    B s = we<xe?*w:*x;
    SLOW2("bitarr expansion", *w, *x);
    switch(we|xe) { default: UD;
      case el_bit: *w = taga(cpyI8Arr(*w)); *x = taga(cpyI8Arr(*x)); return el_i8;
      case el_i8:  s = taga(cpyI8Arr (s)); break;
      case el_i16: s = taga(cpyI16Arr(s)); break;
      case el_i32: s = taga(cpyI32Arr(s)); break;
      case el_f64: s = taga(cpyF64Arr(s)); break;
    }
    *(we<xe?w:x) = s;
    return we|xe;
  }
  static NOINLINE B bit_sel1Fn(BBB2B f, B w, B x, bool bitX) { // consumes both
    B b = bitX? x : w;
    u64* bp = bitarr_ptr(b);
    usz ia = a(b)->ia;
    
    bool b0 = bp[0]&1;
    bool both = false;
    for (usz i = 0; i < ia; i++) if (bitp_get(bp,i) != b0) { both=true; break; }
    
    B e0=m_f64(0), e1=m_f64(0); // initialized to have something to decrement later
    bool h0=both || b0==0; if (h0) e0 = bitX? f(bi_N, inc(w), m_f64(0)) : f(bi_N, m_f64(0), inc(x));
    bool h1=both || b0==1; if (h1) e1 = bitX? f(bi_N,     w,  m_f64(1)) : f(bi_N, m_f64(1), x);
    // non-bitarr arg has been consumed
    B r = bit_sel(b, e0, h0, e1, h1); // and now the bitarr arg is consumed too
    dec(e0); dec(e1);
    return r;
  }
  
  #define DOF(EXPR,A,W,X) {        \
    for (usz i = 0; i < ia; i++) { \
      f64 wv = W; f64 xv = X;      \
      rp[i] = EXPR;                \
    }                              \
  }
  #define DOI16(EXPR,A,W,X,BASE) { RI16(A)          \
    for (usz i = 0; i < ia; i++) {                  \
      i32 wv = W; i32 xv = X; i32 rv = EXPR;        \
      if (RARE(rv!=(i16)rv)) { dec(r); goto BASE; } \
      rp[i] = rv;                                   \
    }                                               \
    dec(w); dec(x); return r;                       \
  }
  #define DOI8(EXPR,A,W,X,BASE) { RI8(A)            \
    for (usz i = 0; i < ia; i++) {                  \
      i16 wv = W; i16 xv = X; i16 rv = EXPR;        \
      if (RARE(rv!=(i8)rv)) { dec(r); goto BASE; }  \
      rp[i] = rv;                                   \
    }                                               \
    dec(w); dec(x); return r;                       \
  }
  #define DOI32(EXPR,A,W,X,BASE) { RI32(A)          \
    for (usz i = 0; i < ia; i++) {                  \
      i64 wv = W; i64 xv = X; i64 rv = EXPR;        \
      if (RARE(rv!=(i32)rv)) { dec(r); goto BASE; } \
      rp[i] = rv;                                   \
    }                                               \
    dec(w); dec(x); return r;                       \
  }
  #define GC2i(SYMB, NAME, EXPR, EXTRA, BIT, BX) B NAME##_c2(B t, B w, B x) { \
    if (isF64(w) & isF64(x)) {f64 wv=w.f,xv=x.f;return m_f64(EXPR);} \
    EXTRA                                                            \
    if (isArr(w)|isArr(x)) {                                         \
      if (isArr(w)&isArr(x) && rnk(w)==rnk(x)) {                     \
        if (memcmp(a(w)->sh, a(x)->sh, rnk(w)*sizeof(usz))) thrF(SYMB ": Expected equal shape prefix (%H ≡ ≢𝕨, %H ≡ ≢𝕩)", w, x); \
        usz ia = a(x)->ia;                                           \
        u8 we = TI(w,elType);                                        \
        u8 xe = TI(x,elType);                                        \
        if ((we==el_bit | xe==el_bit) && (we|xe)<=el_f64) {          \
          if (BIT && (we|xe)==0) { u64* rp; B r = m_bitarrc(&rp, x); \
            u64* wp = bitarr_ptr(w); u64* xp = bitarr_ptr(x);        \
            for (usz i=0; i<BIT_N(ia); i++) rp[i] = wp[i] BX xp[i];  \
            dec(w); dec(x); return r;                                \
          }                                                          \
          B wt=w,xt=x;                                               \
          we=xe=iMakeEq(&wt, &xt, we, xe);                           \
          w=wt; x=xt;                                                \
        }                                                            \
        if ((we==el_i32|we==el_f64)&(xe==el_i32|xe==el_f64)) {       \
          bool wei = we==el_i32; bool xei = xe==el_i32;              \
          if (wei&xei) {PI32(w)PI32(x)DOI32(EXPR,w,wp[i],xp[i],aaB);}\
          aaB:; RF(x)                                                \
          if (wei) { PI32(w)                                         \
            if (xei) { PI32(x) DOF(EXPR,w,wp[i],xp[i]) }             \
            else     { PF  (x) DOF(EXPR,w,wp[i],xp[i]) }             \
          } else { PF(w)                                             \
            if (xei) { PI32(x) DOF(EXPR,w,wp[i],xp[i]) }             \
            else     { PF  (x) DOF(EXPR,w,wp[i],xp[i]) }             \
          }                                                          \
          dec(w); dec(x); return num_squeeze(r);                     \
        }                                                            \
        if(we==el_i8  & xe==el_i8 ) { PI8 (w) PI8 (x) DOI8 (EXPR,w,wp[i],xp[i],base); } \
        if(we==el_i16 & xe==el_i16) { PI16(w) PI16(x) DOI16(EXPR,w,wp[i],xp[i],base); } \
        if(we==el_i8  & xe==el_i32) { PI8 (w) PI32(x) DOI32(EXPR,w,wp[i],xp[i],base); } \
        if(we==el_i32 & xe==el_i8 ) { PI32(w) PI8 (x) DOI32(EXPR,w,wp[i],xp[i],base); } \
        if(we==el_i16 & xe==el_i32) { PI16(w) PI32(x) DOI32(EXPR,w,wp[i],xp[i],base); } \
        if(we==el_i32 & xe==el_i16) { PI32(w) PI16(x) DOI32(EXPR,w,wp[i],xp[i],base); } \
        if(we==el_i16 & xe==el_i8 ) { PI16(w) PI8 (x) DOI16(EXPR,w,wp[i],xp[i],base); } \
        if(we==el_i8  & xe==el_i16) { PI8 (w) PI16(x) DOI16(EXPR,w,wp[i],xp[i],base); } \
      } else if (isF64(w)&isArr(x)) { usz ia = a(x)->ia; u8 xe = TI(x,elType);          \
        if (xe==el_bit) {                                          \
          if (BIT && q_fbit(w.f)) { u64* rp; B r=m_bitarrc(&rp,x); \
            u64 wv = bitx(w); u64* xp = bitarr_ptr(x);             \
            for (usz i=0; i<BIT_N(ia); i++) rp[i] = wv BX xp[i];   \
            dec(w); dec(x); return r;                              \
          } else return bit_sel1Fn(NAME##_c2,w,x,1);               \
        }                                                          \
        if (xe==el_i8  && q_i8 (w)) { PI8 (x) i8  wc=o2iu(w); DOI8 (EXPR,x,wc,xp[i],na8B ) } na8B :; \
        if (xe==el_i16 && q_i16(w)) { PI16(x) i16 wc=o2iu(w); DOI16(EXPR,x,wc,xp[i],na16B) } na16B:; \
        if (xe==el_i32 && q_i32(w)) { PI32(x) i32 wc=o2iu(w); DOI32(EXPR,x,wc,xp[i],na32B) } na32B:; \
        if (xe==el_i32) { RF(x) PI32(x) DOF(EXPR,w,w.f,xp[i]) dec(x); return num_squeeze(r); }       \
        if (xe==el_f64) { RF(x) PF  (x) DOF(EXPR,w,w.f,xp[i]) dec(x); return num_squeeze(r); }       \
      } else if (isF64(x)&isArr(w)) { usz ia = a(w)->ia; u8 we = TI(w,elType);                       \
        if (we==el_bit) {                                          \
          if (BIT && q_fbit(x.f)) { u64* rp; B r=m_bitarrc(&rp,w); \
            u64 xv = bitx(x); u64* wp = bitarr_ptr(w);             \
            for (usz i=0; i<BIT_N(ia); i++) rp[i] = wp[i] BX xv;   \
            dec(w); dec(x); return r;                              \
          } else return bit_sel1Fn(NAME##_c2,w,x,0);               \
        }                                                          \
        if (we==el_i8  && q_i8 (x)) { PI8 (w) i8  xc=o2iu(x); DOI8 (EXPR,w,wp[i],xc,an8B ) } an8B :; \
        if (we==el_i16 && q_i16(x)) { PI16(w) i16 xc=o2iu(x); DOI16(EXPR,w,wp[i],xc,an16B) } an16B:; \
        if (we==el_i32 && q_i32(x)) { PI32(w) i32 xc=o2iu(x); DOI32(EXPR,w,wp[i],xc,an32B) } an32B:; \
        if (we==el_i32) { RF(w) PI32(w) DOF(EXPR,x,wp[i],x.f) dec(w); return num_squeeze(r); }       \
        if (we==el_f64) { RF(w) PF  (w) DOF(EXPR,x,wp[i],x.f) dec(w); return num_squeeze(r); }       \
      }                                                              \
      base: P2(NAME)                                                 \
    }                                                                \
    thrM(SYMB ": Unexpected argument types");                        \
  }
#else // if !TYPED_ARITH
  #define GC2i(NAME, EXPR, EXTRA) B NAME##_c2(B t, B w, B x) { \
    if (isF64(w) & isF64(x)) { f64 wv=w.f; f64 xv=x.f; return m_f64(EXPR); } \
    EXTRA \
    P2(NAME) \
    thrM(SYMB ": Unexpected argument types"); \
  }
  #define GC2f(NAME, EXPR, EXTRA) B NAME##_c2(B t, B w, B x) { \
    if (isF64(w) & isF64(x)) return m_f64(EXPR); \
    EXTRA \
    P2(NAME) \
    thrM(SYMB ": Unexpected argument types"); \
  }
#endif // TYPED_ARITH

static f64 pfmod(f64 a, f64 b) {
  f64 r = fmod(a, b);
  if (a<0 != b<0 && r!=0) r+= b;
  return r;
}

GC2i("+", add, wv+xv, {
  if (isC32(w) & isF64(x)) { u64 r = (u64)(o2cu(w)+o2i64(x)); if(r>CHR_MAX)thrM("+: Invalid character"); return m_c32((u32)r); }
  if (isF64(w) & isC32(x)) { u64 r = (u64)(o2cu(x)+o2i64(w)); if(r>CHR_MAX)thrM("+: Invalid character"); return m_c32((u32)r); }
  if (isArr(w)&isC32(x) || isC32(w)&isArr(x)) { if (isArr(w)) { B t=w;w=x;x=t; }
    if (TI(x,elType) == el_i32) {
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
}, 0, -)
GC2i("-", sub, wv-xv, {
  if (isC32(w) & isF64(x)) { u64 r = (u64)((i32)o2cu(w)-o2i64(x)); if(r>CHR_MAX)thrM("-: Invalid character"); return m_c32((u32)r); }
  if (isC32(w) & isC32(x)) return m_f64((i32)(u32)w.u - (i32)(u32)x.u);
  if (isArr(w) && TI(w,elType)==el_c32) {
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
      if (TI(x,elType)==el_i32) {
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
}, 0, -)
GC2i("¬", not, 1+wv-xv, {
  if (isC32(w) & isF64(x)) { u64 r = (u64)(1+(i32)o2cu(w)-o2i64(x)); if(r>CHR_MAX)thrM("¬: Invalid character"); return m_c32((u32)r); }
  if (isC32(w) & isC32(x)) return m_f64(1 + (i32)(u32)w.u - (i32)(u32)x.u);
}, 0, -)
GC2i("×", mul, wv*xv, {}, 1, &)
GC2i("∧", and, wv*xv, {}, 1, &)
GC2i("∨", or , (wv+xv)-(wv*xv), {}, 1, |)
GC2i("⌊", floor, wv>xv?xv:wv, {}, 1, &) // optimizer optimizes out the fallback mess
GC2i("⌈", ceil , wv>xv?wv:xv, {}, 1, |)

GC2f("÷", div  ,           w.f/x.f, {})
GC2f("⋆", pow  ,     pow(w.f, x.f), {})
GC2f("√", root , pow(x.f, 1.0/w.f), {})
GC2f("|", stile,   pfmod(x.f, w.f), {})
GC2f("⋆⁼",log  , log(x.f)/log(w.f), {})

#undef GC2i
#undef GC2f

#undef P2
