#include "core.h"

#if FFI && !defined(CBQN_EXPORT)
  #error "Expected CBQN_EXPORT if FFI is defined"
#endif

#ifndef FFI_CHECKS // option to disable extended correctness checks
  #define FFI_CHECKS 1
#endif

#if CBQN_EXPORT
  #if defined(_WIN32) || defined(_WIN64)
    #define BQN_EXP __attribute__((__visibility__("default"))) __declspec(dllexport)
  #else
    #define BQN_EXP __attribute__((__visibility__("default")))
  #endif
  #include "../include/bqnffi.h"
  #include "utils/calls.h"
  #include "utils/cstr.h"
  #include "nfns.h"
  #include "ns.h"
  #include "utils/file.h"
// ..continuing under "#if CBQN_EXPORT"

// base interface defs for when GC stuff needs to be added in
static B getB(BQNV v) {
  return b(v);
}
static BQNV makeX(B x) {
  return x.u;
}

BQN_EXP void bqn_free(BQNV v) {
  dec(getB(v));
}
BQN_EXP BQNV bqn_copy(BQNV v) {
  return makeX(inc(getB(v)));
}

static void freeTagged(BQNV v) { }

#define DIRECT_BQNV 1

BQN_EXP double   bqn_toF64 (BQNV v) { double   r = o2fG(getB(v)); freeTagged(v); return r; }
BQN_EXP uint32_t bqn_toChar(BQNV v) { uint32_t r = o2cG(getB(v)); freeTagged(v); return r; }
BQN_EXP double   bqn_readF64 (BQNV v) { return o2fG(getB(v)); }
BQN_EXP uint32_t bqn_readChar(BQNV v) { return o2cG(getB(v)); }

BQN_EXP void bqn_init() {
  cbqn_init();
}

B type_c1(B t, B x);
static i32 typeInt(B x) { // doesn't consume
  return o2i(C1(type, inc(x)));
}
BQN_EXP int bqn_type(BQNV v) {
  return typeInt(getB(v));
}

BQN_EXP BQNV bqn_call1(BQNV f, BQNV x) {
  return makeX(c1(getB(f), inc(getB(x))));
}
BQN_EXP BQNV bqn_call2(BQNV f, BQNV w, BQNV x) {
  return makeX(c2(getB(f), inc(getB(w)), inc(getB(x))));
}

BQN_EXP BQNV bqn_eval(BQNV src) {
  return makeX(bqn_exec(inc(getB(src)), bi_N));
}
BQN_EXP BQNV bqn_evalCStr(const char* str) {
  return makeX(bqn_exec(utf8Decode0(str), bi_N));
}


BQN_EXP size_t bqn_bound(BQNV a) { return IA(getB(a)); }
BQN_EXP size_t bqn_rank(BQNV a) { return RNK(getB(a)); }
BQN_EXP void bqn_shape(BQNV a, size_t* buf) { B b = getB(a);
  ur r = RNK(b);
  usz* sh = SH(b);
  vfor (usz i = 0; i < r; i++) buf[i] = sh[i];
}
BQN_EXP BQNV bqn_pick(BQNV a, size_t pos) {
  return makeX(IGet(getB(a),pos));
}

// TODO copy directly with some mut.h thing
BQN_EXP void bqn_readI8Arr (BQNV a, i8*   buf) { B c = toI8Any (incG(getB(a))); memcpy(buf, i8any_ptr (c), IA(c) * 1); decG(c); }
BQN_EXP void bqn_readI16Arr(BQNV a, i16*  buf) { B c = toI16Any(incG(getB(a))); memcpy(buf, i16any_ptr(c), IA(c) * 2); decG(c); }
BQN_EXP void bqn_readI32Arr(BQNV a, i32*  buf) { B c = toI32Any(incG(getB(a))); memcpy(buf, i32any_ptr(c), IA(c) * 4); decG(c); }
BQN_EXP void bqn_readF64Arr(BQNV a, f64*  buf) { B c = toF64Any(incG(getB(a))); memcpy(buf, f64any_ptr(c), IA(c) * 8); decG(c); }
BQN_EXP void bqn_readC8Arr (BQNV a, u8*   buf) { B c = toC8Any (incG(getB(a))); memcpy(buf, c8any_ptr (c), IA(c) * 1); decG(c); }
BQN_EXP void bqn_readC16Arr(BQNV a, u16*  buf) { B c = toC16Any(incG(getB(a))); memcpy(buf, c16any_ptr(c), IA(c) * 2); decG(c); }
BQN_EXP void bqn_readC32Arr(BQNV a, u32*  buf) { B c = toC32Any(incG(getB(a))); memcpy(buf, c32any_ptr(c), IA(c) * 4); decG(c); }
BQN_EXP void bqn_readObjArr(BQNV a, BQNV* buf) { B b = getB(a);
  usz ia = IA(b);
  if (DIRECT_BQNV && sizeof(BQNV)==sizeof(B)) {
    COPY_TO(buf, el_B, 0, b, 0, ia);
  } else {
    B* p = arr_bptr(b);
    if (p!=NULL) {
      for (usz i = 0; i < ia; i++) buf[i] = makeX(inc(p[i]));
    } else {
      SGet(b)
      for (usz i = 0; i < ia; i++) buf[i] = makeX(Get(b, i));
    }
  }
}

BQN_EXP bool bqn_hasField(BQNV ns, BQNV name) {
  return !q_N(ns_getNU(getB(ns), getB(name), false));
}
BQN_EXP BQNV bqn_getField(BQNV ns, BQNV name) {
  return makeX(inc(ns_getNU(getB(ns), getB(name), true)));
}

BQN_EXP BQNV bqn_makeF64(double d) { return makeX(m_f64(d)); }
BQN_EXP BQNV bqn_makeChar(uint32_t c) { return makeX(m_c32(c)); }


static usz calcIA(size_t rank, const size_t* shape) {
  if (rank>UR_MAX) thrM("Rank too large");
  usz r = 1;
  PLAINLOOP for (size_t i = 0; i < rank; i++) if (mulOn(r, shape[i])) thrM("Size too large");
  return r;
}
static void copyBData(B* r, const BQNV* data, usz ia) {
  for (size_t i = 0; i < ia; i++) {
    BQNV c = data[i];
    #if DIRECT_BQNV
    r[i] = getB(c);
    #else
    r[i] = inc(getB(c));
    bqn_free(c);
    #endif
  }
}
#define CPYSH(R) usz* sh = arr_shAlloc((Arr*)(R), r0); \
  if (sh) PLAINLOOP for (size_t i = 0; RARE(i < r0); i++) sh[i] = sh0[i];

BQN_EXP BQNV bqn_makeI8Arr (size_t r0, const size_t* sh0, const i8*   data) { usz ia=calcIA(r0,sh0); i8*  rp; Arr* r = m_i8arrp (&rp,ia); CPYSH(r); memcpy(rp,data,ia*1); return makeX(taga(r)); }
BQN_EXP BQNV bqn_makeI16Arr(size_t r0, const size_t* sh0, const i16*  data) { usz ia=calcIA(r0,sh0); i16* rp; Arr* r = m_i16arrp(&rp,ia); CPYSH(r); memcpy(rp,data,ia*2); return makeX(taga(r)); }
BQN_EXP BQNV bqn_makeI32Arr(size_t r0, const size_t* sh0, const i32*  data) { usz ia=calcIA(r0,sh0); i32* rp; Arr* r = m_i32arrp(&rp,ia); CPYSH(r); memcpy(rp,data,ia*4); return makeX(taga(r)); }
BQN_EXP BQNV bqn_makeF64Arr(size_t r0, const size_t* sh0, const f64*  data) { usz ia=calcIA(r0,sh0); f64* rp; Arr* r = m_f64arrp(&rp,ia); CPYSH(r); memcpy(rp,data,ia*8); return makeX(taga(r)); }
BQN_EXP BQNV bqn_makeC8Arr (size_t r0, const size_t* sh0, const u8*   data) { usz ia=calcIA(r0,sh0); u8*  rp; Arr* r = m_c8arrp (&rp,ia); CPYSH(r); memcpy(rp,data,ia*1); return makeX(taga(r)); }
BQN_EXP BQNV bqn_makeC16Arr(size_t r0, const size_t* sh0, const u16*  data) { usz ia=calcIA(r0,sh0); u16* rp; Arr* r = m_c16arrp(&rp,ia); CPYSH(r); memcpy(rp,data,ia*2); return makeX(taga(r)); }
BQN_EXP BQNV bqn_makeC32Arr(size_t r0, const size_t* sh0, const u32*  data) { usz ia=calcIA(r0,sh0); u32* rp; Arr* r = m_c32arrp(&rp,ia); CPYSH(r); memcpy(rp,data,ia*4); return makeX(taga(r)); }
BQN_EXP BQNV bqn_makeObjArr(size_t r0, const size_t* sh0, const BQNV* data) { usz ia=calcIA(r0,sh0); HArr_p r = m_harrUp(ia); copyBData(r.a,data,ia); NOGC_E; CPYSH(r.c); return makeX(r.b); }

