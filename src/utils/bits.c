#include "../core.h"
#include "mut.h"


#if SINGELI
  #define SINGELI_FILE bits
  #include "../utils/includeSingeli.h"
  #define bitselFns simd_bitsel
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
        case el_bit: if (f1==0||f1==1) goto t_bit;
        case el_i8:  if (q_fi8(f1)) goto t_i8; if (q_fi16(f1)) goto t_i16; if (q_fi32(f1)) goto t_i32; goto t_f64; // not using fallthrough to allow deduplicating float→int conversion
        case el_i16:                           if (q_fi16(f1)) goto t_i16; if (q_fi32(f1)) goto t_i32; goto t_f64;
        case el_i32:                                                       if (q_fi32(f1)) goto t_i32; goto t_f64;
        case el_f64: goto t_f64;
      }
      t_bit:
        if (f0) {
          if (f1) { Arr* a = allOnes(ia); arr_shCopy(a, b); r = taga(a); goto dec_ret; }
          else return bit_negate(b);
        } else {
          if (f1) return b;
          else { Arr* a = allZeroes(ia); arr_shCopy(a, b); r = taga(a); goto dec_ret; }
        }
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
    #if SINGELI
      bitselFns[width](rp, bp, e0i, e1i, ia);
    #else
      switch(width) {
        case 0: for (usz i=0; i<ia; i++) (( u8*)rp)[i] = bitp_get(bp,i)? e1i : e0i; break;
        case 1: for (usz i=0; i<ia; i++) ((u16*)rp)[i] = bitp_get(bp,i)? e1i : e0i; break;
        case 2: for (usz i=0; i<ia; i++) ((u32*)rp)[i] = bitp_get(bp,i)? e1i : e0i; break;
        case 3: for (usz i=0; i<ia; i++) ((u64*)rp)[i] = bitp_get(bp,i)? e1i : e0i; break;
      }
    #endif
    goto dec_ret;
  }
  
  slow:;
  HArr_p ra = m_harrUc(b);
  SLOW3("bit_sel", e0, e1, b);
  for (usz i = 0; i < ia; i++) ra.a[i] = bitp_get(bp,i)? e1 : e0;
  
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


FORCE_INLINE u64 ruu64(void* p) { u64 r;  memcpy(&r, p, 8); return r; } // read byte-unaligned u64
FORCE_INLINE void wuu64(void* p, u64 v) { memcpy(p, &v, 8); } // write byte-unaligned u64

static NOINLINE B zeroPadToCellBits0(B x, usz lr, usz cam, usz pcsz, usz ncsz) { // consumes; for now assumes ncsz is either a multiple of 64, or one of 8,16,32
  assert((ncsz&7) == 0 && RNK(x)>=1 && pcsz<ncsz);
  // printf("zeroPadToCellBits0 ia=%d cam=%d pcsz=%d ncsz=%d\n", IA(x), cam, pcsz, ncsz);
  if (pcsz==1) {
    if (ncsz== 8) return taga(cpyI8Arr(x));
    if (ncsz==16) return taga(cpyI16Arr(x));
    if (ncsz==32) return taga(cpyI32Arr(x));
  }
  
  u64* rp;
  Arr* r = m_bitarrp(&rp, cam*ncsz);
  usz* rsh = arr_shAlloc(r, lr+1);
  shcpy(rsh, SH(x), lr);
  rsh[lr] = ncsz;
  u64* xp = tyany_ptr(x);
  
  // TODO widen 8/16-bit cells to 16/32 via cpyC(16|32)Arr
  if (ncsz<=64 && (ncsz&(ncsz-1)) == 0) {
    u64 msk = (1ull<<pcsz)-1;
    switch(ncsz) { default: UD;
      case  8: for (ux i=0; i<cam; i++) ((u8* )rp)[i] = rbuu64(xp, i*pcsz)&msk; break;
      case 16: for (ux i=0; i<cam; i++) ((u16*)rp)[i] = rbuu64(xp, i*pcsz)&msk; break;
      case 32: for (ux i=0; i<cam; i++) ((u32*)rp)[i] = rbuu64(xp, i*pcsz)&msk; break;
      case 64: for (ux i=0; i<cam; i++) ((u64*)rp)[i] = rbuu64(xp, i*pcsz)&msk; break;
    }
  } else {
    assert((ncsz&63) == 0 && ncsz-pcsz < 64 && (pcsz&63) != 0);
    ux pfu64 = pcsz>>6; // previous full u64 count in cell
    u64 msk = (1ull<<(pcsz&63))-1;
    for (ux i = 0; i < cam; i++) {
      for (ux j = 0; j < pfu64; j++) rp[j] = rbuu64(xp, i*pcsz + j*64);
      rp[pfu64] = rbuu64(xp, i*pcsz + pfu64*64) & msk;
      rp+= ncsz>>6;
    }
  }
  decG(x);
  return taga(r);
}
NOINLINE B widenBitArr(B x, ur axis) {
  assert(isArr(x) && TI(x,elType)==el_bit && axis>=1 && RNK(x)>=axis);
  usz pcsz = shProd(SH(x), axis, RNK(x));
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

#if defined(__BMI2__) && !SLOW_PDEP
  #define FAST_PDEP 1
#endif

B narrowWidenedBitArr(B x, ur axis, ur cr, usz* csh) { // for now assumes the bits to be dropped are zero, origCellBits is a multiple of 8, and that there's at most 63 padding bits
  if (TI(x,elType)!=el_bit) return taga(cpyBitArr(x));
  
  usz xcsz = shProd(SH(x), axis, RNK(x));
  usz ocsz = shProd(csh, 0, cr);
  // printf("narrowWidenedBitArr ia=%d axis=%d cr=%d ocsz=%d xcsz=%d\n", IA(x), axis, cr, ocsz, xcsz);
  assert((xcsz&7) == 0 && ocsz<xcsz);
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
        // printf("base %04lx %016lx count=%d am=%zu\n", tmsk, msk0, count, am);
        for (ux i=0; i<am; i++) { ab_add(&ab, _pext_u64(*(u64*)xp, msk0), count); xp+= 8; }
        u32 tb = c8? cam&7 : (cam&3)<<1;
        if (tb) {
          u64 msk1 = msk0 & ((1ull<<tb*8)-1);
          // printf("tail %4d %016lx count=%d\n", tb, msk1, POPC(msk1));
          ab_add(&ab, _pext_u64(*(u64*)xp, msk1), POPC(msk1));
        }
      }
      else if (xcsz==32) for (ux i=0; i<cam; i++) ab_add(&ab, ((u32*)xp)[i], ocsz);
      else               for (ux i=0; i<cam; i++) ab_add(&ab, ((u64*)xp)[i], ocsz);
    #else
      switch(xcsz) { default: UD;
        case  8: for (ux i=0; i<cam; i++) ab_add(&ab, ((u8* )xp)[i], ocsz); break; // all assume zero padding
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
      for (ux j = 0; j < rfu64; j++) ab_add(&ab, ruu64(j + (u64*)xp), 64);
      ab_add(&ab, ruu64(rfu64 + (u64*)xp)&msk, ocsz&63);
      rp+= ocsz>>6;
      xp+= xcsz>>3;
    }
  }
  ab_done(ab);
  decG(x);
  return taga(r);
}
