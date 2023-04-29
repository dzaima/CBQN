#include "../../core.h"
#include "../../utils/each.h"
#include <math.h>
// #define ARITH_DEBUG 1

typedef u64 (*CheckedFn)(void* r, void* w, void* x, u64 len);
typedef void (*UncheckedFn)(void* r, void* w, void* x, u64 len);
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


typedef struct FnInfoAA {
  union { CheckedFn cFn; UncheckedFn uFn; };
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
  char* repr;
} DyTableAA;

NOINLINE B dyArith_AA(DyTableAA* table, B w, B x) {
  u8 we = TI(w, elType); if (we==el_B) goto rec;
  u8 xe = TI(x, elType); if (xe==el_B) goto rec;
  ur wr = RNK(w);
  ur xr = RNK(x);
  if (wr!=xr || !eqShPart(SH(w), SH(x), wr)) goto rec;
  
  B r, t;
  usz ia = IA(w);
  
  EntAA* e = &table->entsAA[we*8 + xe];
  newEnt:;
  
  FnInfoAA* fn = &e->bundles[0];
  newFn:;
  
  u8 ex = fn->ex1;
  newEx:
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
    
    case c_call_rbyte: { c_call_rbyte:;
      u64 got = fn->cFn(m_tyarrlc(&r, fn->width, x, fn->type), tyany_ptr(w), tyany_ptr(x), ia);
      if (got==ia) goto decG_ret;
      decG(r);
      fn++;
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
      fn->uFn(rp, tyany_ptr(w), tyany_ptr(x), ia);
      goto decG_ret;
    }
    
    case u_call_wxf64sq: {
      f64* rp; r = m_f64arrc(&rp, x);
      fn->uFn(rp, tyany_ptr(w = toF64Any(w)), tyany_ptr(x = toF64Any(x)), ia);
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
      x = num_squeezeChk(x);
      u8 xe = TI(x,elType);
      if (xe==el_f64) goto rec;
      e = &table->entsAA[TI(w,elType)*8 + xe];
      goto newEnt;
    }
    case fail: goto rec;
  }
  
  rec:
  return arith_recd(table->mainFn, w, x);
  decG_ret:
  decG(w); decG(x);
  return r;
}


typedef struct DyTableSA DyTableSA;
typedef bool (*ForBitsel)(DyTableSA*, B w, B* r);
typedef u64 (*AtomArrFnC)(void* r, u64 w, void* x, u64 len);
typedef B (*DyArithChrFn)(DyTableSA*, B, B, usz, u8);

typedef struct {
  //      >=el_i8        el_bit
  union { AtomArrFnC f1; ForBitsel bitsel; };
  union { AtomArrFnC f2; };
} EntSA;


struct DyTableSA {
  EntSA ents[el_B];
  FC2 mainFn;
  char* repr;
  u8 fill[2][2]; // 0:none 1:int 2:char
  DyTableSA* chrAtom;
};

bool bad_forBitselNN_SA(DyTableSA* table, B w, B* r) { return false; }
#define bad_forBitselCN_SA bad_forBitselNN_SA

B bad_chrAtomSA(DyTableSA* table, B w, B x, usz ia, u8 xe) { return arith_recd(table->mainFn, w, x); }
#define bad_chrAtomAS bad_chrAtomSA

u64 failAtomArr1(void* r, u64 w, void* x, u64 len) { return 0; }
u64 failAtomArr2(void* r, u64 w, void* x, u64 len) { return 1; }

u8 nextType[] = {
  [t_i8arr ] = t_i16arr, [t_c8arr ] = t_c16arr,
  [t_i16arr] = t_i32arr, [t_c16arr] = t_c32arr,
  [t_i32arr] = t_f64arr, [t_c32arr] = t_empty,
  [t_f64arr] = t_empty,
};

