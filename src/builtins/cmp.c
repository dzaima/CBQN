#include "../core.h"
#include "../utils/each.h"

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

B le_c2(B, B, B); B ge_c2(B, B, B);
B lt_c2(B, B, B); B gt_c2(B, B, B);
B eq_c2(B, B, B); B ne_c2(B, B, B);

#define CMP_REC(NAME, RNAME, PRE) NOINLINE B NAME##_rec(i32 swapped, B w, B x) { PRE \
  SLOWIF((!isArr(w) || TI(w,elType)!=el_B)  &&  (!isArr(x) || TI(x,elType)!=el_B)) SLOW2("recursive cmp " #NAME, w, x); \
  return swapped? arith_recd(RNAME##_c2, x, w) : arith_recd(NAME##_c2, w, x); \
}
CMP_REC(le, ge, )
CMP_REC(lt, gt, )
CMP_REC(eq, eq, swapped=0;)
CMP_REC(ne, ne, swapped=0;)
#define ge_rec(S, W, X) le_rec(!S, X, W)
#define gt_rec(S, W, X) lt_rec(!S, X, W)
#undef CMP_REC

#if SINGELI
#include "../singeli/c/cmp.c"
#else

#define AL(X) u64* rp; B r = m_bitarrc(&rp, X); usz ria=IA(r); usz bia = BIT_N(ria);
#define CMP_AA(CN, CR, NAME, OP, BX, PRE) NOINLINE B NAME##_AA(i32 swapped, B w, B x) { PRE \
  u8 xe = TI(x, elType); if (xe==el_B) goto bad; \
  u8 we = TI(w, elType); if (we==el_B) goto bad; \
  if (RNK(w)==RNK(x)) { if (!eqShape(w, x)) thrF("%U: Expected equal shape prefix (%H ≡ ≢𝕨, %H ≡ ≢𝕩)", swapped?CR:CN, swapped?x:w, swapped?w:x); \
    if (we!=xe) { B tw=w,tx=x;                   \
      we = aMakeEq(&tw, &tx, we, xe);            \
      if (we==el_MAX) goto bad;                  \
      w=tw; x=tx;                                \
    }                                            \
    AL(x)                                        \
    switch(we) { default: UD;                    \
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
  bad: return NAME##_rec(swapped, w, x); \
}
CMP_AA("≤", "≥", le, <=, ~wv |  xv, )
CMP_AA("<", ">", lt, < , ~wv &  xv, )
CMP_AA("=", "?", eq, ==, ~wv^xv, swapped=0;)
CMP_AA("≠", "?", ne, !=,  wv^xv, swapped=0;)
#define ge_AA(T, W, X) le_AA(!T, X, W)
#define gt_AA(T, W, X) lt_AA(!T, X, W)
#undef CMP_AA


#define CMP_SA(NAME, OP, BX, PRE) NOINLINE B NAME##_SA(i32 swapped, B w, B x) { PRE \
  u8 xe = TI(x, elType); if (xe==el_B) goto bad; AL(x) \
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
  bad: return NAME##_rec(swapped, w, x); \
}
CMP_SA(eq, ==, ~wv^xv, swapped=0;)
CMP_SA(ne, !=,  wv^xv, swapped=0;)
CMP_SA(le, <=, ~wv |  xv, )
CMP_SA(ge, >=,  wv | ~xv, )
CMP_SA(lt, < , ~wv &  xv, )
CMP_SA(gt, > ,  wv & ~xv, )
#undef CMP_SA

#endif



#define CMP_TO_ARR(NAME, RNAME)                    \
  if (isArr(x)) {                                  \
    if (isArr(w)) return NAME##_AA(0, w, x);       \
    else return NAME##_SA(0, w, x);                \
  } else if (isArr(w)) return RNAME##_SA(1, x, w);

#define CMP_SCALAR(NAME, RNAME, OP, FC, CF) B NAME##_c2(B t, B w, B x) { \
  if (isF64(w)&isF64(x)) return m_i32(w.f OP x.f); \
  if (isC32(w)&isC32(x)) return m_i32(w.u OP x.u); \
  CMP_TO_ARR(NAME, RNAME);                         \
  if (isF64(w)&isC32(x)) return m_i32(FC);         \
  if (isC32(w)&isF64(x)) return m_i32(CF);         \
  return m_i32(compare(w, x) OP 0);                \
}

CMP_SCALAR(le, ge, <=, 1, 0)
CMP_SCALAR(ge, le, >=, 0, 1)
CMP_SCALAR(lt, gt, < , 1, 0)
CMP_SCALAR(gt, lt, > , 0, 1)

NOINLINE B eq_atom(B t, B w, B x) { B r = m_i32( atomEqual(w, x)); dec(w); dec(x); return r; }
NOINLINE B ne_atom(B t, B w, B x) { B r = m_i32(!atomEqual(w, x)); dec(w); dec(x); return r; }
B eq_c2(B t, B w, B x) { if(isF64(w)&isF64(x)) return m_i32(w.f==x.f); if(isC32(w)&isC32(x)) return m_i32(w.u==x.u); CMP_TO_ARR(eq, eq); return eq_atom(t, w, x); }
B ne_c2(B t, B w, B x) { if(isF64(w)&isF64(x)) return m_i32(w.f!=x.f); if(isC32(w)&isC32(x)) return m_i32(w.u!=x.u); CMP_TO_ARR(ne, ne); return ne_atom(t, w, x); }

#undef CMP_SCALAR
#undef CMP_TO_ARR

B gt_c1(B t, B x) {
  if (isAtm(x)) return x;
  return bqn_merge(x);
}
