#pragma once
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdarg.h>
#include <setjmp.h>

#define rtLen 63
#ifdef CATCH_ERRORS
  #define PROPER_FILLS (EACH_FILLS&SFNS_FILLS)
#else
  #undef EACH_FILLS
  #define EACH_FILLS false
  #define PROPER_FILLS false
#endif

#define i8    int8_t
#define u8   uint8_t
#define i16  int16_t
#define u16 uint16_t
#define i32  int32_t
#define u32 uint32_t
#define i64  int64_t
#define u64 uint64_t
#define f64 double
#define I32_MAX ((i32)((1LL<<31)-1))
#define I32_MIN ((i32)(-(1LL<<31)))
#define CHR_MAX 1114111
#define U16_MAX ((u16)~(u16)0)
#define U32_MAX ((u32)~(u32)0)
#define UD __builtin_unreachable();
#define NOINLINE __attribute__ ((noinline))
#define NORETURN __attribute__ ((noreturn))
#define AUTO __auto_type
#define RARE(X) __builtin_expect(X,0)

typedef u32 usz;
typedef u8 ur;
#define USZ_MAX ((u32)((1LL<<32)-1))
#define  UR_MAX 255

#define CTR_FOR(F)
#define CTR_DEF(N) u64 N;
#define CTR_PRINT(N) printf(#N ": %lu\n", N);
CTR_FOR(CTR_DEF)

#define fsizeof(T,F,E,n) (offsetof(T, F) + sizeof(E)*(n)) // type, flexible array member name, flexible array member type, item amount
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

void cbqn_init();

enum Type {
  /* 0*/ t_empty, // empty bucket placeholder
  /* 1*/ t_funBI, t_fun_block,
  /* 3*/ t_md1BI, t_md1_block,
  /* 5*/ t_md2BI, t_md2_block,
  /* 7*/ t_shape, // doesn't get visited, shouldn't be unallocated by gcWMd1
  
  /* 8*/ t_fork, t_atop,
  /*10*/ t_md1D, t_md2D, t_md2H,
  
  /*13*/ t_harr  , t_i8arr  , t_i32arr  , t_fillarr  , t_c32arr  , t_f64arr  ,
  /*19*/ t_hslice, t_i8slice, t_i32slice, t_fillslice, t_c32slice, t_f64slice,
  
  /*25*/ t_comp, t_block, t_body, t_scope,
  /*29*/ t_ns, t_nsDesc, t_fldAlias,
  /*31*/ t_freed, t_harrPartial,
  #ifdef RT_PERF
  /*33*/ t_funPerf, t_md1Perf, t_md2Perf,
  #endif
  t_COUNT
};

enum ElType { // a⌈b shall return the type that can store both, if possible
  el_i32=0,
  el_f64=1,
  el_c32=2,
  el_B  =3,
  el_MAX=4 // also used for incomplete in mut.c
};

char* format_type(u8 u) {
  switch(u) { default: return"(unknown type)";
    case t_empty:return"empty"; case t_shape:return"shape";
    case t_funBI:return"builtin fun"; case t_fun_block:return"fun_block";
    case t_md1BI:return"builtin md1"; case t_md1_block:return"md1_block";
    case t_md2BI:return"builtin md2"; case t_md2_block:return"md2_block";
    case t_fork:return"fork"; case t_atop:return"atop";
    case t_md1D:return"md1D"; case t_md2D:return"md2D"; case t_md2H:return"md2H";
    case t_harr  :return"harr"  ; case t_i32arr  :return"i32arr"  ; case t_fillarr  :return"fillarr"  ; case t_c32arr  :return"c32arr"  ; case t_f64arr  :return"f64arr"  ;
    case t_hslice:return"hslice"; case t_i32slice:return"i32slice"; case t_fillslice:return"fillslice"; case t_c32slice:return"c32slice"; case t_f64slice:return"f64slice";
    case t_comp:return"comp"; case t_block:return"block"; case t_body:return"body"; case t_scope:return"scope";
    case t_freed:return"(freed by GC)";
  }
}

