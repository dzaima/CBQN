#include "../core.h"
#include "../utils/each.h"
#include <math.h>

static f64 pfmod(f64 a, f64 b) {
  f64 r = fmod(a, b);
  if (a<0 != b<0 && r!=0) r+= b;
  return r;
}
static f64 bqn_or(f64 w, f64 x) { return (w+x)-(w*x); }

B add_c2(B, B, B);  B ceil_c2(B, B, B);
B sub_c2(B, B, B);  B div_c2(B, B, B);
B not_c2(B, B, B);  B pow_c2(B, B, B);
B mul_c2(B, B, B);  B root_c2(B, B, B);
B and_c2(B, B, B);  B stile_c2(B, B, B);
B or_c2 (B, B, B);  B log_c2(B, B, B);
B floor_c2(B, B, B);
B atan2_c2(B, B, B);


typedef void (*AndBytesFn)(u8*, u8*, u64, u64);

#if SINGELI_X86_64
  #include "../singeli/c/arithdDispatch.c"
  static AndBytesFn andBytes_fn = avx2_andBytes;
#else
  static void base_andBytes(u8* r, u8* x, u64 repeatedMask, u64 numBytes) {
    u64* x64 = (u64*)x; usz i;
    for (i = 0; i < numBytes/8; i++) ((u64*)r)[i] = x64[i] & repeatedMask;
    if (i*8 != numBytes) {
      u64 v = x64[i]&repeatedMask;
      for (usz j = 0; j < (numBytes&7); j++) r[i*8 + j] = v>>(j*8);
    }
  }
  static AndBytesFn andBytes_fn = base_andBytes;
#endif

#define ARITH_SLOW(N) SLOWIF((!isArr(w) || TI(w,elType)!=el_B)  &&  (!isArr(x) || TI(x,elType)!=el_B)) SLOW2("arithd " #N, w, x)
#define P2(N) { if(isArr(w)|isArr(x)) { ARITH_SLOW(N); \
  return arith_recd(N##_c2, w, x); \
}}
#if !TYPED_ARITH
  #define AR_I_TO_ARR(NAME) P2(NAME)
  #define AR_F_TO_ARR AR_I_TO_ARR
