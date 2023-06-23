#pragma once
#if SINGELI_SIMD
// #define ARITH_DEBUG 1

#define FOR_ExecAA(F) \
  F(ex_fail) /* first to allow zero-initialization to be fail implicitly */ \
  F(ex_swap) /* swap 𝕨 and 𝕩, then run ex2 */ \
  /* cast the specified argument up to the specified size, then either swap or don't, then run ex2 */ \
  F(wi8_reg)  F(xi8_reg)  F(wi8_swap)  F(xi8_swap)  \
  F(wi16_reg) F(xi16_reg) F(wi16_swap) F(xi16_swap) \
  F(wi32_reg) F(xi32_reg) F(wi32_swap) F(xi32_swap) \
  F(wf64_reg) F(xf64_reg) F(wf64_swap) F(xf64_swap) \
  F(wc16_reg) F(xc16_reg) F(wc16_swap) F(xc16_swap) \
  F(wc32_reg) F(xc32_reg) F(wc32_swap) F(xc32_swap) \
  /* c_* - overflow-checked; u_* - no overflow check */ \
  F(c_call_rbyte) /* arguments are already the wanted widths; result isn't a bitarr */ \
  F(u_call_rbyte) /* ↑ */ \
  F(e_call_rbyte) /* calls ChkFnAA but errors on non-zero result */ \
  F(u_call_bit) /* result and arguments are bitarrs */ \
  F(u_call_wxf64sq) /* convert both args up to f64 if needed, make f64arr, and squeeze result; i.e. lazy float fallback case */ \
  F(c_call_wxi8) /* convert both args (which need to be bitarrs) to i8arrs, and invoke checked function (no good reason for it to fail the check, but this allows reusing a c‿i8‿i8 impl) */ \
  F(e_call_sqx) /* squeeze f64arr 𝕩, error if can't; else re-dispatch to new entry */

enum ExecAA {
  #define F(X) X,
  FOR_ExecAA(F)
  #undef F
};

#if ARITH_DEBUG
char* execAA_repr(u8 ex);
#endif

typedef u64 (*ChkFnAA)(void* r, void* w, void* x, u64 len);
typedef void (*UnchkFnAA)(void* r, void* w, void* x, u64 len);

typedef struct FnInfoAA {
  union { ChkFnAA cFn; UnchkFnAA uFn; };
  u8 ex1, ex2; // ExecAA
  u8 type; // t_*; unused for u_call_bit
  u8 width; // width in bytes; unused for u_call_bit
} FnInfoAA;

typedef struct EntAA {
  FnInfoAA bundles[2];
} EntAA;

typedef struct DyTableAA {
  EntAA entsAA[el_B*el_B]; // one for each instruction
  FC2 mainFn;
  B mainFnObj;
  char* repr;
} DyTableAA;





typedef struct DyTableSA DyTableSA;
typedef bool (*ForBitsel)(DyTableSA*, B w, B* r);
typedef u64 (*ChkFnSA)(void* r, u64 w, void* x, u64 len);

typedef struct {
  //      >=el_i8        el_bit
  union { ChkFnSA f1; ForBitsel bitsel; };
  union { ChkFnSA f2; };
} EntSA;


struct DyTableSA {
  EntSA ents[el_B];
  FC2 mainFn;
  char* repr;
  u8 fill[2][2]; // 0:none 1:int 2:char
  DyTableSA* chrAtom;
};

// all assume f is a function, and return NULL if a table is unavailable (either because f isn't applicable dyadic arith, or the requested case always errors)
DyTableAA* dyTableAAFor(B f);
DyTableSA* dyTableSAFor(B f, bool atomChar);
DyTableSA* dyTableASFor(B f, bool atomChar); // returns table taking arguments in reverse order

#endif
