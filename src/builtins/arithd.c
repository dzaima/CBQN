#include "core.h"
#include "utils/each.h"
#include "builtins.h"
#include "builtins/arith.h"

#define mul_fillFlags A_B
#define and_fillFlags A_B
#define or_fillFlags A_B
#define floor_fillFlags A_B
#define ceil_fillFlags A_B
#define pow_fillFlags A_B
#define root_fillFlags A_B
#define log_fillFlags A_B
#define cpxf_fillFlags A_B
#define add_fillFlags  A_B | A_CN(A_CHR) | A_NC(A_CHR)
#define sub_fillFlags  A_B | A_CN(A_CHR) | A_CC(A_NUM)
#define subR_fillFlags A_B | A_NC(A_CHR) | A_CC(A_NUM)

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


B leading_axis_arith(FC2 fc2, B w, B x, usz* wsh, usz* xsh, ur mr);

typedef void (*AndBytesFn)(u8*, u8*, u64, u64);
extern INIT_GLOBAL const AndBytesFn andBytes_fn;

#if SINGELI_SIMD
  #include "singeli/c/arithdDispatch.c"
#endif

B floor_c1(B t, B x);
B sub_c1(B t, B x);
B fne_c1(B t, B x);
B shape_c2(B t, B w, B x);

// all divint/floordiv/modint assume integer arguments
// floordiv will return float result only on ¯2147483648÷¯1 or n÷0, but may not otherwise squeeze integer types; integer argument requirement may be relaxed in the future
// divint will return float result if there's a fractional result, or in overflow cases same as floordiv
// TODO overflow-checked Singeli code for exact integer divint, and maybe floordiv_AA
#define DIVLOOP(RE, WE, EXPR) RE* rp; B r=m_##RE##arrc(&rp, w); usz ia=IA(w); WE* wp=WE##any_ptr(w); vfor(ux i=0; i<ia; i++) rp[i] = (EXPR);
static B divint_AA(B w, B x) { // consumes both
  w = toI32Any(w);
  x = toI32Any(x); i32* xp = tyany_ptr(x);
  DIVLOOP(f64, i32, wp[i]/(f64)xp[i]);
  r = squeeze_numNewTy(el_f64,r); decG(w); decG(x); return r;
}
static B divint_AS(B w, i32 xv) { // consumes
  w = toI32Any(w);
  if (xv==1) return w;
  if (xv==-1) return C1(sub, w);
  if (xv==0) return C2(mul, w, m_f64(1.0/0.0));
  DIVLOOP(f64, i32, wp[i]/(f64)xv);
  r = squeeze_numNewTy(el_f64,r); decG(w); return r;
}

static B floordiv_AA(B w, B x) { // consumes both
  u8 we=TI(w,elType); assert(we<=el_i32);
  u8 xe=TI(x,elType); assert(xe<=el_i32);
  if (we<=el_i16) {
    w = taga(cpyI16Arr(w));
    x = toI32Any(x); i32* xp = i32any_ptr(x);
    DIVLOOP(f64, i16, floorf((f32)wp[i] / (f32)xp[i]));
    r = squeeze_numNewTy(el_f64,r); decG(w); decG(x); return r;
  }
  return C1(floor, divint_AA(w, x));
}
static B floordiv_AS(B w, i32 xv) { // consumes
  u8 we = TI(w,elType);
  assert(we<=el_i32);
  if (xv==1) return w;
  if (xv==-1) return C1(sub, w);
  if (xv==0) return C2(mul, w, m_f64(1.0/0.0));
  if (we<=el_i16) {
    w = toI16Any(w);
    f32 xr = 1/(f32)xv;
    f32 off = 0.5*fabsf(xr);
    DIVLOOP(i16, i16, floorf((f32)wp[i] * xr + off));
    decG(w); return r;
  } else {
    w = toI32Any(w);
    f64 xr = 1/(f64)xv;
    f64 off = 0.5*fabs(xr);
    DIVLOOP(i32, i32, floor((f64)wp[i] * xr + off));
    decG(w); return r;
  }
}
#undef DIVLOOP

