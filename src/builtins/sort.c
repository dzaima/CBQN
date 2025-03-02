#include "../core.h"
#include "../utils/talloc.h"

// Defines Sort, Grade, and Bins

#if SINGELI
  #define SINGELI_FILE bins
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
