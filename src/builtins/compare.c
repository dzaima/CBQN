#include "../core.h"
#include "../utils/calls.h"

NOINLINE i32 compareF(B w, B x) {
  if (isNum(w) & isC32(x)) return -1;
  if (isC32(w) & isNum(x)) return  1;
  
  i32 atmNeg;
  if (isAtm(w)) {
    atmNeg = 0;
    
    atmW:;
    if (isAtm(x)) thrM("Invalid comparison");
    
    if (IA(x)==0) return atmNeg? -1 : 1;
    i32 c = compare(w, IGetU(x,0));
    return (c<=0)^atmNeg? -1 : 1;
  }
  if (isAtm(x)) { B t=w; w=x; x=t; atmNeg=1; goto atmW; }
  if (w.u==x.u && TI(w,arrD1)) return 0;
  
  ur wr=RNK(w); usz* wsh=SH(w);
  ur xr=RNK(x); usz* xsh=SH(x);
  
  i32 rc = ICMP(wr, xr);
  ur rr = wr<xr? wr : xr;
  i32 ri = 0; // matching shape tail
  usz rm = 1;
  while (ri<rr  &&  wsh[wr-1-ri] == xsh[xr-1-ri]) {
    rm*= wsh[wr-ri-1];
    ri++;
  }
  if (ri<rr) {
    usz wm = wsh[wr-1-ri];
    usz xm = xsh[xr-1-ri];
    rc = ICMP(wm, xm);
    rm*= wm<xm? wm : xm;
  }
  
  usz wia = IA(w);
  usz xia = IA(x);
  if (wia==0 || xia==0) {
    i32 rc2 = ICMP(wia, xia);
    return rc2!=0? rc2 : rc;
  }
  
  assert(rm<=wia && rm<=xia);
  SGetU(w) SGetU(x)
  for (u64 i = 0; i < rm; i++) {
    i32 c = compare(GetU(w,i), GetU(x,i));
    if (c!=0) return c;
  }
  return rc;
}

NOINLINE bool atomEqualF(B w, B x) {
  if (TI(w,byRef) || TY(w)!=TY(x)) return false;
  
  B2B dcf = TI(w,decompose);
  B xd=dcf(incG(x)); B* xdp=harr_ptr(xd);
  if (o2i(xdp[0])<=1) goto decx_ne;
  B wd=dcf(incG(w)); B* wdp=harr_ptr(wd);
  
  usz wia = IA(wd);
  if (wia != IA(xd)) goto dec_ne;
  for (ux i = 0; i < wia; i++) {
    if(!eequal(wdp[i], xdp[i])) goto dec_ne;
  }
  decG(wd); decG(xd);
  return true;
  
  dec_ne:; decG(wd);
  decx_ne:; decG(xd);
  return false;
}

bool atomEEqual(B w, B x) { // doesn't consume
  #if !NEEQUAL_NEGZERO
    if (isF64(w) & isF64(x)) return w.f==x.f;
  #endif
  if (!isVal(w) || !isVal(x)) return false;
  
  if (TI(w,byRef) || TY(w)!=TY(x)) return false;
  B2B dcf = TI(w,decompose);
  B xd=dcf(incG(x)); B* xdp=harr_ptr(xd);
  if (o2i(xdp[0])<=1) goto decx_ne;
  B wd=dcf(incG(w)); B* wdp=harr_ptr(wd);
  
  usz wia = IA(wd);
  if (wia != IA(xd)) goto dec_ne;
  for (ux i = 0; i < wia; i++) {
    if(!eequal(wdp[i], xdp[i])) goto dec_ne;
  }
  decG(wd); decG(xd);
  return true;
  
  dec_ne:; decG(wd);
  decx_ne:; decG(xd);
  return false;
}

static const u8 n = 99;
u8 const matchFnData[] = { // for the main diagonal, amount to shift length by; otherwise, whether to swap arguments
  0,0,0,0,0,n,n,n,
  1,0,0,0,0,n,n,n,
  1,1,1,0,0,n,n,n,
  1,1,1,2,0,n,n,n,
  1,1,1,1,0,n,n,n,
  n,n,n,n,n,0,0,0,
  n,n,n,n,n,1,1,0,
  n,n,n,n,n,1,1,2,
};

#if SINGELI_SIMD
  #define F(X) simd_equal_##X
  #define SINGELI_FILE equal
  #include "../utils/includeSingeli.h"
