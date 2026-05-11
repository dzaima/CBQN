#pragma once

#include "core.h"
#include "utils/calls.h"
#include "utils/nfns.h"

#ifndef FFI_CHECKS // enable or disable correctness checks of •FFI
  #define FFI_CHECKS 1
#endif

DEFINE_NFN foreignFnDesc;

#if !__has_include(<ffi.h>)
  #error "<ffi.h> not found. Either install libffi and pkg-config, or add 'FFI=0' as a make argument to disable •FFI"
#endif
#include <dlfcn.h>
#include <ffi.h>
static void libffiOk(ffi_status st) {
  if (DEBUG && st != FFI_OK) thrF("libffi call failed: %i\n", (i32) st);
}

#ifndef GPR6_SYSV64
  // ABIs where booleans, pointers, and ≤64-bit integers, are all passed sign-/zero-extended to 64-bit in an equivalent GPR sequence, up to 6 args, and returns in single register
  #define GPR6_SYSV64 ((__APPLE__ || __MACH__ || __linux__ || __FreeBSD__ || __unix__) && (__x86_64__ || __aarch64__) && __SIZEOF_POINTER__==8)
  // TODO: RISC-V should mostly fit, though u32 needs to be sign-extended, not zero-extended
#endif

typedef u8 PrimType; // not sty_bit nor sty_c8..sty_c32
typedef u8 ConvType; // sty_bit, sty_i8..sty_i32, sty_c8..sty_c32, i.e. all valid el_* equivalents (sty_f64 is also allowed internally, but T:f64 isn't exposed)
enum { // used for both PrimType and ConvType
  sty_void,
  sty_bit=el_bit+1, // ConvType :u1 / bit boolean - 1 bit
  sty_i8=el_i8+1, sty_i16=el_i16+1, sty_i32=el_i32+1, sty_f64=el_f64+1,
  sty_c8=el_c8+1, sty_c16=el_c16+1, sty_c32=el_c32+1,
  sty_UNUSED = el_B+1,
  sty_f32 = el_MAX+1,
  sty_u8, sty_u16, sty_u32, sty_u64, sty_i64,
  sty_bool, // PrimType bool - 1 byte
  sty_rawPtr, // all objects after shall have no fill
  sty_a,
};
static bool sty_noFill(u8 sty) { return sty >= sty_rawPtr; }
static bool sty_char(u8 sty) { return sty >= sty_c8 && sty <= sty_c32; }
static const u8 ptr_size_lb = 3 + 63-CLZ64(sizeof(void*));
static i8 const sty_size_lb[] = {
  [sty_a]=6,
  [sty_bool]=3,
  [sty_rawPtr]=ptr_size_lb,
  [sty_u8]=3, [sty_u16]=4, [sty_u32]=5, [sty_u64]=6,
  [sty_i8]=3, [sty_i16]=4, [sty_i32]=5, [sty_i64]=6,
  [sty_c8]=3, [sty_c16]=4, [sty_c32]=5,
  [sty_f32]=5, [sty_f64]=6,
  [sty_bit]=0,
};
static i32 sty_lb(u8 sty) { return sty_size_lb[sty]; }
static char* sty_name(u8 sty);

static bool isFFIPrim(B x) {
  return isC32(x);
}
static PrimType ffiPrimTy(B x) {
  return (u8) o2cG(x);
}
static ConvType ffiPrimConv(B x) {
  return (u8) (o2cG(x)>>8);
}
static u8 styToEltype(ConvType t) {
  return t-sty_bit + el_bit;
}

static B m_ffiPrim(PrimType ty, ConvType conv) {
  return m_c32(ty | conv<<8);
}
#define FFIPRIM_VOID m_ffiPrim(sty_void, sty_void)

typedef u8 CompoundType;
enum CompoundType {
  cty_ptr, // *T, &T
  cty_starr, // non-top-level array, aka struct field or pointer element
  cty_tlarr, // top-level array, aka pointer but just with fixed length
  cty_struct, // {...}
  cty_argData, // main FFIFn argument & result type data holder
  cty_ret1, // only return allowed as result type; when result is a single value ("&", or no mutated args) and freeing temp elements is needed
  cty_retList, // only return allowed as result type; when result is a list
  cty_ptrh, // pointer object holder
};
typedef enum PtrKind {
  ptrk_in, // *T
  ptrk_mut, // &T
  ptrk_out, // ⥊T
} PtrKind;
static char* const ptrk_names[] = {[ptrk_in]="*", [ptrk_mut]="&", [ptrk_out]="⥊"};

typedef struct FFIEnt {
  B o;
  union {
    struct { u32 fieldOffset; } st; // cty_struct
    struct { u32 scratchMemOffset; i32 srcPos; } argData; // cty_argData
    struct { // cty_ptr
      u32 dataPtrOffset; // for ptrk_mut & ptrk_out, where in ScratchMem to read a pointer to my data
      u32 ptrObjRefOffset; // for ptrk_mut, where in ScratchMem is a `B` of a passed-in pointer object (bi_z if was array)
    } ptr;
  };
} FFIEnt;

typedef struct FFICompoundType {
  struct Value;
  union {
    ffi_type foreign;
    struct { uintptr_t ptr; ux stride; } ptrh;
  };
  union {
    struct { u32 inpLen, arrElts; } starr; // cty_starr; will only have full proper ffi_type if necessary for ffi_cif; ia==1 always
    struct { // cty_ptr, cty_tlarr
      u32 inpLen; // cty_tlarr expected input length (factors in conv)
      u8 kind; // ptrk_*
      bool read;
    } ptr;
  };
  CompoundType cty;
  u32 ia; // number of elements in `a`
  FFIEnt a[];
  // foreign->elements points to after all elements of `a`
} FFICompoundType;

