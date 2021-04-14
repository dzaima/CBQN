#pragma once
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdarg.h>

#define i8  int8_t
#define u8 uint8_t
#define i16  int16_t
#define u16 uint16_t
#define i32  int32_t
#define u32 uint32_t
#define i64  int64_t
#define u64 uint64_t
#define f64 double
#define I32_MAX ((i32)((1LL<<31)-1))
#define U16_MAX ((u16)-1)
#define UD __builtin_unreachable();
#define NOINLINE __attribute__ ((noinline))

#define usz u32
#define ur u8

#define CTR_FOR(F)
#define CTR_DEF(N) u64 N;
#define CTR_PRINT(N) printf(#N ": %lu\n", N);
CTR_FOR(CTR_DEF)

#ifdef DEBUG
  #include<assert.h>
  #define VALIDATE(x) validate(x)
  #define VALIDATEP(x) validateP(x)
#else
  #define assert(x) {if (!(x)) __builtin_unreachable();}
  #define VALIDATE(x) (x)
  #define VALIDATEP(x) (x)
#endif

#define fsizeof(T,F,E,n) (offsetof(T, F) + sizeof(E)*(n)) // type; FAM name; FAM type; amount
#define ftag(x) ((u64)(x) << 48)
#define tag(v, t) b(((u64)(v)) | ftag(t))
                                        // .111111111110000000000000000000000000000000000000000000000000000 infinity
                                        // .111111111111000000000000000000000000000000000000000000000000000 qNaN
                                        // .111111111110nnn................................................ sNaN aka tagged aka not f64, if nnn≠0
                                        // 0111111111110................................................... direct value with no need of refcounting
const u16 C32_TAG = 0b0111111111110001; // 0111111111110001................00000000000ccccccccccccccccccccc char
const u16 TAG_TAG = 0b0111111111110010; // 0111111111110010................nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn special value (0=nothing, 1=undefined var, 2=bad header; 3=optimized out; 4=error?; 5=no fill)
const u16 VAR_TAG = 0b0111111111110011; // 0111111111110011ddddddddddddddddnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn variable reference
const u16 I32_TAG = 0b0111111111110111; // 0111111111110111................nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn 32-bit int; unused
const u16 MD1_TAG = 0b1111111111110010; // 1111111111110010ppppppppppppppppppppppppppppppppppppppppppppp000 1-modifier
const u16 MD2_TAG = 0b1111111111110011; // 1111111111110011ppppppppppppppppppppppppppppppppppppppppppppp000 2-modifier
const u16 FUN_TAG = 0b1111111111110100; // 1111111111110100ppppppppppppppppppppppppppppppppppppppppppppp000 function
const u16 NSP_TAG = 0b1111111111110101; // 1111111111110101ppppppppppppppppppppppppppppppppppppppppppppp000 namespace maybe?
const u16 OBJ_TAG = 0b1111111111110110; // 1111111111110110ppppppppppppppppppppppppppppppppppppppppppppp000 custom object (e.g. bigints)
const u16 ARR_TAG = 0b1111111111110111; // 1111111111110111ppppppppppppppppppppppppppppppppppppppppppppp000 array (everything else is an atom)
const u16 VAL_TAG = 0b1111111111110   ; // 1111111111110................................................... pointer to Value, needs refcounting

enum Type {
  /* 0*/ t_empty, // empty bucket placeholder
  /* 1*/ t_fun_def, t_fun_block,
  /* 3*/ t_md1_def, t_md1_block,
  /* 5*/ t_md2_def, t_md2_block,
  /* 7*/ t_shape, // doesn't get visited, shouldn't be unallocated by gc
  
  /* 8*/ t_fork, t_atop,
  /*10*/ t_md1D, t_md2D, t_md2H,
  
  /*13*/ t_harr  , t_i32arr  , t_fillarr  , t_c32arr  ,
  /*17*/ t_hslice, t_i32slice, t_fillslice, t_c32slice,
  