#else
  #define F(X) equal_##X
  bool F(1_1)(void* w, void* x, u64 l, u64 d) {
    assert(l>0);
    u64* wp = w; u64* xp = x;
    usz q = l/64;
    for (usz i=0; i<q; i++) if (wp[i] != xp[i]) return false;
    usz r = (-l)%64; return r==0 || (wp[q]^xp[q])<<r == 0;
  }
  #define DEF_EQ_U1(N, T) \
    bool F(1_##N)(void* w, void* x, u64 l, u64 d) { assert(l>0);       \
      if (d!=0) { void* t=w; w=x; x=t; }                               \
      u64* wp = w; T* xp = x;                                          \
      for (usz i=0; i<l; i++) if (bitp_get(wp,i)!=xp[i]) return false; \
      return true;                                                     \
    }
  DEF_EQ_U1(8, i8)
  DEF_EQ_U1(16, i16)
  DEF_EQ_U1(32, i32)
  DEF_EQ_U1(f64, f64)
  #undef DEF_EQ_U1

  #define DEF_EQ_I(NAME, S, T, INIT) \
    bool F(NAME)(void* w, void* x, u64 l, u64 d) {            \
      assert(l>0); INIT                                       \
      S* wp = w; T* xp = x;                                   \
      for (usz i=0; i<l; i++) if (wp[i]!=xp[i]) return false; \
      return true;                                            \
    }
  #define DEF_EQ(N,S,T) DEF_EQ_I(N,S,T, if (d!=0) { void* t=w; w=x; x=t; })
  DEF_EQ_I(8_8, u8, u8, l<<=d;)
  DEF_EQ_I(f64_f64, f64, f64, )
  DEF_EQ(u8_16,  u8, u16)
  DEF_EQ(u8_32,  u8, u32) DEF_EQ(u16_32,  u16, u32)
  DEF_EQ(s8_16,  i8, i16)
  DEF_EQ(s8_32,  i8, i32) DEF_EQ(s16_32,  i16, i32)
  DEF_EQ(s8_f64, i8, f64) DEF_EQ(s16_f64, i16, f64) DEF_EQ(s32_f64, i32, f64)
  #undef DEF_EQ_I
  #undef DEF_EQ
#endif
static NOINLINE bool notEq(void* a, void* b, u64 l, u64 data) { assert(l>0); return false; }
static NOINLINE bool eequalFloat(void* wp0, void* xp0, u64 ia, u64 data) {
  f64* wp = wp0;
  f64* xp = xp0;
  u64 r = 1;
  for (ux i = 0; i < (ux)ia; i++) {
    #if NEEQUAL_NEGZERO
    r&= ((u64*)wp)[i] == ((u64*)xp)[i];
    #else
    r&= (wp[i]==xp[i]) | (wp[i]!=wp[i] & xp[i]!=xp[i]);
    #endif
  }
  return r;
}

#define MAKE_TABLE(NAME, F64_F64) \
INIT_GLOBAL MatchFn NAME[] = { \
  F(1_1),   F(1_8),    F(1_16),    F(1_32),    F(1_f64),   notEq,    notEq,     notEq,     \
  F(1_8),   F(8_8),    F(s8_16),   F(s8_32),   F(s8_f64),  notEq,    notEq,     notEq,     \
  F(1_16),  F(s8_16),  F(8_8),     F(s16_32),  F(s16_f64), notEq,    notEq,     notEq,     \
  F(1_32),  F(s8_32),  F(s16_32),  F(8_8),     F(s32_f64), notEq,    notEq,     notEq,     \
  F(1_f64), F(s8_f64), F(s16_f64), F(s32_f64), F64_F64,    notEq,    notEq,     notEq,     \
  notEq,    notEq,     notEq,      notEq,      notEq,      F(8_8),   F(u8_16),  F(u8_32),  \
  notEq,    notEq,     notEq,      notEq,      notEq,      F(u8_16), F(8_8),    F(u16_32), \
  notEq,    notEq,     notEq,      notEq,      notEq,      F(u8_32), F(u16_32), F(8_8),    \
};
MAKE_TABLE(matchFns, F(f64_f64));
MAKE_TABLE(matchFnsR, eequalFloat);
#undef MAKE_TABLE

#undef F



static NOINLINE bool equalSlow(B w, B x, usz ia) {
  SLOW2("equal", w, x);
  SGetU(x) SGetU(w)
  for (usz i = 0; i < ia; i++) if(!equal(GetU(w,i),GetU(x,i))) return false;
  return true;
}
static NOINLINE bool eequalSlow(B w, B x, usz ia) {
  SLOW2("eequal", w, x);
  SGetU(x) SGetU(w)
  for (usz i = 0; i < ia; i++) if(!eequal(GetU(w,i),GetU(x,i))) return false;
  return true;
}



#define MATCH_IMPL(ATOM, SLOW, MATCH) \
  if (isAtm(w)) {                \
    if (!isAtm(x)) return false; \
    return ATOM(w, x);           \
  }                              \
  if (isAtm(x)) return false;    \
  ur wr = RNK(w);                \
  if (wr!=RNK(x)) return false;  \
  usz ia = IA(x);                \
  if (LIKELY(wr==1)) { if (ia != IA(w)) return false; } \
  else if (!eqShPart(SH(w), SH(x), wr)) return false;   \
  if (ia==0) return true;        \
  u8 we = TI(w,elType);          \
  u8 xe = TI(x,elType);          \
  if (we!=el_B && xe!=el_B) {    \
    MatchFnObj f = MATCH(we,xe); \
    return MATCH_CALL(f, tyany_ptr(w), tyany_ptr(x), ia); \
  }                              \
  return SLOW(w, x, ia);

NOINLINE bool equal(B w, B x) { // doesn't consume
  NOGC_CHECK("cannot use equal(w,x) during noAlloc");
  MATCH_IMPL(atomEqual, equalSlow, MATCH_GET);
}

bool eequal(B w, B x) { // doesn't consume
  NOGC_CHECK("cannot use eequal(w,x) during noAlloc");
  if (w.u==x.u) return true;
  MATCH_IMPL(atomEEqual, eequalSlow, MATCHR_GET);
}
