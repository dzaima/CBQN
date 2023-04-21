#include "core.h"

#if FFI && !defined(CBQN_EXPORT)
  #error "Expected CBQN_EXPORT if FFI is defined"
#endif

#if CBQN_EXPORT
  #include "../include/bqnffi.h"
  #include "utils/utf.h"
  #include "utils/cstr.h"
  #include "nfns.h"
  #include "ns.h"
  #include "utils/file.h"
  #define EXPORT __attribute__((__visibility__("default")))
// ..continuing under "#if CBQN_EXPORT"

// base interface defs for when GC stuff needs to be added in
static B getB(BQNV v) {
  return b(v);
}
static BQNV makeX(B x) {
  return x.u;
}

EXPORT void bqn_free(BQNV v) {
  dec(getB(v));
}
EXPORT BQNV bqn_copy(BQNV v) {
  return makeX(inc(getB(v)));
}

static void freeTagged(BQNV v) { }

#define DIRECT_BQNV 1

EXPORT double   bqn_toF64 (BQNV v) { double   r = o2fG(getB(v)); freeTagged(v); return r; }
EXPORT uint32_t bqn_toChar(BQNV v) { uint32_t r = o2cG(getB(v)); freeTagged(v); return r; }
EXPORT double   bqn_readF64 (BQNV v) { return o2fG(getB(v)); }
EXPORT uint32_t bqn_readChar(BQNV v) { return o2cG(getB(v)); }

EXPORT void bqn_init() {
  cbqn_init();
}

B type_c1(B t, B x);
EXPORT int bqn_type(BQNV v) {
  return o2i(type_c1(bi_N, inc(getB(v))));
}

EXPORT BQNV bqn_call1(BQNV f, BQNV x) {
  return makeX(c1(getB(f), inc(getB(x))));
}
EXPORT BQNV bqn_call2(BQNV f, BQNV w, BQNV x) {
  return makeX(c2(getB(f), inc(getB(w)), inc(getB(x))));
}

EXPORT BQNV bqn_eval(BQNV src) {
  return makeX(bqn_exec(inc(getB(src)), bi_N, bi_N));
}
EXPORT BQNV bqn_evalCStr(const char* str) {
  return makeX(bqn_exec(utf8Decode0(str), bi_N, bi_N));
}


EXPORT size_t bqn_bound(BQNV a) { return IA(getB(a)); }
EXPORT size_t bqn_rank(BQNV a) { return RNK(getB(a)); }
EXPORT void bqn_shape(BQNV a, size_t* buf) { B b = getB(a);
  ur r = RNK(b);
  usz* sh = SH(b);
  for (usz i = 0; i < r; i++) buf[i] = sh[i];
}
EXPORT BQNV bqn_pick(BQNV a, size_t pos) {
  return makeX(IGet(getB(a),pos));
}