#define FOR_PF(F) F(none, "(unknown fn)") \
    /*arith.c*/ F(add,"+") F(sub,"-") F(mul,"×") F(div,"÷") F(pow,"⋆") F(floor,"⌊") F(ceil,"⌈") F(stile,"|") F(eq,"=") \
    /*arith.c*/ F(ne,"≠") F(le,"≤") F(ge,"≥") F(lt,"<") F(gt,">") F(and,"∧") F(or,"∨") F(not,"¬") F(log,"⋆⁼") \
    /*fns.c*/   F(ud,"↕") F(fne,"≢") F(feq,"≡") F(ltack,"⊣") F(rtack,"⊢") F(fmtF,"•FmtF") F(fmtN,"•FmtN") \
    /*sfns.c*/  F(shape,"⥊") F(pick,"⊑") F(pair,"{𝕨‿𝕩}") F(select,"⊏") F(slash,"/") F(join,"∾") F(couple,"≍") F(shiftb,"»") F(shifta,"«") F(take,"↑") F(drop,"↓") F(group,"⊔") \
    /*derv.c*/  F(fork,"(fork)") F(atop,"(atop)") F(md1d,"(derived 1-modifier)") F(md2d,"(derived 2-modifier)") \
    /*sort.c*/  F(gradeUp,"⍋") \
    /*sysfn.c*/ F(type,"•Type") F(decp,"•Decompose") F(primInd,"•PrimInd") F(glyph,"•Glyph") F(fill,"•FillFn") \
    /*sysfn.c*/ F(grLen,"•GroupLen") F(grOrd,"•groupOrd") F(asrt,"!") F(sys,"•getsys") F(bqn,"•BQN") F(cmp,"•Cmp") F(internal,"•Internal") F(show,"•Show") F(out,"•Out") \
    /*sysfn.c*/ F(fchars,"•FChars") F(fbytes,"•FBytes")

enum PrimFns {
  #define F(N,X) pf_##N,
  FOR_PF(F)
  #undef F
};
char* format_pf(u8 u) {
  switch(u) { default: return "(unknown fn)";
    #define F(N,X) case pf_##N: return X;
    FOR_PF(F)
    #undef F
  }
}
enum PrimMd1 {
  pm1_none,
  pm1_tbl, pm1_each, pm1_fold, pm1_scan, pm1_const, pm1_swap, pm1_timed, // md1.c
};
char* format_pm1(u8 u) {
  switch(u) {
    default: case pf_none: return"(unknown 1-modifier)";
    case pm1_tbl:return"⌜"; case pm1_each:return"¨"; case pm1_fold:return"´"; case pm1_scan:return"`"; case pm1_const:return"˙"; case pm1_swap:return"˜"; case pm1_timed:return"•_timed";
  }
}
enum PrimMd2 {
  pm2_none,
  pm2_val, pm2_atop, pm2_over, pm2_before, pm2_after, pm2_cond, pm2_repeat, pm2_fillBy, pm2_catch, // md2.c
};
char* format_pm2(u8 u) {
  switch(u) {
    default: case pf_none: return"(unknown 1-modifier)";
    case pm2_val:return"⊘"; case pm2_repeat:return"⍟"; case pm2_fillBy:return"•_fillBy_"; case pm2_catch:return"⎊";
    case pm2_atop:return"∘"; case pm2_over:return"○"; case pm2_before:return"⊸"; case pm2_after:return"⟜"; case pm2_cond:return"◶";
  }
}



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

#ifdef DEBUG
  #include<assert.h>
  B VALIDATE(B x);
  Value* VALIDATEP(Value* x);
#else
  #define assert(x) {if (!(x)) __builtin_unreachable();}
  #define VALIDATE(x) (x)
  #define VALIDATEP(x) (x)
#endif

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

void gsAdd(B x); // may throw
B gsPop();

// some primitive actions
void dec(B x);
B    inc(B x);
void ptr_dec(void* x);
void ptr_inc(void* x);
void printUTF8(u32 c);
void printRaw(B x);       // doesn't consume
void print(B x);          // doesn't consume
bool equal(B w, B x);     // doesn't consume
i32  compare(B w, B x);   // doesn't consume; -1 if w<x, 1 if w>x, 0 if w≡x; 0==compare(NaN,NaN)
void arr_print(B x);      // doesn't consume
u8   fillElType(B x);     // doesn't consume
bool eqShape(B w, B x);   // doesn't consume
usz  arr_csz(B x);        // doesn't consume
bool atomEqual(B w, B x); // doesn't consume
B    toCells(B x);        // consumes
B    toKCells(B x, ur k); // consumes
B    withFill(B x, B f);  // consumes both
bool eqShPrefix(usz* w, usz* x, ur len);

