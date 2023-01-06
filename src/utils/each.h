#pragma once
#include "mut.h"

B eachd_fn(B fo, B w, B x, BBB2B f); // consumes w,x; assumes at least one is array
B eachm_fn(B fo, B x, BB2B f); // consumes x; x must be array


#if CATCH_ERRORS
NOINLINE B arith_recd(BBB2B f, B w, B x);
#else
static inline B arith_recd(BBB2B f, B w, B x) { return eachd_fn(bi_N, w, x, f); }
#endif