  /*21*/ t_comp, t_block, t_body, t_scope,
  /*25*/ t_freed,
  t_COUNT
};
char* format_type(u8 u) {
  switch(u) { default: return"(unknown type)";
    case t_empty:return"empty"; case t_shape:return"shape";
    case t_fun_def:return"fun_def"; case t_fun_block:return"fun_block";
    case t_md1_def:return"md1_def"; case t_md1_block:return"md1_block";
    case t_md2_def:return"md2_def"; case t_md2_block:return"md2_block";
    case t_fork:return"fork"; case t_atop:return"atop";
    case t_md1D:return"md1D"; case t_md2D:return"md2D"; case t_md2H:return"md2H";
    case t_harr  :return"harr"  ; case t_i32arr  :return"i32arr"  ; case t_fillarr  :return"fillarr"  ; case t_c32arr  :return"c32arr"  ;
    case t_hslice:return"hslice"; case t_i32slice:return"i32slice"; case t_fillslice:return"fillslice"; case t_c32slice:return"c32slice";
    case t_comp:return"comp"; case t_block:return"block"; case t_body:return"body"; case t_scope:return"scope";
    case t_freed:return"(freed by GC)";
  }
}

enum PrimFns {
  pf_none,
  pf_add, pf_sub, pf_mul, pf_div, pf_pow, pf_floor, pf_eq, pf_le, pf_log, // arith.c
  pf_shape, pf_pick, pf_ud, pf_pair, pf_fne, pf_feq, pf_lt, pf_rt, pf_fmtF, pf_fmtN, // sfns.c
  pf_fork, pf_atop, pf_md1d, pf_md2d, // derv.c
  pf_type, pf_decp, pf_primInd, pf_glyph, pf_fill, // sysfn.c
  pf_grLen, pf_grOrd, pf_asrt, pf_sys, pf_internal, // sysfn.c
};
char* format_pf(u8 u) {
  switch(u) { default: case pf_none: return"(unknown fn)";
    case pf_add:return"+"; case pf_sub:return"-"; case pf_mul:return"×"; case pf_div:return"÷"; case pf_pow:return"⋆"; case pf_floor:return"⌊"; case pf_eq:return"="; case pf_le:return"≤"; case pf_log:return"⋆⁼";
    case pf_shape:return"⥊"; case pf_pick:return"⊑"; case pf_ud:return"↕"; case pf_pair:return"{𝕨‿𝕩}"; case pf_fne:return"≢"; case pf_feq:return"≡"; case pf_lt:return"⊣"; case pf_rt:return"⊢"; case pf_fmtF:case pf_fmtN:return"⍕";
    case pf_fork:return"(fork)"; case pf_atop:return"(atop)"; case pf_md1d:return"(derived 1-modifier)"; case pf_md2d:return"(derived 2-modifier)";
    case pf_type:return"•Type"; case pf_decp:return"•Decompose"; case pf_primInd:return"•PrimInd"; case pf_glyph:return"•Glyph"; case pf_fill:return"•FillFn"; 
    case pf_grLen:return"•GroupLen"; case pf_grOrd:return"•groupOrd"; case pf_asrt:return"!"; case pf_sys:return"•getsys"; case pf_internal:return"•Internal"; 
  }
}
enum PrimMd1 {
  pm1_none,
  pm1_tbl, pm1_scan, // md1.c
};
char* format_pm1(u8 u) {
  switch(u) {
    default: case pf_none: return"(unknown 1-modifier)";
    case pm1_tbl: return"⌜"; case pm1_scan: return"`";
  }
}
enum PrimMd2 {
  pm2_none,
  pm2_val, pm2_fillBy, // md2.c
};
char* format_pm2(u8 u) {
  switch(u) {
    default: case pf_none: return"(unknown 1-modifier)";
    case pm2_val: return"⊘"; case pm2_fillBy: return"•_fillBy_";
  }
}


#ifdef USE_VALGRIND
#include <valgrind/valgrind.h>
#include <valgrind/memcheck.h>
void pst(char* msg) {
  VALGRIND_PRINTF_BACKTRACE("%s", msg);
}
#endif

typedef union B {
  u64 u;
  i64 s;
  f64 f;
} B;
#define b(x) ((B)(x))