// TODO copy directly with some mut.h thing
EXPORT void bqn_readI8Arr (BQNV a, i8*   buf) { B c = toI8Any (incG(getB(a))); memcpy(buf, i8any_ptr (c), IA(c) * 1); dec(c); }
EXPORT void bqn_readI16Arr(BQNV a, i16*  buf) { B c = toI16Any(incG(getB(a))); memcpy(buf, i16any_ptr(c), IA(c) * 2); dec(c); }
EXPORT void bqn_readI32Arr(BQNV a, i32*  buf) { B c = toI32Any(incG(getB(a))); memcpy(buf, i32any_ptr(c), IA(c) * 4); dec(c); }
EXPORT void bqn_readF64Arr(BQNV a, f64*  buf) { B c = toF64Any(incG(getB(a))); memcpy(buf, f64any_ptr(c), IA(c) * 8); dec(c); }
EXPORT void bqn_readC8Arr (BQNV a, u8*   buf) { B c = toC8Any (incG(getB(a))); memcpy(buf, c8any_ptr (c), IA(c) * 1); dec(c); }
EXPORT void bqn_readC16Arr(BQNV a, u16*  buf) { B c = toC16Any(incG(getB(a))); memcpy(buf, c16any_ptr(c), IA(c) * 2); dec(c); }
EXPORT void bqn_readC32Arr(BQNV a, u32*  buf) { B c = toC32Any(incG(getB(a))); memcpy(buf, c32any_ptr(c), IA(c) * 4); dec(c); }
EXPORT void bqn_readObjArr(BQNV a, BQNV* buf) { B b = getB(a);
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

EXPORT bool bqn_hasField(BQNV ns, BQNV name) {
  return !q_N(ns_getNU(getB(ns), getB(name), false));
}
EXPORT BQNV bqn_getField(BQNV ns, BQNV name) {
  return makeX(inc(ns_getNU(getB(ns), getB(name), true)));
}

EXPORT BQNV bqn_makeF64(double d) { return makeX(m_f64(d)); }
EXPORT BQNV bqn_makeChar(uint32_t c) { return makeX(m_c32(c)); }


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

EXPORT BQNV bqn_makeI8Arr (size_t r0, const size_t* sh0, const i8*   data) { usz ia=calcIA(r0,sh0); i8*  rp; Arr* r = m_i8arrp (&rp,ia); CPYSH(r); memcpy(rp,data,ia*1); return makeX(taga(r)); }
EXPORT BQNV bqn_makeI16Arr(size_t r0, const size_t* sh0, const i16*  data) { usz ia=calcIA(r0,sh0); i16* rp; Arr* r = m_i16arrp(&rp,ia); CPYSH(r); memcpy(rp,data,ia*2); return makeX(taga(r)); }
EXPORT BQNV bqn_makeI32Arr(size_t r0, const size_t* sh0, const i32*  data) { usz ia=calcIA(r0,sh0); i32* rp; Arr* r = m_i32arrp(&rp,ia); CPYSH(r); memcpy(rp,data,ia*4); return makeX(taga(r)); }
EXPORT BQNV bqn_makeF64Arr(size_t r0, const size_t* sh0, const f64*  data) { usz ia=calcIA(r0,sh0); f64* rp; Arr* r = m_f64arrp(&rp,ia); CPYSH(r); memcpy(rp,data,ia*8); return makeX(taga(r)); }
EXPORT BQNV bqn_makeC8Arr (size_t r0, const size_t* sh0, const u8*   data) { usz ia=calcIA(r0,sh0); u8*  rp; Arr* r = m_c8arrp (&rp,ia); CPYSH(r); memcpy(rp,data,ia*1); return makeX(taga(r)); }
EXPORT BQNV bqn_makeC16Arr(size_t r0, const size_t* sh0, const u16*  data) { usz ia=calcIA(r0,sh0); u16* rp; Arr* r = m_c16arrp(&rp,ia); CPYSH(r); memcpy(rp,data,ia*2); return makeX(taga(r)); }
EXPORT BQNV bqn_makeC32Arr(size_t r0, const size_t* sh0, const u32*  data) { usz ia=calcIA(r0,sh0); u32* rp; Arr* r = m_c32arrp(&rp,ia); CPYSH(r); memcpy(rp,data,ia*4); return makeX(taga(r)); }
EXPORT BQNV bqn_makeObjArr(size_t r0, const size_t* sh0, const BQNV* data) { usz ia=calcIA(r0,sh0); HArr_p r = m_harrUp(ia); copyBData(r.a,data,ia); NOGC_E; CPYSH(r.c); return makeX(r.b); }

EXPORT BQNV bqn_makeI8Vec (size_t len, const i8*   data) { i8*  rp; B r = m_i8arrv (&rp,len); memcpy(rp,data,len*1); return makeX(r); }
EXPORT BQNV bqn_makeI16Vec(size_t len, const i16*  data) { i16* rp; B r = m_i16arrv(&rp,len); memcpy(rp,data,len*2); return makeX(r); }
EXPORT BQNV bqn_makeI32Vec(size_t len, const i32*  data) { i32* rp; B r = m_i32arrv(&rp,len); memcpy(rp,data,len*4); return makeX(r); }
EXPORT BQNV bqn_makeF64Vec(size_t len, const f64*  data) { f64* rp; B r = m_f64arrv(&rp,len); memcpy(rp,data,len*8); return makeX(r); }
EXPORT BQNV bqn_makeC8Vec (size_t len, const u8*   data) { u8*  rp; B r = m_c8arrv (&rp,len); memcpy(rp,data,len*1); return makeX(r); }
EXPORT BQNV bqn_makeC16Vec(size_t len, const u16*  data) { u16* rp; B r = m_c16arrv(&rp,len); memcpy(rp,data,len*2); return makeX(r); }
EXPORT BQNV bqn_makeC32Vec(size_t len, const u32*  data) { u32* rp; B r = m_c32arrv(&rp,len); memcpy(rp,data,len*4); return makeX(r); }
EXPORT BQNV bqn_makeObjVec(size_t len, const BQNV* data) { HArr_p r = m_harrUv(len); copyBData(r.a,data,len); NOGC_E;return makeX(r.b); }
EXPORT BQNV bqn_makeUTF8Str(size_t len, const char* str) { return makeX(utf8Decode(str, len)); }

typedef struct BoundFn {
  struct NFn;
  void* w_c1;
  void* w_c2;
  #if FFI==2
    i32 mutCount;
  #endif
} BoundFn;
NFnDesc* boundFnDesc;
NFnDesc* foreignFnDesc;

B boundFn_c1(B t,      B x) { BoundFn* c = c(BoundFn,t); return getB(((bqn_boundFn1)c->w_c1)(makeX(inc(c->obj)),           makeX(x))); }
B boundFn_c2(B t, B w, B x) { BoundFn* c = c(BoundFn,t); return getB(((bqn_boundFn2)c->w_c2)(makeX(inc(c->obj)), makeX(w), makeX(x))); }

typedef BQNV (*bqn_foreignFn1)(BQNV x);
typedef BQNV (*bqn_foreignFn2)(BQNV w, BQNV x);
B directFn_c1(B t,      B x) { BoundFn* c = c(BoundFn,t); return getB(((bqn_foreignFn1)c->w_c1)(          makeX(x))); }
B directFn_c2(B t, B w, B x) { BoundFn* c = c(BoundFn,t); return getB(((bqn_foreignFn2)c->w_c2)(makeX(w), makeX(x))); }

static B m_ffiFn(NFnDesc* desc, B obj, BB2B c1, BBB2B c2, void* wc1, void* wc2) {
  BoundFn* r = mm_alloc(sizeof(BoundFn), t_nfn);
  nfn_lateInit((NFn*)r, desc);
  r->obj = obj;
  r->c1 = c1;
  r->c2 = c2;
  r->w_c1 = wc1;
  r->w_c2 = wc2;
  return tag(r, FUN_TAG);
}
EXPORT BQNV bqn_makeBoundFn1(bqn_boundFn1 f, BQNV obj) { return makeX(m_ffiFn(boundFnDesc, inc(getB(obj)), boundFn_c1, c2_bad, f, NULL)); }
EXPORT BQNV bqn_makeBoundFn2(bqn_boundFn2 f, BQNV obj) { return makeX(m_ffiFn(boundFnDesc, inc(getB(obj)), c1_bad, boundFn_c2, NULL, f)); }

const static u8 typeMap[] = {
  [el_bit] = elt_unk,
  [el_B  ] = elt_unk,
  [el_i8 ] = elt_i8,  [el_c8 ] = elt_c8,
  [el_i16] = elt_i16, [el_c16] = elt_c16,
  [el_i32] = elt_i32, [el_c32] = elt_c32,
  [el_f64] = elt_f64,
};
EXPORT BQNElType bqn_directArrType(BQNV a) {
  B b = getB(a);
  if (!isArr(b)) return elt_unk;
  return typeMap[TI(b,elType)];
}
EXPORT const i8*  bqn_directI8 (BQNV a) { return i8any_ptr (getB(a)); }
EXPORT const i16* bqn_directI16(BQNV a) { return i16any_ptr(getB(a)); }
EXPORT const i32* bqn_directI32(BQNV a) { return i32any_ptr(getB(a)); }
EXPORT const f64* bqn_directF64(BQNV a) { return f64any_ptr(getB(a)); }
EXPORT const u8*  bqn_directC8 (BQNV a) { return c8any_ptr (getB(a)); }
EXPORT const u16* bqn_directC16(BQNV a) { return c16any_ptr(getB(a)); }
EXPORT const u32* bqn_directC32(BQNV a) { return c32any_ptr(getB(a)); }

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
  union {
    B o; // usual case
    TAlloc* structData; // pointer stored in last element of cty_struct
  };
#if FFI==2
  ffi_type t;
#endif
  //      generic    ffi_parseType ffi_parseTypeStr  cty_ptr    cty_repr
  union { u8 extra;  u8 onW;       u8 canRetype;     u8 mutPtr; u8 reType;  };
  union { u8 extra2; u8 mutates;   /*mutates*/                  u8 reWidth; };
  union {
    struct { u8 wholeArg; u8 resSingle; }; // ffi_parseType
    u16 offset; // cty_struct
  };
  u16 staticOffset; // only at the top level; offset into allocation
} BQNFFIEnt;
typedef struct BQNFFIType {
  struct Value;
  union { u16 structSize; u16 arrCount; u16 staticAllocTotal; };
  u8 ty;
  usz ia;
  BQNFFIEnt a[];
} BQNFFIType;