BQN_EXP BQNV bqn_makeI8Vec (size_t len, const i8*   data) { i8*  rp; B r = m_i8arrv (&rp,len); memcpy(rp,data,len*1); return makeX(r); }
BQN_EXP BQNV bqn_makeI16Vec(size_t len, const i16*  data) { i16* rp; B r = m_i16arrv(&rp,len); memcpy(rp,data,len*2); return makeX(r); }
BQN_EXP BQNV bqn_makeI32Vec(size_t len, const i32*  data) { i32* rp; B r = m_i32arrv(&rp,len); memcpy(rp,data,len*4); return makeX(r); }
BQN_EXP BQNV bqn_makeF64Vec(size_t len, const f64*  data) { f64* rp; B r = m_f64arrv(&rp,len); memcpy(rp,data,len*8); return makeX(r); }
BQN_EXP BQNV bqn_makeC8Vec (size_t len, const u8*   data) { u8*  rp; B r = m_c8arrv (&rp,len); memcpy(rp,data,len*1); return makeX(r); }
BQN_EXP BQNV bqn_makeC16Vec(size_t len, const u16*  data) { u16* rp; B r = m_c16arrv(&rp,len); memcpy(rp,data,len*2); return makeX(r); }
BQN_EXP BQNV bqn_makeC32Vec(size_t len, const u32*  data) { u32* rp; B r = m_c32arrv(&rp,len); memcpy(rp,data,len*4); return makeX(r); }
BQN_EXP BQNV bqn_makeObjVec(size_t len, const BQNV* data) { HArr_p r = m_harrUv(len); copyBData(r.a,data,len); NOGC_E;return makeX(r.b); }
BQN_EXP BQNV bqn_makeUTF8Str(size_t len, const char* str) { return makeX(utf8Decode(str, len)); }

typedef struct BoundFn {
  struct NFn;
  void* w_c1;
  void* w_c2;
  #if FFI==2
    i32 mutCount;
    i32 wLen; // 0: not needed; -1: is whole array; ≥1: length
    i32 xLen; // 0: length 0 array; else, ↑
  #endif
} BoundFn;
STATIC_GLOBAL NFnDesc* boundFnDesc;
STATIC_GLOBAL NFnDesc* foreignFnDesc;

B boundFn_c1(B t,      B x) { BoundFn* c = c(BoundFn,t); return getB(((bqn_boundFn1)c->w_c1)(makeX(inc(c->obj)),           makeX(x))); }
B boundFn_c2(B t, B w, B x) { BoundFn* c = c(BoundFn,t); return getB(((bqn_boundFn2)c->w_c2)(makeX(inc(c->obj)), makeX(w), makeX(x))); }

typedef BQNV (*bqn_foreignFn1)(BQNV x);
typedef BQNV (*bqn_foreignFn2)(BQNV w, BQNV x);
B directFn_c1(B t,      B x) { BoundFn* c = c(BoundFn,t); return getB(((bqn_foreignFn1)c->w_c1)(          makeX(x))); }
B directFn_c2(B t, B w, B x) { BoundFn* c = c(BoundFn,t); return getB(((bqn_foreignFn2)c->w_c2)(makeX(w), makeX(x))); }

static B m_ffiFn(NFnDesc* desc, B obj, FC1 c1, FC2 c2, void* wc1, void* wc2) {
  BoundFn* r = mm_alloc(sizeof(BoundFn), t_nfn);
  nfn_lateInit((NFn*)r, desc);
  r->obj = obj;
  r->c1 = c1;
  r->c2 = c2;
  r->w_c1 = wc1;
  r->w_c2 = wc2;
  return tag(r, FUN_TAG);
}
BQN_EXP BQNV bqn_makeBoundFn1(bqn_boundFn1 f, BQNV obj) { return makeX(m_ffiFn(boundFnDesc, inc(getB(obj)), boundFn_c1, c2_bad, f, NULL)); }
BQN_EXP BQNV bqn_makeBoundFn2(bqn_boundFn2 f, BQNV obj) { return makeX(m_ffiFn(boundFnDesc, inc(getB(obj)), c1_bad, boundFn_c2, NULL, f)); }

const static u8 typeMap[] = {
  [el_bit] = elt_unk,
  [el_B  ] = elt_unk,
  [el_i8 ] = elt_i8,  [el_c8 ] = elt_c8,
  [el_i16] = elt_i16, [el_c16] = elt_c16,
  [el_i32] = elt_i32, [el_c32] = elt_c32,
  [el_f64] = elt_f64,
};
BQN_EXP BQNElType bqn_directArrType(BQNV a) {
  B b = getB(a);
  if (!isArr(b)) return elt_unk;
  return typeMap[TI(b,elType)];
}
BQN_EXP const i8*  bqn_directI8 (BQNV a) { return i8any_ptr (getB(a)); }
BQN_EXP const i16* bqn_directI16(BQNV a) { return i16any_ptr(getB(a)); }
BQN_EXP const i32* bqn_directI32(BQNV a) { return i32any_ptr(getB(a)); }
BQN_EXP const f64* bqn_directF64(BQNV a) { return f64any_ptr(getB(a)); }
BQN_EXP const u8*  bqn_directC8 (BQNV a) { return c8any_ptr (getB(a)); }
BQN_EXP const u16* bqn_directC16(BQNV a) { return c16any_ptr(getB(a)); }
BQN_EXP const u32* bqn_directC32(BQNV a) { return c32any_ptr(getB(a)); }

void ffiFn_visit(Value* v) { mm_visit(((BoundFn*)v)->obj); }
DEF_FREE(ffiFn) { dec(((BoundFn*)x)->obj); }
#endif // #if CBQN_EXPORT





#if FFI
  #if FFI!=2
    #error "Only FFI=0 and FFI=2 are supported"
  #endif
  #if !__has_include(<ffi.h>)
    #error "<ffi.h> not found. Either install libffi, or add 'FFI=0' as a make argument to disable •FFI"
  #endif
  #include <dlfcn.h>
  #include <ffi.h>
  #include "utils/mut.h"
// ..continuing under "#if FFI"

typedef struct BQNFFIEnt {
  B o;
  union {
    void* ptrh_ptr; // cty_ptrh
    u64 structOffset; // cty_struct, cty_starr
    struct { bool wholeArg, isMutated, resSingle, onW; u32 staticOffset; }; // cty_arglist, DecoratedType
  };
} BQNFFIEnt;
typedef struct BQNFFIType {
  struct Value;
  u8 ty;
  usz ia; // num0ber of entries in 'a', each with refcounted .o
  union {
    u32 staticAllocTotal; // cty_arglist
    struct { u32 structSize; }; // cty_struct, cty_starr
    struct { u32 arrCount; }; // cty_tlarr
    struct { bool mutPtr; }; // cty_ptr
    struct { u8 reType, reWidth; }; // cty_repr
    u32 ptrh_stride; // cty_ptrh
  };
  BQNFFIEnt a[];
} BQNFFIType;
STATIC_GLOBAL B ty_voidptr;

B vfyStr(B x, char* name, char* arg);
static u32 styG(B x) {
  assert(isC32(x));
  return o2cG(x);
}

#if FFI==2

#if FFI_CHECKS
  static B nonNumber(B x) { // returns m_f64(0) if x is all numeric, otherwise •Type of the offending element
    if (elNum(TI(x,elType))) return m_f64(0);
    usz ia = IA(x); SGetU(x)
    for (ux i = 0; i < ia; i++) {
      B c = GetU(x,i);
      if (!isNum(c)) return inc(c);
    }
    return m_f64(0);
  }
  static NOINLINE void ffi_checkRange(B x, i32 mode, char* desc, i64 min, i64 max) { // doesn't consume; assumes non-array has already been checked for; if min==max, doesn't check range
    if (IA(x)==0) return;
    
    char* ref = mode==1? "&" : mode==0? "*" : ":";
    B nonNum = nonNumber(x);
    if (nonNum.u!=m_f64(0).u) thrF("FFI: Array provided for %S%S contained %S", ref, desc, genericDesc(nonNum));
    
    if (min==max) return;
    incG(x); x = elNum(TI(x,elType))? x : taga(cpyF64Arr(x));
    
    i64 buf[2];
    if (!getRange_fns[TI(x,elType)](tyany_ptr(x), buf, IA(x))) thrF("FFI: Array provided for %S%S contained non-integer", ref, desc);
    if (buf[0]<min) thrF("FFI: Array provided for %S%S contained %l", ref, desc, buf[0]);
    if (buf[1]>max) thrF("FFI: Array provided for %S%S contained %l", ref, desc, buf[1]);
    decG(x);
  }
#else
  static void ffi_checkRange(B x, i32 mode, char* desc, i64 min, i64 max) { }
#endif


enum ScalarTy {
  sty_void, sty_a,
  sty_u8, sty_u16, sty_u32, sty_u64,
  sty_i8, sty_i16, sty_i32, sty_i64,
  sty_f32, sty_f64
};
static const u8 sty_w[] = {
  [sty_void]=0, [sty_a]=sizeof(BQNV),
  [sty_u8]=1, [sty_u16]=2, [sty_u32]=4, [sty_u64]=8,
  [sty_i8]=1, [sty_i16]=2, [sty_i32]=4, [sty_i64]=8,
  [sty_f32]=4, [sty_f64]=8
};
static char* const sty_names[] = {
  [sty_void]="void", [sty_a]="a",
  [sty_u8]="u8", [sty_u16]="u16", [sty_u32]="u32", [sty_u64]="u64",
  [sty_i8]="i8", [sty_i16]="i16", [sty_i32]="i32", [sty_i64]="i64",
  [sty_f32]="f32", [sty_f64]="f64"
};
enum CompoundTy {
  cty_ptr, // *... / &...; .a = {{.o=eltype}}, .mutPtr = startsWith("&")
  cty_tlarr, // top-level array; {{.o=eltype}}, .arrCount=n
  cty_struct, // {...}; .a = {{.o=el0,structOffset=0}, {.o=el1,structOffset=n}, ..., {.o=elLast,structOffset=123}, {.o = taga(ffi_type**,OBJ_TYPE)}}
  cty_starr, // struct-based array; same as cty_struct
  cty_repr, // something:type; {{.o=el}}, .reType=one of "icuf", .reWidth=log bit width of retype
  cty_ptrh, // data holder for pointer objects; {{.o=eltype, .ptrh_ptr}, .ptrh_stride
  cty_arglist, // top-level object holding argument & result data; {res, arg0, arg1, ...}
};