static FFICompoundType* m_ffiCompound(u32 ia, CompoundType cty, ux ffiType, ux ffiElts) { // if ffiType!=FFI_TYPE_VOID, sets up a buffer for result->foreign.elements and gives it its null terminator; need to still set size/alignment/elements!
  ux head = fsizeof(FFICompoundType,a,FFIEnt,ia);
  FFICompoundType* r = mm_alloc(head + (ffiType? sizeof(ffi_type*) * (ffiElts+1) : 0), t_ffiType);
  r->cty = cty;
  r->ia = ia;
  if (ia > 0) NOGC_S;
  if (ffiType) {
    r->foreign.type = ffiType;
    ffi_type** elts = r->foreign.elements = (ffi_type**) ((u8*)r + head);
    elts[ffiElts] = NULL;
  }
  return r;
}
static ux arrEltsFromLen(ux n, FFICompoundType* ct) {
  B elt = ct->a[0].o;
  if (isFFIPrim(elt) && ffiPrimConv(elt)!=sty_void) {
    n = n << sty_lb(ffiPrimConv(elt)) >> sty_lb(ffiPrimTy(elt));
  }
  return n;
}



typedef struct FFIFn {
  struct NFn;
  void* sym;
  u32 scratchMemSize;
  u32 argListOffset;
  u32 argCount;
  bool gprSysv; // if GPR6_SYSV64, for bypassing ffi_call for simple cases
  FFIEnt* argData; // c(FFICompoundType, this->obj)->a+1; has argCount elements
  B retObj; // unowned copy of c(FFICompoundType, this->obj)->a[0].o
  i32 wLen, xLen;
  ffi_cif cif;
  ffi_type* cif_args[];
} FFIFn;
static NFn* m_ffiFn(ux size, NFnDesc* desc, B obj, FC1 c1, FC2 c2);



static B m_ptrobj(uintptr_t ptr, B ty, ux stride);
static B m_ptrobj_s(uintptr_t ptr, B o);
static ffi_type* foreignType(B t);
static ux foreignSize(B t);

static B ptrobj_checkget(B x, u8 couldAlsoBe); // doesn't consume
static bool ty_compat(B a, B b);
static FFICompoundType* ptrh_self(B n) { return c(FFICompoundType, n); }
static B ptrh_elt(B n) { return ptrh_self(n)->a[0].o; }
static uintptr_t ptrh_ptr(B n) { return ptrh_self(n)->ptrh.ptr; }
static ux ptrh_stride(B n) { return ptrh_self(n)->ptrh.stride; }



typedef struct ParseContext {
  B* xp;
  ux scratchMemSize; // only initialized for function processing, and will just cycle through garbage for other things
  u32* currOff;
  u32* currEnd;
  u32 xia, curr; // if xia>=2, •FFI; else, xia==1 and curr can be uninitialized
} ParseContext;
typedef struct ArgParseState {
  ParseContext pc;
  bool mayNeedTmpBufs; // whether tmpBufs in ScratchMemAlloc is necessary
  B* mutList; // list of &T / ⥊T FFICompoundType values
  u8 kind; // 0: arg; 1: result type; 2: cast
  
  // state mutated in parseFFIType0:
  bool argMayRead; // whether any ⥊T / &T is present in this arg, and as such needs a pass afterwards for computing its offset
  bool outermost; // if true, arrays are pointers instead of inline structs
  bool needFull; // if true, stored ffi_type is necessary (i.e. will be used by the final ffi_cif); if not, values will only be read/written by CBQN code
  bool allowMut; // whether &T / ⥊T are allowed
} ArgParseState;

typedef struct { u32* start; u32* end; } U32Span;
static NOINLINE U32Span toC32Null(B* xp, bool acceptEmpty) { // assumes *xp is already a string
  ux ia = IA(*xp);
  u32* src;
  B x = m_c32arrv(&src, ia+1);
  if (ia==0) {
    if (!acceptEmpty) thrM("FFI type: Type string cannot be empty");
    src[0] = '\0';
  } else {
    if (!elChr(TI(*xp,elType))) *xp = squeeze_chrOut(*xp);
    src[ia] = '\0';
    COPY_TO(src, el_c32, 0, *xp, 0, ia);
  }
  
  decG(*xp);
  *xp = x;
  return (U32Span) {src, src+ia};
}
static B parseFFIType(ArgParseState* st, u8 kind, U32Span span);
B vfyStr(B x, char* name, char* arg);



typedef struct ScratchMemAlloc ScratchMemAlloc;
typedef union { ScratchMemAlloc* objPtr; ux* ptrObjMeta; } ScratchMem;
#define AT_SM(T, OFF) ((T*)(sm.objPtr->rest + (OFF)))
#define SM_NONE ((ScratchMem){.objPtr = NULL})
#define SM_IS_NONE (sm.objPtr==NULL)
#define SM_PTROBJ_META(N) ((ScratchMem){.ptrObjMeta = (N)})
static NOINLINE B foreignToBQN(void* mem, ScratchMem sm, B type);
static NOINLINE B foreignCtyToBQN(void* mem, ScratchMem sm, FFICompoundType* ct);
static NOINLINE void foreignMemFromBQN(ScratchMem sm, void* mem, B type, B x);
static NOINLINE void foreignMemWriteHArray(void* mem, B el, B x, ux ia, ux stride);
static NOINLINE void foreignMemWriteArray(void* mem, B el, B x);
static NOINLINE B ffiFn_core(FFIFn* f, B* wp, B* xp, B x);