B vfyStr(B x, char* name, char* arg);
static void printFFIType(FILE* f, B x) {
  if (isC32(x)) fprintf(f, "%d", o2cG(x));
  else fprintI(f, x);
}

#if FFI==2

enum ScalarTy {
  sty_void, sty_a, sty_ptr,
  sty_u8, sty_u16, sty_u32, sty_u64,
  sty_i8, sty_i16, sty_i32, sty_i64,
  sty_f32, sty_f64
};
static const u8 sty_w[] = {
  [sty_void]=0, [sty_a]=sizeof(BQNV), [sty_ptr]=sizeof(void*),
  [sty_u8]=1, [sty_u16]=2, [sty_u32]=4, [sty_u64]=8,
  [sty_i8]=1, [sty_i16]=2, [sty_i32]=4, [sty_i64]=8,
  [sty_f32]=4, [sty_f64]=8
};
static const char* sty_names[] = {
  [sty_void]="void", [sty_a]="a", [sty_ptr]="*",
  [sty_u8]="u8", [sty_u16]="u16", [sty_u32]="u32", [sty_u64]="u64",
  [sty_i8]="i8", [sty_i16]="i16", [sty_i32]="i32", [sty_i64]="i64",
  [sty_f32]="f32", [sty_f64]="f64"
};
enum CompoundTy {
  cty_ptr, // *... / &...
  cty_repr, // something:type
  cty_struct, // {...}
  cty_starr, // struct-based array
  cty_tlarr, // top-level array
};

static B m_bqnFFIType(BQNFFIEnt** rp, u8 ty, usz ia) {
  BQNFFIType* r = mm_alloc(fsizeof(BQNFFIType, a, BQNFFIEnt, ia), t_ffiType);
  r->ty = ty;
  r->ia = ia;
  memset(r->a, 0, ia*sizeof(BQNFFIEnt));
  *rp = r->a;
  return tag(r, OBJ_TAG);
}

static u32 readUInt(u32** p) {
  u32* c = *p;
  u32 r = 0;
  while (*c>='0' & *c<='9') {
    if (r >= U32_MAX/10 - 10) thrM("FFI: number literal too large");
    r = r*10 + *c-'0';
    c++;
  }
  *p = c;
  return r;
}
BQNFFIEnt ffi_parseTypeStr(u32** src, bool inPtr, bool top) { // parse actual type
  u32* c = *src;
  u32 c0 = *c++;
  ffi_type rt;
  B ro;
  bool parseRepr=false, canRetype=false, mut=false;
  u32 myWidth = 0; // used if parseRepr
  switch (c0) {
    default: thrF("FFI: Error parsing type: Unexpected character '%c'", c0);
    case '[': {
      u32 n = readUInt(&c);
      if (*c++!=']') thrM("FFI: Bad array type");
      if (n==0) thrM("FFI: 0-item arrays not supported");
      BQNFFIEnt e = ffi_parseTypeStr(&c, top, false);
      
      if (top) { // largely copy-pasted from `case '*': case '&':`
        myWidth = sizeof(void*);
        BQNFFIEnt* rp; ro = m_bqnFFIType(&rp, cty_tlarr, 1);
        rp[0] = e;
        if (n>U16_MAX) thrM("FFI: Top-level array too large; limit is 65535 elements");
        c(BQNFFIType, ro)->arrCount = n;
        mut|= rp[0].mutates;
        parseRepr = rp[0].canRetype;
        rp[0].mutPtr = false;
        rt = ffi_type_pointer;
      } else { // largely copy-pasted from `case '{':`
        BQNFFIEnt* rp; ro = m_bqnFFIType(&rp, cty_starr, n+1);
        for (int i = 0; i < n; i++) rp[i] = e;
        incBy(e.o, n-1);
        rp[n].structData = NULL;
        
        TAlloc* ao = ARBOBJ(sizeof(ffi_type*) * (n+1));
        rp[n].structData = ao;
        ffi_type** els = rt.elements = (ffi_type**) ao->data;
        for (usz i = 0; i < n; i++) els[i] = &rp[i].t;
        els[n] = NULL;
      
        rt.type = FFI_TYPE_STRUCT;
        
        TALLOC(size_t, offsets, n);
        if (ffi_get_struct_offsets(FFI_DEFAULT_ABI, &rt, offsets) != FFI_OK) thrM("FFI: Failed getting array offsets");
        if (rt.size>=U16_MAX) thrM("FFI: Array too large; limit is 65534 bytes");
        for (usz i = 0; i < n; i++) rp[i].offset = offsets[i];
        c(BQNFFIType, ro)->structSize = rt.size;
        TFREE(offsets);
      }
      break;
    }
    
    case 'i': case 'u': case 'f': {
      u32 n = readUInt(&c);
      if (c0=='f') {
        if      (n==32) rt = ffi_type_float;
        else if (n==64) rt = ffi_type_double;
        else thrM("FFI: Bad float width");
        ro = m_c32(n==32? sty_f32 : sty_f64);
      } else {
        u32 scty;
        if      (n== 8) { scty = c0=='i'? sty_i8  : sty_u8;  rt = c0=='i'? ffi_type_sint8  : ffi_type_uint8;  }
        else if (n==16) { scty = c0=='i'? sty_i16 : sty_u16; rt = c0=='i'? ffi_type_sint16 : ffi_type_uint16; }
        else if (n==32) { scty = c0=='i'? sty_i32 : sty_u32; rt = c0=='i'? ffi_type_sint32 : ffi_type_uint32; }
        else if (n==64) { scty = c0=='i'? sty_i64 : sty_u64; rt = c0=='i'? ffi_type_sint64 : ffi_type_uint64; }
        else thrM("FFI: Bad integer width");
        ro = m_c32(scty);
      }
      parseRepr = !inPtr; myWidth = sty_w[o2cG(ro)];
      canRetype = inPtr;
      break;
    }
    
    case 'a': {
      ro = m_c32(sty_a);
      assert(sizeof(BQNV)==8); // ffi_type_uint64 must be the same as BQNV
      rt = ffi_type_uint64;
      break;
    }
    
    case '*': case '&': {
      myWidth = sizeof(void*);
      if (c0=='*' && (0==*c || ':'==*c)) {
        ro = m_c32(sty_ptr);
        parseRepr = !inPtr;
        canRetype = inPtr;
      } else {
        if (c0=='&') mut = true;
        BQNFFIEnt* rp; ro = m_bqnFFIType(&rp, cty_ptr, 1);
        rp[0] = ffi_parseTypeStr(&c, true, false);
        mut|= rp[0].mutates;
        parseRepr = rp[0].canRetype;
        
        rp[0].mutPtr = c0=='&';
      }
      rt = ffi_type_pointer;
      break;
    }
    
    case '{': {
      TSALLOC(BQNFFIEnt, es, 4);
      while (true) {
        BQNFFIEnt e = TSADD(es, ffi_parseTypeStr(&c, false, false));
        if (e.mutates) thrM("FFI: Structs currently cannot contain mutable references");
        u32 m = *c++;
        if (m=='}') break;
        if (m!=',') thrM("FFI: Invalid struct member");
      }
      usz n = TSSIZE(es);
      BQNFFIEnt* rp; ro = m_bqnFFIType(&rp, cty_struct, n+1);
      memcpy(rp, es, n*sizeof(BQNFFIEnt));
      rp[n].structData = NULL;
      
      TAlloc* ao = ARBOBJ(sizeof(ffi_type*) * (n+1));
      rp[n].structData = ao;
      ffi_type** els = rt.elements = (ffi_type**) ao->data;
      for (usz i = 0; i < n; i++) els[i] = &rp[i].t;
      els[n] = NULL;
      TSFREE(es);
      rt.type = FFI_TYPE_STRUCT;
      
      TALLOC(size_t, offsets, n);
      if (ffi_get_struct_offsets(FFI_DEFAULT_ABI, &rt, offsets) != FFI_OK) thrM("FFI: Failed getting struct offsets");
      if (rt.size>=U16_MAX) thrM("FFI: Struct too large; limit is 65534 bytes");
      for (usz i = 0; i < n; i++) rp[i].offset = offsets[i];
      c(BQNFFIType, ro)->structSize = rt.size;
      TFREE(offsets);
      break;
    }
  }
  if (parseRepr && *c==':') {
    c++;
    u8 t = *c++;
    u32 n = readUInt(&c);
    if (t=='i' | t=='c') if (n!=8 & n!=16 & n!=32) { badW: thrF("Bad width in :%c%i", (u32)t, n); }
    if (t=='u') if (n!=1 & n!=8 & n!=16 & n!=32) goto badW;
    if (t=='f') if (n!=64) goto badW;
    
    if (isC32(ro) && n > myWidth*8) thrF("FFI: Representation wider than the value for \"%S:%c%i\"", sty_names[o2cG(ro)], (u32)t, n);
    // TODO figure out what to do with i32:i32 etc
    
    B roP = ro;
    BQNFFIEnt* rp; ro = m_bqnFFIType(&rp, cty_repr, 1);
    rp[0] = (BQNFFIEnt){.o=roP, .t=rt, .reType=t, .reWidth=63-CLZ(n)};
  }
  *src = c;
  return (BQNFFIEnt){.t=rt, .o=ro, .canRetype=canRetype, .extra2=mut};
}

