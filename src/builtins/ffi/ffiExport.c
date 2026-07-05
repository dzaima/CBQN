#pragma once

#if !CBQN_EXPORT
  static void bqnffi_init() { }
#else // rest of the file continues under this

#if defined(_WIN32) || defined(_WIN64)
  #define BQN_EXP __attribute__((__visibility__("default"))) __declspec(dllexport)
#elif __EMSCRIPTEN__
  #include <emscripten.h>
  #define BQN_EXP __attribute__((used))
#else
  #define BQN_EXP __attribute__((__visibility__("default")))
#endif
#include "../include/bqnffi.h"
#include "utils/nfns.h"
#include "core/ns.h"
#include "utils/toplevel.h"
#include "vm/load.h"

#ifndef DIRECT_BQNV
  #define DIRECT_BQNV 0
#endif

#ifndef BVW_ARENA_SZ
  #define BVW_ARENA_SZ 62
#endif
#ifndef BQNV_NO_REUSE
  #define BQNV_NO_REUSE 0
#endif

#if DIRECT_BQNV
  static B bv_get(BQNV v) {
    return r_uB(v);
  }
  static BQNV bv_mk(B x) {
    return x.u;
  }
  static B bv_to(BQNV v) {
    return bv_get(v);
  }
#else
  typedef struct BVW BVW;
  typedef struct BVWArena BVWArena;
  static BVW* bvw_freelist = NULL;
  static BVWArena* bvwarena_last = NULL;
  
  // off-CBQN-heap data structures
  struct BVW { // BQNV wrapper
    B value; // RAW_TAGged for freelist entries
  };
  struct BVWArena {
    struct Value;
    BVWArena* prev;
    BVW block[BVW_ARENA_SZ];
  };
  
  #if DEBUG || BQNV_NO_REUSE
    void bqnv_assert(bool b) {
      if (!b) fatal("Incorrect bqnffi.h BQNV usage detected - use-after-free or double-free");
    }
  #else
    #define bqnv_assert assert
  #endif
  
  #define BV2W(X) PTR_FROM_INT(BVW,X)
  static bool isRaw(B x) { return x.u>>48 == RAW_TAG; }
  static B bv_get(BQNV v) {
    B r = BV2W(v)->value;
    bqnv_assert(!isRaw(r));
    return r;
  }
  static NOINLINE BQNV bv_mkF(B x);
  static BQNV bv_mk(B x) {
    BVW* c = bvw_freelist;
    if (RARE(c==NULL)) return bv_mkF(x);
    bqnv_assert(isRaw(c->value));
    bvw_freelist = c(BVW, c->value);
    c->value = x;
    return PTR_TO_U64(c);
  }
  static void bvw_pushEmpty(BVW* p) {
    p->value = tag(bvw_freelist, RAW_TAG);
    bvw_freelist = p;
  }
  static B bv_to(BQNV v) {
    B r = BV2W(v)->value;
    bqnv_assert(!isRaw(r));
    if (BQNV_NO_REUSE) {
      BV2W(v)->value = tag(0, RAW_TAG);
    } else {
      bvw_pushEmpty(BV2W(v));
    }
    return r;
  }
  
  static NOINLINE BQNV bv_mkF(B x) {
    BVWArena* m = mm_alloc(sizeof(BVWArena), t_bvwArena);
    m->prev = bvwarena_last;
    for (ux i = 0; i < BVW_ARENA_SZ; i++) bvw_pushEmpty(&m->block[i]);
    bvwarena_last = m;
    return bv_mk(x);
  }
  
  typedef void (*BVWVisitor)(BVW*);
  typedef void (*BVWArenaVisitor)(BVWArena*);
  static void bvw_forEach(BVWVisitor f, BVWArenaVisitor g) {
    BVWArena* c = bvwarena_last;
    while (c) {
      g(c);
      for (ux i = 0; i < BVW_ARENA_SZ; i++) {
        f(&c->block[i]);
      }
      c = c->prev;
    }
  }
  
  static void bvw_visit(BVW* c) {
    // printf("visiting %p → %016llx\n", c, (long long)c->value.u);
    mm_visit(c->value);
  }
  static void bvwArena_visit(BVWArena* c) {
    mm_visitP(c);
  }
  static void bvw_gcFn(void) {
    bvw_forEach(bvw_visit, bvwArena_visit);
  }
#endif

B boundFn_c1(B t,      B x);
B boundFn_c2(B t, B w, B x);
DEFINE_NFN boundFnDesc;
static void bqnffi_init() {
  boundFnDesc = registerNFn(m_c8vec_0("(foreign function)"), boundFn_c1, boundFn_c2);
  #if !DIRECT_BQNV
    gc_addFn(bvw_gcFn);
    TIi(t_bvwArena,visit) = noop_visit;
  #endif
}

