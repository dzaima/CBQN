#include "../core.h"
#include "../utils/calls.h"
#include "mut.h"


#if SINGELI_SIMD
  #define SINGELI_FILE bits
  #include "../utils/includeSingeli.h"
  INIT_GLOBAL BitSelFn* bitselFns = simd_bitsel;
#else
  #define BITSEL_DEF(E) void bitsel_##E(void* rp, u64* bp, u64 e0i, u64 e1i, u64 ia) { for (usz i=0; i<ia; i++) ((E*)rp)[i] = bitp_get(bp,i)? e1i : e0i; }
  BITSEL_DEF(u8)
  BITSEL_DEF(u16)
  BITSEL_DEF(u32)
  BITSEL_DEF(u64)
  INIT_GLOBAL BitSelFn bitselFnsRaw[] = {bitsel_u8, bitsel_u16, bitsel_u32, bitsel_u64};
  INIT_GLOBAL BitSelFn* bitselFns = bitselFnsRaw;
#endif

#if defined(__BMI2__) && !SLOW_PDEP
  #define FAST_PDEP 1
  #include <immintrin.h>
#endif

NOINLINE Arr* allZeroes(usz ia) { u64* rp; Arr* r = m_bitarrp(&rp, ia); for (usz i = 0; i < BIT_N(ia); i++) rp[i] =  0;    return r; }
NOINLINE Arr* allOnes  (usz ia) { u64* rp; Arr* r = m_bitarrp(&rp, ia); for (usz i = 0; i < BIT_N(ia); i++) rp[i] = ~0ULL; return r; }

NOINLINE B bit_sel(B b, B e0, B e1) {
  u8 t0 = selfElType(e0);
  u64* bp = bitarr_ptr(b);
  usz ia = IA(b);
  B r;
  {
    u8 type, width;
    u64 e0i, e1i;
    if (elNum(t0) && isF64(e1)) {
      f64 f0 = o2fG(e0);
      f64 f1 = o2fG(e1);
      switch (t0) { default: UD;
        case el_bit: if (q_fbit(f1)) goto t_bit;
        case el_i8:  if (q_fi8(f1)) goto t_i8; if (q_fi16(f1)) goto t_i16; if (q_fi32(f1)) goto t_i32; goto t_f64; // not using fallthrough to allow deduplicating float→int conversion
        case el_i16:                           if (q_fi16(f1)) goto t_i16; if (q_fi32(f1)) goto t_i32; goto t_f64;
        case el_i32:                                                       if (q_fi32(f1)) goto t_i32; goto t_f64;
        case el_f64: goto t_f64;
      }
      t_bit:
        if (f0) return f1? i64EachDec(1, b) : bit_negate(b);
        else    return f1? b : i64EachDec(0, b);
      t_i8:  type=t_i8arr;  width=0; e0i=( u8)( i8)f0; e1i=( u8)( i8)f1; goto sel;
      t_i16: type=t_i16arr; width=1; e0i=(u16)(i16)f0; e1i=(u16)(i16)f1; goto sel;
      t_i32: type=t_i32arr; width=2; e0i=(u32)(i32)f0; e1i=(u32)(i32)f1; goto sel;
      t_f64: type=t_f64arr; width=3; e0i=     b(f0).u; e1i=     b(f1).u; goto sel;
      
    } else if (elChr(t0) && isC32(e1)) {
      u32 u0 = o2cG(e0); u32 u1 = o2cG(e1);
      switch(t0) { default: UD;
        case el_c8:  if (u1==( u8)u1) { type=t_c8arr;  width=0; e0i=u0; e1i=u1; goto sel; } // else fallthrough
        case el_c16: if (u1==(u16)u1) { type=t_c16arr; width=1; e0i=u0; e1i=u1; goto sel; } // else fallthrough
        case el_c32:                  { type=t_c32arr; width=2; e0i=u0; e1i=u1; goto sel; }
      }
    } else goto slow;
    
    sel:;
    void* rp = m_tyarrlc(&r, width, b, type);
    bitselFns[width](rp, bp, e0i, e1i, ia);
    goto dec_ret;
  }
  
  slow:;
  HArr_p ra = m_harrUc(b);
  SLOW3("bit_sel", e0, e1, b);
  for (usz i = 0; i < ia; i++) ra.a[i] = bitp_get(bp,i)? e1 : e0;
  NOGC_E;
  
  u64 c1 = bit_sum(bp, ia);
  u64 c0 = ia-c1;
  incBy(e0,c0);
  incBy(e1,c1);
  r = ra.b;
  
  dec_ret:
  decG(b); return r;
}