BQNFFIEnt ffi_parseType(B arg, bool forRes) { // doesn't consume; parse argument side & other global decorators
  vfyStr(arg, "FFI", "type");
  usz ia = IA(arg);
  if (ia==0) {
    if (!forRes) thrM("FFI: Argument type empty");
    return (BQNFFIEnt){.t = ffi_type_void, .o=m_c32(sty_void), .resSingle=false};
  }
  arg = chr_squeezeChk(inc(arg));
  
  MAKE_MUT_INIT(tmp, ia+1, el_c32); MUTG_INIT(tmp);
  mut_copyG(tmp, 0, arg, 0, ia);
  mut_setG(tmp, ia, m_c32(0));
  u32* xp = tmp->a;
  u32* xpN = xp + ia;
  
  BQNFFIEnt t;
  if (xp[0]=='&' && xp[1]=='\0') {
    t = (BQNFFIEnt){.t = ffi_type_void, .o=m_c32(sty_void), .resSingle=true};
  } else {
    u8 side = 0;
    bool whole = false;
    while (true) {
      if      (*xp == U'𝕩') { if (side) thrM("FFI: Multiple occurrences of argument side specified"); side = 1; }
      else if (*xp == U'𝕨') { if (side) thrM("FFI: Multiple occurrences of argument side specified"); side = 2; }
      else if (*xp == U'>') { if (whole) thrM("FFI: Multiple occurrences of '>'"); whole = true; }
      else break;
      xp++;
    }
    if (forRes && side) thrM("FFI: Argument side cannot be specified for the result");
    if (side) side--;
    else side = 0;
    
    t = ffi_parseTypeStr(&xp, false, true);
    // printI(arg); printf(": "); printFFIType(stdout, t.o); printf("\n");
    if (xp!=xpN) thrM("FFI: Bad type descriptor");
    t.onW = side;
    // keep .mutates
    t.wholeArg = whole;
    t.resSingle = false;
  }
  mut_pfree(tmp, 0);
  decG(arg);
  return t;
}

static usz ffiTmpAlign(usz n) {
  u64 align = _Alignof(max_align_t);
  n = (n+align-1) & ~(align-1);
  return n;
}

static B ffiObjs;

