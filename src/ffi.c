#include "core.h"

#if FFI
#include "../include/bqnffi.h"
#include "utils/utf.h"
#include "utils/cstr.h"
#include "nfns.h"
#include "utils/file.h"
#include <dlfcn.h>
#if FFI==2
#include <ffi.h>
#include "utils/mut.h"
#endif

// base interface defs for when GC stuff needs to be added in
static B getB(BQNV v) {
  return b(v);
}
static BQNV makeX(B x) {
  return x.u;
}
void bqn_free(BQNV v) {
  dec(getB(v));
}
static void freeTagged(BQNV v) { }
#define DIRECT_BQNV 1

double   bqn_toF64 (BQNV v) { double   r = o2fu(getB(v)); freeTagged(v); return r; }
uint32_t bqn_toChar(BQNV v) { uint32_t r = o2cu(getB(v)); freeTagged(v); return r; }
double   bqn_readF64 (BQNV v) { return o2fu(getB(v)); }
uint32_t bqn_readChar(BQNV v) { return o2cu(getB(v)); }

void bqn_init() {
  cbqn_init();
}

B type_c1(B t, B x);
int bqn_type(BQNV v) {
  return o2i(type_c1(bi_N, inc(getB(v))));
}

BQNV bqn_call1(BQNV f, BQNV x) {
  return makeX(c1(getB(f), inc(getB(x))));
}
BQNV bqn_call2(BQNV f, BQNV w, BQNV x) {
  return makeX(c2(getB(f), inc(getB(w)), inc(getB(x))));
}

BQNV bqn_eval(BQNV src) {
  return makeX(bqn_exec(inc(getB(src)), bi_N, bi_N));
}
BQNV bqn_evalCStr(const char* str) {
  return makeX(bqn_exec(fromUTF8l(str), bi_N, bi_N));
}


size_t bqn_bound(BQNV a) { return a(getB(a))->ia; }
size_t bqn_rank(BQNV a) { return rnk(getB(a)); }
void bqn_shape(BQNV a, size_t* buf) { B b = getB(a);
  ur r = rnk(b);
  usz* sh = a(b)->sh;
  for (usz i = 0; i < r; i++) buf[i] = sh[i];
}
BQNV bqn_pick(BQNV a, size_t pos) {
  return makeX(IGet(getB(a),pos));
}

// TODO copy directly with some mut.h thing
void bqn_readI8Arr (BQNV a, i8*   buf) { B c = toI8Any (inc(getB(a))); memcpy(buf, i8any_ptr (c), a(c)->ia * 1); dec(c); }
void bqn_readI16Arr(BQNV a, i16*  buf) { B c = toI16Any(inc(getB(a))); memcpy(buf, i16any_ptr(c), a(c)->ia * 2); dec(c); }
void bqn_readI32Arr(BQNV a, i32*  buf) { B c = toI32Any(inc(getB(a))); memcpy(buf, i32any_ptr(c), a(c)->ia * 4); dec(c); }
void bqn_readF64Arr(BQNV a, f64*  buf) { B c = toF64Any(inc(getB(a))); memcpy(buf, f64any_ptr(c), a(c)->ia * 8); dec(c); }
void bqn_readC8Arr (BQNV a, u8*   buf) { B c = toC8Any (inc(getB(a))); memcpy(buf, c8any_ptr (c), a(c)->ia * 1); dec(c); }
void bqn_readC16Arr(BQNV a, u16*  buf) { B c = toC16Any(inc(getB(a))); memcpy(buf, c16any_ptr(c), a(c)->ia * 2); dec(c); }
void bqn_readC32Arr(BQNV a, u32*  buf) { B c = toC32Any(inc(getB(a))); memcpy(buf, c32any_ptr(c), a(c)->ia * 4); dec(c); }
void bqn_readObjArr(BQNV a, BQNV* buf) { B b = getB(a);
  usz ia = a(b)->ia;
  B* p = arr_bptr(b);
  if (p!=NULL) {
    for (usz i = 0; i < ia; i++) buf[i] = makeX(inc(p[i]));
  } else {
    SGet(b)
    for (usz i = 0; i < ia; i++) buf[i] = makeX(Get(b, i));
  }
}