static NOINLINE B m_bqnFFIType(BQNFFIType** rp, u8 ty, usz ia) {
  BQNFFIType* r = mm_alloc(fsizeof(BQNFFIType, a, BQNFFIEnt, ia), t_ffiType);
  r->ty = ty;
  r->ia = ia;
  memset(r->a, 0, ia*sizeof(BQNFFIEnt));
  *rp = r;
  return tag(r, OBJ_TAG);
}

static u32 readUInt(u32** p) {
  u32* c = *p;
  u32 r = 0;
  while (*c>='0' & *c<='9') {
    if (r >= U32_MAX/10 - 10) thrM("Type parser: number literal too large");
    r = r*10 + *c-'0';
    c++;
  }
  if (c == *p) thrM("Type parser: expected number");
  *p = c;
  return r;
}
static ffi_type storeStructOffsets(BQNFFIType* rp, ffi_type** list, ux n) {
  ffi_type rt;
  rt.alignment = rt.size = 0;
  rt.type = FFI_TYPE_STRUCT;
  rt.elements = list;
  
  TALLOC(size_t, offsets, n);
  if (ffi_get_struct_offsets(FFI_DEFAULT_ABI, &rt, offsets) != FFI_OK) thrM("Type parser: Failed getting struct offsets");
  PLAINLOOP for (usz i = 0; i < n; i++) rp->a[i].structOffset = offsets[i];
  rp->structSize = rt.size;
  TFREE(offsets);
  
  return rt;
}
#define ALLOC_TYPE_LIST(NELTS, NLIST) \
  ux alloc_elsSz_ = sizeof(ffi_type)*(NELTS); \
  TAlloc* ao = ARBOBJ(alloc_elsSz_ + sizeof(ffi_type*) * ((NLIST)+1)); \
  ffi_type* elts = (ffi_type*)ao->data; \
  ffi_type** list = (ffi_type**)(alloc_elsSz_ + (char*)ao->data); \
  list[NLIST] = NULL;

typedef struct ParsedType {
  B o;
  ffi_type ffitype;
  bool canRetype, isMutated;
} ParsedType;
ParsedType ffi_parseType(u32** src, bool inPtr, bool top) { // parse actual type
  u32* c = *src;
  u32 c0 = *c++;
  ffi_type rt;
  B ro;
  bool parseRepr=false, canRetype=false, isMutated=false;
  u32 myWidth = 0; // used if parseRepr
  switch (c0) {
    default:
      if (c0 == '\0') thrF("Type parser: Unexpected end of input");
      thrF("Type parser: Unexpected character '%c'", c0);
    case '[': {
      u32 n = readUInt(&c);
      if (*c++!=']') thrM("Type parser: Bad array type");
      if (n==0) thrM("Type parser: 0-item arrays not supported");
      ParsedType e = ffi_parseType(&c, top, false);
      
      if (top) {
        myWidth = sizeof(void*);
        BQNFFIType* rp; ro = m_bqnFFIType(&rp, cty_tlarr, 1);
        rp->a[0].o = e.o;
        if (n>U32_MAX) thrM("Type parser: Top-level array too large");
        rp->arrCount = n;
        parseRepr = e.canRetype;
        rt = ffi_type_pointer;
      } else {
        BQNFFIType* rp; ro = m_bqnFFIType(&rp, cty_starr, n+1); // +1 for ARBOBJ
        
        ALLOC_TYPE_LIST(1, n);
        rp->a[n].o = tag(ao, OBJ_TAG);
        *elts = e.ffitype;
        PLAINLOOP for (usz i = 0; i < n; i++) {
          list[i] = elts;
          rp->a[i].o = e.o;
        }
        incBy(e.o, n-1);
        
        rt = storeStructOffsets(rp, list, n);
      }
      break;
    }
    
    case 'i': case 'u': case 'f': {
      u32 n = readUInt(&c);
      if (c0=='f') {
        if      (n==32) rt = ffi_type_float;
        else if (n==64) rt = ffi_type_double;
        else thrM("Type parser: Bad float width");
        ro = m_c32(n==32? sty_f32 : sty_f64);
      } else {
        u32 scty;
        if      (n== 8) { scty = c0=='i'? sty_i8  : sty_u8;  rt = c0=='i'? ffi_type_sint8  : ffi_type_uint8;  }
        else if (n==16) { scty = c0=='i'? sty_i16 : sty_u16; rt = c0=='i'? ffi_type_sint16 : ffi_type_uint16; }
        else if (n==32) { scty = c0=='i'? sty_i32 : sty_u32; rt = c0=='i'? ffi_type_sint32 : ffi_type_uint32; }
        else if (n==64) { scty = c0=='i'? sty_i64 : sty_u64; rt = c0=='i'? ffi_type_sint64 : ffi_type_uint64; }
        else thrM("Type parser: Bad integer width");
        ro = m_c32(scty);
      }
      parseRepr = !inPtr; myWidth = sty_w[styG(ro)];
      canRetype = inPtr;
      break;
    }
    
    case 'a': {
      ro = m_c32(sty_a);
      assert(sizeof(BQNV)==8); // if changed, change the below ffi_type_uint64 too
      rt = ffi_type_uint64;
      break;
    }
    
    case '*': case '&': {
      myWidth = sizeof(void*);
      if (c0=='*' && (0==*c || ':'==*c || '}'==*c || ','==*c)) {
        ro = incG(ty_voidptr);
        parseRepr = !inPtr;
        canRetype = inPtr;
      } else {
        isMutated = c0=='&';
        if (isMutated && !top) thrM("Type parser: '&' currently can only be used as the outermost type");
        ParsedType e = ffi_parseType(&c, true, false);
        
        BQNFFIType* rp; ro = m_bqnFFIType(&rp, cty_ptr, 1);
        rp->a[0].o = e.o;
        rp->mutPtr = isMutated;
        
        parseRepr = e.canRetype;
      }
      rt = ffi_type_pointer;
      break;
    }
    
    case '{': {
      TSALLOC(ParsedType, es, 4);
      while (true) {
        TSADD(es, ffi_parseType(&c, false, false));
        u32 m = *c++;
        if (m=='}') break;
        if (m!=',') thrM("Type parser: Improper struct separator or end");
      }
      usz n = TSSIZE(es);
      
      BQNFFIType* rp; ro = m_bqnFFIType(&rp, cty_struct, n+1); // +1 for ARBOBJ
      
      ALLOC_TYPE_LIST(n, n);
      rp->a[n].o = tag(ao, OBJ_TAG);
      PLAINLOOP for (usz i = 0; i < n; i++) {
        rp->a[i].o = es[i].o;
        elts[i] = es[i].ffitype;
        list[i] = &elts[i];
      }
      TSFREE(es);
      
      rt = storeStructOffsets(rp, list, n);
      break;
    }
  }
  if (parseRepr && *c==':') {
    c++;
    u8 t = *c++;
    u32 n = readUInt(&c);
    if (t=='i' || t=='c') {
      if (n!=8 & n!=16 & n!=32) { badW: thrF("Type parser: Unsupported width in :%c%i", (u32)t, n); }
    } else if (t=='u') {
      if (n!=1) goto badW;
    } else if (t=='f') {
      if (n!=64) goto badW;
    } else thrM("Type parser: Unexpected character after \":\"");
    
    if (isC32(ro) && n > myWidth*8) thrF("Type parser: Representation wider than the value within", sty_names[styG(ro)], (u32)t, n);
    // TODO figure out what to do with i32:i32 etc
    
    B roP = ro;
    BQNFFIType* rp; ro = m_bqnFFIType(&rp, cty_repr, 1);
    rp->reType = t;
    rp->reWidth = 63-CLZ(n);
    rp->a[0].o = roP;
  }
  *src = c;
  return (ParsedType){.ffitype=rt, .o=ro, .canRetype=canRetype, .isMutated=isMutated};
}
static NOINLINE u32* parseType_pre(B arg, ux ia) { // doesn't consume; arg can be freed immediately after
  TALLOC(u32, xp, ia+1);
  xp[ia] = 0;
  COPY_TO(xp, el_c32, 0, arg, 0, ia);
  return xp;
}
static NOINLINE void parseType_post(u32* xp0, u32* xp, usz ia) {
  if (xp0+ia != xp) thrM("Type parser: Garbage at end of type");
  TFREE(xp0);
}