static B toW(u8 reT, u8 reW, B x) {
  switch(reW) { default: UD;
    case 0: return taga(toBitArr(x)); break;
    case 3: return reT=='c'?  toC8Any(x) :  toI8Any(x); break;
    case 4: return reT=='c'? toC16Any(x) : toI16Any(x); break;
    case 5: return reT=='c'? toC32Any(x) : toI32Any(x); break;
    case 6: return toF64Any(x); break;
  }
}
static u8 reTyMapC[] = { [3]=t_c8arr, [4]=t_c16arr, [5]=t_c32arr };
static u8 reTyMapI[] = { [3]=t_i8arr, [4]=t_i16arr, [5]=t_i32arr, [6]=t_f64arr };
static B makeRe(u8 reT, u8 reW/*log*/, u8* src, u32 elW/*bytes*/) {
  u8* dst; B r;
  usz ia = (elW*8)>>reW;
  if (reW) dst = m_tyarrv(&r, 1<<reW, ia, reT=='c'? reTyMapC[reW] : reTyMapI[reW]);
  else { u64* d2; r = m_bitarrv(&d2, ia); dst = (u8*) d2; }
  memcpy(dst, src, elW);
  return r;
}

FORCE_INLINE u64 i64abs(i64 x) { return x<0?-x:x; }


void genObj(B o, B c, bool anyMut, void* ptr) {
  // printFFIType(stdout,o); printf(" = "); printI(c); printf("\n");
  if (isC32(o)) { // scalar
    u32 t = o2cG(o);
    f64 f = c.f;
    switch(t) { default: UD; // thrF("FFI: Unimplemented scalar type \"%S\"", sty_names[t]);
      case sty_a:   *(BQNV*)ptr = makeX(inc(c)); break;
      case sty_ptr: thrM("FFI: \"*\" unimplemented"); break;
      case sty_u8:  if(f!=( u8)f) thrM("FFI: u8 argument not exact" ); *( u8*)ptr = f; break;
      case sty_i8:  if(f!=( i8)f) thrM("FFI: i8 argument not exact" ); *( i8*)ptr = f; break;
      case sty_u16: if(f!=(u16)f) thrM("FFI: u16 argument not exact"); *(u16*)ptr = f; break;
      case sty_i16: if(f!=(i16)f) thrM("FFI: i16 argument not exact"); *(i16*)ptr = f; break;
      case sty_u32: if(f!=(u32)f) thrM("FFI: u32 argument not exact"); *(u32*)ptr = f; break;
      case sty_i32: if(f!=(i32)f) thrM("FFI: i32 argument not exact"); *(i32*)ptr = f; break;
      case sty_u64: if(f!=(u64)f) thrM("FFI: u64 argument not exact"); if (                 (u64)f  >= (1ULL<<53)) thrM("FFI: u64 argument value ≥ 2⋆53");          *(u64*)ptr = f; break;
      case sty_i64: if(f!=(i64)f) thrM("FFI: i64 argument not exact"); if ((u64)((1ULL<<53)+(u64)f) >= (2ULL<<53)) thrM("FFI: i64 argument absolute value ≥ 2⋆53"); *(i64*)ptr = f; break;
      case sty_f32: *(float* )ptr = f; break;
      case sty_f64: *(double*)ptr = f; break;
    }
  } else {
    BQNFFIType* t = c(BQNFFIType, o);
    if (t->ty==cty_ptr || t->ty==cty_tlarr) { // *any / &any
      
      B e = t->a[0].o;
      if (!isArr(c)) {
        if (isC32(e)) thrF("FFI: Expected array corresponding to \"*%S\"", sty_names[o2cG(e)]);
        else thrM("FFI: Expected array corresponding to *{...}");
      }
      usz ia = IA(c);
      if (t->ty==cty_tlarr && t->arrCount!=ia) thrF("FFI: Incorrect item count of %s corresponding to \"[%s]...\"", ia, (usz)t->arrCount);
      if (isC32(e)) { // *num / &num
        inc(c);
        B cG;
        bool mut = t->a[0].mutPtr;
        switch(o2cG(e)) { default: thrF("FFI: \"*%S\" argument type NYI", sty_names[o2cG(e)]);
          case sty_i8:  cG = mut? taga(cpyI8Arr (c)) : toI8Any (c); break;
          case sty_i16: cG = mut? taga(cpyI16Arr(c)) : toI16Any(c); break;
          case sty_i32: cG = mut? taga(cpyI32Arr(c)) : toI32Any(c); break;
          case sty_f64: cG = mut? taga(cpyF64Arr(c)) : toF64Any(c); break;
          case sty_u8:  { B t=toI16Any(c); i16* tp=i16any_ptr(t); i8*  gp; cG= m_i8arrv(&gp, ia); u8*    np=(u8*   )gp; for (usz i=0; i<ia; i++) np[i]=tp[i]; dec(t); break; }
          case sty_u16: { B t=toI32Any(c); i32* tp=i32any_ptr(t); i16* gp; cG=m_i16arrv(&gp, ia); u16*   np=(u16*  )gp; for (usz i=0; i<ia; i++) np[i]=tp[i]; dec(t); break; }
          case sty_u32: { B t=toF64Any(c); f64* tp=f64any_ptr(t); i32* gp; cG=m_i32arrv(&gp, ia); u32*   np=(u32*  )gp; for (usz i=0; i<ia; i++) np[i]=tp[i]; dec(t); break; }
          case sty_f32: { B t=toF64Any(c); f64* tp=f64any_ptr(t); i32* gp; cG=m_i32arrv(&gp, ia); float* np=(float*)gp; for (usz i=0; i<ia; i++) np[i]=tp[i]; dec(t); break; }
        }
        
        ffiObjs = vec_addN(ffiObjs, cG);
        *(void**)ptr = tyany_ptr(cG);
      } else { // *{...} / &{...} / *[n]any
        BQNFFIType* t2 = c(BQNFFIType, e);
        if (t2->ty!=cty_struct && t2->ty!=cty_starr) thrM("FFI: Unimplemented pointer element type");
        
        
        usz elSz = t2->structSize;
        TALLOC(u8, dataAll, elSz*ia + sizeof(usz));
        void* dataStruct = dataAll+sizeof(usz);
        *((usz*)dataAll) = ia;
        SGetU(c)
        for (usz i = 0; i < ia; i++) genObj(t->a[0].o, GetU(c, i), anyMut, dataStruct + elSz*i);
        *(void**)ptr = dataStruct;
        ffiObjs = vec_addN(ffiObjs, tag(TOBJ(dataAll), OBJ_TAG));
      }
    } else if (t->ty==cty_repr) { // any:any
      B o2 = t->a[0].o;
      u8 reT = t->a[0].reType;
      u8 reW = t->a[0].reWidth;
      if (isC32(o2)) { // scalar:any
        u8 et = o2cG(o2);
        u8 etw = sty_w[et]*8;
        if (!isArr(c)) thrF("FFI: Expected array corresponding to \"%S:%c%i\"", sty_names[et], (u32)reT, 1<<reW);
        if (IA(c) != etw>>reW) thrM("FFI: Bad input array length");
        B cG = toW(reT, reW, inc(c));
        memcpy(ptr, tyany_ptr(cG), 8); // may over-read, ¯\_(ツ)_/¯
        dec(cG);
      } else { // *scalar:any / &scalar:any
        if (!isArr(c)) thrM("FFI: Expected array corresponding to a pointer");
        BQNFFIType* t2 = c(BQNFFIType, o2);
        B ore = t2->a[0].o;
        assert(t2->ty==cty_ptr && isC32(ore)); // we shouldn't be generating anything else
        inc(c);
        B cG;
        bool mut = t->a[0].mutPtr;
        if (mut) {
          Arr* cGp;
          switch(reW) { default: UD;
            case 0: cGp = (Arr*) cpyBitArr(c); break;
            case 3: cGp = reT=='c'? (Arr*) cpyC8Arr(c) : (Arr*) cpyI8Arr(c); break;
            case 4: cGp = reT=='c'? (Arr*)cpyC16Arr(c) : (Arr*)cpyI16Arr(c); break;
            case 5: cGp = reT=='c'? (Arr*)cpyC32Arr(c) : (Arr*)cpyI32Arr(c); break;
            case 6: cGp = (Arr*) cpyF64Arr(c); break;
          }
          cG = taga(cGp);
        } else cG = toW(reT, reW, c);
        *(void**)ptr = tyany_ptr(cG);
        ffiObjs = vec_addN(ffiObjs, cG);
      }
    } else if (t->ty==cty_struct || t->ty==cty_starr) {
      if (!isArr(c)) thrM("FFI: Expected array corresponding to a struct");
      if (IA(c)!=t->ia-1) thrF("FFI: Incorrect list length corresponding to %S: expected %s, got %s", t->ty==cty_struct? "a struct" : "an array", (usz)(t->ia-1), IA(c));
      SGetU(c)
      for (usz i = 0; i < t->ia-1; i++) {
        BQNFFIEnt e = t->a[i];
        genObj(e.o, GetU(c, i), anyMut, e.offset + (u8*)ptr);
      }
    } else thrM("FFI: Unimplemented type (genObj)");
  }
}