typedef struct Value {
  i32 refc;  // plain old reference count
  u8 mmInfo; // bucket size, mark&sweep bits when that's needed; currently unused
  u8 flags;  // is sorted/a permutation/whatever in the future, currently primitive index for self-hosted runtime
  u8 type;   // access into TypeInfo among generally knowing what type of object this is
  ur extra;  // whatever object-specific stuff. Rank for arrays, id for functions
  #ifdef OBJ_COUNTER
  u64 uid;
  #endif
} Value;
typedef struct Arr {
  struct Value;
  usz ia;
  usz* sh;
} Arr;

// memory manager
typedef void (*V2v)(Value*);
typedef void (*vfn)();

void gc_add(B x); // add permanent root object
void gc_addFn(vfn f); // add function that calls mm_visit/mm_visitP for dynamic roots
void gc_disable(); // gc starts disabled
void gc_enable();  // can be nested (e.g. gc_disable(); gc_disable(); gc_enable(); will keep gc disabled until another gc_enable(); )
void gc_maybeGC(); // gc if that seems necessary
void gc_forceGC(); // force a gc; who knows what happens if gc is disabled (probably should error)
void gc_visitRoots();
void* mm_allocN(usz sz, u8 type);
void mm_free(Value* x);
void mm_visit(B x);
void mm_visitP(void* x);
u64  mm_round(usz x);
u64  mm_size(Value* x);

u64  mm_heapAllocated();
u64  mm_heapUsed();

void mm_forHeap(V2v f);
B mm_alloc(usz sz, u8 type, u64 tag) {
  assert(tag>1LL<<16 || tag==0); // make sure it's `ftag`ged :|
  return b((u64)mm_allocN(sz,type) | tag);
}

// some primitive actions
void dec(B x);
B    inc(B x);
void ptr_dec(void* x);
void ptr_inc(void* x);
void print(B x);
void arr_print(B x);
B m_v1(B a               );
B m_v2(B a, B b          );
B m_v3(B a, B b, B c     );
B m_v4(B a, B b, B c, B d);

#define c(T,x) ((T*)((x).u&0xFFFFFFFFFFFFull))
#define v(x) c(Value, x)
#define a(x) c(Arr  , x)
#define  rnk(x  ) (v(x)->extra)   // expects argument to be Arr
#define srnk(x,r) (v(x)->extra=(r))
#define VT(x,t) assert(isVal(x) && v(x)->type==t)

void print_vmStack();
#ifdef DEBUG
  B validate(B x);
  Value* validateP(Value* x);
#endif
B err(char* s) {
  puts(s); fflush(stdout);
  print_vmStack();
  __builtin_trap();
  exit(1);
}

// tag checks
#ifdef ATOM_I32
bool isI32(B x) { return (x.u>>48) == I32_TAG; }
#else
bool isI32(B x) { return false; }
#endif
bool isFun(B x) { return (x.u>>48) == FUN_TAG; }
bool isArr(B x) { return (x.u>>48) == ARR_TAG; }
bool isC32(B x) { return (x.u>>48) == C32_TAG; }
bool isVar(B x) { return (x.u>>48) == VAR_TAG; }
bool isMd1(B x) { return (x.u>>48) == MD1_TAG; }
bool isMd2(B x) { return (x.u>>48) == MD2_TAG; }
bool isMd (B x) { return (x.u>>49) ==(MD2_TAG>>1); }
bool isNsp(B x) { return (x.u>>48) == NSP_TAG; }
bool isObj(B x) { return (x.u>>48) == OBJ_TAG; }
// bool isVal(B x) { return ((x.u>>51) == VAL_TAG)  &  ((x.u<<13) != 0); }
// bool isF64(B x) { return ((x.u>>51&0xFFF) != 0xFFE)  |  ((x.u<<1)==(b(1.0/0.0).u<<1)); }
bool isVal(B x) { return (x.u - (((u64)VAL_TAG<<51) + 1)) < ((1ull<<51) - 1); } // ((x.u>>51) == VAL_TAG)  &  ((x.u<<13) != 0);
bool isF64(B x) { return (x.u<<1) - ((0xFFEull<<52) + 2) >= (1ull<<52) - 2; }