typedef struct {
  BQNFFIEnt ent;
  ffi_type ffitype;
} DecoratedType;
DecoratedType ffi_parseDecoratedType(B arg, bool forRes) { // doesn't consume; parse argument side & other global decorators
  vfyStr(arg, "FFI", "type");
  usz ia = IA(arg);
  if (ia==0) {
    if (!forRes) thrM("Type parser: Type was empty");
    return (DecoratedType){{.o=m_c32(sty_void), .resSingle=false}, .ffitype = ffi_type_void};
  }
  arg = chr_squeezeChk(incG(arg));
  
  u32* xp0 = parseType_pre(arg, ia);
  u32* xp = xp0;
  
  DecoratedType t;
  if (forRes && xp[0]=='&' && xp[1]=='\0') {
    t = (DecoratedType){{.o=m_c32(sty_void), .resSingle=true}, .ffitype = ffi_type_void};
    xp+= 1;
  } else {
    u8 side = 0;
    bool whole = false;
    while (true) {
      if      (*xp == U'𝕩') { if (side) thrM("FFI: Multiple occurrences of argument side specified"); side = 1; }
      else if (*xp == U'𝕨') { if (side) thrM("FFI: Multiple occurrences of argument side specified"); side = 2; }
      else if (*xp == U'>') { if (whole) thrM("FFI: Multiple occurrences of '>' within one argument"); whole = true; }
      else break;
      xp++;
    }
    if (forRes && side) thrM("FFI: Argument side cannot be specified for the result");
    if (side) side--;
    else side = 0;
    
    ParsedType e = ffi_parseType(&xp, false, true);
    t = (DecoratedType) {
      {.o=e.o, .onW=side, .isMutated=e.isMutated, .wholeArg=whole, .resSingle=false},
      .ffitype = e.ffitype
    };
  }
  parseType_post(xp0, xp, ia);
  
  decG(arg);
  return t;
}

static usz ffiTmpAlign(usz n) {
  u64 align = _Alignof(max_align_t);
  n = (n+align-1) & ~(align-1);
  return n;
}

static NOINLINE B toW(u8 reT, u8 reW, B x) {
  switch(reW) { default: UD;
    case 0:               ffi_checkRange(x, 2, "u1", 0, 1);              return taga(toBitArr(x)); break;
    case 3: if (reT=='i') ffi_checkRange(x, 2, "i8",  I8_MIN,  I8_MAX);  return reT=='c'?  toC8Any(x) :  toI8Any(x); break;
    case 4: if (reT=='i') ffi_checkRange(x, 2, "i16", I16_MIN, I16_MAX); return reT=='c'? toC16Any(x) : toI16Any(x); break;
    case 5: if (reT=='i') ffi_checkRange(x, 2, "i32", I32_MIN, I32_MAX); return reT=='c'? toC32Any(x) : toI32Any(x); break;
    case 6:               ffi_checkRange(x, 2, "f64", 0, 0);             return toF64Any(x); break;
  }
}

FORCE_INLINE u64 i64abs(i64 x) { return x<0?-x:x; }

