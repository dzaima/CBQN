#include "../../core.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#include "../gen/cmp.c"
#pragma GCC diagnostic pop

#define avx2_eqAA_u8  avx2_eqAA_i8
#define avx2_eqAA_u16 avx2_eqAA_i16
#define avx2_eqAA_u32 avx2_eqAA_i32
#define avx2_neAA_u8  avx2_neAA_i8
#define avx2_neAA_u16 avx2_neAA_i16
#define avx2_neAA_u32 avx2_neAA_i32

#define avx2_gtAA_u32 avx2_gtAA_i32
#define avx2_geAA_u32 avx2_geAA_i32

#define avx2_eqAS_u8( d,w,x,l) avx2_eqAS_i8 (d,(i8 *)(w),x,l)
#define avx2_eqAS_u16(d,w,x,l) avx2_eqAS_i16(d,(i16*)(w),x,l)
#define avx2_eqAS_u32(d,w,x,l) avx2_eqAS_i32(d,(i32*)(w),x,l)
#define avx2_neAS_u8( d,w,x,l) avx2_neAS_i8 (d,(i8 *)(w),x,l)
#define avx2_neAS_u16(d,w,x,l) avx2_neAS_i16(d,(i16*)(w),x,l)
#define avx2_neAS_u32(d,w,x,l) avx2_neAS_i32(d,(i32*)(w),x,l)

#define avx2_ltAA_u1(d,w,x,l) avx2_gtAA_u1(d,x,w,l)
#define avx2_leAA_u1(d,w,x,l) avx2_geAA_u1(d,x,w,l)
#define avx2_ltAA_i8(d,w,x,l) avx2_gtAA_i8(d,x,w,l)
#define avx2_leAA_i8(d,w,x,l) avx2_geAA_i8(d,x,w,l)
#define avx2_ltAA_i16(d,w,x,l) avx2_gtAA_i16(d,x,w,l)
#define avx2_leAA_i16(d,w,x,l) avx2_geAA_i16(d,x,w,l)
#define avx2_ltAA_i32(d,w,x,l) avx2_gtAA_i32(d,x,w,l)
#define avx2_leAA_i32(d,w,x,l) avx2_geAA_i32(d,x,w,l)
#define avx2_leAA_u8( d,w,x,l) avx2_geAA_u8 (d,x,w,l)
#define avx2_leAA_u16(d,w,x,l) avx2_geAA_u16(d,x,w,l)
#define avx2_leAA_u32(d,w,x,l) avx2_geAA_u32(d,x,w,l)
#define avx2_ltAA_u8( d,w,x,l) avx2_gtAA_u8 (d,x,w,l)
#define avx2_ltAA_u16(d,w,x,l) avx2_gtAA_u16(d,x,w,l)
#define avx2_ltAA_u32(d,w,x,l) avx2_gtAA_u32(d,x,w,l)
#define avx2_ltAA_f64(d,w,x,l) avx2_gtAA_f64(d,x,w,l)
#define avx2_leAA_f64(d,w,x,l) avx2_geAA_f64(d,x,w,l)

typedef void (*CmpFn)(u64*, void*, void*, u64);
#define CMPFN(A,F,S,T) (CmpFn) A##_##F##S##_##T
#define FN_LUT(A,F,S) static CmpFn lut_##A##_##F##AA[] = {CMPFN(A,F,S,u1), CMPFN(A,F,S,i8), CMPFN(A,F,S,i16), CMPFN(A,F,S,i32), CMPFN(A,F,S,f64), CMPFN(A,F,S,u8), CMPFN(A,F,S,u16), CMPFN(A,F,S,u32)};
FN_LUT(avx2, eq, AA)
FN_LUT(avx2, ne, AA)
FN_LUT(avx2, gt, AA)
FN_LUT(avx2, ge, AA)


static void* tyany_ptr(B x) {
  u8 t = v(x)->type;
  return IS_SLICE(t)? c(TySlice,x)->a : c(TyArr,x)->a;
}

