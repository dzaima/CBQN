#include "../core.h"
#include "../utils/each.h"
#include <math.h>

#if SINGELI
#define BCALL(N, X) N(b(X))
#define interp_f64(X) b(X).f

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#include "../singeli/gen/dyarith.c"
#pragma GCC diagnostic pop
#endif

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
          decG(w); decG(x); return num_squeeze(r);                           \
        }                                                                    \
      } else if (isF64(w)&isArr(x)) { usz ia = a(x)->ia;                     \
        u8 xe = TI(x,elType); f64*rp;                                        \
        if (xe==el_i32) { B r=m_f64arrc(&rp, x); i32*xp=i32any_ptr(x);       \
          for (usz i = 0; i < ia; i++) {B x/*shadow*/;x.f=xp[i];rp[i]=EXPR;} \
          decG(x); return num_squeeze(r);                                    \
        }                                                                    \
        if (xe==el_f64) { B r=m_f64arrc(&rp, x); f64*xp=f64any_ptr(x);       \
          for (usz i = 0; i < ia; i++) {B x/*shadow*/;x.f=xp[i];rp[i]=EXPR;} \
          decG(x); return num_squeeze(r);                                    \
        }                                                                    \
      } else if (isF64(x)&isArr(w)) { usz ia = a(w)->ia;                     \
        u8 we = TI(w,elType); f64*rp;                                        \
        if (we==el_i32) { B r=m_f64arrc(&rp, w); i32*wp=i32any_ptr(w);       \
          for (usz i = 0; i < ia; i++) {B w/*shadow*/;w.f=wp[i];rp[i]=EXPR;} \
          decG(w); return num_squeeze(r);                                    \
        }                                                                    \
        if (we==el_f64) { B r=m_f64arrc(&rp, w); f64*wp=f64any_ptr(w);       \
          for (usz i = 0; i < ia; i++) {B w/*shadow*/;w.f=wp[i];rp[i]=EXPR;} \
          decG(w); return num_squeeze(r);                                    \
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
  #define Ri8(A)  i8*  rp; B r=m_i8arrc (&rp, A);
  #define Ri16(A) i16* rp; B r=m_i16arrc(&rp, A);
  #define Ri32(A) i32* rp; B r=m_i32arrc(&rp, A);
  #define Rf64(A) f64* rp; B r=m_f64arrc(&rp, A);
  
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
    
    bool b0 = ia? bp[0]&1 : 0;
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
  #define DOI16(EXPR,A,W,X,BASE) { Ri16(A)           \
    for (usz i = 0; i < ia; i++) {                   \
      i32 wv = W; i32 xv = X; i32 rv = EXPR;         \
      if (RARE(rv!=(i16)rv)) { decG(r); goto BASE; } \
      rp[i] = rv;                                    \
    }                                                \
    dec(w); dec(x); return r;                        \
  }
  #define DOI8(EXPR,A,W,X,BASE) { Ri8(A)            \
    for (usz i = 0; i < ia; i++) {                  \
      i16 wv = W; i16 xv = X; i16 rv = EXPR;        \
      if (RARE(rv!=(i8)rv)) { decG(r); goto BASE; } \
      rp[i] = rv;                                   \
    }                                               \
    dec(w); dec(x); return r;                       \
  }
  #define DOI32(EXPR,A,W,X,BASE) { Ri32(A)           \
    for (usz i = 0; i < ia; i++) {                   \
      i64 wv = W; i64 xv = X; i64 rv = EXPR;         \
      if (RARE(rv!=(i32)rv)) { decG(r); goto BASE; } \
      rp[i] = rv;                                    \
    }                                                \
    dec(w); dec(x); return r;                        \
  }
  
  static B bitAA0(B w, B x, usz ia) { UD; }
  static NOINLINE B bitAA1(B w, B x, usz ia) {
    u64* rp; B r = m_bitarrc(&rp, x);
    u64* wp=bitarr_ptr(w); u64* xp=bitarr_ptr(x);
    for (usz i=0; i<BIT_N(ia); i++) rp[i] = wp[i]|xp[i];
    decG(w); decG(x); return r;
  }
  static NOINLINE B bitAA2(B w, B x, usz ia) {
    u64* rp; B r = m_bitarrc(&rp, x);
    u64* wp=bitarr_ptr(w); u64* xp=bitarr_ptr(x);
    for (usz i=0; i<BIT_N(ia); i++) rp[i] = wp[i]&xp[i];
    decG(w); decG(x); return r;
  }
  
  #define GC2i(SYMB, NAME, EXPR, EXTRA1, EXTRA2, BIT, SI_AA, DO_AS, DO_SA) \
  static NOINLINE B NAME##_c2_arr(B t, B w, B x);                    \
  B NAME##_c2(B t, B w, B x) {                                       \
    if (isF64(w) & isF64(x)) {f64 wv=w.f,xv=x.f;return m_f64(EXPR);} \
    EXTRA1  return NAME##_c2_arr(t,w,x);                             \
  }                                                                  \
  static B NAME##_c2_arr(B t, B w, B x) {                            \
    EXTRA2                                                           \
    if (isArr(w)|isArr(x)) {                                         \
      if (isArr(w)&isArr(x) && rnk(w)==rnk(x)) {                     \
        if (memcmp(a(w)->sh, a(x)->sh, rnk(w)*sizeof(usz))) thrF(SYMB ": Expected equal shape prefix (%H ≡ ≢𝕨, %H ≡ ≢𝕩)", w, x); \
        usz ia = a(x)->ia;                                           \
        u8 we = TI(w,elType);                                        \
        u8 xe = TI(x,elType);                                        \
        if ((we==el_bit | xe==el_bit) && (we|xe)<=el_f64) {          \
          if (BIT && (we|xe)==0) return bitAA##BIT(w,x,ia);          \
          B wt=w,xt=x;                                               \
          we=xe=iMakeEq(&wt, &xt, we, xe);                           \
          w=wt; x=xt;                                                \
        }                                                            \
        if ((we==el_i32|we==el_f64)&(xe==el_i32|xe==el_f64)) {       \
          bool wei = we==el_i32; bool xei = xe==el_i32;              \
          if (wei&xei) { PI32(w)PI32(x)SI_AA(NAME,i32,rcf64) DOI32(EXPR,w,wp[i],xp[i],rcf64) } \
          if (!wei&!xei) {PF(w)PF(x) { SI_AA(NAME,f64,base) } Rf64(x) DOF(EXPR,w,wp[i],xp[i]) decG(w);decG(x);return r; } \
          rcf64:; Rf64(x)                                            \
          if (wei) { PI32(w)                                         \
            if (xei) { PI32(x) DOF(EXPR,w,wp[i],xp[i]) }             \
            else     { PF  (x) DOF(EXPR,w,wp[i],xp[i]) }             \
          } else {PF(w)PI32(x) DOF(EXPR,w,wp[i],xp[i]) }             \
          decG(w); decG(x); return num_squeeze(r);                     \
        }                                                            \
        if(we==el_i8  & xe==el_i8 ) { PI8 (w) PI8 (x) SI_AA(NAME, i8,base) DOI8 (EXPR,w,wp[i],xp[i],base) } \
        if(we==el_i16 & xe==el_i16) { PI16(w) PI16(x) SI_AA(NAME,i16,base) DOI16(EXPR,w,wp[i],xp[i],base) } \
        if(we==el_i8  & xe==el_i32) { PI8 (w) PI32(x) DOI32(EXPR,w,wp[i],xp[i],base) } \
        if(we==el_i32 & xe==el_i8 ) { PI32(w) PI8 (x) DOI32(EXPR,w,wp[i],xp[i],base) } \
        if(we==el_i16 & xe==el_i32) { PI16(w) PI32(x) DOI32(EXPR,w,wp[i],xp[i],base) } \
        if(we==el_i32 & xe==el_i16) { PI32(w) PI16(x) DOI32(EXPR,w,wp[i],xp[i],base) } \
        if(we==el_i16 & xe==el_i8 ) { PI16(w) PI8 (x) DOI16(EXPR,w,wp[i],xp[i],base) } \
        if(we==el_i8  & xe==el_i16) { PI8 (w) PI16(x) DOI16(EXPR,w,wp[i],xp[i],base) } \
      } \
      else if (isF64(w)&isArr(x)) { usz ia=a(x)->ia; u8 xe=TI(x,elType); DO_SA(NAME,EXPR) } \
      else if (isF64(x)&isArr(w)) { usz ia=a(w)->ia; u8 we=TI(w,elType); DO_AS(NAME,EXPR) } \
      base: P2(NAME)                          \
    }                                         \
    thrM(SYMB ": Unexpected argument types"); \
  }