static B modint_AA(B w,    B x) { return squeeze_numNew(C2(sub, x, C2(mul, w,         floordiv_AA(incG(x), incG(w))))); } // consumes both
static B modint_SA(i32 wv, B x) { return squeeze_numNew(C2(sub, x, C2(mul, m_i32(wv), floordiv_AS(incG(x), wv)))); } // consumes
static B modint_AS(B w,   B xv) { return modint_AA(w, C2(shape, C1(fne, incG(w)), xv)); } // consumes w, assumes xv is number




#define ARITH_SLOW(N) SLOWIF((!isArr(w) || TI(w,elType)!=el_B)  &&  (!isArr(x) || TI(x,elType)!=el_B)) SLOW2("arithd " #N, w, x)
#define P2(N, FILL_FLAGS) { if(isArr(w)|isArr(x)) { ARITH_SLOW(N); toRec: MAYBE_UNUSED; return arith_recd(N##_c2, w, x, FILL_FLAGS); }}

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

#define DEF_CPX_COMM(CHR, NAME, F) NOINLINE B NAME##_cpx_c2(B t, B cpx, B other, bool swapped) { \
  CpxVal va = o2cpxXGD(cpx); \
  CpxVal vb; if (!q_cpxD(&vb, other)) thrM("𝕨"CHR"𝕩: One argument was a complex number, but the other wasn't a number"); \
  return F; \
}
#define TO_CPX_COMM(NAME) ({ \
  if (isCpx(w)) return NAME##_cpx_c2(t, w, x, 0); \
  if (isCpx(x)) return NAME##_cpx_c2(t, x, w, 1); \
})

#define DEF_CPX(CHR, NAME, F) \
NOINLINE B NAME##_cpx_w_c2(B t, B w, B x) { \
  CpxVal wv = o2cpxXGD(w); \
  CpxVal xv; if (!q_cpxD(&xv, x)) thrM("𝕨"CHR"𝕩: 𝕨 was a complex number, but 𝕩 wasn't a number"); \
  return F; \
} \
NOINLINE B NAME##_cpx_x_c2(B t, B w, B x) { \
  CpxVal xv = o2cpxXGD(x); \
  CpxVal wv; if (!q_cpxD(&wv, w)) thrM("𝕨"CHR"𝕩: 𝕩 was a complex number, but 𝕨 wasn't a number"); \
  return F; \
}
#define TO_CPX(NAME) ({ \
  if (isCpx(w)) return NAME##_cpx_w_c2(t, w, x); \
  if (isCpx(x)) return NAME##_cpx_x_c2(t, w, x); \
})

DEF_CPX_COMM("+", add, m_cpxv(cpx_add(va, vb)))
DEF_CPX     ("-", sub, m_cpxv(cpx_sub(wv, xv)))
DEF_CPX_COMM("×", mul, m_cpxv(cpx_mul(va, vb)))
DEF_CPX     ("÷", div, m_cpxv(cpx_div(wv, xv)))
DEF_CPX("√",  root,  m_cpxv(cpx_pow(xv, cpx_div((CpxVal){1,0}, wv))))
DEF_CPX("⋆",  pow,   m_cpxv(cpx_pow(wv, xv)))
DEF_CPX("⋆⁼", log,   m_cpxv(cpx_div(cpx_log(xv), cpx_log(wv))))
DEF_CPX("|",  stile, (thrM("𝕨|𝕩: TODO complex"),(void)wv,(void)xv,bi_N))
DEF_CPX_COMM("∧", and,    (thrM("𝕨∧𝕩: TODO complex"),(void)va,(void)vb,bi_N))
DEF_CPX_COMM("∨", or,     (thrM("𝕨∨𝕩: TODO complex"),(void)va,(void)vb,bi_N))
DEF_CPX_COMM("⌊", floor,  (thrM("𝕨⌊𝕩: TODO complex"),(void)va,(void)vb,bi_N))
DEF_CPX_COMM("⌈", ceil,   (thrM("𝕨⌈𝕩: TODO complex"),(void)va,(void)vb,bi_N))
B cpxf_c2(B t, B w, B x) {
  CpxVal wv, xv;
  if (q_cpx(&wv,w) && q_cpxD(&xv,x)) {
    dec(w);
    return m_cpx(wv.re-xv.im, wv.im+xv.re);
  }
  if (isArr(w) || isArr(x)) return arith_recd(cpxf_c2, w, x, cpxf_fillFlags);
  thrM("𝕨⍳𝕩: bad args TODO message");
}