B dyArith_SA(DyTableSA* table, B w, B x) {
  usz ia = IA(x);
  u8 xe = TI(x,elType);
  
  u8 width, type; // the currently supported character functions (+ and -) have the result type be character when left xor right is character, so it can be hard-coded
  u64 wa;
  
  EntSA* e;
  
  if (ia==0) {
    u8 fillVal;
    bool charX;
    if (xe!=el_B) {
      charX = elChr(xe);
    } else {
      B xf = getFillQ(x);
      if (isNum(xf)) charX=0;
      else if (isC32(xf)) charX=1;
      else if (noFill(xf)) { fillVal=0; goto fillSel; }
      else { dec(xf); goto rec; } // whatever
    }
    bool charW;
    if (isNum(w)) charW=0;
    else if (isC32(w)) charW=1;
    else goto rec;
    
    fillVal = table->fill[charW][charX];
    fillSel:
    if (RNK(x)==1) {
      decG(x);
      if (fillVal==1) return emptyIVec();
      if (fillVal==0) return emptyHVec();
      assert(fillVal==2);
      return emptyCVec();
    } else {
      Arr* r;
      if (fillVal==1) {         u64* rp; r = m_bitarrp(&rp, 0); }
      else if (fillVal==0) {      r = (Arr*) m_harrUp(0).c; }
      else { assert(fillVal==2); u8* rp; r = m_c8arrp (&rp, 0); }
      arr_shCopy(r, x);
      decG(x);
      return taga(r);
    }
  }
  
  if (!isF64(w)) {
    if (!isC32(w)) goto rec;
    DyTableSA* t2 = table->chrAtom;
    if (t2==NULL) goto rec;
    table = t2;
    
    
    wa = o2cG(w);
    newXEc:
    switch(xe) { default: UD;
      case el_bit: goto bitsel;
      case el_i8:  if (wa==( u8)wa) { e=&table->ents[el_i8 ]; width=0; type=t_c8arr;  goto f1; } else goto cwiden_i8;
      case el_i16: if (wa==(u16)wa) { e=&table->ents[el_i16]; width=1; type=t_c16arr; goto f1; } else goto cwiden_i16;
      case el_i32:                    e=&table->ents[el_i32]; width=2; type=t_c32arr; goto f1;
      case el_f64: x=num_squeezeChk(x); xe=TI(x,elType); if (xe!=el_f64) goto newXEc; else goto rec;
      case el_c8:  if (wa==( u8)wa) { e=&table->ents[el_c8 ]; width=0; type=t_i8arr;  goto f1; } goto cwiden_c8; // TODO check for & use unsigned w
      case el_c16: if (wa==(u16)wa) { e=&table->ents[el_c16]; width=1; type=t_i16arr; goto f1; } goto cwiden_c16;
      case el_c32:                    e=&table->ents[el_c32]; width=2; type=t_i32arr; goto f1;
      case el_B: goto rec;
    }
    
    cwiden_i8:  if (wa==(u16)wa) { type=t_c16arr; goto cpy_i16; }
    cwiden_i16:                  { type=t_c32arr; goto cpy_i32; }
    
    cwiden_c8:  if (wa==(u16)wa) { type=t_i16arr; goto cpy_c16; }
    cwiden_c16:                  { type=t_i32arr; goto cpy_c32; }
    goto rec;
  }
  
  switch(xe) { default: UD;
    case el_bit: goto bitsel;
    case el_i8:  if (q_i8 (w)) { e=&table->ents[el_i8 ]; width=0; type=t_i8arr;  goto wint; } goto iwiden_i8;
    case el_i16: if (q_i16(w)) { e=&table->ents[el_i16]; width=1; type=t_i16arr; goto wint; } goto iwiden_i16;
    case el_i32: if (q_i32(w)) { e=&table->ents[el_i32]; width=2; type=t_i32arr; goto wint; } goto iwiden_i32;
    case el_f64:                 e=&table->ents[el_f64]; width=3; type=t_f64arr; goto wf64;
    case el_c8:  if (q_i8 (w)) { e=&table->ents[el_c8 ]; width=0; type=t_c8arr;  goto wint; } goto iwiden_c8;
    case el_c16: if (q_i16(w)) { e=&table->ents[el_c16]; width=1; type=t_c16arr; goto wint; } goto iwiden_c16;
    case el_c32: if (q_i32(w)) { e=&table->ents[el_c32]; width=2; type=t_c32arr; goto wint; } goto rec;
    case el_B: goto rec;
  }
  
  wint: wa=(u32)o2iG(w); goto f1;
  wf64: wa=w.u; goto f1;
  
  iwiden_i8:  if (q_i16(w)) { wa=(u32)o2iG(w); type=t_i16arr; goto cpy_i16; }
  iwiden_i16: if (q_i32(w)) { wa=(u32)o2iG(w); type=t_i32arr; goto cpy_i32; }
  iwiden_i32:               { wa=w.u;          type=t_f64arr; goto cpy_f64; }
  
  iwiden_c8:  if (q_i16(w)) { wa=(u32)o2iG(w); type=t_c16arr; goto cpy_c16; }
  iwiden_c16: if (q_i32(w)) { wa=(u32)o2iG(w); type=t_c32arr; goto cpy_c32; }
  goto rec;
  
  // TODO reuse the copied array for the result; maybe even alternate copy & operation to stay in cache
  cpy_c16: x = taga(cpyC16Arr(x)); width=1; e=&table->ents[el_c16]; goto f1;
  cpy_c32: x = taga(cpyC32Arr(x)); width=2; e=&table->ents[el_c32]; goto f1;
  
  cpy_i16: x = taga(cpyI16Arr(x)); width=1; e=&table->ents[el_i16]; goto f1;
  cpy_i32: x = taga(cpyI32Arr(x)); width=2; e=&table->ents[el_i32]; goto f1;
  cpy_f64: x = taga(cpyF64Arr(x)); width=3; e=&table->ents[el_f64]; goto f1;
  
  AtomArrFnC fn; B r;
  f1: fn = e->f1;
  u64 got = fn(m_tyarrlc(&r, width, x, type), wa, tyany_ptr(x), ia);
  if (got==ia) goto decG_ret;
  decG(r);
  
  type = nextType[type];
  if (type==t_empty) goto rec;
  fn = e->f2;
  if (fn(m_tyarrlc(&r, width+1, x, type), wa, tyany_ptr(x), ia)==0) goto decG_ret;
  decG(r);
  goto rec;
  
  
  decG_ret: decG(x); return r;
  
  rec: return arith_recd(table->mainFn, w, x);
  
  bitsel: {
    B opts[2];
    if (!table->ents[el_bit].bitsel(table, w, opts)) goto rec;
    return bit_sel(x, opts[0], opts[1]);
  }
}