BQNV bqn_makeF64(double d) { return makeX(m_f64(d)); }
BQNV bqn_makeChar(uint32_t c) { return makeX(m_c32(c)); }


static usz calcIA(size_t rank, size_t* shape) {
  if (rank>UR_MAX) thrM("Rank too large");
  usz r = 1;
  NOUNROLL for (size_t i = 0; i < rank; i++) if (mulOn(r, shape[i])) thrM("Size too large");
  return r;
}
static void copyBData(B* r, BQNV* data, usz ia) {
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
#define CPYSH(R) usz* sh = arr_shAlloc(R, r0); \
  if (sh) NOUNROLL for (size_t i = 0; RARE(i < r0); i++) sh[i] = sh0[i];

BQNV bqn_makeI8Arr (size_t r0, size_t* sh0, i8*   data) { usz ia=calcIA(r0,sh0); i8*  rp; Arr* r = m_i8arrp (&rp,ia); CPYSH(r); memcpy(rp,data,ia*1); return makeX(taga(r)); }
BQNV bqn_makeI16Arr(size_t r0, size_t* sh0, i16*  data) { usz ia=calcIA(r0,sh0); i16* rp; Arr* r = m_i16arrp(&rp,ia); CPYSH(r); memcpy(rp,data,ia*2); return makeX(taga(r)); }
BQNV bqn_makeI32Arr(size_t r0, size_t* sh0, i32*  data) { usz ia=calcIA(r0,sh0); i32* rp; Arr* r = m_i32arrp(&rp,ia); CPYSH(r); memcpy(rp,data,ia*4); return makeX(taga(r)); }
BQNV bqn_makeF64Arr(size_t r0, size_t* sh0, f64*  data) { usz ia=calcIA(r0,sh0); f64* rp; Arr* r = m_f64arrp(&rp,ia); CPYSH(r); memcpy(rp,data,ia*8); return makeX(taga(r)); }
BQNV bqn_makeC8Arr (size_t r0, size_t* sh0, u8*   data) { usz ia=calcIA(r0,sh0); u8*  rp; Arr* r = m_c8arrp (&rp,ia); CPYSH(r); memcpy(rp,data,ia*1); return makeX(taga(r)); }
BQNV bqn_makeC16Arr(size_t r0, size_t* sh0, u16*  data) { usz ia=calcIA(r0,sh0); u16* rp; Arr* r = m_c16arrp(&rp,ia); CPYSH(r); memcpy(rp,data,ia*2); return makeX(taga(r)); }
BQNV bqn_makeC32Arr(size_t r0, size_t* sh0, u32*  data) { usz ia=calcIA(r0,sh0); u32* rp; Arr* r = m_c32arrp(&rp,ia); CPYSH(r); memcpy(rp,data,ia*4); return makeX(taga(r)); }
BQNV bqn_makeObjArr(size_t r0, size_t* sh0, BQNV* data) { usz ia=calcIA(r0,sh0); HArr_p r = m_harrUp(ia); CPYSH((Arr*)r.c); copyBData(r.a,data,ia); return makeX(r.b); }

BQNV bqn_makeI8Vec (size_t len, i8*   data) { i8*  rp; B r = m_i8arrv (&rp,len); memcpy(rp,data,len*1); return makeX(r); }
BQNV bqn_makeI16Vec(size_t len, i16*  data) { i16* rp; B r = m_i16arrv(&rp,len); memcpy(rp,data,len*2); return makeX(r); }
BQNV bqn_makeI32Vec(size_t len, i32*  data) { i32* rp; B r = m_i32arrv(&rp,len); memcpy(rp,data,len*4); return makeX(r); }
BQNV bqn_makeF64Vec(size_t len, f64*  data) { f64* rp; B r = m_f64arrv(&rp,len); memcpy(rp,data,len*8); return makeX(r); }
BQNV bqn_makeC8Vec (size_t len, u8*   data) { u8*  rp; B r = m_c8arrv (&rp,len); memcpy(rp,data,len*1); return makeX(r); }
BQNV bqn_makeC16Vec(size_t len, u16*  data) { u16* rp; B r = m_c16arrv(&rp,len); memcpy(rp,data,len*2); return makeX(r); }
BQNV bqn_makeC32Vec(size_t len, u32*  data) { u32* rp; B r = m_c32arrv(&rp,len); memcpy(rp,data,len*4); return makeX(r); }
BQNV bqn_makeObjVec(size_t len, BQNV* data) { HArr_p r = m_harrUv(len);      copyBData(r.a,data,len  ); return makeX(r.b); }
BQNV bqn_makeUTF8Str(size_t len, char* str) { return makeX(fromUTF8(str, len)); }

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
BQNV bqn_makeBoundFn1(bqn_boundFn1 f, BQNV obj) { return makeX(m_ffiFn(boundFnDesc, inc(getB(obj)), boundFn_c1, c2_bad, f, NULL)); }
BQNV bqn_makeBoundFn2(bqn_boundFn2 f, BQNV obj) { return makeX(m_ffiFn(boundFnDesc, inc(getB(obj)), c1_bad, boundFn_c2, NULL, f)); }

const static u8 typeMap[] = {
  [el_bit] = elt_unk,
  [el_B  ] = elt_unk,
  [el_i8 ] = elt_i8,  [el_c8 ] = elt_c8,
  [el_i16] = elt_i16, [el_c16] = elt_c16,
  [el_i32] = elt_i32, [el_c32] = elt_c32,
  [el_f64] = elt_f64,
};
BQNElType bqn_directArrType(BQNV a) {
  B b = getB(a);
  if (!isArr(b)) return elt_unk;
  return typeMap[TI(b,elType)];
}
i8*  bqn_directI8 (BQNV a) { return i8any_ptr (getB(a)); }
i16* bqn_directI16(BQNV a) { return i16any_ptr(getB(a)); }
i32* bqn_directI32(BQNV a) { return i32any_ptr(getB(a)); }
f64* bqn_directF64(BQNV a) { return f64any_ptr(getB(a)); }
u8*  bqn_directC8 (BQNV a) { return c8any_ptr (getB(a)); }
u16* bqn_directC16(BQNV a) { return c16any_ptr(getB(a)); }
u32* bqn_directC32(BQNV a) { return c32any_ptr(getB(a)); }

void ffiFn_visit(Value* v) { mm_visit(((BoundFn*)v)->obj); }
DEF_FREE(ffiFn) { dec(((BoundFn*)x)->obj); }



typedef struct BQNFFIEnt {
  B o;
#if FFI==2
  ffi_type t;
#endif
  //      generic    ffi_parseType ffi_parseTypeStr  cty_ptr    cty_repr
  union { u8 extra;  u8 onW;       u8 canRetype;     u8 mutPtr; u8 reType;  };
  union { u8 extra2; u8 mutates;   /*mutates*/                  u8 reWidth; };
  union { u8 extra3; u8 wholeArg;                                           };
  union { u8 extra4; u8 resSingle;                                          };
} BQNFFIEnt;
typedef struct BQNFFIType {
  struct Value;
  u8 ty;
  usz ia;
  BQNFFIEnt a[];
} BQNFFIType;

B vfyStr(B x, char* name, char* arg);
static void printFFIType(FILE* f, B x) {
  if (isC32(x)) fprintf(f, "%d", o2cu(x));
  else fprint(f, x);
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
  cty_ptr,
  cty_repr
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
BQNFFIEnt ffi_parseTypeStr(u32** src, bool inPtr) { // parse actual type
  u32* c = *src;
  u32 c0 = *c++;
  ffi_type rt;
  B ro;
  bool parseRepr=false, canRetype=false, mut=false;
  u32 myWidth = 0; // used if parseRepr
  switch (c0) {
    default:
      thrM("FFI: Error parsing type");
    
    case 'i': case 'u': case 'f':;
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
      parseRepr = !inPtr; myWidth = sty_w[o2cu(ro)];
      canRetype = inPtr;
      break;
    
    case 'a':
      ro = m_c32(sty_a);
      rt = ffi_type_uint64;
      break;
    
    case '*': case '&':;
      myWidth = sizeof(void*);
      if (c0=='*' && (0==*c || ':'==*c)) {
        ro = m_c32(sty_ptr);
        parseRepr = !inPtr;
        canRetype = inPtr;
      } else {
        if (c0=='&') mut = true;
        BQNFFIEnt* rp; ro = m_bqnFFIType(&rp, cty_ptr, 1);
        rp[0] = ffi_parseTypeStr(&c, true);
        mut|= rp[0].mutates;
        parseRepr = rp[0].canRetype;
        
        rp[0].mutPtr = c0=='&';
      }
      rt = ffi_type_pointer;
      break;
  }
  if (parseRepr && *c==':') {
    c++;
    u8 t = *c++;
    u32 n = readUInt(&c);
    if (t=='i' | t=='c') if (n!=8 & n!=16 & n!=32) { badW: thrF("Bad width in :%c%i", t, n); }
    if (t=='u') if (n!=1 & n!=8 & n!=16 & n!=32) goto badW;
    if (t=='f') if (n!=64) goto badW;
    
    if (isC32(ro) && n > myWidth*8) thrF("FFI: Representation wider than the value for \"%S:%c%i\"", sty_names[o2cu(ro)], t, n);
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
  usz ia = a(arg)->ia;
  if (ia==0) {
    if (!forRes) thrM("FFI: Argument type empty");
    return (BQNFFIEnt){.t = ffi_type_void, .o=m_c32(sty_void), .resSingle=false};
  }
  
  MAKE_MUT(tmp, ia+1); mut_init(tmp, el_c32); MUTG_INIT(tmp);
  mut_copyG(tmp, 0, arg, 0, ia);
  mut_setG(tmp, ia, m_c32(0));
  u32* xp = tmp->ac32;
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
    
    t = ffi_parseTypeStr(&xp, false);
    // print(arg); printf(": "); printFFIType(stdout, t.o); printf("\n");
    if (xp!=xpN) thrM("FFI: Bad type descriptor");
    t.onW = side;
    // keep .mutates
    t.wholeArg = whole;
    t.resSingle = false;
  }
  mut_pfree(tmp, 0);
  return t;
}

TAlloc* ffiTmpObj;
u8* ffiTmpS;
u8* ffiTmpC;
u8* ffiTmpE;
static NOINLINE usz ffiTmpX(usz n) {
  usz psz = ffiTmpC-ffiTmpS;
  TAlloc* na = mm_alloc(sizeof(TAlloc)+psz+n, t_temp);
  memcpy(na->data, ffiTmpS, psz);
  
  ffiTmpS = na->data;
  ffiTmpC = ffiTmpS + psz;
  ffiTmpE = (u8*)na + mm_size((Value*) na);
  if (ffiTmpObj) mm_free((Value*)ffiTmpObj);
  ffiTmpObj = na;
  return psz;
}
static NOINLINE usz ffiTmpAA(usz n) {
  u64 align = _Alignof(max_align_t);
  n = (n+align-1) & ~(align-1);
  if (ffiTmpC+n > ffiTmpE) return ffiTmpX(n);
  usz r = ffiTmpC-ffiTmpS;
  ffiTmpC+= n;
  return r;
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

usz genObj(BQNFFIEnt ent, B c, bool anyMut) {
  // printFFIType(stdout,ent.o); printf(" = "); print(c); printf("\n");
  usz pos;
  if (isC32(ent.o)) { // scalar
    u32 t = o2cu(ent.o);
    pos = ffiTmpAA(t==0? sizeof(BQNV) : 8);
    void* ptr = ffiTmpS+pos;
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
    BQNFFIType* t = c(BQNFFIType, ent.o);
    if (t->ty==cty_ptr) { // *any / &any
      pos = ffiTmpAA(sizeof(void*));
      B e = t->a[0].o;
      if (!isC32(e)) thrM("FFI: Complex pointer elements NYI");
      inc(c);
      B cG;
      if (!isArr(c)) thrF("FFI: Expected array corresponding to \"*%S\"", sty_names[o2cu(e)]);
      usz ia = a(c)->ia;
      bool mut = t->a[0].mutPtr;
      switch(o2cu(e)) { default: thrF("FFI: \"*%S\" argument type NYI", sty_names[o2cu(e)]);
        case sty_i8:  cG = mut? taga(cpyI8Arr (c)) : toI8Any (c); break;
        case sty_i16: cG = mut? taga(cpyI16Arr(c)) : toI16Any(c); break;
        case sty_i32: cG = mut? taga(cpyI32Arr(c)) : toI32Any(c); break;
        case sty_f64: cG = mut? taga(cpyF64Arr(c)) : toF64Any(c); break;
        case sty_u8:  { B t=toI16Any(c); i16* tp=i16any_ptr(t); i8*  gp; cG= m_i8arrv(&gp, ia); u8*    np=(u8*   )gp; for (usz i=0; i<ia; i++) np[i]=tp[i]; dec(t); break; }
        case sty_u16: { B t=toI32Any(c); i32* tp=i32any_ptr(t); i16* gp; cG=m_i16arrv(&gp, ia); u16*   np=(u16*  )gp; for (usz i=0; i<ia; i++) np[i]=tp[i]; dec(t); break; }
        case sty_u32: { B t=toF64Any(c); f64* tp=f64any_ptr(t); i32* gp; cG=m_i32arrv(&gp, ia); u32*   np=(u32*  )gp; for (usz i=0; i<ia; i++) np[i]=tp[i]; dec(t); break; }
        case sty_f32: { B t=toF64Any(c); f64* tp=f64any_ptr(t); i32* gp; cG=m_i32arrv(&gp, ia); float* np=(float*)gp; for (usz i=0; i<ia; i++) np[i]=tp[i]; dec(t); break; }
      }
      
      *(void**)(ffiTmpS+pos) = tyany_ptr(cG);
      ffiObjs = vec_addN(ffiObjs, cG);
    } else if (t->ty==cty_repr) { // any:any
      B o2 = t->a[0].o;
      u8 reT = t->a[0].reType;
      u8 reW = t->a[0].reWidth;
      if (isC32(o2)) { // scalar:any
        pos = ffiTmpAA(8);
        u8 et = o2cu(o2);
        u8 etw = sty_w[et]*8;
        if (!isArr(c)) thrF("FFI: Expected array corresponding to \"%S:%c%i\"", sty_names[et], reT, 1<<reW);
        if (a(c)->ia != etw>>reW) thrM("FFI: Bad input array length");
        B cG = toW(reT, reW, inc(c));
        memcpy(ffiTmpS+pos, tyany_ptr(cG), 8); // may over-read, ¯\_(ツ)_/¯
        dec(cG);
      } else { // *scalar:any / &scalar:any
        pos = ffiTmpAA(sizeof(void*));
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
        *(void**)(ffiTmpS+pos) = tyany_ptr(cG);
        ffiObjs = vec_addN(ffiObjs, cG);
      }
    } else thrM("FFI: Unimplemented type");
  }
  return pos;
}

B buildObj(BQNFFIEnt ent, bool anyMut, B* objs, usz* objPos) {
  if (isC32(ent.o)) return m_f64(0); // scalar
  BQNFFIType* t = c(BQNFFIType, ent.o);
  if (t->ty==cty_ptr) { // *any / &any
    B e = t->a[0].o;
    B f = objs[(*objPos)++];
    bool mut = t->a[0].mutPtr;
    if (mut) {
      usz ia = a(f)->ia;
      switch(o2cu(e)) { default: UD;
        case sty_i8: case sty_i16: case sty_i32: case sty_f64: return inc(f);
        case sty_u8:  { u8*   tp=tyarr_ptr(f); i16* rp; B r=m_i16arrv(&rp, ia); for (usz i=0; i<ia; i++) rp[i]=tp[i]; return r; }
        case sty_u16: { u16*  tp=tyarr_ptr(f); i32* rp; B r=m_i32arrv(&rp, ia); for (usz i=0; i<ia; i++) rp[i]=tp[i]; return r; }
        case sty_u32: { u32*  tp=tyarr_ptr(f); f64* rp; B r=m_f64arrv(&rp, ia); for (usz i=0; i<ia; i++) rp[i]=tp[i]; return r; }
        case sty_f32: { float*tp=tyarr_ptr(f); f64* rp; B r=m_f64arrv(&rp, ia); for (usz i=0; i<ia; i++) rp[i]=tp[i]; return r; }
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
  } else thrM("FFI: Unimplemented type");
}

B libffiFn_c2(B t, B w, B x) {
  BoundFn* c = c(BoundFn,t);
  B argObj = c(HArr,c->obj)->a[0];
  B cifObj = c(HArr,c->obj)->a[1];
  ffi_cif* cif = (void*) c(TAlloc,cifObj)->data;
  void* sym = c->w_c2;
  
  usz argn = cif->nargs;
  ffiTmpS = ffiTmpC = ffiTmpE = NULL;
  ffiTmpObj = NULL;
  ffiObjs = emptyHVec();
  ffiTmpX(128);
  ffiTmpAA(argn * sizeof(u32));
  #define argOffs ((u32*)ffiTmpS)
  
  u32 flags = (u64)c->w_c1;
  
  Arr* wa; AS2B wf;
  Arr* xa; AS2B xf;
  if (flags&1) { wa=a(w); wf=TIv(wa,getU); }
  if (flags&2) { xa=a(x); xf=TIv(xa,getU); }
  i32 idxs[2] = {0,0};
  
  BQNFFIEnt* ents = c(BQNFFIType,argObj)->a;
  for (usz i = 0; i < argn; i++) {
    BQNFFIEnt e = ents[i+1];
    B o = e.wholeArg? (e.onW? w : x) : (e.onW? wf : xf)(e.onW?wa:xa, idxs[e.onW]++);
    argOffs[i] = genObj(e, o, e.extra2);
  }
  
  u32 resPos;
  bool simpleRes = isC32(ents[0].o);
  u32 resCType;
  if (simpleRes) {
    resCType = o2cu(ents[0].o);
  } else {
    BQNFFIType* t = c(BQNFFIType, ents[0].o);
    if (t->ty == cty_repr) {
      B o2 = t->a[0].o;
      if (!isC32(o2)) thrM("FFI: Unimplemented result type");
      resCType = o2cu(o2);
    } else thrM("FFI: Unimplemented result type");
  }
  resPos = ffiTmpAA(resCType==sty_a? sizeof(BQNV) : sizeof(ffi_arg)>8? sizeof(ffi_arg) : 8);
  
  void** argPtrs = (void**) (ffiTmpS + ffiTmpAA(argn*sizeof(void*))); // must be the very last alloc to be able to store pointers
  for (usz i = 0; i < argn; i++) argPtrs[i] = ffiTmpS + argOffs[i];
  void* res = ffiTmpS + resPos;
  
  // for (usz i = 0; i < ffiTmpC-ffiTmpS; i++) { // simple hexdump of ffiTmp
  //   if (!(i&15)) printf("%s%p  ", i?"\n":"", (void*)(ffiTmpS+i));
  //   else if (!(i&3)) printf(" ");
  //   printf("%x%x ", ffiTmpS[i]>>4, ffiTmpS[i]&15);
  // }
  // printf("\n");
  
  ffi_call(cif, sym, res, argPtrs);
  
  B r;
  bool resVoid = false;
  if (simpleRes) {
    switch(resCType) { default: UD; // thrM("FFI: Unimplemented type");
      case sty_void: r = m_c32(0); resVoid = true; break;
      case sty_ptr: thrM("FFI: \"*\" unimplemented"); break;
      case sty_a:   r = getB(*(BQNV*)res); break;
      case sty_i8:  r = m_i32(*( i8*)res); break;  case sty_u8:  r = m_i32(*( u8*)res); break;
      case sty_i16: r = m_i32(*(i16*)res); break;  case sty_u16: r = m_i32(*(u16*)res); break;
      case sty_i32: r = m_i32(*(i32*)res); break;  case sty_u32: r = m_f64(*(u32*)res); break;
      case sty_i64: { i64 v = *(i64*)res; if (i64abs(v)>=(1ULL<<53)) thrM("FFI: i64 result absolute value ≥ 2⋆53"); r = m_f64(v); break; }
      case sty_u64: { u64 v = *(u64*)res; if (       v >=(1ULL<<53)) thrM("FFI: u64 result ≥ 2⋆53");                r = m_f64(v); break; }
      case sty_f32: r = m_f64(*(float* )res); break;
      case sty_f64: r = m_f64(*(double*)res); break;
    }
  } else { // cty_repr, scalar:x
    BQNFFIType* t = c(BQNFFIType, ents[0].o);
    u8 et = o2cu(t->a[0].o);
    u8 reT = t->a[0].reType;
    u8 reW = t->a[0].reWidth;
    u8 etw = sty_w[et];
    r = makeRe(reT, reW, res, etw);
  }
  mm_free((Value*)ffiTmpObj);
  
  i32 mutArgs = c->mutCount;
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
  if (!isArr(arg) || a(arg)->ia!=1 || IGetU(arg,0).u!=m_c32('a').u) thrM("FFI: Only \"a\" arguments & return value supported with compile flag FFI=1");
  return (BQNFFIEnt){};
}
#endif




B ffiload_c2(B t, B w, B x) {
  usz xia = a(x)->ia;
  if (xia<2) thrM("FFI: Function specification must have at least two items");
  usz argn = xia-2;
  SGetU(x)
  B name = GetU(x,1);
  vfyStr(name, "FFI", "type");
  
  BQNFFIEnt tRes = ffi_parseType(GetU(x,0), true);
  
  #if FFI==2
    BQNFFIEnt* args; B argObj = m_bqnFFIType(&args, 255, argn+1);
    args[0] = tRes;
    bool one [2]={0,0};
    bool many[2]={0,0};
    i32 mutCount = 0;
    for (usz i = 0; i < argn; i++) {
      BQNFFIEnt e = args[i+1] = ffi_parseType(GetU(x,i+2), false);
      (e.wholeArg? one : many)[e.extra] = true;
      if (e.mutates) mutCount++;
    }
    if (one[0] && many[0]) thrM("FFI: Multiple arguments for 𝕩 specified, but one has '>'");
    if (one[1] && many[1]) thrM("FFI: Multiple arguments for 𝕨 specified, but one has '>'");
  #else
    i32 mutCount = 0;
    for (usz i = 0; i < argn; i++) ffi_parseType(GetU(x,i+2), false);
    (void)tRes;
  #endif
  if (tRes.resSingle && mutCount!=1) thrF("FFI: Return was \"&\", but found %i mutated variables", mutCount);
  
  w = path_rel(nfn_objU(t), w);
  char* ws = toCStr(w);
  void* dl = dlopen(ws, RTLD_NOW);
  
  freeCStr(ws);
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

DEF_FREE(ffiType) {
  usz ia = ((BQNFFIType*)x)->ia;
  for (usz i = 0; i < ia; i++) dec(((BQNFFIType*)x)->a[i].o);
}
void ffiType_visit(Value* x) {
  usz ia = ((BQNFFIType*)x)->ia;
  for (usz i = 0; i < ia; i++) mm_visit(((BQNFFIType*)x)->a[i].o);
}
void ffiType_print(FILE* f, B x) {
  BQNFFIType* t = c(BQNFFIType,x);
  fprintf(f, "cty_%d⟨", t->ty);
  for (usz i=0, ia=t->ia; i<ia; i++) {
    if (i) fprintf(f, ", ");
    printFFIType(f, t->a[i].o);
  }
  fprintf(f, "⟩");
}

void ffi_init() {
  boundFnDesc   = registerNFn(m_str8l("(foreign function)"), boundFn_c1, boundFn_c2);
  foreignFnDesc = registerNFn(m_str8l("(foreign function)"), directFn_c1, directFn_c2);
  TIi(t_ffiType,freeO) = ffiType_freeO;
  TIi(t_ffiType,freeF) = ffiType_freeF;
  TIi(t_ffiType,visit) = ffiType_visit;
  TIi(t_ffiType,print) = ffiType_print;
}

#else
void ffi_init() { }
B ffiload_c2(B t, B w, B x) { thrM("CBQN was compiled without FFI"); }
#endif
