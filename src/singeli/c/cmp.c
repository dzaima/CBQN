#include <math.h>
#include "../../core.h"
#include "../../builtins.h"

static NOINLINE void fillBits(u64* dst, u64 sz, bool v) {
  u64 x = 0-(u64)v;
  u64 am = (sz+63)/64; assert(am>0);
  for (usz i = 0; i < am; i++) dst[i] = x;
}
static NOINLINE void fillBitsDec(u64* dst, u64 sz, bool v, u64 x) {
  dec(b(x));
  fillBits(dst, sz, v);
}

extern bool please_tail_call_err;
static NOINLINE void cmp_err() { if (please_tail_call_err) thrM("Invalid comparison"); }


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
#undef FN_LUT


#define AL(X) u64* rp; B r = m_bitarrc(&rp, X); usz ria=IA(r)
#define CMP_AA(CN, CR, NAME, PRE) NOINLINE B NAME##_AA(i32 swapped, B w, B x) { PRE \
  u8 xe = TI(x, elType); if (xe==el_B) goto bad; \
  u8 we = TI(w, elType); if (we==el_B) goto bad; \
  if (RNK(w)==RNK(x)) { if (!eqShape(w, x)) thrF("%U: Expected equal shape prefix (%H ≡ ≢𝕨, %H ≡ ≢𝕩)", swapped?CR:CN, swapped?x:w, swapped?w:x); \
    if (we!=xe) { B tw=w,tx=x;           \
      we = aMakeEq(&tw, &tx, we, xe);    \
      if (we==el_MAX) goto bad;          \
      w=tw; x=tx;                        \
    }                                    \
    AL(x);                               \
    if (ria) lut_avx2_##NAME##AA[we](rp, (u8*)tyany_ptr(w), (u8*)tyany_ptr(x), ria); \
    decG(w);decG(x); return r;           \
  }                                      \
  bad: return NAME##_rec(swapped, w, x); \
}
CMP_AA("≥", "≤", ge, )
CMP_AA(">", "<", gt, )
CMP_AA("=", "?", eq, swapped=0;)
CMP_AA("≠", "?", ne, swapped=0;)
#define le_AA(T, W, X) ge_AA(!T, X, W)
#define lt_AA(T, W, X) gt_AA(!T, X, W)
#undef CMP_AA




#define CMP_SA(NAME, RNAME, PRE) B NAME##_SA(i32 swapped, B w, B x) { PRE \
  u8 xe = TI(x, elType); if (xe==el_B) goto bad; \
  AL(x);                                 \
  if (ria) lut_avx2_##RNAME##AS[xe](rp, (u8*)tyany_ptr(x), w.u, ria); \
  else dec(w);                           \
  decG(x); return r;                     \
  bad: return NAME##_rec(swapped, w, x); \
}
CMP_SA(eq, eq, swapped=0;)
CMP_SA(ne, ne, swapped=0;)
CMP_SA(le, ge, )
CMP_SA(ge, le, )
CMP_SA(lt, gt, )
CMP_SA(gt, lt, )
#undef CMP_SA
#undef AL