bool isAtm(B x) { return !isVal(x); }

// shape mess
typedef struct ShArr {
  struct Value;
  usz a[];
} ShArr;
ShArr* shObj(B x) { return (ShArr*)((u64)a(x)->sh-offsetof(ShArr,a)); }
void decSh(B x) { if (rnk(x)>1) ptr_dec(shObj(x)); }

void arr_shVec(B x, usz ia) {
  a(x)->ia = ia;
  srnk(x, 1);
  a(x)->sh = &a(x)->ia;
}
bool gotShape[t_COUNT];
usz* arr_shAlloc(B x, usz ia, usz r) {
  a(x)->ia = ia;
  srnk(x,r);
  if (r>1) return a(x)->sh = ((ShArr*)mm_allocN(fsizeof(ShArr, a, usz, r), t_shape))->a;
  a(x)->sh = &a(x)->ia;
  return 0;
}
void arr_shCopy(B n, B o) { // copy shape from o to n
  assert(isArr(o));
  a(n)->ia = a(o)->ia;
  ur r = srnk(n,rnk(o));
  if (r<=1) {
    a(n)->sh = &a(n)->ia;
  } else {
    ptr_inc(shObj(o));
    a(n)->sh = a(o)->sh;
  }
}
bool shEq(B w, B x) { assert(isArr(w)); assert(isArr(x));
  ur wr = rnk(w); usz* wsh = a(w)->sh;
  ur xr = rnk(x); usz* xsh = a(x)->sh;
  if (wr!=xr) return false;
  if (wsh==xsh) return true;
  return memcmp(wsh,xsh,wr*sizeof(usz))==0;
}
usz arr_csz(B x) {
  ur xr = rnk(x);
  if (xr<=1) return 1;
  usz* sh = a(x)->sh;
  usz r = 1;
  for (i32 i = 1; i < xr; i++) r*= sh[i];
  return r;
}

// make objects
B m_arr(usz min, u8 type) { return mm_alloc(min, type, ftag(ARR_TAG)); }
B m_f64(f64 n) { assert(isF64(b(n))); return b(n); } // assert just to make sure we're actually creating a float
B m_c32(i32 n) { return tag(n, C32_TAG); } // TODO check validity?
#ifdef ATOM_I32
B m_i32(i32 n) { return tag(n, I32_TAG); }
#else
B m_i32(i32 n) { return m_f64(n); }
#endif
B m_error() { return tag(4, TAG_TAG); }
B m_usz(usz n) { return n==(i32)n? m_i32(n) : m_f64(n); }

i32 o2i  (B x) { if ((i32)x.f!=x.f) err("o2i"": expected integer"); return (i32)x.f; }
usz o2s  (B x) { if ((usz)x.f!=x.f) err("o2s"": expected integer"); return (usz)x.f; }
i64 o2i64(B x) { if ((i64)x.f!=x.f) err("o2i64: expected integer"); return (i64)x.f; }
i32 o2iu (B x) { return isI32(x)? (i32)(u32)x.u : (i32)x.f; }
bool q_i32(B x) { return isI32(x) || isF64(x)&(x.f==(i32)x.f); }


typedef struct Slice {
  struct Arr;
  B p;
} Slice;
void slice_free(B x) { dec(c(Slice,x)->p); decSh(x); }
void slice_visit(B x) { mm_visit(c(Slice,x)->p); }
void slice_print(B x) { arr_print(x); }



typedef void (*B2v)(B);
typedef B (* BS2B)(B, usz);
typedef B (*BSS2B)(B, usz, usz);
typedef B (*    B2B)(B);
typedef B (*   BB2B)(B, B);
typedef B (*  BBB2B)(B, B, B);
typedef B (* BBBB2B)(B, B, B, B);
typedef B (*BBBBB2B)(B, B, B, B, B);
typedef bool (*B2b)(B);