B m_v1(B a               ); // consumes all
B m_v2(B a, B b          ); // consumes all
B m_v3(B a, B b, B c     ); // consumes all
B m_v4(B a, B b, B c, B d); // consumes all
B m_unit (B x); // consumes
B m_hunit(B x); // consumes
B m_str32(u32* s); // meant to be used as m_str32(U"{𝕨‿𝕩}"), so doesn't free for you

B bqn_exec(B str); // consumes

NOINLINE NORETURN void thr(B b);
NOINLINE NORETURN void thrM(char* s);
NOINLINE NORETURN void thrOOM();
jmp_buf* prepareCatch();
#ifdef CATCH_ERRORS
#define CATCH setjmp(*prepareCatch()) // use as `if (CATCH) { /*handle error; dec(catchMessage);*/ } /*potentially erroring thing*/ popCatch();`
#else                                 // note: popCatch() must always be called if no error was caught, so no returns before it!
#define CATCH false
#endif
void popCatch();
B catchMessage;
u64 envPrevHeight; // envStart+prevEnvHeight will give the original envCurr



#define c(T,X) ((T*)((X).u&0xFFFFFFFFFFFFull))
#define v(X) c(Value, X)
#define a(X) c(Arr  , X)
#define  prnk(X  ) (X->extra)
#define sprnk(X,R) (X->extra=(R))
#define  rnk(X  )  prnk(v(X))
#define srnk(X,R) sprnk(v(X),R)
#define VT(X,T) assert(isVal(X) && v(X)->type==(T))

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
bool isTag(B x) { return (x.u>>48) == TAG_TAG; }
bool isMd1(B x) { return (x.u>>48) == MD1_TAG; }
bool isMd2(B x) { return (x.u>>48) == MD2_TAG; }
bool isMd (B x) { return (x.u>>49) ==(MD2_TAG>>1); }
bool isNsp(B x) { return (x.u>>48) == NSP_TAG; }
bool isObj(B x) { return (x.u>>48) == OBJ_TAG; }
// bool isVal(B x) { return ((x.u>>51) == VAL_TAG)  &  ((x.u<<13) != 0); }
// bool isF64(B x) { return ((x.u>>51&0xFFF) != 0xFFE)  |  ((x.u<<1)==(b(1.0/0.0).u<<1)); }
bool isVal(B x) { return (x.u - (((u64)VAL_TAG<<51) + 1)) < ((1ull<<51) - 1); } // ((x.u>>51) == VAL_TAG)  &  ((x.u<<13) != 0);
bool isF64(B x) { return (x.u<<1) - ((0xFFEull<<52) + 2) >= (1ull<<52) - 2; }
bool isNum(B x) { return isF64(x)|isI32(x); }