#define PF(N)   f64* N##p = f64any_ptr(N);
#define PI8(N)  i8*  N##p = i8any_ptr (N);
#define PI16(N) i16* N##p = i16any_ptr(N);
#define PI32(N) i32* N##p = i32any_ptr(N);
#define Ri8(A)  i8*  rp; r=m_i8arrc (&rp, A);
#define Ri16(A) i16* rp; r=m_i16arrc(&rp, A);
#define Ri32(A) i32* rp; r=m_i32arrc(&rp, A);
#define Rf64(A) f64* rp; r=m_f64arrc(&rp, A);
#define DOF(EXPR,A,W,X) { vfor (usz i = 0; i < ia; i++) { f64 wv=W; f64 xv=X; rp[i]=EXPR; } }
#define DOI8(EXPR,A,W,X,BASE)  { Ri8(A)  for (usz i=0; i<ia; i++) { i16 wv=W; i16 xv=X; i16 rv=EXPR; if (RARE(rv!=( i8)rv)) { decG(r); goto BASE; } rp[i]=rv; } goto dec_ret; }
#define DOI16(EXPR,A,W,X,BASE) { Ri16(A) for (usz i=0; i<ia; i++) { i32 wv=W; i32 xv=X; i32 rv=EXPR; if (RARE(rv!=(i16)rv)) { decG(r); goto BASE; } rp[i]=rv; } goto dec_ret; }
#define DOI32(EXPR,A,W,X,BASE) { Ri32(A) for (usz i=0; i<ia; i++) { i64 wv=W; i64 xv=X; i64 rv=EXPR; if (RARE(rv!=(i32)rv)) { decG(r); goto BASE; } rp[i]=rv; } goto dec_ret; }
static NOINLINE bool realToCpx(f64* rp, B r, ux ia) {
  bool bad = false;
  vfor (usz i = 0; i < ia; i++) bad |= isnan(rp[i]);
  if (bad) { decG(r); return true; }
  return false;
}
#define CPX_END(REAL_TO_CPX) if (COMPLEX_SUPPORT && REAL_TO_CPX && RARE(realToCpx(rp, r, ia))) goto toRec;