static inline u64 rbuu64(u64* p, ux off) { // read bit-unaligned u64; aka 64↑off↓p
  ux p0 = off>>6;
  ux m0 = off&63;
  u64 v0 = p[p0];
  u64 v1 = p[p0+1];
  #if HAS_U128
    u128 v = v0 | ((u128)v1)<<64;
    return v>>m0;
  #else
    return m0==0? v0 : v0>>m0 | v1<<(64-m0);
  #endif
}

typedef struct {
  u64* ptr;
  u64 tmp;
  ux off;
} ABState;
static ABState ab_new(u64* p) { return (ABState){.ptr=p, .tmp=0, .off=0}; }
static void ab_done(ABState s) { if (s.off) *s.ptr = s.tmp; }
FORCE_INLINE void ab_add(ABState* state, u64 val, ux count) { // assumes bits past count are 0
  assert(count==64 || (val>>count)==0);
  ux off0 = state->off;
  ux off1 = off0 + count;
  state->off = off1&63;
  assert((state->tmp>>off0) == 0);
  state->tmp|= val<<off0;
  if (off1>=64) {
    *state->ptr++ = state->tmp;
    state->tmp = off0==0? 0 : val>>(64-off0);
  }
}



NOINLINE void bitwiden(void* rp, ux rcsz, void* xp, ux xcsz, ux cam) { // for now assumes rcsz is either a multiple of 64, or one of 8,16,32
  assert(rcsz > xcsz && ((rcsz&63) == 0 || rcsz==8 || rcsz==16 || rcsz==32));
  
  if (rcsz<=32 && (xcsz&(xcsz-1)) == 0) {
    if (xcsz==1) {
      assert(rcsz==8 || rcsz==16 || rcsz==32);
      COPY_TO_FROM(rp, rcsz==16? el_i16 : rcsz>16? el_i32 : el_i8, xp, el_bit, cam);
      return;
    }
    if (xcsz==8) {
      assert(rcsz==16 || rcsz==32);
      COPY_TO_FROM(rp, rcsz==16? el_c16 : el_c32, xp, el_c8, cam);
      return;
    }
    if (xcsz==16) {
      assert(rcsz==32);
      COPY_TO_FROM(rp, el_c32, xp, el_c16, cam);
      return;
    }
  }
  
  u64* rp64 = rp;
  if (rcsz<=64) {
    assert((rcsz&(rcsz-1)) == 0);
    #if SINGELI_AARCH64 || SINGELI_AVX2
      if (rcsz==8) {
        si_bitwiden_n_8(xp, rp, xcsz, cam);
        return;
      }
    #endif
    u64 tmsk = (1ull<<xcsz)-1;
    #if FAST_PDEP
      if (rcsz<32) {
        assert(rcsz==8 || rcsz==16);
        bool c8 = rcsz==8;
        u64 msk0 = tmsk * (c8? 0x0101010101010101 : 0x0001000100010001);
        ux am = c8? cam/8 : cam/4;
        u32 count = POPC(msk0);
        // printf("widen base %04lx %016lx count=%d am=%zu\n", tmsk, msk0, count, am);
        for (ux i=0; i<am; i++) { *rp64 = _pdep_u64(rbuu64(xp, i*count), msk0); rp64++; }
        u32 tb = c8? cam&7 : (cam&3)<<1;
        if (tb) {
          u64 msk1 = msk0 & ((1ull<<tb*8)-1);
          // printf("widen tail %4d %016lx count=%d\n", tb, msk1, POPC(msk1));
          *rp64 = _pdep_u64(rbuu64(xp, am*count), msk1);
        }
      }
      else if (rcsz==32) for (ux i=0; i<cam; i++) ((u32*)rp)[i] = rbuu64(xp, i*xcsz)&tmsk;
      else               for (ux i=0; i<cam; i++) ((u64*)rp)[i] = rbuu64(xp, i*xcsz)&tmsk;
    #else
      switch(rcsz) { default: UD;
        case  8: for (ux i=0; i<cam; i++) ((u8* )rp)[i] = rbuu64(xp, i*xcsz)&tmsk; break;
        case 16: for (ux i=0; i<cam; i++) ((u16*)rp)[i] = rbuu64(xp, i*xcsz)&tmsk; break;
        case 32: for (ux i=0; i<cam; i++) ((u32*)rp)[i] = rbuu64(xp, i*xcsz)&tmsk; break;
        case 64: for (ux i=0; i<cam; i++) ((u64*)rp)[i] = rbuu64(xp, i*xcsz)&tmsk; break;
      }
    #endif
  } else {
    assert((rcsz&63) == 0 && rcsz-xcsz < 64 && (xcsz&63) != 0);
    ux pfu64 = xcsz>>6; // previous full u64 count in cell
    u64 msk = (1ull<<(xcsz&63))-1;
    for (ux i = 0; i < cam; i++) {
      for (ux j = 0; j < pfu64; j++) rp64[j] = rbuu64(xp, i*xcsz + j*64);
      rp64[pfu64] = rbuu64(xp, i*xcsz + pfu64*64) & msk;
      rp64+= rcsz>>6;
    }
  }
}