#else
  static NOINLINE u8 iMakeEq(B* w, B* x, u8 we, u8 xe) {
    B* p = we<xe?w:x;
    B s = *p;
    SLOW2("bitarr expansion", *w, *x);
    switch(we|xe) { default: UD;
      case el_bit: *w = taga(cpyI8Arr(*w)); *x = taga(cpyI8Arr(*x)); return el_i8;
      case el_i8:  s = taga(cpyI8Arr (s)); break;
      case el_i16: s = taga(cpyI16Arr(s)); break;
      case el_i32: s = taga(cpyI32Arr(s)); break;
      case el_f64: s = taga(cpyF64Arr(s)); break;
    }
    *p = s;
    return we|xe;
  }
  
  #define PF(N)   f64* N##p = f64any_ptr(N);
  #define PI8(N)  i8*  N##p = i8any_ptr (N);
  #define PI16(N) i16* N##p = i16any_ptr(N);
  #define PI32(N) i32* N##p = i32any_ptr(N);
  #define Ri8(A)  i8*  rp; r=m_i8arrc (&rp, A);
  #define Ri16(A) i16* rp; r=m_i16arrc(&rp, A);
  #define Ri32(A) i32* rp; r=m_i32arrc(&rp, A);
  #define Rf64(A) f64* rp; r=m_f64arrc(&rp, A);
  #define DOF(EXPR,A,W,X) { for (usz i = 0; i < ia; i++) { f64 wv=W; f64 xv=X; rp[i]=EXPR; } }
  #define DOI8(EXPR,A,W,X,BASE)  { Ri8(A)  for (usz i=0; i<ia; i++) { i16 wv=W; i16 xv=X; i16 rv=EXPR; if (RARE(rv!=( i8)rv)) { decG(r); goto BASE; } rp[i]=rv; } goto dec_ret; }
  #define DOI16(EXPR,A,W,X,BASE) { Ri16(A) for (usz i=0; i<ia; i++) { i32 wv=W; i32 xv=X; i32 rv=EXPR; if (RARE(rv!=(i16)rv)) { decG(r); goto BASE; } rp[i]=rv; } goto dec_ret; }
  #define DOI32(EXPR,A,W,X,BASE) { Ri32(A) for (usz i=0; i<ia; i++) { i64 wv=W; i64 xv=X; i64 rv=EXPR; if (RARE(rv!=(i32)rv)) { decG(r); goto BASE; } rp[i]=rv; } goto dec_ret; }
  
  #define GC2f(SYMB, NAME, EXPR, DECOR, INT_SA) B NAME##_c2_arr(B t, B w, B x) { \
    if (isArr(w)|isArr(x)) { B r;                                 \
      if (isArr(w)&isArr(x) && RNK(w)==RNK(x)) {                  \
        if (!eqShPart(SH(w), SH(x), RNK(w))) thrF(SYMB ": Expected equal shape prefix (%H ≡ ≢𝕨, %H ≡ ≢𝕩)", w, x); \
        usz ia = IA(x);                                           \
        u8 we = TI(w,elType);                                     \
        u8 xe = TI(x,elType);                                     \
        if (elNum(we) && elNum(xe)) {                             \
          if (we<el_i32) { w=taga(cpyI32Arr(w)); we=el_i32; } void* wp = tyany_ptr(w); \
          if (xe<el_i32) { x=taga(cpyI32Arr(x)); xe=el_i32; } void* xp = tyany_ptr(x); \
          Rf64(x);                                                \
          if (we==el_i32) { B w,x /*shadow*/;                     \
            if (xe==el_i32) { DECOR for (usz i = 0; i < ia; i++) {w.f=((i32*)wp)[i]; x.f=((i32*)xp)[i]; rp[i]=EXPR;} } \
            else            { DECOR for (usz i = 0; i < ia; i++) {w.f=((i32*)wp)[i]; x.f=((f64*)xp)[i]; rp[i]=EXPR;} } \
          } else {          B w,x /*shadow*/;                     \
            if (xe==el_i32) { DECOR for (usz i = 0; i < ia; i++) {w.f=((f64*)wp)[i]; x.f=((i32*)xp)[i]; rp[i]=EXPR;} } \
            else            { DECOR for (usz i = 0; i < ia; i++) {w.f=((f64*)wp)[i]; x.f=((f64*)xp)[i]; rp[i]=EXPR;} } \
          }                                                       \
          decG(w); decG(x); return num_squeeze(r);                \
        }                                                         \
      } else if (isF64(w)&isArr(x)) { usz ia=IA(x); u8 xe=TI(x,elType); \
        if (elInt(xe)){INT_SA Rf64(x); x=toI32Any(x); PI32(x) DECOR for (usz i=0; i<ia; i++) {B x/*shadow*/;x.f=xp[i];rp[i]=EXPR;} decG(x); return num_squeeze(r); } \
        if (xe==el_f64) { Rf64(x);                    PF(x)   DECOR for (usz i=0; i<ia; i++) {B x/*shadow*/;x.f=xp[i];rp[i]=EXPR;} decG(x); return num_squeeze(r); } \
      } else if (isF64(x)&isArr(w)) { usz ia=IA(w); u8 we=TI(w,elType); \
        if (elInt(we)){       Rf64(w); w=toI32Any(w); PI32(w) DECOR for (usz i=0; i<ia; i++) {B w/*shadow*/;w.f=wp[i];rp[i]=EXPR;} decG(w); return num_squeeze(r); } \
        if (we==el_f64) { Rf64(w);                    PF(w)   DECOR for (usz i=0; i<ia; i++) {B w/*shadow*/;w.f=wp[i];rp[i]=EXPR;} decG(w); return num_squeeze(r); } \
      }                                                           \
      P2(NAME)                                                    \
    }                                                             \
    thrM(SYMB ": Unexpected argument types");                     \
  }
  GC2f("÷", div  ,           w.f/x.f, , )
  GC2f("√", root , pow(x.f, 1.0/w.f), NOUNROLL, )
  GC2f("⋆", pow  ,     pow(w.f, x.f), NOUNROLL, )
  GC2f("⋆⁼",log  , log(x.f)/log(w.f), NOUNROLL, )
  static u64 repeatNum[] = {
    [el_i8 ] = 0x0101010101010101ULL,
    [el_i16] = 0x0001000100010001ULL,
    [el_i32] = 0x0000000100000001ULL,
  };
  GC2f("|", stile,   pfmod(x.f, w.f), NOUNROLL,
    f64 wf64 = o2fG(w); i32 wi32 = wf64;
    if (wf64==(f64)wi32 && wi32>0 && (wi32&(wi32-1))==0) {
      if (wi32==1) { Arr* ra=allZeroes(IA(x)); arr_shCopy(ra, x); r = taga(ra); decG(x); return r; }
      if (xe==el_bit) return x; // if n>1 (true from the above), 0‿1 ≡ (2⋆n)|0‿1
      u8 elw = elWidth(xe);
      u32 mask0 = (u32)wi32;
      if (mask0 > (1 << (elw*8-1))) {
        if      (mask0 > 32768) { x=taga(cpyI32Arr(x)); xe=el_i32; elw=4; }
        else if (mask0 >   128) { x=taga(cpyI16Arr(x)); xe=el_i16; elw=2; }
        else UD;
      }
      u64 mask = (mask0-1)*repeatNum[xe];
      usz bytes = IA(x)*elw;
      u8* rp = m_tyarrc(&r, elw, x, el2t(xe));
      andBytes_fn(rp, tyany_ptr(x), mask, bytes);
      decG(x);
      if (wi32==2) return taga(cpyBitArr(r));
      if (wi32<256) return taga(cpyI8Arr(r)); // these won't widen, as the code doesn't even get to here if 𝕨 > max possible in 𝕩
      if (wi32<65536) return taga(cpyI16Arr(r));
      return r;
    }
  )
  #undef GC2f
  
  
  #if SINGELI_X86_64
    #define AA_DISPATCH(NAME) FORCE_INLINE B NAME##_AA(B t, B w, B x) { return dyArith_AA(&NAME##DyTableAA, w, x); }
    AA_DISPATCH(add) AA_DISPATCH(or)
    AA_DISPATCH(sub)
    AA_DISPATCH(mul) AA_DISPATCH(and)
    AA_DISPATCH(ceil) AA_DISPATCH(floor)
    #undef AA_DISPATCH
    
    #define SA_DISPATCH(NAME) FORCE_INLINE B NAME##_SA(B t, B w, B x) { return dyArith_SA(&NAME##DyTableNA, w, x); }
    #define AS_DISPATCH(NAME) FORCE_INLINE B NAME##_AS(B t, B w, B x) { return dyArith_SA(&NAME##DyTableAN, x, w); }
    SA_DISPATCH(add)
    SA_DISPATCH(sub) AS_DISPATCH(sub)
    SA_DISPATCH(mul) SA_DISPATCH(and)
    SA_DISPATCH(ceil) SA_DISPATCH(floor)
    #undef SA_DISPATCH
  #else
    #define NO_SI_AA(N)
    #define SI_AA NO_SI_AA
    #define SI_AS REG_AS
    #define SI_SA REG_SA
    
    #define REG_SA(NAME, EXPR) \
      if (xe==el_bit) return bit_sel1Fn(NAME##_c2,w,x,1); \
      if (xe==el_i8  && q_i8 (w)) { PI8 (x) i8  wc=o2iG(w); DOI8 (EXPR,x,wc,xp[i],sa8B ) } sa8B :; \
      if (xe==el_i16 && q_i16(w)) { PI16(x) i16 wc=o2iG(w); DOI16(EXPR,x,wc,xp[i],sa16B) } sa16B:; \
      if (xe==el_i32 && q_i32(w)) { PI32(x) i32 wc=o2iG(w); DOI32(EXPR,x,wc,xp[i],sa32B) } sa32B:; \
      if (xe==el_f64) { Rf64(x) PF(x) DOF(EXPR,w,w.f,xp[i]) goto dec_ret; }
    #define REG_AS(NAME, EXPR) \
      if (we==el_bit) return bit_sel1Fn(NAME##_c2,w,x,0); \
      if (we==el_i8  && q_i8 (x)) { PI8 (w) i8  xc=o2iG(x); DOI8 (EXPR,w,wp[i],xc,as8B ) } as8B :; \
      if (we==el_i16 && q_i16(x)) { PI16(w) i16 xc=o2iG(x); DOI16(EXPR,w,wp[i],xc,as16B) } as16B:; \
      if (we==el_i32 && q_i32(x)) { PI32(w) i32 xc=o2iG(x); DOI32(EXPR,w,wp[i],xc,as32B) } as32B:; \
      if (we==el_f64) { Rf64(w) PF(w) DOF(EXPR,x,wp[i],x.f) goto dec_ret; }
    
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
    
    #define AR_I_AA(CHR, NAME, EXPR, BIT, EXTRA) NOINLINE B NAME##_AA(B t, B w, B x) { \
      if (RNK(w)!=RNK(x)) goto bad;                              \
      if (!eqShPart(SH(w), SH(x), RNK(w))) thrF(CHR ": Expected equal shape prefix (%H ≡ ≢𝕨, %H ≡ ≢𝕩)", w, x); \
      usz ia = IA(x); B r;                                       \
      u8 we = TI(w,elType);                                      \
      u8 xe = TI(x,elType);                                      \
      if ((we==el_bit | xe==el_bit) && (we|xe)<=el_f64) {        \
        if (BIT && (we|xe)==0) return bitAA##BIT(w,x,ia);        \
        B wt=w,xt=x;                                             \
        we=xe=iMakeEq(&wt, &xt, we, xe);                         \
        w=wt; x=xt;                                              \
      }                                                          \
      if ((we==el_i32|we==el_f64)&(xe==el_i32|xe==el_f64)) {     \
        bool wei = we==el_i32; bool xei = xe==el_i32;            \
        if (wei&xei) { PI32(w) PI32(x)     DOI32(EXPR,w,wp[i],xp[i],rcf64) } \
        if (!wei&!xei) { PF(w) PF(x) Rf64(x) DOF(EXPR,w,wp[i],xp[i]) goto dec_ret; } \
        rcf64:; Rf64(x)                                          \
        if (wei) { PI32(w)                                       \
          if (xei) { PI32(x) DOF(EXPR,w,wp[i],xp[i]) }           \
          else     { PF(x)   DOF(EXPR,w,wp[i],xp[i]) }           \
        } else {PF(w)PI32(x) DOF(EXPR,w,wp[i],xp[i]) }           \
        decG(w); decG(x); return num_squeeze(r);                 \
      }                                                          \
      EXTRA                                                      \
      if(we==el_i8  & xe==el_i8 ) { PI8 (w) PI8 (x) DOI8 (EXPR,w,wp[i],xp[i],bad) } \
      if(we==el_i16 & xe==el_i16) { PI16(w) PI16(x) DOI16(EXPR,w,wp[i],xp[i],bad) } \
      if(we==el_i8  & xe==el_i32) { PI8 (w) PI32(x) DOI32(EXPR,w,wp[i],xp[i],bad) } \
      if(we==el_i32 & xe==el_i8 ) { PI32(w) PI8 (x) DOI32(EXPR,w,wp[i],xp[i],bad) } \
      if(we==el_i16 & xe==el_i32) { PI16(w) PI32(x) DOI32(EXPR,w,wp[i],xp[i],bad) } \
      if(we==el_i32 & xe==el_i16) { PI32(w) PI16(x) DOI32(EXPR,w,wp[i],xp[i],bad) } \
      if(we==el_i16 & xe==el_i8 ) { PI16(w) PI8 (x) DOI16(EXPR,w,wp[i],xp[i],bad) } \
      if(we==el_i8  & xe==el_i16) { PI8 (w) PI16(x) DOI16(EXPR,w,wp[i],xp[i],bad) } \
      bad: ARITH_SLOW(CHR); return arith_recd(NAME##_c2, w, x);  \
      dec_ret: decG(w); decG(x); return r; \
    }
    AR_I_AA("×", mul, wv*xv, 2, {})
    AR_I_AA("∧", and, wv*xv, 2, {})
    AR_I_AA("∨", or , bqn_or(wv, xv), 1, {})
    AR_I_AA("⌊", floor, wv>xv?xv:wv, 2, {})
    AR_I_AA("⌈", ceil , wv>xv?wv:xv, 1, {})
    AR_I_AA("+", add, wv+xv, 0, {})
    AR_I_AA("-", sub, wv-xv, 0, {
      if (we==el_c32 && xe==el_i32) {
        u32* wp = c32any_ptr(w); usz wia = IA(w);
        u32* rp; r = m_c32arrc(&rp, w);
        i32* xp = i32any_ptr(x);
        for (usz i = 0; i < wia; i++) {
          rp[i] = (u32)((i32)wp[i] - (i32)xp[i]);
          if (rp[i]>CHR_MAX) thrM("-: Invalid character"); // safe - see add
        }
        goto dec_ret;
      }
    })
    #undef AR_I_AA
    #define AR_I_AS(CHR, NAME, EXPR, DO_AS, EXTRA) NOINLINE B NAME##_AS(B t, B w, B x) { \
      B r; u8 we=TI(w,elType); EXTRA                       \
      if (isF64(x)) { usz ia=IA(w); DO_AS(NAME,EXPR) }     \
      ARITH_SLOW(CHR); return arith_recd(NAME##_c2, w, x); \
      dec_ret: decG(w); return r;                          \
    }
    
    #define AR_I_SA(CHR, NAME, EXPR, DO_SA, EXTRA) NOINLINE B NAME##_SA(B t, B w, B x) { \
      B r; u8 xe=TI(x,elType); EXTRA                       \
      if (isF64(w)) { usz ia=IA(x); DO_SA(NAME,EXPR) }     \
      ARITH_SLOW(CHR); return arith_recd(NAME##_c2, w, x); \
      dec_ret: decG(x); return r;                          \
    }
    
    static NOINLINE B bit_sel1Fn(BBB2B f, B w, B x, bool bitX) { // consumes both
      B b = bitX? x : w;
      u64* bp = bitarr_ptr(b);
      usz ia = IA(b);
      
      bool b0 = ia? bp[0]&1 : 0;
      bool both = false;
      for (usz i = 0; i < ia; i++) if (bitp_get(bp,i) != b0) { both=true; break; }
      
      B e0=m_f64(0), e1=m_f64(0); // initialized to have something to decrement later
      bool h0=both || b0==0; if (h0) e0 = bitX? f(bi_N, inc(w), m_f64(0)) : f(bi_N, m_f64(0), inc(x));
      bool h1=both || b0==1; if (h1) e1 = bitX? f(bi_N,     w,  m_f64(1)) : f(bi_N, m_f64(1), x);
      // non-bitarr arg has been consumed
      B r = bit_sel(b, e0, e1); // and now the bitarr arg is consumed too
      dec(e0); dec(e1);
      return r;
    }
    
    AR_I_SA("-", sub, wv-xv, SI_SA, {})
    AR_I_SA("×", mul, wv*xv, SI_SA, {})
    AR_I_SA("∧", and, wv*xv, REG_SA, {})
    AR_I_SA("∨", or , (wv+xv)-(wv*xv), REG_SA, {})
    AR_I_SA("⌊", floor, wv>xv?xv:wv, REG_SA, {})
    AR_I_SA("⌈", ceil , wv>xv?wv:xv, REG_SA, {})
    AR_I_SA("+", add, wv+xv, SI_SA, {
      if (isC32(w) && xe==el_i32) {
        u32 wv = o2cG(w);
        i32* xp = i32any_ptr(x); usz xia = IA(x);
        u32* rp; r = m_c32arrc(&rp, x);
        for (usz i = 0; i < xia; i++) {
          rp[i] = (u32)(xp[i]+(i32)wv);
          if (rp[i]>CHR_MAX) thrM("+: Invalid character"); // safe to only check this as wv already must be below CHR_MAX, which is less than U32_MAX/2
        }
        goto dec_ret;
      }
    })
    #undef AR_I_SA
    
    
    AR_I_AS("-", sub, wv-xv, SI_AS, {
      if (we==el_c32 && isC32(x)) {
        i32 xv = (i32)o2cG(x);
        u32* wp = c32any_ptr(w); usz wia = IA(w);
        i32* rp; r = m_i32arrc(&rp, w);
        for (usz i = 0; i < wia; i++) rp[i] = (i32)wp[i] - xv;
        goto dec_ret;
      }
    })
    #undef AR_I_AS
  #endif // !SINGELI
  
  #define add_AS(T, W, X) add_SA(T, X, W)
  #define mul_AS(T, W, X) mul_SA(T, X, W)
  #define and_AS(T, W, X) and_SA(T, X, W)
  #define or_AS(T, W, X) or_SA(T, X, W)
  #define floor_AS(T, W, X) floor_SA(T, X, W)
  #define ceil_AS(T, W, X) ceil_SA(T, X, W)
  
  #define AR_F_TO_ARR(NAME) return NAME##_c2_arr(t, w, x);
  #define AR_I_TO_ARR(NAME) \
    if (isArr(x)) {                                  \
      if (isArr(w)) return NAME##_AA(t, w, x);       \
      else return NAME##_SA(t, w, x);                \
    } else if (isArr(w)) return NAME##_AS(t, w, x);
  
#endif // TYPED_ARITH

#define AR_I_SCALAR(CHR, NAME, EXPR, MORE) B NAME##_c2(B t, B w, B x) { \
  if (isF64(w) & isF64(x)) return m_f64(EXPR); \
  MORE \
  AR_I_TO_ARR(NAME) \
  thrM(CHR ": Unexpected argument types"); \
}

AR_I_SCALAR("+", add, w.f+x.f, {
  if (isC32(w) & isF64(x)) { u64 r = (u64)(o2cG(w)+o2i64(x)); if(r>CHR_MAX)thrM("+: Invalid character"); return m_c32((u32)r); }
  if (isF64(w) & isC32(x)) { u64 r = (u64)(o2cG(x)+o2i64(w)); if(r>CHR_MAX)thrM("+: Invalid character"); return m_c32((u32)r); }
});
AR_I_SCALAR("-", sub, w.f-x.f, {
  if (isC32(w) & isF64(x)) { u64 r = (u64)((i32)o2cG(w)-o2i64(x)); if(r>CHR_MAX)thrM("-: Invalid character"); return m_c32((u32)r); }
  if (isC32(w) & isC32(x)) return m_f64((i32)(u32)w.u - (i32)(u32)x.u);
})
AR_I_SCALAR("×", mul, w.f*x.f, {})
AR_I_SCALAR("∧", and, w.f*x.f, {})
AR_I_SCALAR("∨", or , bqn_or(w.f, x.f), {})
AR_I_SCALAR("⌊", floor, w.f>x.f?x.f:w.f, {})
AR_I_SCALAR("⌈", ceil , w.f>x.f?w.f:x.f, {})
#undef AR_I_SCALAR
B not_c2(B t, B w, B x) {
  return add_c2(m_f64(1), m_f64(1), sub_c2(t, w, x));
}

#define AR_F_SCALAR(CHR, NAME, EXPR) B NAME##_c2(B t, B w, B x) { \
  if (isF64(w) & isF64(x)) return m_f64(EXPR); \
  AR_F_TO_ARR(NAME) \
  thrM(CHR ": Unexpected argument types"); \
}
AR_F_SCALAR("÷", div  ,           w.f/x.f)
AR_F_SCALAR("⋆", pow  ,     pow(w.f, x.f))
AR_F_SCALAR("√", root , pow(x.f, 1.0/w.f))
AR_F_SCALAR("|", stile,   pfmod(x.f, w.f))
AR_F_SCALAR("⋆⁼",log  , log(x.f)/log(w.f))
#undef AR_F_SCALAR