#define GC2f(SYMB, NAME, REAL_TO_CPX, EXPR, DECOR, INT_SA, INT_AS, INT_AA, FLT_SAI, ANY_AS) B NAME##_c2_arr(B t, B w, B x) { \
  if (isArr(w)|isArr(x)) { B r;                                 \
    if (isArr(w)&isArr(x) && RNK(w)==RNK(x)) {                  \
      if (!eqShPart(SH(w), SH(x), RNK(w))) thrF("𝕨" SYMB "𝕩: Expected equal shape prefix (%H ≡ ≢𝕨, %H ≡ ≢𝕩)", w, x); \
      usz ia = IA(x);                                           \
      u8 we = TI(w,elType);                                     \
      u8 xe = TI(x,elType);                                     \
      if (elNum(we) && elNum(xe)) {                             \
        if (we<=el_i32 && xe<=el_i32) { INT_AA; }               \
        if (we<el_i32) { w=taga(cpyI32Arr(w)); we=el_i32; } void* wp=tyany_ptr(w); \
        if (xe<el_i32) { x=taga(cpyI32Arr(x)); xe=el_i32; } void* xp=tyany_ptr(x); \
        Rf64(x);                                                \
        if (we==el_i32) { f64 wf, xf;                           \
          if (xe==el_i32) { DECOR vfor (usz i = 0; i < ia; i++) { wf=((i32*)wp)[i]; xf=((i32*)xp)[i]; rp[i]=EXPR; } } \
          else            { DECOR vfor (usz i = 0; i < ia; i++) { wf=((i32*)wp)[i]; xf=((f64*)xp)[i]; rp[i]=EXPR; } } \
        } else {          f64 wf, xf;                           \
          if (xe==el_i32) { DECOR vfor (usz i = 0; i < ia; i++) { wf=((f64*)wp)[i]; xf=((i32*)xp)[i]; rp[i]=EXPR; } } \
          else            { DECOR vfor (usz i = 0; i < ia; i++) { wf=((f64*)wp)[i]; xf=((f64*)xp)[i]; rp[i]=EXPR; } } \
        }                                                       \
        CPX_END(REAL_TO_CPX);                                   \
        decG(w); decG(x); return squeeze_numNewTy(el_f64,r);    \
      }                                                         \
    } else if (q_f64(w)&isArr(x)) { usz ia=IA(x); u8 xe=TI(x,elType); f64 wf=o2fG(w); \
      if (elInt(xe)) {INT_SA Rf64(x); x=toI32Any(x); PI32(x) DECOR vfor (usz i=0; i<ia; i++) {f64 xf=xp[i]; rp[i]=EXPR;} CPX_END(REAL_TO_CPX); decG(x); return squeeze_numNewTy(el_f64,r); } \
      if (xe==el_f64){       Rf64(x);         PF(x)  FLT_SAI DECOR vfor (usz i=0; i<ia; i++) {f64 xf=xp[i]; rp[i]=EXPR;} CPX_END(REAL_TO_CPX); decG(x); return squeeze_numNewTy(el_f64,r); } \
    } else if (q_f64(x)&isArr(w)) { usz ia=IA(w); u8 we=TI(w,elType); f64 xf=o2fG(x); ANY_AS \
      if (elInt(we)) {INT_AS Rf64(w); w=toI32Any(w); PI32(w) DECOR vfor (usz i=0; i<ia; i++) {f64 wf=wp[i]; rp[i]=EXPR;} CPX_END(REAL_TO_CPX); decG(w); return squeeze_numNewTy(el_f64,r); } \
      if (we==el_f64){       Rf64(w);         PF(w)          DECOR vfor (usz i=0; i<ia; i++) {f64 wf=wp[i]; rp[i]=EXPR;} CPX_END(REAL_TO_CPX); decG(w); return squeeze_numNewTy(el_f64,r); } \
    }                                                           \
    P2(NAME, A_B)                                               \
  }                                                             \
  TO_CPX(NAME); thrM("𝕨" SYMB "𝕩: Unexpected argument types");  \
}
GC2f("÷", div, 0, wf/(xf+0),
  , /*INT_SA*/
  , /*INT_AS*/ if(q_i32(x)) { r = divint_AS(w, o2iG(x)); /*decG(w);         */ return r; }
  , /*INT_AA*/                r = divint_AA(w, x);       /*decG(w); decG(x);*/ return r;
  , /*FLT_SAI*/
  , /*ANY_AS*/ if((r_f64u(o2fG(x)) & TAIL(u64,52)) == 0 && elNum(we)) return squeeze_numNew(C2(mul, w, m_f64(1/(o2fG(x)+0))));
)