typedef struct TypeInfo {
  B2v free;   // expects refc==0, type may be cleared to t_empty for garbage collection
  BS2B get;   // increments result, doesn't consume arg; TODO figure out if this should never allocate, so GC wouldn't happen
  BS2B getU;  // like get, but doesn't increment result (mostly equivalent to `B t=get(…); dec(t); t`)
  BB2B  m1_d; // consume all args; (m, f)
  BBB2B m2_d; // consume all args; (m, f, g)
  BS2B slice; // consumes; create slice from given starting position; add ia, rank, shape yourself
  B2b canStore; // doesn't consume
  
  B2v print;  // doesn't consume
  B2v visit;  // call mm_visit for all referents
  B2B decompose; // consumes; must return a HArr
  bool isArr;
} TypeInfo;
TypeInfo ti[t_COUNT];
#define TI(x) (ti[v(x)->type])


void do_nothing(B x) { }
void empty_free(B x) { err("FREEING EMPTY\n"); }
void builtin_free(B x) { err("FREEING BUILTIN\n"); }
void def_visit(B x) { printf("(no visit for %d=%s)\n", v(x)->type, format_type(v(x)->type)); }
void freeed_visit(B x) { err("visiting t_freed\n"); }
void def_print(B x) { printf("(%d=%s)", v(x)->type, format_type(v(x)->type)); }
B    def_get (B x, usz n) { return inc(x); }
B    def_getU(B x, usz n) { return x; }
B    def_m1_d(B m, B f     ) { return err("cannot derive this"); }
B    def_m2_d(B m, B f, B g) { return err("cannot derive this"); }
B    def_slice(B x, usz s) { return err("cannot slice non-array!"); }
B    def_decompose(B x) { return m_v2(m_i32((isFun(x)|isMd(x))? 0 : -1),x); }
bool def_canStore(B x) { return false; }
B bi_nothing, bi_noVar, bi_badHdr, bi_optOut, bi_noFill;
void hdr_init() {
  for (i32 i = 0; i < t_COUNT; i++) {
    ti[i].free  = do_nothing;
    ti[i].visit = def_visit;
    ti[i].get   = def_get;
    ti[i].getU  = def_get;
    ti[i].print = def_print;
    ti[i].m1_d  = def_m1_d;
    ti[i].m2_d  = def_m2_d;
    ti[i].isArr = false;
    ti[i].decompose = def_decompose;
    ti[i].slice     = def_slice;
    ti[i].canStore  = def_canStore;
  }
  ti[t_empty].free = empty_free;
  ti[t_freed].free = do_nothing;
  ti[t_freed].visit = freeed_visit;
  ti[t_shape].visit = do_nothing;
  ti[t_fun_def].visit = ti[t_md1_def].visit = ti[t_md2_def].visit = do_nothing;
  ti[t_fun_def].free  = ti[t_md1_def].free  = ti[t_md2_def].free  = builtin_free;
  bi_nothing = tag(0, TAG_TAG);
  bi_noVar   = tag(1, TAG_TAG);
  bi_badHdr  = tag(2, TAG_TAG);
  bi_optOut  = tag(3, TAG_TAG);
  bi_noFill  = tag(5, TAG_TAG);
  assert((MD1_TAG>>1) == (MD2_TAG>>1)); // just to be sure it isn't changed incorrectly, `isMd` depends on this
}

bool isNothing(B b) { return b.u==bi_nothing.u; }


// refcount
static inline void value_free(B x, Value* vx) {
  ti[vx->type].free(x);
  mm_free(vx);
}
static NOINLINE void value_freeR1(Value* x) { value_free(tag(x, OBJ_TAG), x); }
static NOINLINE void value_freeR2(Value* vx, B x) { value_free(x, vx); }
void dec(B x) {
  if (!isVal(VALIDATE(x))) return;
  Value* vx = v(x);
  if(!--vx->refc) value_free(x, vx);
}
B inc(B x) {
  if (isVal(VALIDATE(x))) v(x)->refc++;
  return x;
}
void ptr_dec(void* x) { if(!--VALIDATEP((Value*)x)->refc) value_free(tag(x, OBJ_TAG), x); }
void ptr_inc(void* x) { VALIDATEP((Value*)x)->refc++; }
void ptr_decR(void* x) { if(!--VALIDATEP((Value*)x)->refc) value_freeR1(x); }
void decR(B x) {
  if (!isVal(VALIDATE(x))) return;
  Value* vx = v(x);
  if(!--vx->refc) value_freeR2(vx, x);
}
bool reusable(B x) { return v(x)->refc==1; }



