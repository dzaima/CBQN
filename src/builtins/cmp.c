#include "../core.h"
#include "../utils/each.h"
#include "../utils/calls.h"

static NOINLINE void fillBits(u64* dst, u64 sz, bool v) {
  u64 x = 0-(u64)v;
  u64 am = (sz+63)/64; assert(am>0);
  for (usz i = 0; i < am; i++) dst[i] = x;
}
static NOINLINE void fillBitsDec(u64* dst, u64 sz, bool v, u64 x) {
  dec(b(x));
  fillBits(dst, sz, v);
}

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

#define CMPFN(A,F,S,T) A##_##F##S##_##T
#define FN_LUT(B,A,F,S) const Cmp##S##Fn B##_##F##S[] = {CMPFN(A,F,S,u1), CMPFN(A,F,S,i8), CMPFN(A,F,S,i16), CMPFN(A,F,S,i32), CMPFN(A,F,S,f64), CMPFN(A,F,S,u8), CMPFN(A,F,S,u16), CMPFN(A,F,S,u32)}

#if SINGELI
  #include "../singeli/c/cmp.c"
#else
  #define CMP_AA0(N, T, BODY) void base_##N##AA##_##T(u64* r, void* w, void* x, u64 l) { BODY }
  #define CMP_AA1(N, T, OP) CMP_AA0(N, T, for (usz i=0; i<l; i++) bitp_set(r, i, ((T*)w)[i] OP ((T*)x)[i]);)
  #define CMP_AA_F(N, OP, BX) \
    CMP_AA0(N, u1, ({usz bia = BIT_N(l); for (usz i=0; i<bia; i++) { u64 wv=((u64*)w)[i], xv=((u64*)x)[i]; ((u64*)r)[i] = BX; }});) \
    CMP_AA1(N, i8, OP) CMP_AA1(N, i16, OP) CMP_AA1(N, i32, OP) CMP_AA1(N, f64, OP) \
    const CmpAAFn base_##N##AA##_u32 = base_##N##AA##_i32;
  
  #define CMP_AA_C0(N, OP) const CmpAAFn base_##N##AA##_u8 = base_##N##AA##_i8; const CmpAAFn base_##N##AA##_u16 = base_##N##AA##_i16;
  #define CMP_AA_C1(N, OP) CMP_AA1(N, u8, OP) CMP_AA1(N, u16, OP)
  CMP_AA_F(eq, ==, ~wv ^ xv) CMP_AA_C0(eq, ==)
  CMP_AA_F(ne, !=,  wv ^ xv) CMP_AA_C0(ne, !=)
  CMP_AA_F(gt, > , wv & ~xv) CMP_AA_C1(gt, > )
  CMP_AA_F(ge, >=, wv | ~xv) CMP_AA_C1(ge, >=)
  #undef CMP_AA_F
  
  
  #define CMP_SLOW(T, GW) void cmp_slow_##T(void* r, void* w, B x, u64 l, BBB2B fn) { \
    assert(l>0); incBy(x,l-1); \
    for (usz i=0; i<l; i++) bitp_set(r, i, o2bG(fn(m_f64(0), GW, x))); \
  }
  #define CMP_SLOWi(T,M) CMP_SLOW(T, m_##M(((T*)w)[i]))
  CMP_SLOW(u1,m_i32(bitp_get(w,i)))
  CMP_SLOWi(i8,i32) CMP_SLOWi(i16,i32) CMP_SLOWi(i32,i32) CMP_SLOWi(f64,f64)
  CMP_SLOWi(u8,c32) CMP_SLOWi(u16,c32) CMP_SLOWi(u32,c32)
  
  static inline void cmp_fill_eq(u64* r, u64 l, u64 x) { fillBitsDec(r, l, 0, x); }
  static inline void cmp_fill_ne(u64* r, u64 l, u64 x) { fillBitsDec(r, l, 1, x); }
  #define CMP_TO_SLOW(N, T) cmp_slow_##T(r, w, x, l, N##_c2)
  #define CMP_TO_FILL(N, T) cmp_fill_##N(r, l, xr)
  
  #define CMP_SA0(N, T, Q, SLOW, BODY) void base_##N##AS##_##T(u64* r, void* w, u64 xr, u64 l) { \
    assert(l>0); B x=b(xr);     \
    if (LIKELY(q_##Q(x))) BODY; \
    else SLOW(N, T);            \
  }
  
  #define CMP_SA1(N, T, Q, C, SLOW, OP) CMP_SA0(N, T, Q, SLOW, ({T xv = C(x); for (usz i=0; i<l; i++) bitp_set(r, i, ((T*)w)[i] OP xv);}))
  #define CMP_SA_F(N, OP, SLOW, BX) \
    CMP_SA0(N, u1, bit, SLOW, ({usz bia = BIT_N(l); u64 xv=bitx(x); for (usz i=0; i<bia; i++) { u64 wv=((u64*)w)[i]; ((u64*)r)[i] = BX; }})) \
    CMP_SA1(N,i8,i8,o2iG,SLOW,OP) CMP_SA1(N,i16,i16,o2iG,SLOW,OP) CMP_SA1(N,i32,i32,o2iG,SLOW,OP) CMP_SA1(N,f64,f64,o2fG,SLOW,OP) \
    CMP_SA1(N,u8,c8,o2cG,SLOW,OP) CMP_SA1(N,u16,c16,o2cG,SLOW,OP) CMP_SA1(N,u32,c32,o2cG,SLOW,OP)
  
  CMP_SA_F(eq, ==, CMP_TO_FILL, ~wv^xv)
  CMP_SA_F(ne, !=, CMP_TO_FILL,  wv^xv)
  CMP_SA_F(le, <=, CMP_TO_SLOW, ~wv |  xv)
  CMP_SA_F(ge, >=, CMP_TO_SLOW,  wv | ~xv)
  CMP_SA_F(lt, < , CMP_TO_SLOW, ~wv &  xv)
  CMP_SA_F(gt, > , CMP_TO_SLOW,  wv & ~xv)
  #undef CMP_SA_F
  
  FN_LUT(cmp_fns, base, eq, AS); FN_LUT(cmp_fns, base, eq, AA);
  FN_LUT(cmp_fns, base, ne, AS); FN_LUT(cmp_fns, base, ne, AA);
  FN_LUT(cmp_fns, base, gt, AS); FN_LUT(cmp_fns, base, gt, AA);
  FN_LUT(cmp_fns, base, ge, AS); FN_LUT(cmp_fns, base, ge, AA);
  FN_LUT(cmp_fns, base, lt, AS);
  FN_LUT(cmp_fns, base, le, AS);
#endif
#undef FN_LUT



#define AL(X) u64* rp; B r = m_bitarrc(&rp, X); usz ria=IA(r)
#define CMP_AA_D(CN, CR, NAME, PRE) NOINLINE B NAME##_AA(i32 swapped, B w, B x) { PRE \
  u8 xe = TI(x, elType); if (xe==el_B) goto bad; \
  u8 we = TI(w, elType); if (we==el_B) goto bad; \
  if (RNK(w)==RNK(x)) { if (!eqShape(w, x)) thrF("%U: Expected equal shape prefix (%H ≡ ≢𝕨, %H ≡ ≢𝕩)", swapped?CR:CN, swapped?x:w, swapped?w:x); \
    if (we!=xe) { B tw=w,tx=x;           \
      we = aMakeEq(&tw, &tx, we, xe);    \
      if (we==el_MAX) goto bad;          \
      w=tw; x=tx;                        \
    }                                    \
    AL(x);                               \
    if (ria) cmp_fns_##NAME##AA[we](rp, tyany_ptr(w), tyany_ptr(x), ria); \
    decG(w);decG(x); return r;           \
  }                                      \
  bad: return NAME##_rec(swapped, w, x); \
}
CMP_AA_D("≥", "≤", ge, )
CMP_AA_D(">", "<", gt, )
CMP_AA_D("=", "?", eq, swapped=0;)
CMP_AA_D("≠", "?", ne, swapped=0;)
#define le_AA(T, W, X) ge_AA(!T, X, W)
#define lt_AA(T, W, X) gt_AA(!T, X, W)
#undef CMP_AA_D




#define CMP_SA_D(NAME, RNAME, PRE) B NAME##_SA(i32 swapped, B w, B x) { PRE \
  u8 xe = TI(x, elType); if (xe==el_B) goto bad; \
  AL(x);                                 \
  if (ria) cmp_fns_##RNAME##AS[xe](rp, tyany_ptr(x), w.u, ria); \
  else dec(w);                           \
  decG(x); return r;                     \
  bad: return NAME##_rec(swapped, w, x); \
}
CMP_SA_D(eq, eq, swapped=0;)
CMP_SA_D(ne, ne, swapped=0;)
CMP_SA_D(le, ge, )
CMP_SA_D(ge, le, )
CMP_SA_D(lt, gt, )
CMP_SA_D(gt, lt, )
#undef CMP_SA_D
#undef AL



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