#define CPX_FALLBACK(NAME) \
  { bool has_nan = false; vfor (usz i = 0; i < ia; i++) has_nan |= isnan(rp[i]); \
    if (RARE(has_nan)) { decG(r); return arith_recd(NAME##_c2, w, x, NAME##_fillFlags); } }

GC2f("√",  root, 1, pow(xf+0, 1.0/(wf+0)), NOUNROLL,,,,,)
GC2f("⋆",  pow,  1, pow(wf+0, xf), NOUNROLL,,,,,)
GC2f("⋆⁼", log,  1, log(xf)/log(wf), NOUNROLL,,,,,)
static u64 const repeatNum[] = {
  [el_i8 ] = 0x0101010101010101ULL,
  [el_i16] = 0x0001000100010001ULL,
  [el_i32] = 0x0000000100000001ULL,
};
GC2f("|", stile, 0, pfmod(xf, wf), NOUNROLL,
  /*INT_SA*/
  if (q_i32(w)) {
    i32 wi32 = o2iG(w);
    if (wi32>0 && (wi32&(wi32-1))==0) {
      if (wi32==1) return i64EachDec(0, x);
      if (xe==el_bit) return x; // if n>1 (true from the above), 0‿1 ≡ (2⋆n)|0‿1
      u8 elw = elWidth(xe);
      u32 mask0 = (u32)wi32;
      if (mask0 > ((u32)1 << (elw*8-1))) {
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
      else if (wi32<256) return taga(cpyI8Arr(r)); // these won't widen, as the code doesn't even get to here if 𝕨 > max possible in 𝕩
      else if (wi32<65536) return taga(cpyI16Arr(r));
      return r;
    } else {
      return modint_SA(wi32, x);
    }
  }
  , /*INT_AS*/ if (q_i32(x)) return modint_AS(w, x);
  , /*INT_AA*/ return modint_AA(w, x);
  , /*FLT_SAI*/ if (o2fG(w)==1) { vfor (usz i=0; i<ia; i++) rp[i] = xp[i]-floor(xp[i]); } else
  , /*ANY_AS*/
)
#undef GC2f


#if SINGELI_SIMD
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
  
#else // !SINGELI_SIMD
  static NOINLINE B bit_sel1Fn(FC2 f, B w, B x, bool bitX) { // consumes both
    B b = bitX? x : w;
    u64* bp = bitany_ptr(b);
    usz ia = IA(b);
    
    bool b0 = ia? bp[0]&1 : 0;
    bool both = bit_has(bp, ia, !b0);
    
    B e0=bi_z, e1=bi_z; // initialized to have something to decrement later
    bool h0=both || b0==0; if (h0) e0 = bitX? f(bi_z, w, m_f64(0)) : f(bi_z, m_f64(0), x);
    bool h1=both || b0==1; if (h1) e1 = bitX? f(bi_z, w, m_f64(1)) : f(bi_z, m_f64(1), x);
    // non-bitarr arg has been consumed
    B r = bit_sel(b, e0, e1); // and now the bitarr arg is consumed too
    dec(e0); dec(e1);
    return r;
  }
  
  #define REG_SA(NAME, EXPR) \
    if (xe==el_bit) return bit_sel1Fn(NAME##_c2,w,x,1); \
    if (xe==el_i8  && q_i8 (w)) { PI8 (x) i8  wc=o2iG(w); DOI8 (EXPR,x,wc,xp[i],sa8B ) } sa8B :; \
    if (xe==el_i16 && q_i16(w)) { PI16(x) i16 wc=o2iG(w); DOI16(EXPR,x,wc,xp[i],sa16B) } sa16B:; \
    if (xe==el_i32 && q_i32(w)) { PI32(x) i32 wc=o2iG(w); DOI32(EXPR,x,wc,xp[i],sa32B) } sa32B:; \
    if (xe==el_f64) { Rf64(x) PF(x) DOF(EXPR,w,o2fG(w),xp[i]) goto dec_ret; }
  #define REG_AS(NAME, EXPR) \
    if (we==el_bit) return bit_sel1Fn(NAME##_c2,w,x,0); \
    if (we==el_i8  && q_i8 (x)) { PI8 (w) i8  xc=o2iG(x); DOI8 (EXPR,w,wp[i],xc,as8B ) } as8B :; \
    if (we==el_i16 && q_i16(x)) { PI16(w) i16 xc=o2iG(x); DOI16(EXPR,w,wp[i],xc,as16B) } as16B:; \
    if (we==el_i32 && q_i32(x)) { PI32(w) i32 xc=o2iG(x); DOI32(EXPR,w,wp[i],xc,as32B) } as32B:; \
    if (we==el_f64) { Rf64(w) PF(w) DOF(EXPR,x,wp[i],o2fG(x)) goto dec_ret; }
  
  static B bitAA0(B w, B x, usz ia) { UD; }
  static NOINLINE B bitAA1(B w, B x, usz ia) {
    u64* rp; B r = m_bitarrc(&rp, x);
    u64* wp=bitany_ptr(w); u64* xp=bitany_ptr(x);
    vfor (usz i=0; i<BIT_N(ia); i++) rp[i] = wp[i]|xp[i];
    decG(w); decG(x); return r;
  }
  static NOINLINE B bitAA2(B w, B x, usz ia) {
    u64* rp; B r = m_bitarrc(&rp, x);
    u64* wp=bitany_ptr(w); u64* xp=bitany_ptr(x);
    vfor (usz i=0; i<BIT_N(ia); i++) rp[i] = wp[i]&xp[i];
    decG(w); decG(x); return r;
  }
  
  #define AR_I_AA(CHR, NAME, EXPR, BIT, EXTRA) NOINLINE B NAME##_AA(B t, B w, B x) { \
    ur wr=RNK(w); usz* xsh=SH(x);                              \
    ur xr=RNK(x); usz* wsh=SH(w); ur mr=IMIN(wr,xr);           \
    if (!eqShPart(wsh, xsh, mr)) thrF("𝕨"CHR "𝕩: Expected equal shape prefix (%H ≡ ≢𝕨, %H ≡ ≢𝕩)", w, x); \
    if (wr!=xr) {                                              \
      if (TI(w,elType)!=el_B && TI(x,elType)!=el_B && IA(w)!=0 && IA(x)!=0) return leading_axis_arith(NAME##_c2, w, x, wsh, xsh, mr); \
      else goto bad;                                           \
    }                                                          \
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
      decG(w); decG(x); return squeeze_numNewTy(el_f64,r);     \
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
    bad: ARITH_SLOW(CHR); return arith_recd(NAME##_c2, w, x, NAME##_fillFlags);   \
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
        if (rp[i]>CHR_MAX) thrM("𝕨-𝕩: Invalid character"); // safe - see add
      }
      goto dec_ret;
    }
  })
  #undef AR_I_AA
  
  #define AR_I_AS(CHR, NAME, EXPR, DO_AS, EXTRA) NOINLINE B NAME##_AS(B t, B w, B x) { \
    B r; u8 we=TI(w,elType); EXTRA                       \
    if (q_f64(x)) { usz ia=IA(w); DO_AS(NAME,EXPR) }     \
    ARITH_SLOW(CHR); return arith_recd(NAME##_c2, w, x, NAME##_fillFlags); \
    dec_ret: decG(w); return r;                          \
  }
  
  #define AR_I_SA(CHR, NAME, EXPR, DO_SA, EXTRA) NOINLINE B NAME##_SA(B t, B w, B x) { \
    B r; u8 xe=TI(x,elType); EXTRA                       \
    if (q_f64(w)) { usz ia=IA(x); DO_SA(NAME,EXPR) }     \
    ARITH_SLOW(CHR); return arith_recd(NAME##_c2, w, x, NAME##_fillFlags); \
    dec_ret: decG(x); return r;                          \
  }
  
  AR_I_SA("-", sub, wv-xv, REG_SA, {})
  AR_I_SA("×", mul, wv*xv, REG_SA, {})
  AR_I_SA("∧", and, wv*xv, REG_SA, {})
  AR_I_SA("∨", or , (wv+xv)-(wv*xv), REG_SA, {})
  AR_I_SA("⌊", floor, wv>xv?xv:wv, REG_SA, {})
  AR_I_SA("⌈", ceil , wv>xv?wv:xv, REG_SA, {})
  AR_I_SA("+", add, wv+xv, REG_SA, {
    if (isC32(w) && elInt(xe)) {
      u32 wv = o2cG(w);
      if (xe!=el_i32) x = taga(cpyI32Arr(x));
      i32* xp = i32any_ptr(x); usz xia = IA(x);
      u32* rp; r = m_c32arrc(&rp, x);
      for (usz i = 0; i < xia; i++) {
        rp[i] = (u32)(xp[i]+(i32)wv);
        if (rp[i]>CHR_MAX) thrM("𝕨+𝕩: Invalid character"); // safe to only check this as wv already must be below CHR_MAX, which is less than U32_MAX/2
      }
      goto dec_ret;
    }
  })
  #undef AR_I_SA
  
  
  AR_I_AS("-", sub, wv-xv, REG_AS, {
    if (elChr(we) && isC32(x)) {
      i32 xv = (i32)o2cG(x);
      if (we!=el_c32) w = taga(cpyC32Arr(w));
      u32* wp = c32any_ptr(w); usz wia = IA(w);
      i32* rp; r = m_i32arrc(&rp, w);
      vfor (usz i = 0; i < wia; i++) rp[i] = (i32)wp[i] - xv;
      goto dec_ret;
    }
  })
  #undef AR_I_AS
#endif // !SINGELI_SIMD

#define add_AS(T, W, X) add_SA(T, X, W)
#define mul_AS(T, W, X) mul_SA(T, X, W)
#define and_AS(T, W, X) and_SA(T, X, W)
#define or_AS(T, W, X) or_SA(T, X, W)
#define floor_AS(T, W, X) floor_SA(T, X, W)
#define ceil_AS(T, W, X) ceil_SA(T, X, W)

#define AR_F_TO_ARR(NAME) return NAME##_c2_arr(t, w, x);
#define AR_I_TO_ARR(NAME) \
  if (isArr(x)) return isArr(w)? NAME##_AA(t, w, x) : NAME##_SA(t, w, x); \
  else if (isArr(w)) return NAME##_AS(t, w, x);

#define AR_I_SCALAR(CHR, NAME, EXPR, MORE, COMPLEX) B NAME##_c2(B t, B w, B x) { \
  if (q_f64(w) & q_f64(x)) return m_f64(EXPR); \
  MORE; AR_I_TO_ARR(NAME); COMPLEX;            \
  thrM("𝕨"CHR"𝕩: Unexpected argument types");  \
}

AR_I_SCALAR("+", add, w.f+x.f, {
  if (isC32(w) & q_f64(x)) { u64 r = (u64)(o2cG(w)+o2i64(x)); if(r>CHR_MAX)thrM("𝕨+𝕩: Invalid character"); return m_c32((u32)r); }
  if (q_f64(w) & isC32(x)) { u64 r = (u64)(o2cG(x)+o2i64(w)); if(r>CHR_MAX)thrM("𝕨+𝕩: Invalid character"); return m_c32((u32)r); }
}, TO_CPX_COMM(add))
AR_I_SCALAR("-", sub, w.f-x.f, {
  if (isC32(w) & q_f64(x)) { u64 r = (u64)((i32)o2cG(w)-o2i64(x)); if(r>CHR_MAX)thrM("𝕨-𝕩: Invalid character"); return m_c32((u32)r); }
  if (isC32(w) & isC32(x)) return m_f64((i32)(u32)w.u - (i32)(u32)x.u);
}, TO_CPX(sub))
AR_I_SCALAR("×", mul, w.f*x.f, {}, TO_CPX_COMM(mul))
AR_I_SCALAR("∧", and, w.f*x.f, {}, TO_CPX_COMM(and))
AR_I_SCALAR("∨", or , bqn_or(w.f, x.f), {}, TO_CPX_COMM(or))
AR_I_SCALAR("⌊", floor, w.f>x.f?x.f:w.f, {}, TO_CPX_COMM(floor))
AR_I_SCALAR("⌈", ceil , w.f>x.f?w.f:x.f, {}, TO_CPX_COMM(ceil))
#undef AR_I_SCALAR
B not_c2(B t, B w, B x) {
  return C2(add, m_f64(1), sub_c2(t, w, x));
}

#define AR_F_SCALAR(CHR, NAME, EXPR) B NAME##_c2(B t, B w, B x) { \
  if (q_f64(w) & q_f64(x)) { f64 wf=o2fG(w), xf=o2fG(x); return m_f64(EXPR); } \
  AR_F_TO_ARR(NAME); \
}
#define AR_F_SCALAR_CPX(CHR, NAME, EXPR, CPX_EXPR) B NAME##_c2(B t, B w, B x) { \
  if (q_f64(w) & q_f64(x)) { \
    f64 wf=o2fG(w), xf=o2fG(x); \
    f64 rv = EXPR; \
    if (RARE(isnan(rv)) && !isnan(wf) && !isnan(xf)) return m_cpxv(CPX_EXPR); \
    return m_f64(rv); \
  } \
  AR_F_TO_ARR(NAME); \
}
AR_F_SCALAR    ("÷", div  , wf/(xf+0))
AR_F_SCALAR_CPX("⋆", pow  , pow(wf+0, xf), cpx_pow((CpxVal){wf,0}, (CpxVal){xf,0}))
AR_F_SCALAR_CPX("√", root , pow(xf+0, 1.0/(0+wf)), cpx_pow((CpxVal){xf,0}, cpx_div((CpxVal){1,0}, (CpxVal){wf,0})))
AR_F_SCALAR    ("|", stile, pfmod(xf, wf))
AR_F_SCALAR_CPX("⋆⁼",log  , log(xf)/log(wf), cpx_div(cpx_log((CpxVal){xf,0}), cpx_log((CpxVal){wf,0})))
#undef AR_F_SCALAR
#undef AR_F_SCALAR_CPX

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
static f64 bqn_atan2  (f64 x, f64 w) { return atan2(x+0, w+0); }
static f64 bqn_atan2ix(f64 x, f64 w) { return w * tan(x); }
static f64 bqn_atan2iw(f64 x, f64 w) { return w / (tan(x)+0); }

#define MATH(n,N,I) B n##_c2(B t, B w, B x) { \
  if (q_f64(w) && q_f64(x)) return m_f64(I(o2fG(x), o2fG(w))); \
  P2(n, A_B); thrM("𝕨 •math." N " 𝕩: Unexpected argument types"); \
}
MATH(atan2,"Atan2",bqn_atan2)
MATH(atan2ix,"Atan2⁼",bqn_atan2ix)
MATH(atan2iw,"Atan2˜⁼",bqn_atan2iw)
MATH(hypot,"Hypot",hypot) MATH(comb,"Comb",comb)
#undef MATH

