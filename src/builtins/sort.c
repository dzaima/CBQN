#include "../core.h"
#include "../utils/talloc.h"

// Defines Sort, Grade, isSortedUp/Down, and Bins

static bool bit_isSorted(void* data, u64 down, usz n) {
  ux xia = n+1;
  u64* xp = data;
  u64 i = bit_find(xp, xia, !down);
  usz iw = i/64;
  u64 m = ~(u64)0;
  u64 mid = xp[iw];
  u64 d = (down? ~mid : mid) ^ (m<<(i%64));
  if (iw == xia/64) return (d &~ (m<<(xia%64))) == 0;
  if (d) return 0;
  usz o = iw + 1;
  usz l = xia - 64*o;
  return (bit_find(xp+o, l, down) == l);
}

#if SINGELI
  #define SINGELI_FILE bins
  #include "../utils/includeSingeli.h"
#endif

#if SINGELI_SIMD
  typedef bool (*IsSortedFn)(void*, u64, usz);
  IsSortedFn is_sorted[el_B];
  #define SINGELI_FILE sort
  #include "../utils/includeSingeli.h"
#endif

#define CAT0(A,B) A##_##B
#define CAT(A,B) CAT0(A,B)
typedef struct BI32p { B k; i32 v; } BI32p;
typedef struct I32I32p { i32 k; i32 v; } I32I32p;

static NOINLINE void generic_grade(B x, usz ia, B r, i32* rp, void (*fn)(BI32p*, size_t)) {
  TALLOC(BI32p, tmp, ia);
  SGetU(x)
  for (usz i = 0; i < ia; i++) {
    tmp[i].v = i;
    tmp[i].k = GetU(x,i);
  }
  fn(tmp, ia);
  vfor (usz i = 0; i < ia; i++) rp[i] = tmp[i].v;
  TFREE(tmp);
}
static bool q_nan(B x) {
  assert(isNum(x));
  double f = o2fG(x);
  return f!=f;
}

#define GRADE_UD(U,D) U
#include "grade.h"
#define GRADE_UD(U,D) D
#include "grade.h"

void sort_init() {
  #if SINGELI_SIMD
    is_sorted[el_bit] = bit_isSorted;
    for (ux i = el_i8; i <= el_c32; i++) is_sorted[i] = si_is_sorted[i-el_i8];
  #endif
}