BQN_EXP void bqn_free(BQNV v) {
  dec(bv_to(v));
}
BQN_EXP BQNV bqn_copy(BQNV v) {
  return bv_mk(inc(bv_get(v)));
}

BQN_EXP double   bqn_toF64 (BQNV v) { return o2fG(bv_to(v)); }
BQN_EXP uint32_t bqn_toChar(BQNV v) { return o2cG(bv_to(v)); }
BQN_EXP double   bqn_readF64 (BQNV v) { return o2fG(bv_get(v)); }
BQN_EXP uint32_t bqn_readChar(BQNV v) { return o2cG(bv_get(v)); }

BQN_EXP void bqn_init() {
  cbqn_init();
}

B type_c1(B t, B x);
static i32 typeInt(B x) { // doesn't consume
  return o2i(C1(type, inc(x)));
}
BQN_EXP int bqn_type(BQNV v) {
  return typeInt(bv_get(v));
}

#define RUN_END_R(R) ({ AUTO r_ = (R); run_end(r); r_; })

BQN_EXP BQNV bqn_call1(BQNV f, BQNV x) {
  Run r = run_start();
  return RUN_END_R(bv_mk(c1(bv_get(f), inc(bv_get(x)))));
}
BQN_EXP BQNV bqn_call2(BQNV f, BQNV w, BQNV x) {
  Run r = run_start();
  return RUN_END_R(bv_mk(c2(bv_get(f), inc(bv_get(w)), inc(bv_get(x)))));
}

BQN_EXP BQNV bqn_eval(BQNV src) {
  Run r = run_start();
  return RUN_END_R(bv_mk(bqn_exec(inc(bv_get(src)), defaultUnknownState())));
}
BQN_EXP BQNV bqn_evalCStr(const char* str) {
  Run r = run_start();
  return RUN_END_R(bv_mk(bqn_exec(utf8Decode0(str), defaultUnknownState())));
}


BQN_EXP size_t bqn_bound(BQNV a) { return IA(bv_get(a)); }
BQN_EXP size_t bqn_rank(BQNV a) { return RNK(bv_get(a)); }
BQN_EXP void bqn_shape(BQNV a, size_t* buf) { B b = bv_get(a);
  ur r = RNK(b);
  usz* sh = SH(b);
  vfor (usz i = 0; i < r; i++) buf[i] = sh[i];
}
BQN_EXP BQNV bqn_pick(BQNV a, size_t pos) {
  return bv_mk(IGet(bv_get(a),pos));
}

// can't use the regular copy-from-array helpers as those assume the ability to write past the end
BQN_EXP void bqn_readI8Arr (BQNV a, i8*   buf) { B c = toI8Any (incG(bv_get(a))); memcpy(buf, i8any_ptr (c), IA(c) * 1); decG(c); }
BQN_EXP void bqn_readI16Arr(BQNV a, i16*  buf) { B c = toI16Any(incG(bv_get(a))); memcpy(buf, i16any_ptr(c), IA(c) * 2); decG(c); }
BQN_EXP void bqn_readI32Arr(BQNV a, i32*  buf) { B c = toI32Any(incG(bv_get(a))); memcpy(buf, i32any_ptr(c), IA(c) * 4); decG(c); }
BQN_EXP void bqn_readF64Arr(BQNV a, f64*  buf) { B c = toF64Any(incG(bv_get(a))); memcpy(buf, f64any_ptr(c), IA(c) * 8); decG(c); }
BQN_EXP void bqn_readC8Arr (BQNV a, u8*   buf) { B c = toC8Any (incG(bv_get(a))); memcpy(buf, c8any_ptr (c), IA(c) * 1); decG(c); }
BQN_EXP void bqn_readC16Arr(BQNV a, u16*  buf) { B c = toC16Any(incG(bv_get(a))); memcpy(buf, c16any_ptr(c), IA(c) * 2); decG(c); }
BQN_EXP void bqn_readC32Arr(BQNV a, u32*  buf) { B c = toC32Any(incG(bv_get(a))); memcpy(buf, c32any_ptr(c), IA(c) * 4); decG(c); }
BQN_EXP void bqn_readObjArr(BQNV a, BQNV* buf) { B b = bv_get(a);
  usz ia = IA(b);
  if (DIRECT_BQNV && sizeof(BQNV)==sizeof(B)) {
    COPY_TO(buf, el_B, 0, b, 0, ia);
  } else {
    B* p = arr_bptr(b);
    if (p!=NULL) {
      for (usz i = 0; i < ia; i++) buf[i] = bv_mk(inc(p[i]));
    } else {
      SGet(b)
      for (usz i = 0; i < ia; i++) buf[i] = bv_mk(Get(b, i));
    }
  }
}