bool isAtm(B x) { return !isArr(x); }
bool isCallable(B x) { return isMd(x) | isFun(x); }
bool noFill(B x);

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
ShArr* m_shArr(ur r) {
  assert(r>1);
  return ((ShArr*)mm_allocN(fsizeof(ShArr, a, usz, r), t_shape));
}
usz* arr_shAllocR(B x, ur r) { // allocates shape, sets rank
  srnk(x,r);
  if (r>1) return a(x)->sh = m_shArr(r)->a;
  a(x)->sh = &a(x)->ia;
  return 0;
}
usz* arr_shAllocI(B x, usz ia, ur r) { // allocates shape, sets ia,rank
  a(x)->ia = ia;
  return arr_shAllocR(x, r);
}
void arr_shSetI(B x, usz ia, ur r, ShArr* sh) {
  srnk(x,r);
  a(x)->ia = ia;
  if (r>1) { a(x)->sh = sh->a; ptr_inc(sh); }
  else     { a(x)->sh = &a(x)->ia; }
}
void arr_shCopy(B n, B o) { // copy shape,rank,ia from o to n
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

// make objects
B m_arr(usz min, u8 type) { return mm_alloc(min, type, ftag(ARR_TAG)); }
B m_f64(f64 n) { assert(isF64(b(n))); return b(n); } // assert just to make sure we're actually creating a float
B m_c32(u32 n) { return tag(n, C32_TAG); } // TODO check validity?
#ifdef ATOM_I32
B m_i32(i32 n) { return tag(n, I32_TAG); }
#else
B m_i32(i32 n) { return m_f64(n); }
#endif
B m_error() { return tag(4, TAG_TAG); }
B m_usz(usz n) { return n<I32_MAX? m_i32((i32)n) : m_f64(n); }

i32 o2i   (B x) { if (x.f!=(f64)(i32)x.f) thrM("Expected integer"); return (i32)x.f; } // i have no clue whether these consume or not, but it doesn't matter
usz o2s   (B x) { if (x.f!=(f64)(usz)x.f) thrM("Expected integer"); return (usz)x.f; }
i64 o2i64 (B x) { if (x.f!=(f64)(i64)x.f) thrM("Expected integer"); return (i64)x.f; }
u64 o2u64 (B x) { if (x.f!=(f64)(u64)x.f) thrM("Expected integer"); return (u64)x.f; }
f64 o2f   (B x) { if (!isNum(x)) thrM("Expected integer"); return x.f; }
u32 o2c   (B x) { if (!isC32(x)) thrM("Expected character"); return (u32)x.u; }
i32 o2iu  (B x) { return isI32(x)? (i32)(u32)x.u : (i32)x.f; }
i32 o2cu  (B x) { return (u32)x.u; }
usz o2su  (B x) { return (usz)x.f; }
f64 o2fu  (B x) { return      x.f; }
i64 o2i64u(B x) { return (i64)x.f; }
bool o2b  (B x) { usz t=o2s(x); if(t!=0&t!=1)thrM("Expected boolean"); return t; }
bool q_i32(B x) { return isI32(x) | (isF64(x) && x.f==(f64)(i32)x.f); }
bool q_f64(B x) { return isF64(x) || isI32(x); }


typedef struct Slice {
  struct Arr;
  B p;
} Slice;
void slice_free(B x) { dec(c(Slice,x)->p); decSh(x); }
void slice_visit(B x) { mm_visit(c(Slice,x)->p); }
void slice_print(B x) { arr_print(x); }

B* harr_ptr(B x);


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
  BS2B slice; // consumes; create slice from given starting position; add ia, rank, shape yourself; may not actually be a Slice object
  B2B identity; // return identity element of this function; doesn't consume
  
  B2b canStore; // doesn't consume
  u8 elType;
  
  B2v print;  // doesn't consume
  B2v visit;  // call mm_visit for all referents
  B2B decompose; // consumes; must return a HArr
  bool isArr;
  bool arrD1; // is always an array with depth 1
} TypeInfo;
TypeInfo ti[t_COUNT];
#define TI(x) (ti[v(x)->type])


B bi_N, bi_noVar, bi_badHdr, bi_optOut, bi_noFill, bi_emptyHVec, bi_emptyIVec;

void do_nothing(B x) { }
bool isNothing(B b) { return b.u==bi_N.u; }

// refcount
bool reusable(B x) { return v(x)->refc==1; }
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
void ptr_dec(void* x) { if(!--VALIDATEP((Value*)x)->refc) value_free(tag(x, OBJ_TAG), x); }
void ptr_decR(void* x) { if(!--VALIDATEP((Value*)x)->refc) value_freeR1(x); }
void decR(B x) {
  if (!isVal(VALIDATE(x))) return;
  Value* vx = v(x);
  if(!--vx->refc) value_freeR2(vx, x);
}
B inc(B x) {
  if (isVal(VALIDATE(x))) v(x)->refc++;
  return x;
}
void ptr_inc(void* x) { VALIDATEP((Value*)x)->refc++; }



typedef struct Fun {
  struct Value;
  BB2B c1;
  BBB2B c2;
} Fun;


B c1_rare(B f, B x) { dec(x);
  if (isMd(f)) thrM("Calling a modifier");
  return inc(VALIDATE(f));
}
B c2_rare(B f, B w, B x) { dec(w); dec(x);
  if (isMd(f)) thrM("Calling a modifier");
  return inc(VALIDATE(f));
}
BB2B c1fn(B f);
BBB2B c2fn(B f);
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



#include <time.h>
static inline u64 nsTime() {
  struct timespec t;
  timespec_get(&t, TIME_UTC);
  // clock_gettime(CLOCK_REALTIME, &t);
  return t.tv_sec*1000000000ull + t.tv_nsec;
}

u64 allocB; // currently allocated number of bytes
