#include "../core.h"
#include "../utils/calls.h"

#define CMP(W,X) ({ AUTO wt = (W); AUTO xt = (X); (wt>xt?1:0)-(wt<xt?1:0); })
NOINLINE i32 compareF(B w, B x) {
  if (isNum(w) & isC32(x)) return -1;
  if (isC32(w) & isNum(x)) return  1;
  if (isAtm(w) & isAtm(x)) thrM("Invalid comparison");
  bool wa=isAtm(w); usz wia; ur wr; usz* wsh; AS2B wgetU; Arr* wArr;
  bool xa=isAtm(x); usz xia; ur xr; usz* xsh; AS2B xgetU; Arr* xArr;
  if(wa) { wia=1; wr=0; wsh=NULL; } else { wia=IA(w); wr=RNK(w); wsh=SH(w); wgetU=TI(w,getU); wArr = a(w); }
  if(xa) { xia=1; xr=0; xsh=NULL; } else { xia=IA(x); xr=RNK(x); xsh=SH(x); xgetU=TI(x,getU); xArr = a(x); }
  if (wia==0 || xia==0) return CMP(wia, xia);
  
  i32 rc = CMP(wr+(wa?0:1), xr+(xa?0:1));
  ur rr = wr<xr? wr : xr;
  i32 ri = 0; // matching shape tail
  i32 rm = 1;
  while (ri<rr  &&  wsh[wr-1-ri] == xsh[xr-1-ri]) {
    rm*= wsh[wr-ri-1];
    ri++;
  }
  if (ri<rr) {
    usz wm = wsh[wr-1-ri];
    usz xm = xsh[xr-1-ri];
    rc = CMP(wm, xm);
    rm*= wm<xm? wm : xm;
  }
  for (u64 i = 0; i < (u64)rm; i++) {
    int c = compare(wa? w : wgetU(wArr,i), xa? x : xgetU(xArr,i));
    if (c!=0) return c;
  }
  return rc;
}
#undef CMP

NOINLINE bool atomEqualF(B w, B x) {
  if (TY(w)!=TY(x)) return false;
  B2B dcf = TI(w,decompose);
  if (dcf == def_decompose) return false;
  B wd=dcf(incG(w)); B* wdp = harr_ptr(wd);
  B xd=dcf(incG(x)); B* xdp = harr_ptr(xd);
  if (o2i(wdp[0])<=1) { decG(wd);decG(xd); return false; }
  usz wia = IA(wd);
  if (wia!=IA(xd))    { decG(wd);decG(xd); return false; }
  for (u64 i = 0; i<wia; i++) if(!equal(wdp[i], xdp[i]))
                      { decG(wd);decG(xd); return false; }
                        decG(wd);decG(xd); return true;
}

bool atomEEqual(B w, B x) { // doesn't consume
  if (w.u==x.u) return true;
  #if !NEEQUAL_NEGZERO
    if (isF64(w)&isF64(x)) return w.f==x.f;
  #endif
  if (!isVal(w) | !isVal(x)) return false;
  if (TY(w)!=TY(x)) return false;
  B2B dcf = TI(w,decompose);
  if (dcf == def_decompose) return false;
  B wd=dcf(incG(w)); B* wdp = harr_ptr(wd);
  B xd=dcf(incG(x)); B* xdp = harr_ptr(xd);
  if (o2i(wdp[0])<=1) { decG(wd);decG(xd); return false; }
  usz wia = IA(wd);
  if (wia!=IA(xd))    { decG(wd);decG(xd); return false; }
  for (u64 i = 0; i<wia; i++) if(!eequal(wdp[i], xdp[i]))
                      { decG(wd);decG(xd); return false; }
                        decG(wd);decG(xd); return true;
}

// Functions in eqFns compare segments for matching
// data argument comes from eqFnData
static const u8 n = 99;
u8 eqFnData[] = { // for the main diagonal, amount to shift length by; otherwise, whether to swap arguments
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
    u64* wp = w; u64* xp = x;
    usz q = l/64;
    for (usz i=0; i<q; i++) if (wp[i] != xp[i]) return false;
    usz r = (-l)%64; return r==0 || (wp[q]^xp[q])<<r == 0;
  }
  #define DEF_EQ_U1(N, T) \
    bool F(1_##N)(void* w, void* x, u64 l, u64 d) {                    \
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
      INIT                                                    \
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
bool notEq(void* a, void* b, u64 l, u64 data) { return false; }
EqFn eqFns[] = {
  F(1_1),   F(1_8),    F(1_16),    F(1_32),    F(1_f64),   notEq,    notEq,     notEq,
  F(1_8),   F(8_8),    F(s8_16),   F(s8_32),   F(s8_f64),  notEq,    notEq,     notEq,
  F(1_16),  F(s8_16),  F(8_8),     F(s16_32),  F(s16_f64), notEq,    notEq,     notEq,
  F(1_32),  F(s8_32),  F(s16_32),  F(8_8),     F(s32_f64), notEq,    notEq,     notEq,
  F(1_f64), F(s8_f64), F(s16_f64), F(s32_f64), F(f64_f64), notEq,    notEq,     notEq,
  notEq,    notEq,     notEq,      notEq,      notEq,      F(8_8),   F(u8_16),  F(u8_32),
  notEq,    notEq,     notEq,      notEq,      notEq,      F(u8_16), F(8_8),    F(u16_32),
  notEq,    notEq,     notEq,      notEq,      notEq,      F(u8_32), F(u16_32), F(8_8),
};
#undef F



FORCE_INLINE bool equalTyped(B w, B x, u8 we, u8 xe, usz ia) {
  usz idx = EQFN_INDEX(we, xe);
  return eqFns[idx](tyany_ptr(w), tyany_ptr(x), ia, eqFnData[idx]);
}

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
static NOINLINE bool eequalFloat(f64* wp, f64* xp, usz ia) {
  u64 r = 1;
  for (usz i = 0; i < ia; i++) {
    #if NEEQUAL_NEGZERO
    r&= ((u64*)wp)[i] == ((u64*)xp)[i];
    #else
    r&= (wp[i]==xp[i]) | (wp[i]!=wp[i] & xp[i]!=xp[i]);
    #endif
  }
  return r;
}



#define EQ_START(F)              \
  if (isAtm(w)) {                \
    if (!isAtm(x)) return false; \
    return F(w, x);              \
  }                              \
  if (isAtm(x)) return false;    \
  ur wr = RNK(w);                \
  if (wr!=RNK(x)) return false;  \
  usz ia = IA(x);                \
  if (LIKELY(wr==1)) { if (ia != IA(w)) return false; } \
  else if (!eqShPart(SH(w), SH(x), wr)) return false;   \
  if (ia==0) return true;

NOINLINE bool equal(B w, B x) { // doesn't consume
  EQ_START(atomEqual);
  
  u8 we = TI(w,elType);
  u8 xe = TI(x,elType);
  if (we!=el_B && xe!=el_B) return equalTyped(w, x, we, xe, ia);
  
  return equalSlow(w, x, ia);
}

bool eequal(B w, B x) { // doesn't consume
  if (w.u==x.u) return true;
  EQ_START(atomEEqual);
  
  u8 we = TI(w,elType);
  u8 xe = TI(x,elType);
  if (we==el_f64 && xe==el_f64) return eequalFloat(f64any_ptr(w), f64any_ptr(x), ia);
  if (RARE(we==el_B || xe==el_B)) return eequalSlow(w, x, ia);
  return equalTyped(w, x, we, xe, ia);
}