static f64 comb_nat(f64 k, f64 n) {
  assert(k>=0 && n>=2*k);
  if (k > 514) return INFINITY;
  f64 p = 1;
  for (usz i=0; i<(usz)k; i++) {
    p*= (n-i) / (k-i);
    if (p == INFINITY) return p;
  }
  return round(p);
}
static f64 comb(f64 k, f64 n) { // n choose k
  f64 j = n - k;  // j+k == n
  bool jint = j == round(j);
  if (k == round(k)) {
    if (jint) {
      if (k<j) { f64 t=k; k=j; j=t; } // Now j<k
      if (n >= 0) {
        return j<0? 0 : comb_nat(j, n);
      } else {
        if (k<0) return 0;
        f64 l = -1-n; // l+k == -1-j
        f64 r = comb_nat(k<l? k : l, -1-j);
        return k<(1ull<<53) && ((i64)k&1)? -r : r;
      }
    }
    if (k < 0) return 0;
  } else if (jint) {
    if (j < 0) return 0;
  }
  return exp(lgamma(n+1) - lgamma(k+1) - lgamma(j+1));
}

#define MATH(n,N) \
  B n##_c2(B t, B w, B x) {                              \
    if (isNum(w) && isNum(x)) return m_f64(n(x.f, w.f)); \
    P2(n)                                                \
    thrM("•math." #N ": Unexpected argument types");     \
  }