#define SINGELI_FILE dyarith
#include "../../utils/includeSingeli.h"

static void  rootAAu_f64_f64_f64(void* r, void* w, void* x, u64 len) { for (u64 i = 0; i < len; i++) ((f64*)r)[i] = pow(((f64*)x)[i], 1.0/((f64*)w)[i]); }
static void   powAAu_f64_f64_f64(void* r, void* w, void* x, u64 len) { for (u64 i = 0; i < len; i++) ((f64*)r)[i] =     pow(((f64*)w)[i], ((f64*)x)[i]); }
static void stileAAu_f64_f64_f64(void* r, void* w, void* x, u64 len) { for (u64 i = 0; i < len; i++) ((f64*)r)[i] =   pfmod(((f64*)x)[i], ((f64*)w)[i]); }
static void   logAAu_f64_f64_f64(void* r, void* w, void* x, u64 len) { for (u64 i = 0; i < len; i++) ((f64*)r)[i] = log(((f64*)x)[i])/log(((f64*)w)[i]); }

bool add_forBitselNN_SA (DyTableSA* table, B w, B* r) { f64 f=o2fG(w); r[0] = m_f64(f+0); r[1] = m_f64(f+1); return true; }
bool sub_forBitselNN_SA (DyTableSA* table, B w, B* r) { f64 f=o2fG(w); r[0] = m_f64(f-0); r[1] = m_f64(f-1); return true; }
bool subR_forBitselNN_SA(DyTableSA* table, B w, B* r) { f64 f=o2fG(w); r[0] = m_f64(0-f); r[1] = m_f64(1-f); return true; }
bool mul_forBitselNN_SA (DyTableSA* table, B w, B* r) { f64 f=o2fG(w); r[0] = m_f64(f*0); r[1] = m_f64(f*1); return true; }
bool min_forBitselNN_SA (DyTableSA* table, B w, B* r) { f64 f=o2fG(w); r[0] = m_f64(f<=0?f:0); r[1] = m_f64(f<=1?f:1); return true; }
bool max_forBitselNN_SA (DyTableSA* table, B w, B* r) { f64 f=o2fG(w); r[0] = m_f64(f>=0?f:0); r[1] = m_f64(f>=1?f:1); return true; }

bool add_forBitselCN_SA(DyTableSA* table, B w, B* r) { u32 wc=o2cG(w); if(wc+1<=CHR_MAX) return false; r[0] = m_c32(wc); r[1] = m_c32(wc+1); return true; }
bool sub_forBitselCN_SA(DyTableSA* table, B w, B* r) { u32 wc=o2cG(w); if(wc  !=0      ) return false; r[0] = m_c32(wc); r[1] = m_c32(wc-1); return true; }

B sub_c2R(B t, B w, B x) { return sub_c2(t, x, w); }

static NOINLINE B or_SA(B t, B w, B x) {
  if (!isF64(w)) return arith_recd(or_c2, w, x);
  if (LIKELY(TI(x,elType)==el_bit)) {
    bitsel:;
    f64 wf = o2fG(w);
    return bit_sel(x, m_f64(bqn_or(wf, 0)), m_f64(bqn_or(wf, 1)));
  }
  x = num_squeezeChk(x);
  if (TI(x,elType)==el_bit) goto bitsel;
  
  f64* rp;
  B r = m_f64arrc(&rp, x);
  usz ia = a(x)->ia;
  x = toF64Any(x);
  orSAc_f64_f64_f64(rp, w.u, tyany_ptr(x), ia);
  decG(x);
  return r;
}

#define SINGELI_FILE arTables
#include "../../utils/includeSingeli.h"