static u64 gcd_u64(u64 a, u64 b) {
  if (a == 0) return b;
  if (b == 0) return a;
  u8 az = CTZ(a);
  u8 bz = CTZ(b);
  u8 sh = IMIN(az, bz);
  
  b >>= bz;
  while (1) {
    a >>= az;
    u64 d = b - a;
    b = b<a? b : a;
    if (d == 0) break;
    a = b<a? -d : d;
    az = CTZ(d);
  }
  return b << sh;
}
static u64 lcm_u64(u64 a, u64 b) {
  if (a==0 | b==0) return 0;
  return (a / gcd_u64(a, b)) * b;
}
B gcd_c2(B t, B w, B x) {
  if (q_f64(w) && q_f64(x)) {
    u64 wu, xu;
    if (!q_u64o(&wu, w) || !q_u64o(&xu, x)) thrM("𝕨 •math.GCD 𝕩: Inputs other than natural numbers not yet supported");
    return m_f64(gcd_u64(wu, xu));
  }
  P2(gcd, A_B)
  thrM("𝕨 •math.GCD 𝕩: Unexpected argument types");
}
B lcm_c2(B t, B w, B x) {
  if (q_f64(w) && q_f64(x)) {
    u64 wu, xu;
    if (!q_u64o(&wu, w) || !q_u64o(&xu, x)) thrM("𝕨 •math.LCM 𝕩: Inputs other than natural numbers not yet supported");
    return m_f64(lcm_u64(wu, xu));
  }
  P2(lcm, A_B)
  thrM("𝕨 •math.LCM 𝕩: Unexpected argument types");
}

#undef P2

void arithd_init() {
  c(BFn, bi_atan2)->iw = atan2iw_c2;
  c(BFn, bi_atan2)->ix = atan2ix_c2;
  c(BFn, bi_pow)->ix = log_c2;
}
