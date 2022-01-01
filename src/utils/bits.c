#include "../core.h"

NOINLINE Arr* allZeroes(usz ia) { u64* rp; Arr* r = m_bitarrp(&rp, ia); for (usz i = 0; i < BIT_N(ia); i++) rp[i] =  0;    return r; }
NOINLINE Arr* allOnes  (usz ia) { u64* rp; Arr* r = m_bitarrp(&rp, ia); for (usz i = 0; i < BIT_N(ia); i++) rp[i] = ~0ULL; return r; }

NOINLINE B bit_negate(B x) { // consumes
  u64* xp = bitarr_ptr(x);
  u64* rp; B r = m_bitarrc(&rp, x);
  usz ia = BIT_N(a(x)->ia);
  for (usz i = 0; i < ia; i++) rp[i] = ~xp[i];
  dec(x);
  return r;
}

NOINLINE B bit_sel(B b, B e0, bool h0, B e1, bool h1) {
  u8 t0 = selfElType(e0);
  u8 t1 = selfElType(e1);
  if (!h0) t0=t1; // TODO just do separate impls for !h0 and !h1
  if (!h1) t1=t0;
  u64* bp = bitarr_ptr(b);
  usz ia = a(b)->ia;
  if (elNum(t0) && elNum(t1)) { B r;
    f64 f0 = o2fu(e0); i32 i0 = f0;
    f64 f1 = o2fu(e1); i32 i1 = f1;
    u8 tM = t0>t1? t0 : t1;
    if (tM==el_bit) {
      if (i0) {
        if (i1) { Arr* a = allOnes(ia); arr_shCopy(a, b); r = taga(a); }
        else return bit_negate(b);
      } else {
        if (i1) return b;
        else { Arr* a = allZeroes(ia); arr_shCopy(a, b); r = taga(a); }
      }
    }
    else if (tM==el_i8 ) { i8*  rp; r=m_i8arrc (&rp, b); for (usz i = 0; i < ia; i++) rp[i] = bitp_get(bp,i)? i1 : i0; }
    else if (tM==el_i16) { i16* rp; r=m_i16arrc(&rp, b); for (usz i = 0; i < ia; i++) rp[i] = bitp_get(bp,i)? i1 : i0; }
    else if (tM==el_i32) { i32* rp; r=m_i32arrc(&rp, b); for (usz i = 0; i < ia; i++) rp[i] = bitp_get(bp,i)? i1 : i0; }
    else                              { f64* rp; r=m_f64arrc(&rp, b); for (usz i = 0; i < ia; i++) rp[i] = bitp_get(bp,i)? f1 : f0; }
    dec(b); return r;
  } else if (elChr(t0) && elChr(t1)) { B r; u32 u0 = o2cu(e0); u32 u1 = o2cu(e1);
    if      (t0<=el_c8  & t1<=el_c8 ) { u8*  rp; r=m_c8arrc (&rp, b); for (usz i = 0; i < ia; i++) rp[i] = bitp_get(bp,i)? u1 : u0; }
    else if (t0<=el_c16 & t1<=el_c16) { u16* rp; r=m_c16arrc(&rp, b); for (usz i = 0; i < ia; i++) rp[i] = bitp_get(bp,i)? u1 : u0; }
    else                              { u32* rp; r=m_c32arrc(&rp, b); for (usz i = 0; i < ia; i++) rp[i] = bitp_get(bp,i)? u1 : u0; }
    dec(b); return r;
  }
  HArr_p r = m_harrUc(b);
  SLOW3("bit_sel", e0, e1, b);
  for (usz i = 0; i < ia; i++) r.a[i] = bitp_get(bp,i)? e1 : e0;
  
  u64 c1 = bit_sum(bp, ia);
  u64 c0 = ia-c1;
  incBy(e0,c0);
  incBy(e1,c1);
  dec(b); return r.b;
}