B readAny(BQNFFIEnt e, u8* ptr);

B readStruct(BQNFFIType* t, u8* ptr) {
  usz ia = t->ia-1;
  M_HARR(r, ia);
  for (usz i = 0; i < ia; i++) {
    void* c = ptr + t->a[i].offset;
    HARR_ADD(r, i, readAny(t->a[i], c));
  }
  return HARR_FV(r);
}

B readSimple(u8 resCType, u8* ptr) {
  B r;
  switch(resCType) { default: UD; // thrM("FFI: Unimplemented type");
    case sty_void: r = m_c32(0); break;
    case sty_ptr: thrM("FFI: \"*\" unimplemented"); break;
    case sty_a:   r = getB(*(BQNV*)ptr); break;
    case sty_i8:  r = m_i32(*( i8*)ptr); break;  case sty_u8:  r = m_i32(*( u8*)ptr); break;
    case sty_i16: r = m_i32(*(i16*)ptr); break;  case sty_u16: r = m_i32(*(u16*)ptr); break;
    case sty_i32: r = m_i32(*(i32*)ptr); break;  case sty_u32: r = m_f64(*(u32*)ptr); break;
    case sty_i64: { i64 v = *(i64*)ptr; if (i64abs(v)>=(1ULL<<53)) thrM("FFI: i64 result absolute value ≥ 2⋆53"); r = m_f64(v); break; }
    case sty_u64: { u64 v = *(u64*)ptr; if (       v >=(1ULL<<53)) thrM("FFI: u64 result ≥ 2⋆53");                r = m_f64(v); break; }
    case sty_f32: r = m_f64(*(float* )ptr); break;
    case sty_f64: r = m_f64(*(double*)ptr); break;
  }
  return r;
}

B readRe(BQNFFIEnt e, u8* ptr) {
  u8 et = o2cG(e.o);
  u8 reT = e.reType;
  u8 reW = e.reWidth;
  u8 etw = sty_w[et];
  return makeRe(reT, reW, ptr, etw);
}

B readAny(BQNFFIEnt e, u8* ptr) {
  if (isC32(e.o)) {
    return readSimple(o2cG(e.o), ptr);
  } else {
    BQNFFIType* t = c(BQNFFIType, e.o);
    if (t->ty == cty_repr) { // cty_repr, scalar:x
      return readRe(t->a[0], ptr);
    } else if (t->ty==cty_struct || t->ty==cty_starr) { // {...}
      return readStruct(c(BQNFFIType, e.o), ptr);
    }
  }
  thrM("FFI: Unimplemented struct field type for reading");
}

