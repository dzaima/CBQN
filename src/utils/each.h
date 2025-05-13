#pragma once

B eachd_fn(B fo, B w, B x, FC2 f); // consumes w,x; assumes at least one is array
B eachm_fn(B fo, B x, FC1 f); // consumes x; x must be array

static inline B squeezed_unit(B x) { // same fill as squeeze_any(m_hunit(x))
  return isAtm(x)? m_unit(x) : m_hunit(x);
}

#if SEMANTIC_CATCH
NOINLINE B arith_recd(FC2 f, B w, B x);
#else
static inline B arith_recd(FC2 f, B w, B x) { return eachd_fn(bi_N, w, x, f); }
#endif
