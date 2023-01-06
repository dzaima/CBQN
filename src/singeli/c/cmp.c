#include "../../core.h"
#include "../../builtins.h"
#include <math.h>

extern bool please_tail_call_err;
static NOINLINE void cmp_err() { if (please_tail_call_err) thrM("Invalid comparison"); }


#define SINGELI_FILE cmp
#include "../../utils/includeSingeli.h"

FN_LUT_A(cmp_fns, eq, AS); FN_LUT_A(cmp_fns, eq, AA);
FN_LUT_A(cmp_fns, ne, AS); FN_LUT_A(cmp_fns, ne, AA);
FN_LUT_A(cmp_fns, gt, AS); FN_LUT_A(cmp_fns, gt, AA);
FN_LUT_A(cmp_fns, ge, AS); FN_LUT_A(cmp_fns, ge, AA);
FN_LUT_A(cmp_fns, lt, AS);
FN_LUT_A(cmp_fns, le, AS);

#define ARCH(N) simd_##N
#define DSTE(N) &(cmp_fns_##N[0])
void cmpA_init() {
  {
    CmpAAFn* srcs[4] = {ARCH(eqAA), ARCH(neAA), ARCH(gtAA), ARCH(geAA)};
    CmpAAFn* dsts[4] = {DSTE(eqAA), DSTE(neAA), DSTE(gtAA), DSTE(geAA)};
    for (i32 i=0; i<4; i++) for (i32 j=0; j<8; j++) dsts[i][j] = srcs[i][j];
  }
  {
    CmpASFn* srcs[6] = {ARCH(eqAS), ARCH(neAS), ARCH(gtAS), ARCH(geAS), ARCH(ltAS), ARCH(leAS)};
    CmpASFn* dsts[6] = {DSTE(eqAS), DSTE(neAS), DSTE(gtAS), DSTE(geAS), DSTE(ltAS), DSTE(leAS)};
    for (i32 i=0; i<6; i++) for (i32 j=0; j<8; j++) dsts[i][j] = srcs[i][j];
  }
}
#undef ARCH
#undef DSTE
