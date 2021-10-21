#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#include "../gen/cmp.c"
#pragma GCC diagnostic pop

#define avx2_eqAA_u8( d,w,x,l) avx2_eqAA_i8 (d,(i8 *)(w),(i8 *)(x),l)
#define avx2_eqAA_u16(d,w,x,l) avx2_eqAA_i16(d,(i16*)(w),(i16*)(x),l)
#define avx2_eqAA_u32(d,w,x,l) avx2_eqAA_i32(d,(i32*)(w),(i32*)(x),l)
#define avx2_neAA_u8( d,w,x,l) avx2_neAA_i8 (d,(i8 *)(w),(i8 *)(x),l)
#define avx2_neAA_u16(d,w,x,l) avx2_neAA_i16(d,(i16*)(w),(i16*)(x),l)
#define avx2_neAA_u32(d,w,x,l) avx2_neAA_i32(d,(i32*)(w),(i32*)(x),l)
#define avx2_eqAS_u8( d,w,x,l) avx2_eqAS_i8 (d,(i8 *)(w),x,l)
#define avx2_eqAS_u16(d,w,x,l) avx2_eqAS_i16(d,(i16*)(w),x,l)
#define avx2_eqAS_u32(d,w,x,l) avx2_eqAS_i32(d,(i32*)(w),x,l)
#define avx2_neAS_u8( d,w,x,l) avx2_neAS_i8 (d,(i8 *)(w),x,l)
#define avx2_neAS_u16(d,w,x,l) avx2_neAS_i16(d,(i16*)(w),x,l)
#define avx2_neAS_u32(d,w,x,l) avx2_neAS_i32(d,(i32*)(w),x,l)

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

#define CMP_IMPL(CHR, NAME, RNAME, OP, FC, CF, BX) \
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
        switch(we) { default: UD;                  \
          case el_bit: { u64* wp=bitarr_ptr(w); u64* xp=bitarr_ptr(x); for(usz i=0;i<bia;i++) { u64 wv=wp[i]; u64 xv=xp[i]; rp[i]=BX; } break; } \
          case el_i8:  avx2_##NAME##AA_i8 (rp, i8any_ptr (w), i8any_ptr (x), ria); break; \
          case el_i16: avx2_##NAME##AA_i16(rp, i16any_ptr(w), i16any_ptr(x), ria); break; \
          case el_i32: avx2_##NAME##AA_i32(rp, i32any_ptr(w), i32any_ptr(x), ria); break; \
          case el_f64: avx2_##NAME##AA_f64(rp, f64any_ptr(w), f64any_ptr(x), ria); break; \
          case el_c8:  avx2_##NAME##AA_u8 (rp, c8any_ptr (w), c8any_ptr (x), ria); break; \
          case el_c16: avx2_##NAME##AA_u16(rp, c16any_ptr(w), c16any_ptr(x), ria); break; \
          case el_c32: avx2_##NAME##AA_i32(rp, (i32*)c32any_ptr(w), (i32*)c32any_ptr(x), ria); break; \
        }                            \
        dec(w);dec(x); return r;     \
      }                              \
    } else { AL(w)                   \
      switch(we) { default: UD;      \
        case el_bit: { if (!q_bit(x)) break; u64 xv=bitx(x); u64* wp=bitarr_ptr(w); for(usz i=0;i<bia;i++) { u64 wv=wp[i]; rp[i]=BX; } dec(w); return r; } \
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
        case el_bit: { if (!q_bit(w)) break; u64 wv=bitx(w); u64* xp=bitarr_ptr(x); for(usz i=0;i<bia;i++) { u64 xv=xp[i]; rp[i]=BX; } dec(x); return r; } \
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
