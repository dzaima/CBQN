#include "../core.h"
#include "../utils/each.h"

#define P2(N) { if (isArr(w)|isArr(x)) { \
    SLOWIF((!isArr(w) || TI(w,elType)!=el_B)  &&  (!isArr(x) || TI(x,elType)!=el_B)) SLOW2("cmp " #N, w, x); \
    return arith_recd(N##_c2, w, x); \
}}

#if SINGELI
#include "../singeli/c/cmp.c"
#else
#define AL(X) u64* rp; B r = m_bitarrc(&rp, X); usz ria=IA(r); usz bia = BIT_N(ria);
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
        switch(we) { default: UD;                  \
          case el_bit: { u64* wp=bitarr_ptr(w); u64* xp=bitarr_ptr(x); for(usz i=0;i<bia;i++) { u64 wv=wp[i]; u64 xv=xp[i]; rp[i]=BX; } break; } \
          case el_i8 : { i8*  wp=i8any_ptr (w); i8*  xp=i8any_ptr (x); for(usz i=0;i<ria;i++) bitp_set(rp,i,wp[i] OP xp[i]); break; } \
          case el_i16: { i16* wp=i16any_ptr(w); i16* xp=i16any_ptr(x); for(usz i=0;i<ria;i++) bitp_set(rp,i,wp[i] OP xp[i]); break; } \
          case el_i32: { i32* wp=i32any_ptr(w); i32* xp=i32any_ptr(x); for(usz i=0;i<ria;i++) bitp_set(rp,i,wp[i] OP xp[i]); break; } \
          case el_c8 : { u8*  wp=c8any_ptr (w); u8*  xp=c8any_ptr (x); for(usz i=0;i<ria;i++) bitp_set(rp,i,wp[i] OP xp[i]); break; } \
          case el_c16: { u16* wp=c16any_ptr(w); u16* xp=c16any_ptr(x); for(usz i=0;i<ria;i++) bitp_set(rp,i,wp[i] OP xp[i]); break; } \
          case el_c32: { u32* wp=c32any_ptr(w); u32* xp=c32any_ptr(x); for(usz i=0;i<ria;i++) bitp_set(rp,i,wp[i] OP xp[i]); break; } \
          case el_f64: { f64* wp=f64any_ptr(w); f64* xp=f64any_ptr(x); for(usz i=0;i<ria;i++) bitp_set(rp,i,wp[i] OP xp[i]); break; } \
        }                          \
        decG(w);decG(x); return r; \
      }                            \
    } else { AL(w)                 \
      switch(we) { default: UD;    \
        case el_bit: { if (!q_bit(x)) break; u64 xv=bitx(x); u64* wp=bitarr_ptr(w); for(usz i=0;i<bia;i++) { u64 wv=wp[i]; rp[i]=BX; } decG(w); return r; } \
        case el_i8:  { if (!q_i8 (x)) break; i8  xv=o2iu(x); i8*  wp=i8any_ptr (w); for(usz i=0;i<ria;i++) bitp_set(rp,i,wp[i] OP xv); decG(w); return r; } \
        case el_i16: { if (!q_i16(x)) break; i16 xv=o2iu(x); i16* wp=i16any_ptr(w); for(usz i=0;i<ria;i++) bitp_set(rp,i,wp[i] OP xv); decG(w); return r; } \
        case el_i32: { if (!q_i32(x)) break; i32 xv=o2iu(x); i32* wp=i32any_ptr(w); for(usz i=0;i<ria;i++) bitp_set(rp,i,wp[i] OP xv); decG(w); return r; } \
        case el_c8:  { if (!q_c8 (x)) break; u8  xv=o2cu(x); u8*  wp=c8any_ptr (w); for(usz i=0;i<ria;i++) bitp_set(rp,i,wp[i] OP xv); decG(w); return r; } \
        case el_c16: { if (!q_c16(x)) break; u16 xv=o2cu(x); u16* wp=c16any_ptr(w); for(usz i=0;i<ria;i++) bitp_set(rp,i,wp[i] OP xv); decG(w); return r; } \
        case el_c32: { if (!q_c32(x)) break; u32 xv=o2cu(x); u32* wp=c32any_ptr(w); for(usz i=0;i<ria;i++) bitp_set(rp,i,wp[i] OP xv); decG(w); return r; } \
        case el_f64: { if (!q_f64(x)) break; f64 xv=o2fu(x); f64* wp=f64any_ptr(w); for(usz i=0;i<ria;i++) bitp_set(rp,i,wp[i] OP xv); decG(w); return r; } \
      }                         \
      decG(r);                  \
    }                           \
  } else if (isArr(x)) { u8 xe = TI(x,elType); if (xe==el_B) goto end; AL(x) \
      switch(xe) { default: UD; \
        case el_bit: { if (!q_bit(w)) break; u64 wv=bitx(w); u64* xp=bitarr_ptr(x); for(usz i=0;i<bia;i++) { u64 xv=xp[i]; rp[i]=BX; } decG(x); return r; } \
        case el_i8:  { if (!q_i8 (w)) break; i8  wv=o2iu(w); i8*  xp=i8any_ptr (x); for(usz i=0;i<ria;i++) bitp_set(rp,i,wv OP xp[i]); decG(x); return r; } \
        case el_i16: { if (!q_i16(w)) break; i16 wv=o2iu(w); i16* xp=i16any_ptr(x); for(usz i=0;i<ria;i++) bitp_set(rp,i,wv OP xp[i]); decG(x); return r; } \
        case el_i32: { if (!q_i32(w)) break; i32 wv=o2iu(w); i32* xp=i32any_ptr(x); for(usz i=0;i<ria;i++) bitp_set(rp,i,wv OP xp[i]); decG(x); return r; } \
        case el_c8:  { if (!q_c8 (w)) break; u8  wv=o2cu(w); u8*  xp=c8any_ptr (x); for(usz i=0;i<ria;i++) bitp_set(rp,i,wv OP xp[i]); decG(x); return r; } \
        case el_c16: { if (!q_c16(w)) break; u16 wv=o2cu(w); u16* xp=c16any_ptr(x); for(usz i=0;i<ria;i++) bitp_set(rp,i,wv OP xp[i]); decG(x); return r; } \
        case el_c32: { if (!q_c32(w)) break; u32 wv=o2cu(w); u32* xp=c32any_ptr(x); for(usz i=0;i<ria;i++) bitp_set(rp,i,wv OP xp[i]); decG(x); return r; } \
        case el_f64: { if (!q_f64(w)) break; f64 wv=o2fu(w); f64* xp=f64any_ptr(x); for(usz i=0;i<ria;i++) bitp_set(rp,i,wv OP xp[i]); decG(x); return r; } \
      }        \
      decG(r); \
    }          \
  end:;