BQN_EXP bool bqn_hasField(BQNV ns, BQNV name) {
  return !q_N(ns_getNU(bv_get(ns), bv_get(name), false));
}
BQN_EXP BQNV bqn_getField(BQNV ns, BQNV name) {
  return bv_mk(inc(ns_getNU(bv_get(ns), bv_get(name), true)));
}

BQN_EXP BQNV bqn_makeF64(double d) { return bv_mk(m_f64(d)); }
BQN_EXP BQNV bqn_makeChar(uint32_t c) { return bv_mk(m_c32(c)); }


static usz calcIA(size_t rank, const size_t* shape) {
  if (rank>UR_MAX) thrM("Rank too large");
  usz r = 1;
  PLAINLOOP for (size_t i = 0; i < rank; i++) if (MUL_ON(r, shape[i])) thrM("Size too large");
  return r;
}
static void copyBData(B* r, const BQNV* data, usz ia) {
  for (size_t i = 0; i < ia; i++) {
    r[i] = bv_to(data[i]);
  }
}
#define CPYSH(R) usz* sh = arr_shAlloc((Arr*)(R), r0); \
  if (sh) PLAINLOOP for (size_t i = 0; RARE(i < r0); i++) sh[i] = sh0[i];

BQN_EXP BQNV bqn_makeI8Arr (size_t r0, const size_t* sh0, const i8*   data) { usz ia=calcIA(r0,sh0); i8*  rp; Arr* r = m_i8arrp (&rp,ia); CPYSH(r); memcpy(rp,data,ia*1); return bv_mk(taga(r)); }
BQN_EXP BQNV bqn_makeI16Arr(size_t r0, const size_t* sh0, const i16*  data) { usz ia=calcIA(r0,sh0); i16* rp; Arr* r = m_i16arrp(&rp,ia); CPYSH(r); memcpy(rp,data,ia*2); return bv_mk(taga(r)); }
BQN_EXP BQNV bqn_makeI32Arr(size_t r0, const size_t* sh0, const i32*  data) { usz ia=calcIA(r0,sh0); i32* rp; Arr* r = m_i32arrp(&rp,ia); CPYSH(r); memcpy(rp,data,ia*4); return bv_mk(taga(r)); }
BQN_EXP BQNV bqn_makeF64Arr(size_t r0, const size_t* sh0, const f64*  data) { usz ia=calcIA(r0,sh0); f64* rp; Arr* r = m_f64arrp(&rp,ia); CPYSH(r); memcpy(rp,data,ia*8); return bv_mk(taga(r)); }
BQN_EXP BQNV bqn_makeC8Arr (size_t r0, const size_t* sh0, const u8*   data) { usz ia=calcIA(r0,sh0); u8*  rp; Arr* r = m_c8arrp (&rp,ia); CPYSH(r); memcpy(rp,data,ia*1); return bv_mk(taga(r)); }
BQN_EXP BQNV bqn_makeC16Arr(size_t r0, const size_t* sh0, const u16*  data) { usz ia=calcIA(r0,sh0); u16* rp; Arr* r = m_c16arrp(&rp,ia); CPYSH(r); memcpy(rp,data,ia*2); return bv_mk(taga(r)); }
BQN_EXP BQNV bqn_makeC32Arr(size_t r0, const size_t* sh0, const u32*  data) { usz ia=calcIA(r0,sh0); u32* rp; Arr* r = m_c32arrp(&rp,ia); CPYSH(r); memcpy(rp,data,ia*4); return bv_mk(taga(r)); }
BQN_EXP BQNV bqn_makeObjArr(size_t r0, const size_t* sh0, const BQNV* data) { usz ia=calcIA(r0,sh0); HArr_p r = m_harrUp(ia); copyBData(r.a,data,ia); NOGC_E; CPYSH(r.c); return bv_mk(r.b); }

