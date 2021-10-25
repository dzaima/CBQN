#include <math.h>
#include "../../core.h"
#include "../../builtins.h"

static NOINLINE void fillBits(u64* dst, u64 sz, bool v) {
  u64 x = 0-(u64)v;
  u64 am = (sz+63)/64; assert(am>0);
  for (usz i = 0; i < am; i++) dst[i] = x;
}
static NORETURN void cmp_err() { thrM("Invalid comparison"); }


#define BCALL(N, X) N(b(X))
#define interp_f64(X) b(X).f

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#include "../gen/cmp.c"
#pragma GCC diagnostic pop

typedef void (*CmpAAFn)(u64*, u8*, u8*, u64);
typedef void (*CmpASFn)(u64*, u8*, u64, u64);
#define CMPFN(A,F,S,T) A##_##F##S##_##T
#define FN_LUT(A,F,S) static const Cmp##S##Fn lut_##A##_##F##S[] = {CMPFN(A,F,S,u1), CMPFN(A,F,S,i8), CMPFN(A,F,S,i16), CMPFN(A,F,S,i32), CMPFN(A,F,S,f64), CMPFN(A,F,S,u8), CMPFN(A,F,S,u16), CMPFN(A,F,S,u32)}

FN_LUT(avx2, eq, AS); FN_LUT(avx2, eq, AA);
FN_LUT(avx2, ne, AS); FN_LUT(avx2, ne, AA);
FN_LUT(avx2, gt, AS); FN_LUT(avx2, gt, AA);
FN_LUT(avx2, ge, AS); FN_LUT(avx2, ge, AA);
FN_LUT(avx2, lt, AS);
FN_LUT(avx2, le, AS);

static void* tyany_ptr(B x) {
  u8 t = v(x)->type;
  return IS_SLICE(t)? c(TySlice,x)->a : c(TyArr,x)->a;
}

#define AL(X) u64* rp; B r = m_bitarrc(&rp, X); usz ria=a(r)->ia
#define CMP_IMPL(CHR, NAME, RNAME, PNAME, L, R, OP, FC, CF, BX) \
  if (isF64(w)&isF64(x)) return m_i32(w.f OP x.f); \
  if (isC32(w)&isC32(x)) return m_i32(w.u OP x.u); \
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
        AL(x);                                      \
        if (ria) lut_avx2_##PNAME##AA[we](rp, (u8*)tyany_ptr(L), (u8*)tyany_ptr(R), ria); \
        dec(w);dec(x); return r; \
      } else goto end;           \
    }                            \
    AL(w);                       \
    if (ria) lut_avx2_##NAME##AS [we](rp, (u8*)tyany_ptr(w), x.u, ria); \
    dec(w); return r;            \
  } else if (isArr(x)) { u8 xe = TI(x,elType); if (xe==el_B) goto end; AL(x); \
    if (ria) lut_avx2_##RNAME##AS[xe](rp, (u8*)tyany_ptr(x), w.u, ria); \
    dec(x); return r;                      \
  }                                        \
  if (isF64(w)&isC32(x)) return m_i32(FC); \
  if (isC32(w)&isF64(x)) return m_i32(CF); \
  end:;