#define AL(X) u64* rp; B r = m_bitarrc(&rp, X); usz ria=a(r)->ia;
#define CMP_IMPL(CHR, NAME, RNAME, PNAME, L, R, OP, FC, CF, BX) \
  if (isF64(w)&isF64(x)) return m_i32(w.f OP x.f); \
  if (isC32(w)&isC32(x)) return m_i32(w.u OP x.u); \
  if (isF64(w)&isC32(x)) return m_i32(FC);         \
  if (isC32(w)&isF64(x)) return m_i32(CF);         \
  if (isArr(w)) { u8 we = TI(w,elType);            \
    if (we==el_B) goto end;                        \
    if (isArr(x)) { u8 xe = TI(x,elType);          \
      if (xe==el_B) goto end;                      \
      if (rnk(w)==rnk(x)) { if (!eqShape(w, x)) thrF(CHR": Expected equal shape prefix (%H ≡ ≢𝕨, %H ≡ ≢𝕩)", w, x); \
        if (we!=xe) { B tw=w,tx=x;                 \
          we = aMakeEq(&tw, &tx, we, xe);          \
          if (we==el_MAX) goto end;                \
          w=tw; x=tx;                              \
        }                                          \
        AL(x)                                      \
        lut_avx2_##PNAME##AA[we](rp, tyany_ptr(L), tyany_ptr(R), ria); \
        dec(w);dec(x); return r;     \
      }                              \
    } else { AL(w)                   \
      switch(we) { default: UD;      \
        case el_bit: if (!q_bit(x))break; avx2_##NAME##AS_u1 (rp, bitarr_ptr(w), o2bu(x), ria); dec(w); return r; \
        case el_i8:  if (!q_i8 (x))break; avx2_##NAME##AS_i8 (rp, i8any_ptr (w), o2iu(x), ria); dec(w); return r; \
        case el_i16: if (!q_i16(x))break; avx2_##NAME##AS_i16(rp, i16any_ptr(w), o2iu(x), ria); dec(w); return r; \
        case el_i32: if (!q_i32(x))break; avx2_##NAME##AS_i32(rp, i32any_ptr(w), o2iu(x), ria); dec(w); return r; \
        case el_f64: if (!q_f64(x))break; avx2_##NAME##AS_f64(rp, f64any_ptr(w), o2fu(x), ria); dec(w); return r; \
        case el_c8:  if (!q_c8 (x))break; avx2_##NAME##AS_u8 (rp, c8any_ptr (w), o2cu(x), ria); dec(w); return r; \
        case el_c16: if (!q_c16(x))break; avx2_##NAME##AS_u16(rp, c16any_ptr(w), o2cu(x), ria); dec(w); return r; \
        case el_c32: if (!q_c32(x))break; avx2_##NAME##AS_i32(rp, (i32*)c32any_ptr(w), o2cu(x), ria); dec(w); return r; \
      }                         \
      dec(r);                   \
    }                           \
  } else if (isArr(x)) { u8 xe = TI(x,elType); if (xe==el_B) goto end; AL(x) \
      switch(xe) { default: UD; \
        case el_bit: if (!q_bit(w))break; avx2_##RNAME##AS_u1 (rp, bitarr_ptr(x), o2bu(w), ria); dec(x); return r; \
        case el_i8:  if (!q_i8 (w))break; avx2_##RNAME##AS_i8 (rp, i8any_ptr (x), o2iu(w), ria); dec(x); return r; \
        case el_i16: if (!q_i16(w))break; avx2_##RNAME##AS_i16(rp, i16any_ptr(x), o2iu(w), ria); dec(x); return r; \
        case el_i32: if (!q_i32(w))break; avx2_##RNAME##AS_i32(rp, i32any_ptr(x), o2iu(w), ria); dec(x); return r; \
        case el_f64: if (!q_f64(w))break; avx2_##RNAME##AS_f64(rp, f64any_ptr(x), o2fu(w), ria); dec(x); return r; \
        case el_c8:  if (!q_c8 (w))break; avx2_##RNAME##AS_u8 (rp, c8any_ptr (x), o2cu(w), ria); dec(x); return r; \
        case el_c16: if (!q_c16(w))break; avx2_##RNAME##AS_u16(rp, c16any_ptr(x), o2cu(w), ria); dec(x); return r; \
        case el_c32: if (!q_c32(w))break; avx2_##RNAME##AS_i32(rp, (i32*)c32any_ptr(x), o2cu(w), ria); dec(x); return r; \
      }       \
      dec(r); \
    }         \
  end:;
