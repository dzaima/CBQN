#include "../../core.h"
#include "../../utils/each.h"
#include <math.h>
// #define ARITH_DEBUG 1

typedef u64 (*CheckedFn)(u8* r, u8* w, u8* x, u64 len);
typedef void (*UncheckedFn)(u8* r, u8* w, u8* x, u64 len);
#define FOR_ExecAA(F) \
  F(fail) /* first to allow zero-initialization to be fail implicitly */ \
  F(swap) /* swap 𝕨 and 𝕩, then run ex2 */ \
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
  F(e_call_rbyte) /* calls CheckedFn but errors on non-zero result */ \
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
char* execAA_repr(u8 ex) {
  switch(ex) { default: return "(unknown)";
    #define F(X) case X: return #X;
    FOR_ExecAA(F)
    #undef F
  }
}
#endif


typedef struct FnInfo {
  union { CheckedFn cFn; UncheckedFn uFn; };
  u8 ex1, ex2; // ExecAA
  u8 type; // t_*; unused for u_call_bit
  u8 width; // width in bytes; unused for u_call_bit
} FnInfo;
typedef struct EntAA {
  FnInfo a, b;
} EntAA;

typedef struct DyTable {
  EntAA entsAA[8*8]; // one for each instruction
  BBB2B mainFn;
  char* repr;
} DyTable;