void printUTF8(u32 c);

void print(B x) {
  if (isF64(x)) {
    printf("%g", x.f);
  } else if (isC32(x)) {
    if ((u32)x.u>=32) { printf("'"); printUTF8((u32)x.u); printf("'"); }
    else if((u32)x.u>15) printf("\\x%x", (u32)x.u);
    else printf("\\x0%x", (u32)x.u);
  } else if (isI32(x)) {
    printf("%d", (i32)x.u);
  } else if (isVal(x)) {
    #ifdef DEBUG
    if (isVal(x) && (v(x)->type==t_freed || v(x)->type==t_empty)) {
      u8 t = v(x)->type;
      v(x)->type = v(x)->flags;
      printf(t==t_freed?"FREED:":"EMPTY:");
      TI(x).print(x);
      v(x)->type = t;
      return;
    }
    #endif
    TI(x).print(x);
  }
  else if (isVar(x)) printf("(var d=%d i=%d)", (u16)(x.u>>32), (i32)x.u);
  else if (x.u==bi_nothing.u) printf("·");
  else if (x.u==bi_optOut.u) printf("(value optimized out)");
  else if (x.u==bi_noVar.u) printf("(unset variable placeholder)");
  else if (x.u==bi_badHdr.u) printf("(bad header note)");
  else printf("(todo tag %lx)", x.u>>48);
}
void printRaw(B x) {
  if (isAtm(x)) {
    if (isF64(x)) printf("%g", x.f);
    else if (isC32(x)) printUTF8((u32)x.u);
    else err("bad printRaw argument: atom arguments should be either numerical or characters");
  } else {
    usz ia = a(x)->ia;
    BS2B xget = TI(x).get;
    for (usz i = 0; i < ia; i++) {
      B c = xget(x,i);
      if (c.u==0 | c.u==bi_noFill.u) { printf(" "); continue; }
      if (!isC32(c)) err("bad printRaw argument: expected all character items");
      printUTF8((u32)c.u);
    }
  }
}

B eq_c2(B t, B w, B x);
bool equal(B w, B x) { // doesn't consume
  bool wa = isArr(w);
  bool xa = isArr(x);
  if (wa!=xa) return false;
  if (!wa) return o2iu(eq_c2(bi_nothing, inc(w), inc(x)))?1:0;
  if (!shEq(w,x)) return false;
  usz ia = a(x)->ia;
  BS2B xget = TI(x).get;
  BS2B wget = TI(w).get;
  for (usz i = 0; i < ia; i++) {
    B wc=wget(w,i); B xc=xget(x,i); // getdec
    bool eq=equal(wc,xc);
    decR(wc); decR(xc);
    if(!eq) return false;
  }
  return true;
}



typedef struct Fun {
  struct Value;
  BB2B c1;
  BBB2B c2;
} Fun;

B c1_invalid(B f,      B x) { return err("This function can't be called monadically"); }
B c2_invalid(B f, B w, B x) { return err("This function can't be called dyadically"); }

NOINLINE B c1_rare(B f, B x) { dec(x);
  if (isMd(f)) return err("Calling a modifier");
  return inc(VALIDATE(f));
}
NOINLINE B c2_rare(B f, B w, B x) { dec(w); dec(x);
  if (isMd(f)) return err("Calling a modifier");
  return inc(VALIDATE(f));
}
B c1(B f, B x) { // BQN-call f monadically; consumes x
  if (isFun(f)) return VALIDATE(c(Fun,f)->c1(f, x));
  return c1_rare(f, x);
}
B c2(B f, B w, B x) { // BQN-call f dyadically; consumes w,x
  if (isFun(f)) return VALIDATE(c(Fun,f)->c2(f, w, x));
  return c2_rare(f, w, x);
}