B buildObj(BQNFFIEnt ent, bool anyMut, B* objs, usz* objPos) {
  if (isC32(ent.o)) return m_f64(0); // scalar
  BQNFFIType* t = c(BQNFFIType, ent.o);
  if (t->ty==cty_ptr) { // *any / &any
    B e = t->a[0].o;
    B f = objs[(*objPos)++];
    bool mut = t->a[0].mutPtr;
    if (mut) {
      usz ia = IA(f);
      if (isC32(e)) {
        switch(o2cG(e)) { default: UD;
          case sty_i8: case sty_i16: case sty_i32: case sty_f64: return inc(f);
          case sty_u8:  { u8*   tp=tyarr_ptr(f); i16* rp; B r=m_i16arrv(&rp, ia); for (usz i=0; i<ia; i++) rp[i]=tp[i]; return num_squeeze(r); }
          case sty_u16: { u16*  tp=tyarr_ptr(f); i32* rp; B r=m_i32arrv(&rp, ia); for (usz i=0; i<ia; i++) rp[i]=tp[i]; return num_squeeze(r); }
          case sty_u32: { u32*  tp=tyarr_ptr(f); f64* rp; B r=m_f64arrv(&rp, ia); for (usz i=0; i<ia; i++) rp[i]=tp[i]; return num_squeeze(r); }
          case sty_f32: { float*tp=tyarr_ptr(f); f64* rp; B r=m_f64arrv(&rp, ia); for (usz i=0; i<ia; i++) rp[i]=tp[i]; return r; }
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
    // *scalar:any / &scalar:any
    B f = objs[(*objPos)++];
    bool mut = t->a[0].mutPtr;
    if (!mut) return m_f64(0);
    return inc(f);
  } else thrM("FFI: Unimplemented type (buildObj)");
}

B libffiFn_c2(B t, B w, B x) {
  ffiObjs = emptyHVec();
  
  BoundFn* bf = c(BoundFn,t);
  B argObj = c(HArr,bf->obj)->a[0];
  
  u32 flags = (u64)bf->w_c1;
  
  Arr* wa ONLY_GCC(=0); AS2B wf ONLY_GCC(=0);
  Arr* xa ONLY_GCC(=0); AS2B xf ONLY_GCC(=0);
  if (flags&1) { wa=a(w); wf=TIv(wa,getU); }
  if (flags&2) { xa=a(x); xf=TIv(xa,getU); }
  i32 idxs[2] = {0,0};
  
  u8* tmpAlloc = TALLOCP(u8, c(BQNFFIType,argObj)->staticAllocTotal);
  void** argPtrs = (void**) tmpAlloc;
  
  B cifObj = c(HArr,bf->obj)->a[1];
  ffi_cif* cif = (void*) c(TAlloc,cifObj)->data;
  usz argn = cif->nargs;
  
  BQNFFIEnt* ents = c(BQNFFIType,argObj)->a;
  for (usz i = 0; i < argn; i++) {
    BQNFFIEnt e = ents[i+1];
    B o = e.wholeArg? (e.onW? w : x) : (e.onW? wf : xf)(e.onW?wa:xa, idxs[e.onW]++);
    genObj(e.o, o, e.extra2, tmpAlloc + e.staticOffset);
  }
  
  for (usz i = 0; i < argn; i++) argPtrs[i] = tmpAlloc + ents[i+1].staticOffset;
  void* res = tmpAlloc + ents[0].staticOffset;
  
  // for (usz i = 0; i < c(BQNFFIType,argObj)->staticAllocTotal; i++) { // simple hexdump of the allocation
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
    u32 resCType = o2cG(ents[0].o);
    r = readSimple(resCType, res);
    resVoid = resCType==sty_void;
  } else {
    BQNFFIType* t = c(BQNFFIType, ents[0].o);
    if (t->ty == cty_repr) { // cty_repr, scalar:x
      r = readRe(t->a[0], res);
    } else { // {...}
      r = readStruct(c(BQNFFIType, ents[0].o), res);
    }
  }
  TFREE(tmpAlloc);
  
  i32 mutArgs = bf->mutCount;
  if (mutArgs) {
    usz objPos = 0;
    bool resSingle = flags&4;
    if (resSingle) {
      for (usz i = 0; i < argn; i++) {
        BQNFFIEnt e = ents[i+1];
        B c = buildObj(e, e.mutates, harr_ptr(ffiObjs), &objPos);
        if (e.mutates) r = c;
      }
    } else {
      M_HARR(ra, mutArgs+(resVoid? 0 : 1));
      if (!resVoid) HARR_ADDA(ra, r);
      for (usz i = 0; i < argn; i++) {
        BQNFFIEnt e = ents[i+1];
        B c = buildObj(e, e.mutates, harr_ptr(ffiObjs), &objPos);
        if (e.mutates) HARR_ADDA(ra, c);
      }
      r = HARR_FV(ra);
    }
  }
  
  dec(w); dec(x); dec(ffiObjs);
  return r;
}
B libffiFn_c1(B t, B x) { return libffiFn_c2(t, bi_N, x); }

#else

BQNFFIEnt ffi_parseType(B arg, bool forRes) {
  if (!isArr(arg) || IA(arg)!=1 || IGetU(arg,0).u!=m_c32('a').u) thrM("FFI: Only \"a\" arguments & return value supported with compile flag FFI=1");
  return (BQNFFIEnt){};
}
#endif



static u64 atomSize(B chr) {
  return o2cG(chr)==sty_a? sizeof(BQNV) : sizeof(ffi_arg)>8? sizeof(ffi_arg) : 8;
}
static u64 calcStaticSize(BQNFFIEnt e) {
  if (isC32(e.o)) { // scalar
    return atomSize(e.o);
  } else {
    BQNFFIType* t = c(BQNFFIType, e.o);
    if (t->ty==cty_ptr || t->ty==cty_tlarr) return sizeof(void*); // *any / &any / top-level [n]any
    else if (t->ty==cty_struct || t->ty==cty_starr) return t->structSize; // {...}
    else if (t->ty==cty_repr) { // any:any
      B o2 = t->a[0].o;
      if (isC32(o2)) return atomSize(o2);
      if (c(BQNFFIType,o2)->ty != cty_ptr) thrM("FFI: Bad type with reinterpretation");
      return sizeof(void*);
    } else thrM("FFI: Unimplemented type (size calculation)");
  }
}

B ffiload_c2(B t, B w, B x) {
  usz xia = IA(x);
  if (xia<2) thrM("FFI: Function specification must have at least two items");
  usz argn = xia-2;
  SGetU(x)
  B name = GetU(x,1);
  vfyStr(name, "FFI", "type");
  
  u64 staticAlloc = ffiTmpAlign(argn * sizeof(void*)); // initial alloc is the argument pointer list
  #define STATIC_ALLOC(O, SZ) ({ (O).staticOffset = staticAlloc; staticAlloc+= ffiTmpAlign(SZ); })
  
  BQNFFIEnt tRes = ffi_parseType(GetU(x,0), true);
  {
    B atomType;
    usz size;
    if (isC32(tRes.o)) { atomType = tRes.o; goto calcRes; }
    
    BQNFFIType* t = c(BQNFFIType, tRes.o);
    if (t->ty == cty_repr) {
      B o2 = t->a[0].o;
      if (!isC32(o2)) thrM("FFI: Unimplemented result type");
      atomType = o2;
      goto calcRes;
    }
    if (t->ty==cty_struct) { size = t->structSize; goto allocRes; }
    if (t->ty==cty_tlarr) thrM("FFI: Cannot return array");
    thrM("FFI: Unimplemented result type");
    
    calcRes:; size = atomSize(atomType);
    allocRes:; STATIC_ALLOC(tRes, size);
  }
  
  #if FFI==2
    BQNFFIEnt* args; B argObj = m_bqnFFIType(&args, 255, argn+1);
    args[0] = tRes;
    bool one [2]={0,0};
    bool many[2]={0,0};
    i32 mutCount = 0;
    for (usz i = 0; i < argn; i++) {
      BQNFFIEnt e = ffi_parseType(GetU(x,i+2), false);
      
      STATIC_ALLOC(e, calcStaticSize(e));
      
      args[i+1] = e;
      (e.wholeArg? one : many)[e.extra] = true;
      if (e.mutates) mutCount++;
    }
    if (staticAlloc > U16_MAX-64) thrM("FFI: Static argument size too large");
    c(BQNFFIType,argObj)->staticAllocTotal = ffiTmpAlign(staticAlloc);
    if (one[0] && many[0]) thrM("FFI: Multiple arguments for 𝕩 specified, but one has '>'");
    if (one[1] && many[1]) thrM("FFI: Multiple arguments for 𝕨 specified, but one has '>'");
  #else
    i32 mutCount = 0;
    for (usz i = 0; i < argn; i++) ffi_parseType(GetU(x,i+2), false);
    (void)tRes;
  #endif
  if (tRes.resSingle && mutCount!=1) thrF("FFI: Return was \"&\", but found %i mutated values", mutCount);
  
  char* ws = NULL;
  if (w.u != m_c32(0).u) {
    w = path_rel(nfn_objU(t), w);
    ws = toCStr(w);
  }
  void* dl = dlopen(ws, RTLD_NOW);
  
  if (ws) freeCStr(ws);
  dec(w);
  if (dl==NULL) thrF("Failed to load: %S", dlerror());
  
  char* nameStr = toCStr(name);
  void* sym = dlsym(dl, nameStr);
  freeCStr(nameStr);
  dec(x);
  
  if (sym==NULL) thrF("Failed to find symbol: %S", dlerror());
  #if FFI==1
    return m_ffiFn(foreignFnDesc, bi_N, directFn_c1, directFn_c2, sym, sym);
  #else
    u64 sz = argn*sizeof(ffi_type*);
    if (sz<16) sz = 16;
    TAlloc* argsRaw = ARBOBJ(sz);
    ffi_type** argsRawArr = (ffi_type**)argsRaw->data;
    for (usz i = 0; i < argn; i++) argsRawArr[i] = &args[i+1].t;
    // for (usz i = 0; i < argn; i++) {
    //   ffi_type c = *argsRawArr[i];
    //   printf("%zu %d %d %p\n", c.size, c.alignment, c.type, c.elements);
    // }
    TAlloc* cif = ARBOBJ(sizeof(ffi_cif));
    ffi_status s = ffi_prep_cif((ffi_cif*)cif->data, FFI_DEFAULT_ABI, argn, &args[0].t, argsRawArr);
    if (s!=FFI_OK) thrM("FFI: Error preparing call interface");
    // mm_free(argsRaw)
    u32 flags = many[1] | many[0]<<1 | tRes.resSingle<<2;
    B r = m_ffiFn(foreignFnDesc, m_hVec3(argObj, tag(cif, OBJ_TAG), tag(argsRaw, OBJ_TAG)), libffiFn_c1, libffiFn_c2, (void*)(u64)flags, sym);
    c(BoundFn,r)->mutCount = mutCount;
    return r;
  #endif
}

#define FFI_TYPE_FLDS(OBJ, PTR) \
  BQNFFIType* t = (BQNFFIType*) x;   \
  BQNFFIEnt* arr=t->a; usz ia=t->ia; \
  if (t->ty==cty_struct || t->ty==cty_starr) { \
    ia--;                            \
    if (arr[ia].structData!=NULL) {  \
      PTR(arr[ia].structData);       \
    }                                \
  }                                  \
  for (usz i = 0; i < ia; i++) OBJ(arr[i].o);

DEF_FREE(ffiType) {
  FFI_TYPE_FLDS(dec, ptr_dec);
}
void ffiType_visit(Value* x) {
  FFI_TYPE_FLDS(mm_visit, mm_visitP);
}
void ffiType_print(FILE* f, B x) {
  BQNFFIType* t = c(BQNFFIType,x);
  fprintf(f, "cty_%d⟨", t->ty);
  usz ia = t->ia;
  if (t->ty==cty_struct || t->ty==cty_starr) ia--;
  for (usz i=0; i<ia; i++) {
    if (i) fprintf(f, ", ");
    printFFIType(f, t->a[i].o);
  }
  fprintf(f, "⟩");
}

void ffi_init(void) {
  boundFnDesc   = registerNFn(m_c8vec_0("(foreign function)"), boundFn_c1, boundFn_c2);
  foreignFnDesc = registerNFn(m_c8vec_0("(foreign function)"), directFn_c1, directFn_c2);
  TIi(t_ffiType,freeO) = ffiType_freeO;
  TIi(t_ffiType,freeF) = ffiType_freeF;
  TIi(t_ffiType,visit) = ffiType_visit;
  TIi(t_ffiType,print) = ffiType_print;
}

#else // i.e. FFI==0
#if !FOR_BUILD
  void ffi_init() { }
  B ffiload_c2(B t, B w, B x) { err("•FFI called"); }
#else // whatever build.bqn uses from •FFI
  #include "nfns.h"
  #include "utils/cstr.h"
  #include <unistd.h>
  #include <poll.h>
  typedef struct pollfd pollfd;
  NFnDesc* forbuildDesc;
  B forbuild_c1(B t, B x) {
    i32 id = o2i(nfn_objU(t));
    switch (id) { default: err("bad id");
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
    }
  }
  B names;
  B ffiload_c2(B t, B w, B x) {
    B name = IGetU(x, 1);
    i32 id = 0;
    while (id<IA(names) && !equal(IGetU(names, id), name)) id++;
    B r = m_nfn(forbuildDesc, m_f64(id));
    decG(x);
    return r;
  }
  
  void ffi_init(void) {
    HArr_p a = m_harrUv(7);
    a.a[0] = m_c8vec_0("chdir");
    a.a[1] = m_c8vec_0("fork");
    a.a[2] = m_c8vec_0("pipe");
    a.a[3] = m_c8vec_0("read");
    a.a[4] = m_c8vec_0("write");
    a.a[5] = m_c8vec_0("close");
    a.a[6] = m_c8vec_0("poll");
    NOGC_E;
    names = a.b; gc_add(names);
    forbuildDesc = registerNFn(m_c8vec_0("(function for build)"), forbuild_c1, c2_bad);
  }
#endif
#endif