static NOINLINE B zeroPadToCellBits0(B x, usz lr, usz cam, usz pcsz, usz ncsz) { // consumes; for now assumes ncsz is either a multiple of 64, or one of 8,16,32
  assert((ncsz&7) == 0 && RNK(x)>=1 && pcsz<ncsz);
  // printf("zeroPadToCellBits0 ia=%d cam=%d pcsz=%d ncsz=%d\n", IA(x), cam, pcsz, ncsz);
  
  if (lr==UR_MAX) thrM("Rank too large");
  u64* rp;
  ux ria = cam*ncsz;
  Arr* r = m_bitarrp(&rp, ria);
  usz* rsh = arr_shAlloc(r, lr+1);
  shcpy(rsh, SH(x), lr);
  rsh[lr] = ncsz;
  if (ria > 0) bitwiden(rp, ncsz, tyany_ptr(x), pcsz, cam);
  decG(x);
  return taga(r);
}
NOINLINE B widenBitArr(B x, ur axis) {
  assert(isArr(x) && TI(x,elType)!=el_B && axis>=1 && RNK(x)>=axis);
  usz pcsz = shProd(SH(x), axis, RNK(x))<<elwBitLog(TI(x,elType));
  assert(pcsz!=0);
  usz ncsz;
  if (pcsz<=8) ncsz = 8;
  else if (pcsz<=16) ncsz = 16;
  else if (pcsz<=32) ncsz = 32;
  // else if (pcsz<=64) ncsz = 64;
  // else ncsz = (pcsz+7)&~(usz)7;
  else ncsz = (pcsz+63)&~(usz)63;
  if (ncsz==pcsz) return x;
  
  return zeroPadToCellBits0(x, axis, shProd(SH(x), 0, axis), pcsz, ncsz);
}