#else // if !TYPED_ARITH
  #define GC2i(SYMB, NAME, EXPR, EXTRA1, EXTRA2, BIT, SI_AA, SI_AS, SI_SA) B NAME##_c2(B t, B w, B x) { \
    if (isF64(w) & isF64(x)) { f64 wv=w.f; f64 xv=x.f; return m_f64(EXPR); } \
    EXTRA1 EXTRA2 \
    P2(NAME) \
    thrM(SYMB ": Unexpected argument types"); \
  }
  #define GC2f(SYMB, NAME, EXPR, EXTRA) B NAME##_c2(B t, B w, B x) { \
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

#define NO_SI_AA(N,S,BASE)
#define REG_SA(NAME, EXPR) \
  if (xe==el_bit) return bit_sel1Fn(NAME##_c2,w,x,1); \
  if (xe==el_i8  && q_i8 (w)) { PI8 (x) i8  wc=o2iu(w); DOI8 (EXPR,x,wc,xp[i],sa8B ) } sa8B :; \
  if (xe==el_i16 && q_i16(w)) { PI16(x) i16 wc=o2iu(w); DOI16(EXPR,x,wc,xp[i],sa16B) } sa16B:; \
  if (xe==el_i32 && q_i32(w)) { PI32(x) i32 wc=o2iu(w); DOI32(EXPR,x,wc,xp[i],sa32B) } sa32B:; \
  if (xe==el_f64) { Rf64(x) PF(x) DOF(EXPR,w,w.f,xp[i]) decG(x); return num_squeeze(r); }
#define REG_AS(NAME, EXPR) \
  if (we==el_bit) return bit_sel1Fn(NAME##_c2,w,x,0); \
  if (we==el_i8  && q_i8 (x)) { PI8 (w) i8  xc=o2iu(x); DOI8 (EXPR,w,wp[i],xc,as8B ) } as8B :; \
  if (we==el_i16 && q_i16(x)) { PI16(w) i16 xc=o2iu(x); DOI16(EXPR,w,wp[i],xc,as16B) } as16B:; \
  if (we==el_i32 && q_i32(x)) { PI32(w) i32 xc=o2iu(x); DOI32(EXPR,w,wp[i],xc,as32B) } as32B:; \
  if (we==el_f64) { Rf64(w) PF(w) DOF(EXPR,x,wp[i],x.f) decG(w); return num_squeeze(r); }

#if SINGELI
  #define SI_AA(N,S,BASE) R##S(x); usz rlen=avx2_##N##AA##_##S((void*)wp, (void*)xp, (void*)rp, ia); if(RARE(rlen!=ia)) { decG(r); goto BASE; } decG(w);decG(x);return r;
  #define SI_SA_I(N,S,W,BASE) R##S(x); usz rlen=avx2_##N##SA##_##S((W).u, (void*)xp, (void*)rp, ia); if(RARE(rlen!=ia)) { decG(r); goto BASE; } dec (w);decG(x);return r;
  #define SI_AS_I(N,S,X,BASE) R##S(w); usz rlen=avx2_##N##AS##_##S((void*)wp, (X).u, (void*)rp, ia); if(RARE(rlen!=ia)) { decG(r); goto BASE; } decG(w);dec (x);return r;
  #define SI_SA(NAME, EXPR) \
  void* xp = tyany_ptr(x);  \
  switch(xe) { default: UD; \
    case el_bit: return bit_sel1Fn(NAME##_c2,w,x,1);   \
    case el_i8 : { SI_SA_I(NAME, i8,w,saBad) } \
    case el_i16: { SI_SA_I(NAME,i16,w,saBad) } \
    case el_i32: { SI_SA_I(NAME,i32,w,saBad) } \
    case el_f64: { SI_SA_I(NAME,f64,w,saBad) } \
    case el_c8: case el_c16: case el_c32: case el_B:; /*fallthrough*/ \
  } saBad:;
  #define SI_AS(NAME, EXPR)   \
    void* wp = tyany_ptr(w);  \
    switch(we) { default: UD; \
      case el_bit: return bit_sel1Fn(NAME##_c2,w,x,0); \
      case el_i8 : { SI_AS_I(NAME, i8,x,asBad) } \
      case el_i16: { SI_AS_I(NAME,i16,x,asBad) } \
      case el_i32: { SI_AS_I(NAME,i32,x,asBad) } \
      case el_f64: { SI_AS_I(NAME,f64,x,asBad) } \
      case el_c8: case el_c16: case el_c32: case el_B:; /*fallthrough*/ \
    } asBad:;
#else
  #define SI_AA NO_SI_AA
  #define SI_AS REG_AS
  #define SI_SA REG_SA
#endif

GC2i("+", add, wv+xv, {
  if (isC32(w) & isF64(x)) { u64 r = (u64)(o2cu(w)+o2i64(x)); if(r>CHR_MAX)thrM("+: Invalid character"); return m_c32((u32)r); }
  if (isF64(w) & isC32(x)) { u64 r = (u64)(o2cu(x)+o2i64(w)); if(r>CHR_MAX)thrM("+: Invalid character"); return m_c32((u32)r); }
},{
  if (isArr(w)&isC32(x) || isC32(w)&isArr(x)) { if (isArr(w)) { B t=w;w=x;x=t; }
    if (TI(x,elType) == el_i32) {
      u32 wv = o2cu(w);
      i32* xp = i32any_ptr(x); usz xia = a(x)->ia;
      u32* rp; B r = m_c32arrc(&rp, x);
      for (usz i = 0; i < xia; i++) {
        rp[i] = (u32)(xp[i]+(i32)wv);
        if (rp[i]>CHR_MAX) thrM("+: Invalid character"); // safe to only check this as wv already must be below CHR_MAX, which is less than U32_MAX/2
      }
      decG(x);
      return r;
    }
  }
}, 0, SI_AA, SI_AS, SI_SA)

GC2i("-", sub, wv-xv, {
  if (isC32(w) & isF64(x)) { u64 r = (u64)((i32)o2cu(w)-o2i64(x)); if(r>CHR_MAX)thrM("-: Invalid character"); return m_c32((u32)r); }
  if (isC32(w) & isC32(x)) return m_f64((i32)(u32)w.u - (i32)(u32)x.u);
},{
  if (isArr(w) && TI(w,elType)==el_c32) {
    if (isC32(x)) {
      i32 xv = (i32)o2cu(x);
      u32* wp = c32any_ptr(w); usz wia = a(w)->ia;
      i32* rp; B r = m_i32arrc(&rp, w);
      for (usz i = 0; i < wia; i++) rp[i] = (i32)wp[i] - xv;
      decG(w);
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
        decG(w); decG(x);
        return r;
      }
    }
  }
}, 0, SI_AA, SI_AS, SI_SA)

GC2i("¬", not, 1+wv-xv, {
  if (isC32(w) & isF64(x)) { u64 r = (u64)(1+(i32)o2cu(w)-o2i64(x)); if(r>CHR_MAX)thrM("¬: Invalid character"); return m_c32((u32)r); }
  if (isC32(w) & isC32(x)) return m_f64(1 + (i32)(u32)w.u - (i32)(u32)x.u);
}, {}, 0, NO_SI_AA, REG_AS, REG_SA)
GC2i("×", mul, wv*xv, {}, {}, 2, SI_AA, SI_AS, SI_SA)
GC2i("∧", and, wv*xv, {}, {}, 2, NO_SI_AA, REG_AS, REG_SA)
GC2i("∨", or , (wv+xv)-(wv*xv), {}, {}, 1, NO_SI_AA, REG_AS, REG_SA)
GC2i("⌊", floor, wv>xv?xv:wv, {}, {}, 2, NO_SI_AA, REG_AS, REG_SA) // optimizer optimizes out the fallback mess
GC2i("⌈", ceil , wv>xv?wv:xv, {}, {}, 1, NO_SI_AA, REG_AS, REG_SA)

GC2f("÷", div  ,           w.f/x.f, {})
GC2f("⋆", pow  ,     pow(w.f, x.f), {})
GC2f("√", root , pow(x.f, 1.0/w.f), {})
GC2f("|", stile,   pfmod(x.f, w.f), {})
GC2f("⋆⁼",log  , log(x.f)/log(w.f), {})

#undef GC2i
#undef GC2f

#undef P2
