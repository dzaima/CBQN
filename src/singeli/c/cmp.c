#include "../../core.h"
#include "../../builtins.h"
#include <math.h>

extern bool please_tail_call_err;
static NOINLINE void cmp_err() { if (please_tail_call_err) thrM("Invalid comparison"); }


#define BCALL(N, X) N(b(X))
#define interp_f64(X) b(X).f
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#include "../gen/cmp.c"
#pragma GCC diagnostic pop

FN_LUT(cmp_fns, avx2, eq, AS); FN_LUT(cmp_fns, avx2, eq, AA);
FN_LUT(cmp_fns, avx2, ne, AS); FN_LUT(cmp_fns, avx2, ne, AA);
FN_LUT(cmp_fns, avx2, gt, AS); FN_LUT(cmp_fns, avx2, gt, AA);
FN_LUT(cmp_fns, avx2, ge, AS); FN_LUT(cmp_fns, avx2, ge, AA);
FN_LUT(cmp_fns, avx2, lt, AS);
FN_LUT(cmp_fns, avx2, le, AS);

