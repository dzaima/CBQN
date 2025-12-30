#pragma once

B eachd_fn(B fo, B w, B x, FC2 f); // consumes w,x; assumes at least one is array
B eachm_fn(B fo, B x, FC1 f); // consumes x; x must be array

static inline B squeezed_unit(B x) { // same fill as squeeze_any(m_hunit(x))
  return isAtm(x)? m_unit(x) : m_hunit(x);
}

#define A_SHX 2
#define A_SHW 4
#define A_NN(X) (X)<<0
#define A_NC(X) (X)<<A_SHX
#define A_CN(X) (X)<<A_SHW
#define A_CC(X) (X)<<(A_SHW|A_SHX)
#define A_B A_NN(A_NUM) // base arith fill config; numbers must always result in numbers
#define A_NUM 1
#define A_CHR 2

// cfg: A_B | ...A_[NC][NC](A_NUM or A_CHR)
// A_WX(V) specifies that, for 𝕨 of type W and 𝕩 of type X, the expected fill is V; unspecified/V≡0 cases are errors
NOINLINE B arith_recd(FC2 f, B w, B x, u32 cfg);