typedef struct Md1 {
  struct Value;
  BB2B  c1; // f(md1d{this,f},  x); consumes x
  BBB2B c2; // f(md1d{this,f},w,x); consumes w,x
} Md1;
typedef struct Md2 {
  struct Value;
  BB2B  c1; // f(md2d{this,f,g},  x); consumes x
  BBB2B c2; // f(md2d{this,f,g},w,x); consumes w,x
} Md2;
B m_md1D(B m, B f     );
B m_md2D(B m, B f, B g);
B m_md2H(B m,      B g);
B m_fork(B f, B g, B h);
B m_atop(     B g, B h);

void arr_print(B x) { // should accept refc=0 arguments for debugging purposes
  usz r = rnk(x);
  BS2B xgetU = TI(x).getU;
  usz ia = a(x)->ia;
  if (r!=1) {
    if (r==0) {
      printf("<");
      print(xgetU(x,0));
      return;
    }
    usz* sh = a(x)->sh;
    for (i32 i = 0; i < r; i++) {
      if(i==0)printf("%d",sh[i]);
      else printf("‿%d",sh[i]);
    }
    printf("⥊");
  } else if (ia>0) {
    for (usz i = 0; i < ia; i++) {
      B c = xgetU(x,i);
      if (!isC32(c) || (u32)c.u=='\n') goto reg;
    }
    printf("\"");
    for (usz i = 0; i < ia; i++) printUTF8((u32)xgetU(x,i).u); // c32, no need to decrement
    printf("\"");
    return;
  }
  reg:;
  printf("⟨");
  for (usz i = 0; i < ia; i++) {
    if (i!=0) printf(", ");
    print(xgetU(x,i));
  }
  printf("⟩");
}


#include <time.h>
u64 nsTime() {
  struct timespec t;
  timespec_get(&t, TIME_UTC);
  return t.tv_sec*1000000000ull + t.tv_nsec;
}

#ifdef DEBUG
  Value* validateP(Value* x) {
    if (x->refc<=0 || (x->refc>>28) == 'a' || x->type==t_empty) {
      printf("bad refcount for type %d: %d\nattempting to print: ", x->type, x->refc); fflush(stdout);
      print(tag(x,OBJ_TAG)); puts(""); fflush(stdout);
      err("");
    }
    if (ti[x->type].isArr) {
      Arr* a = (Arr*)x;
      if (rnk(tag(x,ARR_TAG))<=1) assert(a->sh == &a->ia);
      else validate(tag(shObj(tag(x,ARR_TAG)),OBJ_TAG));
    }
    return x;
  }
  B validate(B x) {
    if (!isVal(x)) return x;
    validateP(v(x));
    if(isArr(x)!=TI(x).isArr && v(x)->type!=t_freed) {
      printf("wat %d %p\n", v(x)->type, (void*)x.u);
      print(x);
      err("\nk");
    }
    return x;
  }
#endif

#ifdef ALLOC_STAT
u64* ctr_a = 0;
u64* ctr_f = 0;
u64 actrc = 21000;
u64 talloc = 0;
#ifdef ALLOC_SIZES
u32** actrs;
#endif
#endif

static inline void onAlloc(usz sz, u8 type) {
  #ifdef ALLOC_STAT
    if (!ctr_a) {
      #ifdef ALLOC_SIZES
        actrs = malloc(sizeof(u32*)*actrc);
        for (i32 i = 0; i < actrc; i++) actrs[i] = calloc(t_COUNT, sizeof(u32));
      #endif
      ctr_a = calloc(t_COUNT, sizeof(u64));
      ctr_f = calloc(t_COUNT, sizeof(u64));
    }
    assert(type<t_COUNT);
    #ifdef ALLOC_SIZES
      actrs[(sz+3)/4>=actrc? actrc-1 : (sz+3)/4][type]++;
    #endif
    ctr_a[type]++;
    talloc+= sz;
  #endif
}
static inline void onFree(Value* x) {
  #ifdef ALLOC_STAT
    ctr_f[x->type]++;
  #endif
  #ifdef DEBUG
    if (x->type==t_empty) err("double-free");
    // u32 undef;
    // x->refc = undef;
    x->refc = -1431655000;
  #endif
  // x->refc = 0x61616161;
}