B narrowWidenedBitArr(B x, ur axis, ur cr, usz* csh) { // for now assumes the bits to be dropped are zero, origCellBits is a multiple of 8, and that there's at most 63 padding bits
  if (TI(x,elType)!=el_bit) return taga(cpyBitArr(x));
  if (axis+cr>UR_MAX) thrM("Rank too large");
  
  usz xcsz = shProd(SH(x), axis, RNK(x));
  usz ocsz = shProd(csh, 0, cr);
  // printf("narrowWidenedBitArr ia=%d axis=%d cr=%d ocsz=%d xcsz=%d\n", IA(x), axis, cr, ocsz, xcsz);
  assert((xcsz&7) == 0 && ocsz<xcsz && ocsz!=0);
  if (xcsz==ocsz) {
    if (RNK(x)-axis == cr && eqShPart(SH(x)+axis, csh, cr)) return x;
    Arr* r = cpyWithShape(x);
    ShArr* rsh = m_shArr(axis+cr);
    shcpy(rsh->a, SH(x), axis);
    shcpy(rsh->a+axis, csh, cr);
    arr_shReplace(r, axis+cr, rsh);
    return taga(r);
  }
  
  
  usz cam = shProd(SH(x), 0, axis);
  u64* rp;
  Arr* r = m_bitarrp(&rp, cam*ocsz);
  usz* rsh = arr_shAlloc(r, axis+cr);
  shcpy(rsh, SH(x), axis);
  shcpy(rsh+axis, csh, cr);
  if (PIA(r)==0) goto decG_ret;
  
  u8* xp = tyany_ptr(x);
  // FILL_TO(rp, el_bit, 0, m_f64(1), PIA(r));
  ABState ab = ab_new(rp);
  if (xcsz<=64 && (xcsz&(xcsz-1)) == 0) {
    #if FAST_PDEP
      if (xcsz<32) {
        assert(xcsz==8 || xcsz==16);
        bool c8 = xcsz==8;
        u64 tmsk = (1ull<<ocsz)-1;
        u64 msk0 = tmsk * (c8? 0x0101010101010101 : 0x0001000100010001);
        ux am = c8? cam/8 : cam/4;
        u32 count = POPC(msk0);
        // printf("narrow base %04lx %016lx count=%d am=%zu\n", tmsk, msk0, count, am);
        for (ux i=0; i<am; i++) { ab_add(&ab, _pext_u64(*(u64*)xp, msk0), count); xp+= 8; }
        u32 tb = c8? cam&7 : (cam&3)<<1;
        if (tb) {
          u64 msk1 = msk0 & ((1ull<<tb*8)-1);
          // printf("narrow tail %4d %016lx count=%d\n", tb, msk1, POPC(msk1));
          ab_add(&ab, _pext_u64(*(u64*)xp, msk1), POPC(msk1));
        }
      }
      else if (xcsz==32) for (ux i=0; i<cam; i++) ab_add(&ab, ((u32*)xp)[i], ocsz);
      else               for (ux i=0; i<cam; i++) ab_add(&ab, ((u64*)xp)[i], ocsz);
    #else
      switch(xcsz) { default: UD;
        case 8:
          #if SINGELI_NEON
            if (xcsz==8 && ocsz!=1) {
              si_bitnarrow_8_n(xp, rp, ocsz, cam);
              goto decG_ret;
            }
          #endif
        for          (ux i=0; i<cam; i++) ab_add(&ab, ((u8* )xp)[i], ocsz); break; // all assume zero padding
        case 16: for (ux i=0; i<cam; i++) ab_add(&ab, ((u16*)xp)[i], ocsz); break;
        case 32: for (ux i=0; i<cam; i++) ab_add(&ab, ((u32*)xp)[i], ocsz); break;
        case 64: for (ux i=0; i<cam; i++) ab_add(&ab, ((u64*)xp)[i], ocsz); break;
      }
    #endif
  } else {
    assert(xcsz-ocsz<64);
    ux rfu64 = ocsz>>6; // full u64 count per cell in x
    u64 msk = (1ull<<(ocsz&63))-1;
    for (ux i = 0; i < cam; i++) {
      for (ux j = 0; j < rfu64; j++) ab_add(&ab, loadu_u64(j + (u64*)xp), 64);
      ab_add(&ab, loadu_u64(rfu64 + (u64*)xp)&msk, ocsz&63);
      rp+= ocsz>>6;
      xp+= xcsz>>3;
    }
  }
  ab_done(ab);
  decG_ret:;
  decG(x);
  return taga(r);
}