BQN_EXP BQNV bqn_makeI8Vec (size_t len, const i8*   data) { i8*  rp; B r = m_i8arrv (&rp,len); memcpy(rp,data,len*1); return bv_mk(r); }
BQN_EXP BQNV bqn_makeI16Vec(size_t len, const i16*  data) { i16* rp; B r = m_i16arrv(&rp,len); memcpy(rp,data,len*2); return bv_mk(r); }
BQN_EXP BQNV bqn_makeI32Vec(size_t len, const i32*  data) { i32* rp; B r = m_i32arrv(&rp,len); memcpy(rp,data,len*4); return bv_mk(r); }
BQN_EXP BQNV bqn_makeF64Vec(size_t len, const f64*  data) { f64* rp; B r = m_f64arrv(&rp,len); memcpy(rp,data,len*8); return bv_mk(r); }
BQN_EXP BQNV bqn_makeC8Vec (size_t len, const u8*   data) { u8*  rp; B r = m_c8arrv (&rp,len); memcpy(rp,data,len*1); return bv_mk(r); }
BQN_EXP BQNV bqn_makeC16Vec(size_t len, const u16*  data) { u16* rp; B r = m_c16arrv(&rp,len); memcpy(rp,data,len*2); return bv_mk(r); }
BQN_EXP BQNV bqn_makeC32Vec(size_t len, const u32*  data) { u32* rp; B r = m_c32arrv(&rp,len); memcpy(rp,data,len*4); return bv_mk(r); }
BQN_EXP BQNV bqn_makeObjVec(size_t len, const BQNV* data) { HArr_p r = m_harrUv(len); copyBData(r.a,data,len); NOGC_E;return bv_mk(r.b); }
BQN_EXP BQNV bqn_makeUTF8Str(size_t len, const char* str) { return bv_mk(utf8Decode(str, len)); }

typedef struct BoundFn {
  struct NFn;
  void* w_c1;
  void* w_c2;
} BoundFn;

B boundFn_c1(B t,      B x) { BoundFn* c = c(BoundFn,t); return bv_to(((bqn_boundFn1)c->w_c1)(bv_mk(inc(c->obj)),           bv_mk(x))); }
B boundFn_c2(B t, B w, B x) { BoundFn* c = c(BoundFn,t); return bv_to(((bqn_boundFn2)c->w_c2)(bv_mk(inc(c->obj)), bv_mk(w), bv_mk(x))); }

static NFn* m_ffiFn(ux size, NFnDesc* desc, B obj, FC1 c1, FC2 c2) {
  NFn* r = mm_alloc(size, t_nfn);
  nfn_lateInit((NFn*)r, desc);
  r->obj = obj;
  r->c1 = c1;
  r->c2 = c2;
  return r;
}
static B m_boundFn(B obj, FC1 c1, FC2 c2, void* wc1, void* wc2) {
  BoundFn* r = (BoundFn*) m_ffiFn(sizeof(BoundFn), boundFnDesc, obj, c1, c2);
  r->w_c1 = wc1;
  r->w_c2 = wc2;
  return tag(r, FUN_TAG);
}
BQN_EXP BQNV bqn_makeBoundFn1(bqn_boundFn1 f, BQNV obj) { return bv_mk(m_boundFn(inc(bv_get(obj)), boundFn_c1, c2_bad, f, NULL)); }
BQN_EXP BQNV bqn_makeBoundFn2(bqn_boundFn2 f, BQNV obj) { return bv_mk(m_boundFn(inc(bv_get(obj)), c1_bad, boundFn_c2, NULL, f)); }

static const u8 typeMap[] = {
  [el_bit] = elt_unk,
  [el_B  ] = elt_unk,
  [el_i8 ] = elt_i8,  [el_c8 ] = elt_c8,
  [el_i16] = elt_i16, [el_c16] = elt_c16,
  [el_i32] = elt_i32, [el_c32] = elt_c32,
  [el_f64] = elt_f64,
};
BQN_EXP BQNElType bqn_directArrType(BQNV a) {
  B b = bv_get(a);
  if (!isArr(b)) return elt_unk;
  return typeMap[TI(b,elType)];
}
BQN_EXP const i8*  bqn_directI8 (BQNV a) { return i8any_ptr (bv_get(a)); }
BQN_EXP const i16* bqn_directI16(BQNV a) { return i16any_ptr(bv_get(a)); }
BQN_EXP const i32* bqn_directI32(BQNV a) { return i32any_ptr(bv_get(a)); }
BQN_EXP const f64* bqn_directF64(BQNV a) { return f64any_ptr(bv_get(a)); }
BQN_EXP const u8*  bqn_directC8 (BQNV a) { return c8any_ptr (bv_get(a)); }
BQN_EXP const u16* bqn_directC16(BQNV a) { return c16any_ptr(bv_get(a)); }
BQN_EXP const u32* bqn_directC32(BQNV a) { return c32any_ptr(bv_get(a)); }

void ffiFn_visit(Value* v) { mm_visit(((BoundFn*)v)->obj); }
DEF_FREE(ffiFn) { dec(((BoundFn*)x)->obj); }


#endif // CBQN_EXPORT