MATH(atan2,Atan2) MATH(hypot,Hypot) MATH(comb,Comb)
#undef MATH

static u64 gcd_u64(u64 a, u64 b) {
  if (a == 0) return b;
  if (b == 0) return a;
  u8 az = CTZ(a);
  u8 bz = CTZ(b);
  u8 sh = az<bz? az : bz;
  
  b >>= bz;
  while (a > 0) {
    a >>= az;
    u64 d = b - a;
    az = CTZ(d);
    b = b<a? b : a;
    a = b<a? -d : d;
  }
  return b << sh;
}
static u64 lcm_u64(u64 a, u64 b) {
  if (a==0 | b==0) return 0;
  return (a / gcd_u64(a, b)) * b;
}
B gcd_c2(B t, B w, B x) {
  if (isNum(w) && isNum(x)) {
    if (!q_u64(w) || !q_u64(x)) thrM("•math.GCD: Inputs other than natural numbers not yet supported");
    return m_f64(gcd_u64(o2u64G(w), o2u64G(x)));
  }
  P2(gcd)
  thrM("•math.GCD: Unexpected argument types");
}
B lcm_c2(B t, B w, B x) {
  if (isNum(w) && isNum(x)) {
    if (!q_u64(w) || !q_u64(x)) thrM("•math.LCM: Inputs other than natural numbers not yet supported");
    return m_f64(lcm_u64(o2u64G(w), o2u64G(x)));
  }
  P2(gcd)
  thrM("•math.GCD: Unexpected argument types");
}

#undef P2
