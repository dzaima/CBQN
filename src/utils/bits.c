#include "../core.h"


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