#endif


static NOINLINE u8 aMakeEq(B* w, B* x, u8 we, u8 xe) { // returns el_MAX if failed
  B* p = we<xe?w:x;
  B s = *p;
  u8 me = we>xe?we:xe;
  if (elNum(we) & elNum(xe)) {
    switch(me) { default: UD;
      case el_i8:  s = taga(cpyI8Arr (s)); break;
      case el_i16: s = taga(cpyI16Arr(s)); break;
      case el_i32: s = taga(cpyI32Arr(s)); break;
      case el_f64: s = taga(cpyF64Arr(s)); break;
    }
  } else if (elChr(we) & elChr(xe)) {
    switch(me) { default: UD;
      case el_c16: s = taga(cpyC16Arr(s)); break;
      case el_c32: s = taga(cpyC32Arr(s)); break;
    }
  } else return el_MAX;
  *p = s;
  return me;
}

#define CMP(CHR,NAME,RNAME,PNAME,L,R,OP,FC,CF,BX) \
B NAME##_c2(B t, B w, B x) {            \
  CMP_IMPL(CHR,NAME,RNAME,PNAME,L,R,OP,FC,CF,BX); \
  P2(NAME);                             \
  return m_i32(compare(w, x) OP 0);     \
}
CMP("≤", le,ge, ge, x,w, <=, 1, 0, ~wv |  xv)
CMP("≥", ge,le, ge, w,x, >=, 0, 1,  wv | ~xv)
CMP("<", lt,gt, gt, x,w, < , 1, 0, ~wv &  xv)
CMP(">", gt,lt, gt, w,x, > , 0, 1,  wv & ~xv)
#undef CMP

B eq_c2(B t, B w, B x) {
  CMP_IMPL("=", eq, eq, eq, w,x, ==, 0, 0, ~wv^xv);
  P2(eq);
  B r = m_i32(atomEqual(w, x));
  dec(w); dec(x);
  return r;
}
B ne_c2(B t, B w, B x) {
  CMP_IMPL("≠", ne, ne, ne, w,x, !=, 1, 1, wv^xv);
  P2(ne);
  B r = m_i32(!atomEqual(w, x));
  dec(w); dec(x);
  return r;
}


B gt_c1(B t, B x) {
  if (isAtm(x)) return x;
  return bqn_merge(x);
}