#define CPY_UNSIGNED(REL, UEL, DIRECT, WIDEN, WEL) \
  if (TI(x,elType)<=el_##REL) return taga(DIRECT(x)); \
  usz ia = IA(x);                                 \
  B t = WIDEN(x); WEL* tp = WEL##any_ptr(t);      \
  REL* rp; B r = m_##REL##arrv(&rp, ia);          \
  vfor (usz i=0; i<ia; i++) ((UEL*)rp)[i] = tp[i];\
  decG(t); return r;

// copy elements of x to array of unsigned integers (using a signed integer array type as a "container"); consumes argument
// undefined behavior if x contains a number outside the respective unsigned range (incl. any negative numbers)
NOINLINE B cpyU32Bits(B x) { CPY_UNSIGNED(i32, u32, cpyI32Arr, toF64Any, f64) }
NOINLINE B cpyU16Bits(B x) { CPY_UNSIGNED(i16, u16, cpyI16Arr, toI32Any, i32) }
NOINLINE B cpyU8Bits(B x)  { CPY_UNSIGNED(i8,  u8,  cpyI8Arr,  toI16Any, i16) }

NOINLINE B cpyF32Bits(B x) { // copy x to a 32-bit float array (using an i32arr as a "container"). Unspecified rounding for numbers that aren't exactly floats, and undefined behavior on non-numbers
  usz ia = IA(x);
  B t = toF64Any(x); f64* tp = f64any_ptr(t);
  i32* rp; B r = m_i32arrv(&rp, ia);
  vfor (usz i=0; i<ia; i++) ((f32*)rp)[i]=tp[i];
  dec(t); return r;
}

// versions of cpyU(8|16|32)Bits, but without a guarantee of returning a new array; consumes argument
static B toU32Bits(B x) { return TI(x,elType)==el_i32? x : cpyU32Bits(x); }
static B toU16Bits(B x) { return TI(x,elType)==el_i16? x : cpyU16Bits(x); }
static B toU8Bits(B x)  { return TI(x,elType)==el_i8?  x : cpyU8Bits(x); }

// read x as the specified type (assuming a container of the respective width signed integer array); consumes x
NOINLINE B readU8Bits(B x)  { usz ia=IA(x); u8*  xp=tyarr_ptr(x); i16* rp; B r=m_i16arrv(&rp, ia); vfor (usz i=0; i<ia; i++) rp[i]=xp[i]; return num_squeeze(r); }
NOINLINE B readU16Bits(B x) { usz ia=IA(x); u16* xp=tyarr_ptr(x); i32* rp; B r=m_i32arrv(&rp, ia); vfor (usz i=0; i<ia; i++) rp[i]=xp[i]; return num_squeeze(r); }
NOINLINE B readU32Bits(B x) { usz ia=IA(x); u32* xp=tyarr_ptr(x); f64* rp; B r=m_f64arrv(&rp, ia); vfor (usz i=0; i<ia; i++) rp[i]=xp[i]; return num_squeeze(r); }
NOINLINE B readF32Bits(B x) { usz ia=IA(x); f32* xp=tyarr_ptr(x); f64* rp; B r=m_f64arrv(&rp, ia); vfor (usz i=0; i<ia; i++) rp[i]=xp[i]; return r; }
B m_ptrobj_s(void* ptr, B o); // consumes o, sets stride to size of o
B m_ptrobj(void* ptr, B o, ux stride); // consumes o
static NOINLINE B ptrobj_checkget(B x); // doesn't consume
static bool ty_equal(B a, B b);
static B ptrh_type(B n); // returns cty_ptr object
static void* ptrh_ptr(B n);

static NOINLINE void ty_fmt_add(B* s0, B o) {
  B s = *s0;
  if (isC32(o)) {
    A8(sty_names[styG(o)]);
  } else {
    BQNFFIType* t = c(BQNFFIType, o);
    switch (t->ty) {
      default:
        A8("???");
        break;
      case cty_ptr:
        ACHR(t->mutPtr? '&' : '*');
        ty_fmt_add(&s, t->a[0].o);
        break;
      case cty_starr:
        AFMT("[%i]", t->ia-1);
        ty_fmt_add(&s, t->a[0].o);
        break;
      case cty_tlarr:
        AFMT("[%i]", t->arrCount);
        ty_fmt_add(&s, t->a[0].o);
        break;
      case cty_struct:
        A8("{...}");
        break;
      case cty_repr:
        ty_fmt_add(&s, t->a[0].o);
        AFMT(":%c%i", (u32)t->reType, (i32)(1 << t->reWidth));
        break;
    }
  }
  *s0 = s;
}
static NOINLINE B ty_fmt(B o) {
  B s = emptyCVec();
  ACHR('"');
  ty_fmt_add(&s, o);
  ACHR('"');
  return s;
}



static void genObj_writePtr(void* res, B c, B expEl, B* sourceObjs) {
  B h = ptrobj_checkget(c);
  #if FFI_CHECKS
    if (!ty_equal(ptrh_type(h), expEl)) thrF("FFI: Pointer object type isn't compatible with argument type");
  #endif
  *(void**)res = ptrh_ptr(h);
  if (sourceObjs!=NULL) *sourceObjs = vec_addN(*sourceObjs, incG(c)); 
}
void genObj(B o, B c, void* ptr, B* sourceObjs) { // doesn't consume
  if (isC32(o)) { // scalar
    u32 t = styG(o);
    f64 f = c.f;
    switch(t) { default: UD; // thrF("FFI: Unimplemented scalar type \"%S\"", sty_names[t]);
      case sty_a:   *(BQNV*)ptr = makeX(inc(c)); break;
      case sty_u8:  { if(!q_fu8 (f)) thrM("FFI: improper value for u8" ); *( u8*)ptr = ( u8)f; break; }
      case sty_i8:  { if(!q_fi8 (f)) thrM("FFI: improper value for i8" ); *( i8*)ptr = ( i8)f; break; }
      case sty_u16: { if(!q_fu16(f)) thrM("FFI: improper value for u16"); *(u16*)ptr = (u16)f; break; }
      case sty_i16: { if(!q_fi16(f)) thrM("FFI: improper value for i16"); *(i16*)ptr = (i16)f; break; }
      case sty_u32: { if(!q_fu32(f)) thrM("FFI: improper value for u32"); *(u32*)ptr = (u32)f; break; }
      case sty_i32: { if(!q_fi32(f)) thrM("FFI: improper value for i32"); *(i32*)ptr = (i32)f; break; }
      case sty_u64: { if(!q_fu64(f)) thrM("FFI: improper value for u64"); u64 i=(u64)f; if (i>=(1ULL<<53))                 thrM("FFI: u64 argument value ≥ 2⋆53");          *(u64*)ptr = i; break; }
      case sty_i64: { if(!q_fi64(f)) thrM("FFI: improper value for i64"); i64 i=(i64)f; if (i>=(1LL<<53) || i<=-(1LL<<53)) thrM("FFI: i64 argument absolute value ≥ 2⋆53"); *(i64*)ptr = i; break; }
      case sty_f32: { if(!isNum(c))  thrM("FFI: improper value for f32"); *(float* )ptr = f; break; }
      case sty_f64: { if(!isNum(c))  thrM("FFI: improper value for f64"); *(double*)ptr = f; break; }
    }
  } else {
    BQNFFIType* t = c(BQNFFIType, o);
    if (t->ty==cty_ptr || t->ty==cty_tlarr) { // *any / &any / top-level [n]any
      B e = t->a[0].o;
      if (isAtm(c)) {
        if (isNsp(c)) { genObj_writePtr(ptr, c, e, sourceObjs); return; }
        thrF("FFI: Expected array or pointer object corresponding to %R", ty_fmt(o));
      }
      if (sourceObjs==NULL) thrF("ptr.Write: Cannot write array to %R", ty_fmt(o));
      usz ia = IA(c);
      if (t->ty==cty_tlarr && t->arrCount!=ia) thrF("FFI: Incorrect item count of %s corresponding to %R", ia, ty_fmt(o));
      if (isC32(e)) { // *num / &num
        incG(c);
        B cG;
        bool mut = t->ty==cty_ptr? t->mutPtr : false;
        switch(styG(e)) { default: thrF("FFI: Unimplemented pointer element type within %R", ty_fmt(o));
          case sty_i8:  ffi_checkRange(c, mut, "i8",  I8_MIN,  I8_MAX);  cG = mut? taga(cpyI8Arr (c)) : toI8Any (c); break;
          case sty_i16: ffi_checkRange(c, mut, "i16", I16_MIN, I16_MAX); cG = mut? taga(cpyI16Arr(c)) : toI16Any(c); break;
          case sty_i32: ffi_checkRange(c, mut, "i32", I32_MIN, I32_MAX); cG = mut? taga(cpyI32Arr(c)) : toI32Any(c); break;
          case sty_f64: ffi_checkRange(c, mut, "f64", 0, 0);             cG = mut? taga(cpyF64Arr(c)) : toF64Any(c); break;
          case sty_u8:  ffi_checkRange(c, mut, "u8",  0, U8_MAX);        cG = mut?     cpyU8Bits (c) : toU8Bits (c); break;
          case sty_u16: ffi_checkRange(c, mut, "u16", 0, U16_MAX);       cG = mut?     cpyU16Bits(c) : toU16Bits(c); break;
          case sty_u32: ffi_checkRange(c, mut, "u32", 0, U32_MAX);       cG = mut?     cpyU32Bits(c) : toU32Bits(c); break;
          case sty_f32: ffi_checkRange(c, mut, "f64", 0, 0);             cG = cpyF32Bits(c); break; // no direct f32 type, so no direct reference option
        }
        
        *(void**)ptr = tyany_ptr(cG);
        *sourceObjs = vec_addN(*sourceObjs, cG);
      } else { // *{...} / &{...} / *[n]any
        BQNFFIType* t2 = c(BQNFFIType, e);
        if (t2->ty!=cty_struct && t2->ty!=cty_starr) thrM("FFI: Pointer element type not implemented");
        
        usz elSz = t2->structSize;
        TALLOC(u8, dataAll, elSz*ia + sizeof(usz));
        void* dataStruct = dataAll+sizeof(usz);
        *((usz*)dataAll) = ia;
        SGetU(c)
        for (usz i = 0; i < ia; i++) genObj(t->a[0].o, GetU(c, i), dataStruct + elSz*i, sourceObjs);
        *(void**)ptr = dataStruct;
        *sourceObjs = vec_addN(*sourceObjs, tag(TOBJ(dataAll), OBJ_TAG));
      }
    } else if (t->ty==cty_repr) { // any:any
      B o2 = t->a[0].o;
      u8 reT = t->reType;
      u8 reW = t->reWidth;
      u8 elSz;
      if (isC32(o2)) { // scalar:any
        elSz = sty_w[styG(o2)];
        toScalarReinterpret:;
        u8 mul = (elSz*8) >> reW;
        if (isAtm(c)) thrF("FFI: Expected array corresponding to %R", ty_fmt(o));
        if (IA(c) != mul) thrF("FFI: Bad array corresponding to %R: expected %s elements, got %s", ty_fmt(o), (usz)mul, IA(c));
        B cG = toW(reT, reW, incG(c));
        memcpy(ptr, tyany_ptr(cG), 8); // may over-read, but CBQN-allocations allow that; may write past the end, but that's fine too? maybe? idk actually; TODO
        dec(cG);
      } else { // *scalar:any / &scalar:any / *:any / **:any
        BQNFFIType* t2 = c(BQNFFIType, o2);
        B ore = t2->a[0].o;
        assert(t2->ty==cty_ptr && (isC32(ore) || ore.u==ty_voidptr.u));
        if (isNsp(c)) { genObj_writePtr(ptr, c, ore, sourceObjs); return; }
        if (ore.u == m_c32(sty_void).u) { // *:any
          elSz = sizeof(void*);
          goto toScalarReinterpret;
        }
        if (sourceObjs==NULL) thrF("ptr.Write: Cannot write array to %R", ty_fmt(o));
        bool mut = t2->mutPtr;
        
        elSz = isC32(ore)? sty_w[styG(ore)] : sizeof(void*);
        u8 mul = (elSz*8) >> reW;
        if (!isArr(c)) thrF("FFI: Expected array or pointer object corresponding to %R", ty_fmt(o));
        if (mul && (IA(c) & (mul-1)) != 0) thrF("FFI: Bad array corresponding to %R: expected a multiple of %s elements, got %s", ty_fmt(o), (usz)mul, IA(c));
        
        incG(c); B cG;
        if (mut) {
          Arr* cGp;
          switch(reW) { default: UD;
            case 0:               ffi_checkRange(c, 2, "u1", 0, 1);              cGp = (Arr*) cpyBitArr(c); break;
            case 3: if (reT=='i') ffi_checkRange(c, 2, "i8",  I8_MIN,  I8_MAX);  cGp = reT=='c'? (Arr*) cpyC8Arr(c) : (Arr*) cpyI8Arr(c); break;
            case 4: if (reT=='i') ffi_checkRange(c, 2, "i16", I16_MIN, I16_MAX); cGp = reT=='c'? (Arr*)cpyC16Arr(c) : (Arr*)cpyI16Arr(c); break;
            case 5: if (reT=='i') ffi_checkRange(c, 2, "i32", I32_MIN, I32_MAX); cGp = reT=='c'? (Arr*)cpyC32Arr(c) : (Arr*)cpyI32Arr(c); break;
            case 6:               ffi_checkRange(c, 2, "f64", 0, 0);             cGp = (Arr*) cpyF64Arr(c); break;
          }
          cG = taga(cGp);
        } else cG = toW(reT, reW, c);
        *(void**)ptr = tyany_ptr(cG);
        *sourceObjs = vec_addN(*sourceObjs, cG);
      }
    } else if (t->ty==cty_struct || t->ty==cty_starr) {
      if (!isArr(c)) thrF("FFI: Expected array corresponding to %R", ty_fmt(o));
      if (IA(c)!=t->ia-1) thrF("FFI: Incorrect list length corresponding to %S: expected %s, got %s", t->ty==cty_struct? "a struct" : "an array", (usz)(t->ia-1), IA(c));
      SGetU(c)
      for (usz i = 0; i < t->ia-1; i++) {
        BQNFFIEnt e = t->a[i];
        genObj(e.o, GetU(c, i), e.structOffset + (u8*)ptr, sourceObjs);
      }
    } else thrM("FFI: Unimplemented type (genObj)");
  }
}

static B readAny(B o, u8* ptr);
static B readStruct(BQNFFIType* t, u8* ptr) {
  usz ia = t->ia-1;
  M_HARR(r, ia);
  for (usz i = 0; i < ia; i++) {
    void* c = ptr + t->a[i].structOffset;
    HARR_ADD(r, i, readAny(t->a[i].o, c));
  }
  return HARR_FV(r);
}

static B readSimple(u8 resCType, u8* ptr) {
  switch(resCType) { default: UD; // thrM("FFI: Unimplemented type");
    case sty_void:return m_c32(0);
    case sty_a:   return getB(*(BQNV*)ptr);
    case sty_i8:  return m_i32(*( i8*)ptr);  case sty_u8:  return m_i32(*( u8*)ptr);
    case sty_i16: return m_i32(*(i16*)ptr);  case sty_u16: return m_i32(*(u16*)ptr);
    case sty_i32: return m_i32(*(i32*)ptr);  case sty_u32: return m_f64(*(u32*)ptr);
    case sty_i64: { i64 v = *(i64*)ptr; if (i64abs(v)>=(1ULL<<53)) thrM("FFI: i64 result absolute value ≥ 2⋆53"); return m_f64(v); }
    case sty_u64: { u64 v = *(u64*)ptr; if (       v >=(1ULL<<53)) thrM("FFI: u64 result ≥ 2⋆53");                return m_f64(v); }
    case sty_f32: return m_f64(*(float* )ptr);
    case sty_f64: return m_f64(*(double*)ptr);
  }
}

static u8 const reTyMapC[] = { [3]=t_c8arr, [4]=t_c16arr, [5]=t_c32arr };
static u8 const reTyMapI[] = { [3]=t_i8arr, [4]=t_i16arr, [5]=t_i32arr, [6]=t_f64arr, [0]=t_bitarr };
static B readRe(BQNFFIType* t, u8* src) {
  B e = t->a[0].o;
  u8 elW; // bytes
  if (isC32(e)) elW = sty_w[styG(e)];
  else if (e.u==ty_voidptr.u) elW = sizeof(void*);
  else thrF("FFI: Cannot read from %R", ty_fmt(tag(t, OBJ_TAG)));
  u8 reW = t->reWidth; // log bits
  B r;
  char* dst = m_tyarrlbv(&r, reW, (elW*8)>>reW, (t->reType=='c'? reTyMapC : reTyMapI)[reW]);
  memcpy(dst, src, elW);
  return r;
}

static B readAny(B o, u8* ptr) { // doesn't consume
  if (isC32(o)) {
    return readSimple(styG(o), ptr);
  } else {
    BQNFFIType* t = c(BQNFFIType, o);
    if (t->ty == cty_repr) { // cty_repr, scalar:x
      return readRe(t, ptr);
    } else if (t->ty==cty_struct || t->ty==cty_starr) { // {...}, [n]...
      return readStruct(c(BQNFFIType, o), ptr);
    } else if (t->ty==cty_ptr) { // *...
      return m_ptrobj_s(*(void**)ptr, inc(t->a[0].o));
    }
  }
  
  thrM("FFI: Unimplemented in-memory type for reading");
}

B readUpdatedObj(BQNFFIEnt ent, bool anyMut, B** objs) {
  if (isC32(ent.o)) return m_f64(0); // scalar
  BQNFFIType* t = c(BQNFFIType, ent.o);
  if (t->ty==cty_ptr || t->ty==cty_tlarr) { // *any / &any / top-level [n]any
    B e = t->a[0].o;
    B f = *((*objs)++);
    if (t->ty==cty_ptr && t->mutPtr) {
      if (isC32(e)) {
        switch(styG(e)) { default: UD;
          case sty_i8: case sty_i16: case sty_i32: case sty_f64: return inc(f);
          case sty_u8:  return readU8Bits(f);
          case sty_u16: return readU16Bits(f);
          case sty_u32: return readU32Bits(f);
          case sty_f32: return readF32Bits(f);
        }
      } else {
        BQNFFIType* t2 = c(BQNFFIType, e);
        assert(t2->ty==cty_struct || t2->ty==cty_starr);
        
        u8* dataAll = c(TAlloc,f)->data;
        void* dataStruct = dataAll+sizeof(usz);
        
        usz ia = *(usz*)dataAll;
        usz elSz = t2->structSize;
        M_HARR(r, ia);
        for (usz i = 0; i < ia; i++) HARR_ADD(r, i, readStruct(t2, dataStruct + i*elSz));
        return HARR_FV(r);
      }
    } else return m_f64(0);
  } else if (t->ty==cty_repr) { // any:any
    B o2 = t->a[0].o;
    if (isC32(o2)) return m_f64(0); // scalar:any
    
    BQNFFIType* t2 = c(BQNFFIType,o2); // *scalar:any / &scalar:any
    assert(t2->ty == cty_ptr);
    B f = *((*objs)++);
    return t2->mutPtr? inc(f) : m_f64(0);
  } else if (t->ty==cty_struct || t->ty==cty_starr) {
    assert(!anyMut); // Structs currently cannot contain mutable references
    
    usz ia = t->ia-1;
    for (usz i = 0; i < ia; i++) readUpdatedObj(t->a[i], false, objs); // just to forward objPos
    
    return m_f64(0);
  } else thrF("FFI: Unimplemented type (readUpdatedObj: %i)", (i32)t->ty);
}

B libffiFn_c2(B t, B w, B x) {
  BoundFn* bf = c(BoundFn,t);
  BQNFFIType* argObj = c(BQNFFIType, c(HArr,bf->obj)->a[0]);
  
  #define PROC_ARG(ISX, L, U, S) \
    Arr* L##a ONLY_GCC(=0);     \
    AS2B L##f ONLY_GCC(=0);     \
    if (bf->L##Len>0) {         \
      if (FFI_CHECKS) {         \
        if (isAtm(L) || RNK(L)!=1) thrM("FFI: Expected list " S); \
        if (bf->L##Len>0 && IA(L)!=bf->L##Len) thrF("FFI: Wrong argument count in " S ": expected %s, got %s", bf->L##Len, IA(L)); \
      }                         \
      L##a = a(L);              \
      L##f = TIv(L##a,getU);    \
    } else if (FFI_CHECKS && bf->L##Len==0 && (ISX? 1 : !q_N(w)) && (isAtm(L) || RNK(L)!=1 || IA(L)!=0)) { \
      thrF("FFI: " S " must %S", ISX? "be an empty list" : "either be an empty list, or not be present"); \
    }
  
  if (FFI_CHECKS && bf->wLen!=0 && q_N(w)) thrM("FFI: 𝕨 must be present");
  PROC_ARG(0, w, W, "𝕨")
  PROC_ARG(1, x, X, "𝕩")
  
  i32 idxs[2] = {0,0};
  
  u8* tmpAlloc = TALLOCP(u8, argObj->staticAllocTotal);
  void** argPtrs = (void**) tmpAlloc;
  
  B cifObj = c(HArr,bf->obj)->a[1];
  ffi_cif* cif = (void*) c(TAlloc,cifObj)->data;
  usz argn = cif->nargs;
  
  BQNFFIEnt* ents = argObj->a;
  B ffiObjs = emptyHVec(); // implicit parameter to genObj
  for (usz i = 0; i < argn; i++) {
    BQNFFIEnt e = ents[i+1];
    B o;
    if (e.wholeArg) {
      o = e.onW? w : x;
    } else {
      o = (e.onW? wf : xf)(e.onW?wa:xa, idxs[e.onW]++);
    }
    genObj(e.o, o, tmpAlloc + e.staticOffset, &ffiObjs);
  }
  
  for (usz i = 0; i < argn; i++) argPtrs[i] = tmpAlloc + ents[i+1].staticOffset;
  void* res = tmpAlloc + ents[0].staticOffset;
  
  // for (usz i = 0; i < argObj->staticAllocTotal; i++) { // simple hexdump of the allocation
  //   if (!(i&15)) printf("%s%p  ", i?"\n":"", (void*)(tmpAlloc+i));
  //   else if (!(i&3)) printf(" ");
  //   printf("%x%x ", tmpAlloc[i]>>4, tmpAlloc[i]&15);
  // }
  // printf("\n");
  
  void* sym = bf->w_c2;
  ffi_call(cif, sym, res, argPtrs);
  
  B r;
  bool resVoid = false;
  if (isC32(ents[0].o)) {
    u32 resCType = styG(ents[0].o);
    r = readSimple(resCType, res);
    resVoid = resCType==sty_void;
  } else {
    BQNFFIType* t = c(BQNFFIType, ents[0].o);
    if (t->ty == cty_repr) { // scalar:any, *:any
      r = readRe(t, res);
    } else if (t->ty == cty_struct) { // {...}
      r = readStruct(c(BQNFFIType, ents[0].o), res);
    } else if (t->ty == cty_ptr) { // *...
      r = m_ptrobj_s(*(void**)res, inc(t->a[0].o));
    } else UD;
  }
  TFREE(tmpAlloc);
  
  i32 mutArgs = bf->mutCount;
  bool testBuildObj = false;
  if (mutArgs || testBuildObj) {
    u32 flags = ptr2u64(bf->w_c1);
    bool resSingle = flags&4;
    B* objsCurr = harr_ptr(ffiObjs);
    if (resSingle) {
      for (usz i = 0; i < argn; i++) {
        BQNFFIEnt e = ents[i+1];
        B c = readUpdatedObj(e, e.isMutated, &objsCurr);
        if (e.isMutated) r = c;
      }
    } else {
      M_HARR(ra, mutArgs+(resVoid? 0 : 1));
      if (!resVoid) HARR_ADDA(ra, r);
      for (usz i = 0; i < argn; i++) {
        BQNFFIEnt e = ents[i+1];
        B c = readUpdatedObj(e, e.isMutated, &objsCurr);
        if (e.isMutated) HARR_ADDA(ra, c);
      }
      if (testBuildObj && !mutArgs) { inc(r); HARR_ABANDON(ra); }
      else r = HARR_FV(ra);
    }
    assert(objsCurr == harr_ptr(ffiObjs) + IA(ffiObjs));
  }
  
  dec(w); dec(x); decG(ffiObjs);
  return r;
}
B libffiFn_c1(B t, B x) { return libffiFn_c2(t, bi_N, x); }

#else

BQNFFIEnt ffi_parseDecoratedType(B arg, bool forRes) {
  if (!isArr(arg) || IA(arg)!=1 || IGetU(arg,0).u!=m_c32('a').u) thrM("FFI: Only \"a\" arguments & return value supported with compile flag FFI=1");
  return (BQNFFIEnt){};
}
#endif



static u64 calcAtomSize(B chr) {
  return styG(chr)==sty_a? sizeof(BQNV) : sizeof(ffi_arg)>8? sizeof(ffi_arg) : 8;
}
static u64 calcMemSize(B o);
static NOINLINE u64 calcMemSizeCompound(B o) {
  BQNFFIType* t = c(BQNFFIType, o);
  if (t->ty==cty_ptr || t->ty==cty_tlarr) return sizeof(void*); // *any / &any / top-level [n]any
  else if (t->ty==cty_struct || t->ty==cty_starr) return t->structSize; // {...}
  else if (t->ty==cty_repr) { // any:any
    B o2 = t->a[0].o;
    if (isC32(o2)) return calcMemSize(o2);
    if (c(BQNFFIType,o2)->ty != cty_ptr) thrM("FFI: Bad type with reinterpretation");
    return sizeof(void*);
  } else thrM("FFI: Unimplemented type (size calculation)");
}
static u64 calcMemSize(B o) {
  return isC32(o)? sty_w[styG(o)] : calcMemSizeCompound(o);
}
static u64 calcStaticSize(B o) {
  return isC32(o)? calcAtomSize(o) : calcMemSizeCompound(o);
}

B ffiload_c2(B t, B w, B x) {
  usz xia = IA(x);
  if (xia<2) thrM("FFI: Function specification must have at least two items");
  usz argn = xia-2;
  if (argn >= U16_MAX) thrM("FFI: Too many arguments");
  SGetU(x)
  B name = GetU(x,1);
  vfyStr(name, "FFI", "Type");
  
  u64 staticAlloc = ffiTmpAlign(argn * sizeof(void*)); // initial alloc is the argument pointer list
  #define STATIC_ALLOC(O, SZ) ({ (O).staticOffset = staticAlloc; staticAlloc+= ffiTmpAlign(SZ); })
  
  DecoratedType tRes = ffi_parseDecoratedType(GetU(x,0), true);
  BQNFFIEnt eRes = tRes.ent;
  if (eRes.isMutated) thrM("FFI: Result type cannot be mutated");
  {
    B atomType;
    usz size;
    if (isC32(eRes.o)) { atomType = eRes.o; goto calcAtomSize; }
    
    BQNFFIType* t = c(BQNFFIType, eRes.o);
    if (t->ty == cty_repr) {
      B o2 = t->a[0].o;
      if (o2.u==ty_voidptr.u) {
        size = sizeof(void*);
        goto allocRes;
      }
      if (!isC32(o2)) thrM("FFI: Unimplemented result type");
      atomType = o2;
      goto calcAtomSize;
    }
    if (t->ty==cty_struct) { size = t->structSize; goto allocRes; }
    if (t->ty==cty_tlarr) thrM("FFI: Cannot return array");
    if (t->ty==cty_ptr) {
      size = sizeof(void*);
      goto allocRes;
    }
    thrM("FFI: Unimplemented result type");
    
    calcAtomSize:;
      size = calcAtomSize(atomType);
      goto allocRes;
    
    allocRes:; STATIC_ALLOC(eRes, size);
  }
  
  ALLOC_TYPE_LIST(argn+1, argn);
  elts[argn] = tRes.ffitype;
  
  BQNFFIType* ap; B argObj = m_bqnFFIType(&ap, cty_arglist, argn+1);
  ap->a[0] = eRes;
  bool whole[2]={0,0};
  i32 count[2]={0,0};
  i32 mutCount = 0;
  for (usz i = 0; i < argn; i++) {
    DecoratedType tArg = ffi_parseDecoratedType(GetU(x,i+2), false);
    BQNFFIEnt e = tArg.ent;
    STATIC_ALLOC(e, calcStaticSize(e.o));
    
    elts[i] = tArg.ffitype;
    list[i] = &elts[i];
    
    ap->a[i+1] = e;
    if (e.wholeArg) whole[e.onW] = true;
    count[e.onW]++;
    if (e.isMutated) mutCount++;
  }
  if (staticAlloc > U32_MAX-64) thrM("FFI: Static argument size too large");
  ap->staticAllocTotal = ffiTmpAlign(staticAlloc);
  if (count[0]>1 && whole[0]) thrM("FFI: Multiple arguments on 𝕩 specified, some with '>'");
  if (count[1]>1 && whole[1]) thrM("FFI: Multiple arguments on 𝕨 specified, some with '>'");
  if (eRes.resSingle && mutCount!=1) thrF("FFI: Return value is specified as \"&\", but there are %i mutated values", mutCount);
  
  char* ws = NULL;
  if (w.u != m_c32(0).u) {
    w = path_rel(nfn_objU(t), w, "•FFI");
    ws = toCStr(w);
  }
  void* dl = dlopen(ws, RTLD_NOW);
  
  if (ws) freeCStr(ws);
  dec(w);
  if (dl==NULL) thrF("FFI: Failed to load library: %S", dlerror());
  
  char* nameStr = toCStr(name);
  void* sym = dlsym(dl, nameStr);
  freeCStr(nameStr);
  dec(x);
  
  if (sym==NULL) thrF("FFI: Failed to find symbol: %S", dlerror());
  
  TAlloc* cif = ARBOBJ(sizeof(ffi_cif));
  ffi_status s = ffi_prep_cif((ffi_cif*)cif->data, FFI_DEFAULT_ABI, argn, &elts[argn], list);
  if (s!=FFI_OK) thrM("FFI: Error preparing call interface");
  
  u32 flags = eRes.resSingle<<2;
  B r = m_ffiFn(foreignFnDesc, m_hvec3(argObj, tag(cif, OBJ_TAG), tag(ao, OBJ_TAG)), libffiFn_c1, libffiFn_c2, TOPTR(void,flags), sym);
  c(BoundFn,r)->mutCount = mutCount;
  c(BoundFn,r)->wLen = whole[1]? -1 : count[1];
  c(BoundFn,r)->xLen = whole[0]? -1 : count[0];
  return r;
}

#define FFI_TYPE_FLDS(OBJ) \
  BQNFFIType* t = (BQNFFIType*) x;   \
  BQNFFIEnt* arr=t->a; usz ia=t->ia; \
  for (usz i = 0; i < ia; i++) OBJ(arr[i].o);

DEF_FREE(ffiType) {
  FFI_TYPE_FLDS(dec);
}
void ffiType_visit(Value* x) {
  FFI_TYPE_FLDS(mm_visit);
}
void ffiType_print(FILE* f, B x) {
  BQNFFIType* t = c(BQNFFIType,x);
  fprintf(f, "cty_%d⟨", t->ty);
  usz ia = t->ia;
  if (t->ty==cty_struct || t->ty==cty_starr) ia--;
  for (usz i=0; i<ia; i++) {
    if (i) fprintf(f, ", ");
    B e = t->a[i].o;
    if (isC32(e)) fprintf(f, "%d", styG(e));
    else fprintI(f, e);
  }
  fprintf(f, "⟩");
}



STATIC_GLOBAL Body* ptrobj_ns;
STATIC_GLOBAL NFnDesc* ptrReadDesc;
STATIC_GLOBAL NFnDesc* ptrWriteDesc;
STATIC_GLOBAL NFnDesc* ptrCastDesc;
STATIC_GLOBAL NFnDesc* ptrAddDesc;
STATIC_GLOBAL NFnDesc* ptrSubDesc;
STATIC_GLOBAL NFnDesc* ptrFieldDesc;

// ptrh - pointer holder - BQNFFIType object of {{.o=eltype}, {.ptrh_ptr, .ptrh_stride}}
// eltype of sty_void is used for an untyped pointer
static B ptrh_type(B n) { return c(BQNFFIType, n)->a[0].o; } // returns ptrty
static void* ptrh_ptr(B n) { return c(BQNFFIType, n)->a[0].ptrh_ptr; }
static ux ptrh_stride(B n) { return c(BQNFFIType, n)->ptrh_stride; }

B m_ptrobj_s(void* ptr, B o) {
  return m_ptrobj(ptr, o, calcMemSize(o));
}
B m_ptrobj(void* ptr, B o, ux stride) {
  BQNFFIType* op;
  B obj = m_bqnFFIType(&op, cty_ptrh, 1);
  op->ptrh_stride = stride;
  op->a[0].o = o;
  op->a[0].ptrh_ptr = ptr;
  
  return m_nns(ptrobj_ns,
    m_nfn(ptrReadDesc,  incG(obj)),
    m_nfn(ptrWriteDesc, incG(obj)),
    m_nfn(ptrCastDesc,  incG(obj)),
    m_nfn(ptrAddDesc,   incG(obj)),
    m_nfn(ptrSubDesc,   incG(obj)),
    m_nfn(ptrFieldDesc, obj)
  );
}

static NOINLINE B ptrobj_checkget(B x) { // doesn't consume
  if (!isNsp(x)) thrF("Expected pointer object, got %S", genericDesc(x));
  if (c(NS,x)->desc != ptrobj_ns->nsDesc) thrM("Expected pointer object, got some other kind of namespace");
  return nfn_objU(c(NS,x)->sc->vars[0]);
}

static void* ptrobj_elbase(B h, B off, bool negate) { // doesn't consume h, off doesn't matter
  if (ptrh_type(h).u == m_c32(sty_void).u) thrM("Cannot offset an untyped pointer");
  char* ptr = ptrh_ptr(h);
  i64 el = o2i64(off);
  if (negate) el = -el;
  return ptr + ptrh_stride(h)*el;
}
static B ptrobjRead_c1(B t, B x) {
  B h = nfn_objU(t);
  return readAny(ptrh_type(h), ptrobj_elbase(h, x, false));
}
static B ptrobjWrite_c2(B t, B w, B x) {
  B h = nfn_objU(t);
  genObj(ptrh_type(h), x, ptrobj_elbase(h, w, false), NULL);
  
  dec(x);
  return m_f64(1);
}
static B ptrobjWrite_c1(B t, B x) { return ptrobjWrite_c2(t, m_f64(0), x); }

static B ptrobjCast_c1(B t, B x) {
  vfyStr(x, "Type parser", "Pointer type");
  usz ia = IA(x);
  void* ptr = ptrh_ptr(nfn_objU(t));
  if (ia==0) {
    decG(x);
    return m_ptrobj(ptr, m_c32(sty_void), 0);
  }
  
  u32* xp0 = parseType_pre(x, ia);
  u32* xp = xp0;
  decG(x);
  ParsedType parsed = ffi_parseType(&xp, false, false);
  parseType_post(xp0, xp, ia);
  B r = m_ptrobj_s(ptr, parsed.o);
  return r;
}
static B ptrobjField_c1(B t, B x) {
  B h = nfn_objU(t);
  
  u64 fld = o2u64(x);
  B el = ptrh_type(h);
  if (isC32(el)) thrM("Cannot get a field of a pointer to a scalar");
  BQNFFIType* elc = c(BQNFFIType, el);
  
  char* ptr = ptrh_ptr(h);
  B elNew;
  if (elc->ty == cty_starr || elc->ty == cty_struct) {
    bool isStruct = elc->ty == cty_struct;
    u64 n = elc->ia-1;
    if (fld >= n) thrF(isStruct? "Cannot get field %l of a struct with %s fields" : "Cannot get pointer to element %l of an array with %s elements", fld, n);
    BQNFFIEnt* fldptr = &elc->a[fld];
    elNew = inc(fldptr->o);
    ptr+= fldptr->structOffset;
  } else {
    thrM("Pointer object: Field must be applied to a pointer to a struct or array");
  }
  return m_ptrobj(ptr, elNew, ptrh_stride(h));
}
static B ptrobjAdd_c1(B t, B x) {
  B h = nfn_objU(t);
  return m_ptrobj(ptrobj_elbase(h, x, false), inc(ptrh_type(h)), ptrh_stride(h));
}
static B ptrty_simplify(B x) {
  assert(!isC32(x));
  BQNFFIType* xt = c(BQNFFIType,x);
  if (xt->ty == cty_repr) return xt->a[0].o;
  return x;
}
static bool ty_equal(B a, B b) {
  assert(isC32(a) || v(a)->type==t_ffiType);
  assert(isC32(b) || v(b)->type==t_ffiType);
  if (!isC32(a)) a = ptrty_simplify(a);
  if (a.u == m_c32(sty_void).u) return true;
  
  if (!isC32(b)) b = ptrty_simplify(b);
  if (b.u == m_c32(sty_void).u) return true;
  
  if (isC32(a) || isC32(b)) return a.u==b.u; // if only one is a character, this test trivially fails
  
  BQNFFIType* at = c(BQNFFIType,a);
  BQNFFIType* bt = c(BQNFFIType,b);
  if (at->ty != bt->ty) return false;
  switch (at->ty) { default: UD;
    case cty_ptr: case cty_tlarr: {
      return ty_equal(at->a[0].o, bt->a[0].o);
    }
    case cty_repr: {
      UD; // should be handled by ptrty_simplify
      // if (at->a[0].reType != bt->a[0].reType) return false;
      // if (at->a[0].reWidth != bt->a[0].reWidth) return false;
      // if (!ty_equal(at->a[0].o, bt->a[0].o)) return false;
      // return true;
    }
    case cty_struct: case cty_starr: {
      if (at->ia != bt->ia) return false;
      ux n = at->ia-1;
      for (ux i = 0; i < n; i++) {
        if (!ty_equal(at->a[i].o, bt->a[i].o)) return false;
      }
      return true;
    }
  }
}
static B ptrobjSub_c1(B t, B x) {
  B h = nfn_objU(t);
  if (isNum(x)) return m_ptrobj(ptrobj_elbase(h, x, true), inc(ptrh_type(h)), ptrh_stride(h));
  if (isNsp(x)) {
    B h2 = ptrobj_checkget(x);
    B t1 = ptrh_type(h);
    B t2 = ptrh_type(h2);
    if (t1.u==m_c32(sty_void).u || t2.u==m_c32(sty_void).u) thrM("(pointer).Sub ptr: Both pointers must be typed");
    ux stride = ptrh_stride(h);
    if (stride!=ptrh_stride(h2)) thrM("(pointer).Sub ptr: Arguments must have the same stride");
    if (!ty_equal(t1, t2)) thrM("(pointer).Sub ptr: Arguments must have compatible types");
    ptrdiff_t diff = ptrh_ptr(h) - ptrh_ptr(h2);
    ptrdiff_t eldiff = diff / (ptrdiff_t)stride;
    if (eldiff*stride != diff) thrM("(pointer).Sub ptr: Distance between pointers isn't an exact multiple of stride");
    decG(x);
    return m_f64(eldiff);
  }
  thrF("(pointer).Sub: Unexpected argument type: %S", genericDesc(x));
}



void ffi_init(void) {
  assert(sizeof(ffi_type) % sizeof(ffi_type*) == 0); // assumption made by ALLOC_TYPE_LIST
  boundFnDesc   = registerNFn(m_c8vec_0("(foreign function)"), boundFn_c1, boundFn_c2);
  foreignFnDesc = registerNFn(m_c8vec_0("(foreign function)"), directFn_c1, directFn_c2);
  ptrobj_ns = m_nnsDesc("read","write","cast","add","sub","field"); // first field must be an nfn whose object is the ptrh (needed for ptrobj_checkget)
  ptrReadDesc  = registerNFn(m_c8vec_0("(pointer).Read"), ptrobjRead_c1, c2_bad);
  ptrWriteDesc = registerNFn(m_c8vec_0("(pointer).Write"), ptrobjWrite_c1, ptrobjWrite_c2);
  ptrCastDesc  = registerNFn(m_c8vec_0("(pointer).Cast"), ptrobjCast_c1, c2_bad);
  ptrAddDesc   = registerNFn(m_c8vec_0("(pointer).Add"), ptrobjAdd_c1, c2_bad);
  ptrSubDesc   = registerNFn(m_c8vec_0("(pointer).Sub"), ptrobjSub_c1, c2_bad);
  ptrFieldDesc = registerNFn(m_c8vec_0("(pointer).Field"), ptrobjField_c1, c2_bad);
  TIi(t_ffiType,freeO) = ffiType_freeO;
  TIi(t_ffiType,freeF) = ffiType_freeF;
  TIi(t_ffiType,visit) = ffiType_visit;
  TIi(t_ffiType,print) = ffiType_print;
  BQNFFIType* t; gc_add(ty_voidptr = m_bqnFFIType(&t, cty_ptr, 1));
  t->a[0].o = m_c32(sty_void);
  t->mutPtr = false;
}

#else // i.e. FFI==0
#if !FOR_BUILD
  void ffi_init() { }
  B ffiload_c2(B t, B w, B x) { fatal("•FFI called"); }
#else // whatever build.bqn uses from •FFI
  #include "nfns.h"
  #include "utils/cstr.h"
  #include <unistd.h>
  #include <poll.h>
  typedef struct pollfd pollfd;
  NFnDesc* forbuildDesc;
  B forbuild_c1(B t, B x) {
    i32 id = o2i(nfn_objU(t));
    switch (id) { default: thrM("bad id");
      case 0: {
        char* s = toCStr(x);
        decG(x);
        i32 r = chdir(s);
        freeCStr(s);
        return m_f64(r);
      }
      case 1: {
        dec(x);
        return m_f64(fork());
      }
      case 2: {
        decG(x);
        int vs[2];
        int r = pipe(vs);
        return m_vec2(m_f64(r), m_vec2(m_f64(vs[0]), m_f64(vs[1])));
      }
      case 3: {
        SGet(x)
        int fd =        o2i(Get(x,0));
        Arr* buf = cpyI8Arr(Get(x,1));
        usz maxlen =    o2s(Get(x,2));
        decG(x);
        assert(PIA(buf)==maxlen);
        int res = read(fd, ((TyArr*)buf)->a, maxlen);
        return m_vec2(m_f64(res), taga(buf));
      }
      case 4: {
        SGet(x)
        int fd =        o2i(Get(x,0));
        Arr* buf = cpyI8Arr(Get(x,1));
        usz maxlen =    o2s(Get(x,2));
        decG(x);
        int res = write(fd, ((TyArr*)buf)->a, maxlen);
        ptr_dec(buf);
        return m_f64(res);
      }
      case 5: {
        return m_f64(close(o2i(x)));
      }
      case 6: {
        SGet(x)
        Arr* buf = cpyI16Arr(Get(x,0)); i16* a = (i16*)((TyArr*)buf)->a;
        int nfds =       o2i(Get(x,1));
        int timeout =    o2s(Get(x,2));
        decG(x);
        
        TALLOC(pollfd, ps, nfds)
        for (i32 i = 0; i < nfds; i++) ps[i] = (pollfd){.fd = a[i*4+0]|a[i*4+1]<<16, .events=a[i*4+2]};
        int res = poll(&ps[0], nfds, timeout);
        for (i32 i = 0; i < nfds; i++) a[i*4+3] = ps[i].revents;
        TFREE(ps);
        
        return m_vec2(m_f64(res), taga(buf));
      }
      case 7: {
        return m_f64(isatty(o2i(x)));
      }
    }
  }
  static B ffi_names;
  B ffiload_c2(B t, B w, B x) {
    B name = IGetU(x, 1);
    i32 id = 0;
    while (id<IA(ffi_names) && !equal(IGetU(ffi_names, id), name)) id++;
    B r = m_nfn(forbuildDesc, m_f64(id));
    decG(x);
    return r;
  }
  
  void ffi_init(void) {
    HArr_p a = m_harrUv(8);
    a.a[0] = m_c8vec_0("chdir");
    a.a[1] = m_c8vec_0("fork");
    a.a[2] = m_c8vec_0("pipe");
    a.a[3] = m_c8vec_0("read");
    a.a[4] = m_c8vec_0("write");
    a.a[5] = m_c8vec_0("close");
    a.a[6] = m_c8vec_0("poll");
    a.a[7] = m_c8vec_0("isatty");
    NOGC_E;
    ffi_names = a.b; gc_add(ffi_names);
    forbuildDesc = registerNFn(m_c8vec_0("(function for build)"), forbuild_c1, c2_bad);
  }
#endif
#endif