NOINLINE B do_dyArith(DyTable* table, B w, B x) {
  B r;
  
  if (1 || isArr(w)) {
    u8 we = TI(w, elType);
    if (we==el_B) goto rec;
    if (1 || isArr(x)) {
      u8 xe = TI(x, elType);
      if (xe==el_B) goto rec;
      ur wr = RNK(w);
      ur xr = RNK(x);
      if (wr!=xr || !eqShPart(SH(w), SH(x), wr)) goto rec;
      
      usz ia = IA(w);
      EntAA* e = &table->entsAA[we*8 + xe];
      newEnt:
      
      FnInfo* fn = &e->a;
      newFn:
      u8 ex = fn->ex1;
      newEx:
      B t;
      #if ARITH_DEBUG
      printf("opcode %d / %s\n", ex, execAA_repr(ex));
      #endif
      switch(ex) { default: UD;
        case wi8_reg:        w=taga( cpyI8Arr(w));      goto do_ex2;  case xi8_reg:        x=taga( cpyI8Arr(x));      goto do_ex2;
        case wi16_reg:       w=taga(cpyI16Arr(w));      goto do_ex2;  case xi16_reg:       x=taga(cpyI16Arr(x));      goto do_ex2;
        case wi32_reg:       w=taga(cpyI32Arr(w));      goto do_ex2;  case xi32_reg:       x=taga(cpyI32Arr(x));      goto do_ex2;
        case wf64_reg:       w=taga(cpyF64Arr(w));      goto do_ex2;  case xf64_reg:       x=taga(cpyF64Arr(x));      goto do_ex2;
        case wc16_reg:       w=taga(cpyC16Arr(w));      goto do_ex2;  case xc16_reg:       x=taga(cpyC16Arr(x));      goto do_ex2;
        case wc32_reg:       w=taga(cpyC32Arr(w));      goto do_ex2;  case xc32_reg:       x=taga(cpyC32Arr(x));      goto do_ex2;
        case wi8_swap:  t=x; x=taga( cpyI8Arr(w)); w=t; goto do_ex2;  case xi8_swap:  t=w; w=taga( cpyI8Arr(x)); x=t; goto do_ex2;
        case wi16_swap: t=x; x=taga(cpyI16Arr(w)); w=t; goto do_ex2;  case xi16_swap: t=w; w=taga(cpyI16Arr(x)); x=t; goto do_ex2;
        case wi32_swap: t=x; x=taga(cpyI32Arr(w)); w=t; goto do_ex2;  case xi32_swap: t=w; w=taga(cpyI32Arr(x)); x=t; goto do_ex2;
        case wf64_swap: t=x; x=taga(cpyF64Arr(w)); w=t; goto do_ex2;  case xf64_swap: t=w; w=taga(cpyF64Arr(x)); x=t; goto do_ex2;
        case wc16_swap: t=x; x=taga(cpyC16Arr(w)); w=t; goto do_ex2;  case xc16_swap: t=w; w=taga(cpyC16Arr(x)); x=t; goto do_ex2;
        case wc32_swap: t=x; x=taga(cpyC32Arr(w)); w=t; goto do_ex2;  case xc32_swap: t=w; w=taga(cpyC32Arr(x)); x=t; goto do_ex2;
        case swap: t=w; w=x; x=t; goto do_ex2;
        do_ex2: ex = fn->ex2; goto newEx;
        
        case c_call_rbyte: { c_call_rbyte:
          u64 got = fn->cFn(m_tyarrlc(&r, fn->width, x, fn->type), tyany_ptr(w), tyany_ptr(x), ia);
          if (got==ia) goto decG_ret;
          decG(r);
          fn = &e->b;
          goto newFn;
        }
        case u_call_rbyte: {
          fn->uFn(m_tyarrlc(&r, fn->width, x, fn->type), tyany_ptr(w), tyany_ptr(x), ia);
          goto decG_ret;
        }
        case e_call_rbyte: {
          u64 got = fn->cFn(m_tyarrlc(&r, fn->width, x, fn->type), tyany_ptr(w), tyany_ptr(x), ia);
          if (got) goto rec;
          goto decG_ret;
        }
        case u_call_bit: {
          u64* rp; r = m_bitarrc(&rp, x);
          fn->uFn((u8*)rp, tyany_ptr(w), tyany_ptr(x), ia);
          goto decG_ret;
        }
        
        case u_call_wxf64sq: {
          f64* rp; r = m_f64arrc(&rp, x);
          fn->uFn((u8*)rp, tyany_ptr(w = toF64Any(w)), tyany_ptr(x = toF64Any(x)), ia);
          r = num_squeeze(r);
          goto decG_ret;
        }
        case c_call_wxi8: {
          assert(TI(x,elType)==el_bit && TI(w,elType)==el_bit);
          w = taga(cpyI8Arr(w));
          x = taga(cpyI8Arr(x));
          goto c_call_rbyte;
        }
        case e_call_sqx: {
          assert(TI(x,elType)==el_f64);
          x = num_squeeze(x);
          u8 xe = TI(x,elType);
          if (xe==el_f64) goto rec;
          e = &table->entsAA[TI(w,elType)*8 + xe];
          goto newEnt;
        }
        case fail: goto rec;
      }
    } else {
      goto rec;
    }
  } else {
    if (isArr(x)) {
      u8 xe = TI(x, elType);
      if (xe==el_B) goto rec;
      goto rec;
    } else { // TODO decide if this is even a case that needs to be handled here
      return table->mainFn(w, w, x);
    }
  }
  
  rec:
  return arith_recd(table->mainFn, w, x);
  decG_ret:
  decG(w); decG(x);
  return r;
}


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#include "../gen/dyarith2.c"
#pragma GCC diagnostic pop

static void  rootAAu_f64_f64_f64(u8* r, u8* w, u8* x, u64 len) { for (u64 i = 0; i < len; i++) ((f64*)r)[i] = pow(((f64*)x)[i], 1.0/((f64*)w)[i]); }
static void   powAAu_f64_f64_f64(u8* r, u8* w, u8* x, u64 len) { for (u64 i = 0; i < len; i++) ((f64*)r)[i] =     pow(((f64*)w)[i], ((f64*)x)[i]); }
static void stileAAu_f64_f64_f64(u8* r, u8* w, u8* x, u64 len) { for (u64 i = 0; i < len; i++) ((f64*)r)[i] =   pfmod(((f64*)x)[i], ((f64*)w)[i]); }
static void   logAAu_f64_f64_f64(u8* r, u8* w, u8* x, u64 len) { for (u64 i = 0; i < len; i++) ((f64*)r)[i] = log(((f64*)x)[i])/log(((f64*)w)[i]); }

#include "../gen/arTables.c"


B internalTemp_c2(B t, B w, B x) {
  return do_dyArith(&addDyTable, w, x);
}
