#pragma once

#ifdef DEBUG
  // #define DEBUG_VM
#endif
#ifndef CATCH_ERRORS
  #define CATCH_ERRORS 1 // whether to allow catching errors
#endif                   // currently means refcounts won't be accurate and can't be tested for
#ifndef ENABLE_GC
  #define ENABLE_GC    1 // whether to ever garbage-collect
#endif
#ifndef TYPED_ARITH
  #define TYPED_ARITH  1 // whether to use typed arith
#endif
#ifndef VM_POS
  #define VM_POS       1 // whether to store detailed execution position information for stacktraces
#endif
#ifndef CHECK_VALID
  #define CHECK_VALID  1 // whether to check for valid arguments in places where that would be detrimental to performance
#endif                   // e.g. left argument sortedness of ⍋/⍒, incompatible changes in ⌾, etc
#ifndef EACH_FILLS
  #define EACH_FILLS   0 // whether to try to squeeze out fills for ¨ and ⌜
#endif
#ifndef SFNS_FILLS
  #define SFNS_FILLS   1 // whether to generate fills for structural functions (∾, ≍, etc)
#endif
#ifndef FAKE_RUNTIME
  #define FAKE_RUNTIME 0 // whether to disable the self-hosted runtime
#endif
#ifndef MM
  #define MM           1 // memory manager; 0 - malloc (no GC); 1 - buddy; 2 - 2buddy
#endif
#ifndef HEAP_MAX
  #define HEAP_MAX ~0ULL // default heap max size
#endif
#ifndef FORMATTER
  #define FORMATTER 1  // use self-hosted formatter for output
#endif
#ifndef RANDSEED
  #define RANDSEED 0   // random seed used to make •rand (0 for using time)
#endif
#ifndef FFI
  #define FFI 2
#endif

// #define HEAP_VERIFY  // enable usage of heapVerify()
// #define ALLOC_STAT   // store basic allocation statistics
// #define ALLOC_SIZES  // store per-type allocation size statistics
// #define USE_VALGRIND // whether to mark freed memory for valgrind
// #define DONT_FREE    // don't actually ever free objects, such that they can be printed after being freed for debugging
// #define OBJ_COUNTER  // store a unique allocation number with each object for easier analysis
// #define ALL_R0       // use all of r0.bqn for runtime_0
// #define ALL_R1       // use all of r1.bqn for runtime

// #define JIT_START 2  // number of calls for when to start JITting (x86_64-only); default is 2, defined in vm.h
// -1: never JIT
//  0: JIT everything
// >0: JIT after n non-JIT invocations; max ¯1+2⋆16

// #define LOG_GC    // log GC stats
// #define RT_PERF   // time runtime primitives
// #define RT_VERIFY // compare native and runtime versions of primitives
// #define NO_RT     // whether to completely disable self-hosted runtime loading
// #define PRECOMP   // execute just precompiled code at src/gen/interp


#ifdef __OpenBSD__
  #define __wchar_t __wchar_t2 // horrible hack for BSD
#endif
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#if CATCH_ERRORS
#include <setjmp.h>
#endif
#ifdef HEAP_VERIFY
  #undef CATCH_ERRORS
  #define CATCH_ERRORS 0
#endif

#define rtLen 64

#if CATCH_ERRORS
  #define PROPER_FILLS (EACH_FILLS&SFNS_FILLS)
#else
  #undef EACH_FILLS
  #define EACH_FILLS 0
  #define PROPER_FILLS 0
#endif

#if defined(ALL_R0) || defined (ALL_R1)
  #define WRAP_NNBI 1
#endif

#if defined(RT_PERF) || defined(RT_VERIFY)
  #define RT_WRAP 1
  #if defined(RT_PERF) && defined(RT_VERIFY)
    #error "can't have both RT_PERF and RT_VERIFY"
  #endif
#endif
#if defined(OBJ_TRACK)
  #define OBJ_COUNTER 1
#endif

typedef   int8_t i8;
typedef  uint8_t u8;
typedef  int16_t i16;
typedef uint16_t u16;
typedef  int32_t i32;
typedef uint32_t u32;
typedef  int64_t i64;
typedef uint64_t u64;
typedef double   f64;
#define I8_MIN -128
#define I8_MAX  127
#define I16_MIN -32768
#define I16_MAX  32767
#define I32_MIN -2147483648
#define I32_MAX  2147483647
#define I64_MIN ((i64)(1ULL<<63))
#define CHR_MAX 1114111
#define U8_MAX  ((u8 )~(u8 )0)
#define U16_MAX ((u16)~(u16)0)
#define U32_MAX ((u32)~(u32)0)
#define NOINLINE     __attribute__((noinline))
#define FORCE_INLINE __attribute__((always_inline)) static inline
#define NORETURN     __attribute__((noreturn))
#define AUTO __auto_type
#define CLZ(X) __builtin_clzll(X)
#define CTZ(X) __builtin_ctzll(X)
#define POPC(X) __builtin_popcountll(X)
#define LIKELY(X) __builtin_expect((X)!=0,1)
#define RARE(X) __builtin_expect((X)!=0,0)
#define fsizeof(T,F,E,N) (offsetof(T, F) + sizeof(E)*(N)) // type, flexible array member name, flexible array member type, item amount
#define RFLD(X,T,F) ((T*)((char*)(X) - offsetof(T,F))) // value, result type, field name; reverse-read field: `T* x = …; E v = x->f; x == RFLD(v, T, f)`
#define N64x "%"SCNx64
#define N64d "%"SCNd64
#define N64u "%"SCNu64
#if __clang__
  #define NOUNROLL _Pragma("clang loop unroll(disable)") _Pragma("clang loop vectorize(disable)")
#elif __GNUC__
  #define NOUNROLL _Pragma("GCC unroll 1")
#else
  #define NOUNROLL
#endif

#define JOIN0(A,B) A##B
#define JOIN(A,B) JOIN0(A,B)

#if USZ_64
  typedef u64 usz;
  #define USZ_MAX ((u64)(1ULL<<48))
  #define CHECK_IA(IA,W) if(IA>USZ_MAX) thrOOM()
#else
  typedef u32 usz;
  #define USZ_MAX ((u32)((1LL<<32)-1))
  #define CHECK_IA(IA,W) if (IA > ((1LL<<31)/W - 1000)) thrOOM()
#endif
#if UNSAFE_SIZES
  #undef CHECK_IA
  #define CHECK_IA(IA,W)
#endif
typedef u8 ur;
#define UR_MAX 255

#define CTR_FOR(F)
#define CTR_PRINT(N) if(N) printf(#N ": "N64u"\n", N);
#define F(N) extern u64 N;
CTR_FOR(F)
#undef F

                                               // .FF0 .111111111110000000000000000000000000000000000000000000000000000 infinity
                                               // .FF8 .111111111111000000000000000000000000000000000000000000000000000 qNaN
                                               // .FF. .111111111110nnn................................................ sNaN aka tagged aka not f64, if nnn≠0
                                               // 7FF. 0111111111110................................................... direct value with no need of refcounting
static const u16 C32_TAG = 0b0111111111110001; // 7FF1 0111111111110001................00000000000ccccccccccccccccccccc char
static const u16 TAG_TAG = 0b0111111111110010; // 7FF2 0111111111110010................nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn special value (0=nothing, 1=undefined var, 2=bad header; 3=optimized out; 4=error?; 5=no fill)
static const u16 VAR_TAG = 0b0111111111110011; // 7FF3 0111111111110011ddddddddddddddddnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn variable reference
static const u16 EXT_TAG = 0b0111111111110100; // 7FF4 0111111111110100ddddddddddddddddnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn extended variable reference
static const u16 RAW_TAG = 0b0111111111110101; // 7FF5 0111111111110101nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn raw 48 bits of data
static const u16 MD1_TAG = 0b1111111111110010; // FFF2 1111111111110010ppppppppppppppppppppppppppppppppppppppppppppp000 1-modifier
static const u16 MD2_TAG = 0b1111111111110011; // FFF3 1111111111110011ppppppppppppppppppppppppppppppppppppppppppppp000 2-modifier
static const u16 FUN_TAG = 0b1111111111110100; // FFF4 1111111111110100ppppppppppppppppppppppppppppppppppppppppppppp000 function
static const u16 NSP_TAG = 0b1111111111110101; // FFF5 1111111111110101ppppppppppppppppppppppppppppppppppppppppppppp000 namespace
static const u16 OBJ_TAG = 0b1111111111110110; // FFF6 1111111111110110ppppppppppppppppppppppppppppppppppppppppppppp000 custom/internal object
static const u16 ARR_TAG = 0b1111111111110111; // FFF7 1111111111110111ppppppppppppppppppppppppppppppppppppppppppppp000 array (everything else here is an atom)
static const u16 VAL_TAG = 0b1111111111110   ; // FFF. 1111111111110................................................... pointer to Value, needs refcounting
#define ftag(X) ((u64)(X) << 48)
#define ptr2u64(X) ((u64)(uintptr_t)(X))
#define tagu64(V, T) b((u64)(V) | ftag(T))
#define tag(V, T) b(ptr2u64(V) | ftag(T))
#define taga(V) tag(V,ARR_TAG)

void cbqn_init(void);

// #if __STDC_IEC_559__ == 0 // this has some issues on M1, so disabling for now
//   #error "IEEE 754 floating point number support is required for CBQN"
// #endif
typedef union B {
  u64 u;
  f64 f;
} B;
#define b(x) ((B)(x))

#if defined(RT_WRAP) || defined(WRAP_NNBI)
  #define IF_WRAP(X) X
#else
  #define IF_WRAP(X)
#endif

#define FOR_TYPE(F) \
  /* 0*/ F(empty) \
  /* 1*/ F(funBI) F(funBl) \
  /* 3*/ F(md1BI) F(md1Bl) \
  /* 5*/ F(md2BI) F(md2Bl) \
  /* 7*/ F(shape) /* doesn't get F(visited) shouldn't be unallocated by gc */ \
  \
  /* 8*/ F(fork) F(atop) \
  /*10*/ F(md1D) F(md2D) F(md2H) \
  \
  /*13*/ F(hslice) F(fillslice) F(i8slice) F(i16slice) F(i32slice) F(c8slice) F(c16slice) F(c32slice) F(f64slice) \
  /*22*/ F(harr  ) F(fillarr  ) F(i8arr  ) F(i16arr  ) F(i32arr  ) F(c8arr  ) F(c16arr  ) F(c32arr  ) F(f64arr  ) \
  /*31*/ F(bitarr) \
  \
  /*32*/ F(comp) F(block) F(body) F(scope) F(scopeExt) F(blBlocks) F(arbObj) F(ffiType) \
  /*40*/ F(ns) F(nsDesc) F(fldAlias) F(arrMerge) F(vfyObj) F(hashmap) F(temp) F(nfn) F(nfnDesc) \
  /*49*/ F(freed) F(harrPartial) F(customObj) F(mmapH) \
  \
  /*52*/ IF_WRAP(F(funWrap) F(md1Wrap) F(md2Wrap))

enum Type {
  #define F(X) t_##X,
  FOR_TYPE(F)
  #undef F
  t_COUNT
};
#define IS_SLICE(T) ((T)>=t_hslice & (T)<=t_f64slice)
#define IS_ARR(T) ((T)>=t_harr & (T)<=t_bitarr)
#define TO_SLICE(T) ((T) + t_hslice - t_harr) // Assumes T!=t_bitarr

enum ElType { // a⌈b shall return the type that can store both, if possible
  el_bit=0,
  el_i8 =1,
  el_i16=2,
  el_i32=3,
  el_f64=4,
  
  el_c8 =5,
  el_c16=6,
  el_c32=7,
  
  el_B  =8,
  el_MAX=9 // also used for incomplete in mut.c
};



typedef struct Value {
  i32 refc;  // plain old reference count
  u8 mmInfo; // bucket size, mark&sweep bits when that's needed
  u8 flags;  // self-hosted primitive index (plus 1) for callable, fl_* flags for arrays
  u8 type;   // used by TI, among generally knowing what type of object this is
  ur extra;  // whatever object-specific stuff. Rank for arrays, internal id for functions
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
  NOINLINE NORETURN void assert_fail(char* expr, char* file, int line, const char fn[]);
  #define assert(X) do { if (!(X)) assert_fail(#X, __FILE__, __LINE__, __PRETTY_FUNCTION__); } while (0)
  B VALIDATE(B x);
  Value* VALIDATEP(Value* x);
  #define UD assert(false);
#else
  #define assert(X) {if (!(X)) __builtin_unreachable();}
  #define VALIDATE(X) (X)
  #define VALIDATEP(X) (X)
  #define UD __builtin_unreachable();
#endif
#if WARN_SLOW==1
  void warn_slow1(char* s, B x);
  void warn_slow2(char* s, B w, B x);
  void warn_slow3(char* s, B w, B x, B y);
  #define SLOW1(S, X) warn_slow1(S, X)
  #define SLOW2(S, W, X) warn_slow2(S, W, X)
  #define SLOW3(S, W, X, Y) warn_slow3(S, W, X, Y)
  #define SLOWIF(C) if(C)
#else
  #define SLOW1(S, X)
  #define SLOW2(S, W, X)
  #define SLOW3(S, W, X, Y)
  #define SLOWIF(C)
#endif

// memory manager
typedef void (*V2v)(Value*);
typedef void (*vfn)(void);

void gc_add(B x); // add permanent root object
void gc_addFn(vfn f); // add function that calls mm_visit/mm_visitP for dynamic roots
void gc_maybeGC(void); // gc if that seems necessary
void gc_forceGC(void); // force a gc; who knows what happens if gc is disabled (probably should error)
void gc_visitRoots(void);

// some primitive actions
static const B bi_N      = b((u64)0x7FF2000000000000ull); // tag(0,TAG_TAG); // make gcc happy
static const B bi_noVar  = b((u64)0x7FF2000000000001ull); // tag(1,TAG_TAG);
static const B bi_okHdr  = b((u64)0x7FF2000000000002ull); // tag(2,TAG_TAG);
static const B bi_optOut = b((u64)0x7FF2000000000003ull); // tag(3,TAG_TAG);
static const B bi_noFill = b((u64)0x7FF2000000000005ull); // tag(5,TAG_TAG);
extern B bi_emptyHVec, bi_emptyIVec, bi_emptyCVec, bi_emptySVec;
#define emptyHVec() incG(bi_emptyHVec)
#define emptyIVec() incG(bi_emptyIVec)
#define emptyCVec() incG(bi_emptyCVec)
#define emptySVec() incG(bi_emptySVec)
static void* mm_alloc(usz sz, u8 type);
static void  mm_free(Value* x);
static u64   mm_size(Value* x);
static void  mm_visit(B x);
static void  mm_visitP(void* x);
static void dec(B x);
static B    inc(B x);
static void ptr_dec(void* x);
void fprint    (FILE* f, B x); // doesn't consume
void fprintRaw (FILE* f, B x); // doesn't consume
void farr_print(FILE* f, B x); // doesn't consume
void print(B x);          // doesn't consume
void printRaw(B x);       // doesn't consume
void arr_print(B x);      // doesn't consume
bool equal(B w, B x);     // doesn't consume
bool eequal(B w, B x);    // doesn't consume
B    toCells(B x);        // consumes
B    toKCells(B x, ur k); // consumes
B    withFill(B x, B f);  // consumes both

B bqn_exec(B str, B path, B args); // consumes all
B bqn_execFile(B path, B args); // consumes
B bqn_explain(B str, B path); // consumes str
B bqn_fmt(B x); // consumes
B bqn_repr(B x); // consumes

NOINLINE NORETURN void thr(B b);
NOINLINE NORETURN void rethrow();
NOINLINE NORETURN void thrM(char* s);
NOINLINE NORETURN void thrF(char* s, ...);
NOINLINE NORETURN void thrOOM(void);
#if CATCH_ERRORS
jmp_buf* prepareCatch(void);
#define CATCH setjmp(*prepareCatch()) // use as `if (CATCH) { /*handle error*/ freeThrown(); return; } /*potentially erroring thing*/ popCatch(); /*no errors yay*/`
#else                                 // note: popCatch() must always be called if no error was caught, so no returns before it!
#define CATCH false
#endif
void popCatch(void);
extern B thrownMsg;
void freeThrown(void);



#define c(T,X) ((T*)((X).u&0xFFFFFFFFFFFFull))
#define v(X) c(Value, X)
#define a(X) c(Arr  , X)

#define SPRNK(X,R) (X->extra=(R))
#define SRNK(X,R) SPRNK(v(X),R)

#define PRNK(X) ((X)->extra)
#define PIA(X)  ((X)->ia)
#define PSH(X)  ((X)->sh)
#define PTY(X)  ((X)->type)
#define IA(X)  PIA(a(X))
#define SH(X)  PSH(a(X))
#define TY(X)  PTY(v(X))
#define RNK(X) PRNK(v(X))

#define VTY(X,T) assert(isVal(X) && TY(X)==(T))

void print_vmStack(void);
#ifdef DEBUG
  B validate(B x);
  Value* validateP(Value* x);
#endif
NORETURN NOINLINE void err(char* s);

// tag checks
FORCE_INLINE bool isFun(B x) { return (x.u>>48) == FUN_TAG; }
FORCE_INLINE bool isArr(B x) { return (x.u>>48) == ARR_TAG; }
FORCE_INLINE bool isC32(B x) { return (x.u>>48) == C32_TAG; }
FORCE_INLINE bool isVar(B x) { return (x.u>>48) == VAR_TAG; }
FORCE_INLINE bool isExt(B x) { return (x.u>>48) == EXT_TAG; }
FORCE_INLINE bool isTag(B x) { return (x.u>>48) == TAG_TAG; }
FORCE_INLINE bool isMd1(B x) { return (x.u>>48) == MD1_TAG; }
FORCE_INLINE bool isMd2(B x) { return (x.u>>48) == MD2_TAG; }
FORCE_INLINE bool isMd (B x) { return (x.u>>49) ==(MD2_TAG>>1); }
FORCE_INLINE bool isNsp(B x) { return (x.u>>48) == NSP_TAG; }
FORCE_INLINE bool isObj(B x) { return (x.u>>48) == OBJ_TAG; }
// FORCE_INLINE bool isVal(B x) { return ((x.u>>51) == VAL_TAG)  &  ((x.u<<13) != 0); }
// FORCE_INLINE bool isF64(B x) { return ((x.u>>51&0xFFF) != 0xFFE)  |  ((x.u<<1)==(b(1.0/0.0).u<<1)); }
FORCE_INLINE bool isVal(B x) { return (x.u - (((u64)VAL_TAG<<51) + 1)) < ((1ull<<51) - 1); } // ((x.u>>51) == VAL_TAG)  &  ((x.u<<13) != 0);
FORCE_INLINE bool isF64(B x) { return (x.u<<1) - ((0xFFEull<<52) + 2) >= (1ull<<52) - 2; }
FORCE_INLINE bool isNum(B x) { return isF64(x); }

FORCE_INLINE bool isAtm(B x) { return !isArr(x); }
FORCE_INLINE bool isCallable(B x) { return isMd(x) | isFun(x); }
FORCE_INLINE bool isPrim(B x) { return isCallable(x) && v(x)->flags; }


// make objects
static B m_f64(f64 n) { assert(isF64(b(n))); return b(n); } // assert just to make sure we're actually creating a float
static B m_c32(u32 n) { return tagu64(n,C32_TAG); } // TODO check validity?
static B m_i32(i32 n) { return m_f64(n); }
static B m_usz(usz n) { return m_f64(n); }

// two integer casts for i8 & i16 because clang on armv8 otherwise skips the sign extending step
FORCE_INLINE bool q_fbit(f64 x) { return x==0 | x==1; }          FORCE_INLINE bool q_bit(B x) { return isNum(x) & (x.f==0 | x.f==1); }
FORCE_INLINE bool q_fi8 (f64 x) { return x==(f64)(i8 )(i32)x; }  FORCE_INLINE bool q_i8 (B x) { return q_fi8 (x.f); }
FORCE_INLINE bool q_fi16(f64 x) { return x==(f64)(i16)(i32)x; }  FORCE_INLINE bool q_i16(B x) { return q_fi16(x.f); }
FORCE_INLINE bool q_fi32(f64 x) { return x==(f64)(i32)     x; }  FORCE_INLINE bool q_i32(B x) { return q_fi32(x.f); }
FORCE_INLINE bool q_fi64(f64 x) { return x==(f64)(i64)     x; }  FORCE_INLINE bool q_i64(B x) { return q_fi64(x.f); }
FORCE_INLINE bool q_fu64(f64 x) { return x==(f64)(u64)     x; }  FORCE_INLINE bool q_u64(B x) { return q_fu64(x.f); }
FORCE_INLINE bool q_fusz(f64 x) { return x==(f64)(usz)     x; }  FORCE_INLINE bool q_usz(B x) { return q_fusz(x.f); }
/*no need for a q_ff64*/                                         FORCE_INLINE bool q_f64(B x) { return isF64(x); }
FORCE_INLINE bool q_ibit(i64 x) { return x==0 | x==1; }
FORCE_INLINE bool q_ubit(u64 x) { return x==0 | x==1; }
FORCE_INLINE bool q_c8 (B x) { return x.u>>8  == ((u64)C32_TAG)<<40; }
FORCE_INLINE bool q_c16(B x) { return x.u>>16 == ((u64)C32_TAG)<<32; }
FORCE_INLINE bool q_c32(B x) { return isC32(x); }
FORCE_INLINE bool q_N   (B x) { return x.u==bi_N.u; } // is ·
FORCE_INLINE bool noFill(B x) { return x.u==bi_noFill.u; }

FORCE_INLINE bool  o2bG(B x) { return(x.u<<1)!=0;}  FORCE_INLINE bool  o2b(B x) { i32 t=(i32)x.f; if(t!=x.f || t!=0&t!=1)thrM("Expected boolean"); return o2bG(x); }
FORCE_INLINE i32   o2iG(B x) { return (i32)x.f; }   FORCE_INLINE i32   o2i(B x) { if (!q_i32(x)) thrM("Expected integer");              return o2iG(x); }
FORCE_INLINE u32   o2cG(B x) { return (u32)x.u; }   FORCE_INLINE u32   o2c(B x) { if (!isC32(x)) thrM("Expected character");            return o2cG(x); }
FORCE_INLINE usz   o2sG(B x) { return (usz)x.f; }   FORCE_INLINE usz   o2s(B x) { if (!q_usz(x)) thrM("Expected non-negative integer"); return o2sG(x); }
FORCE_INLINE f64   o2fG(B x) { return      x.f; }   FORCE_INLINE f64   o2f(B x) { if (!isNum(x)) thrM("Expected number");               return o2fG(x); }
FORCE_INLINE i64 o2i64G(B x) { return (i64)x.f; }   FORCE_INLINE i64 o2i64(B x) { if (!q_i64(x)) thrM("Expected integer");              return o2i64G(x); }
FORCE_INLINE u64 o2u64G(B x) { return (u64)x.f; }   FORCE_INLINE u64 o2u64(B x) { if (!q_u64(x)) thrM("Expected integer");              return o2u64G(x); }


typedef struct Slice {
  struct Arr;
  Arr* p;
} Slice;
void tyarr_freeO(Value* x); void tyarr_freeF(Value* x);
void slice_freeO(Value* x); void slice_freeF(Value* x);
void slice_visit(Value* x);
void slice_print(B x);

typedef struct Md1 Md1;
typedef struct Md2 Md2;
typedef struct Md1D Md1D;
typedef struct Md2D Md2D;

typedef bool (*  B2b)(B);
typedef void (*  B2v)(B);
typedef void (* FB2v)(FILE*, B);
typedef Arr* (*BSS2A)(B, usz, usz);
typedef B (*    AS2B)(Arr*, usz);
typedef B (*    BS2B)(B, usz);
typedef B (*   BSS2B)(B, usz, usz);
typedef B (*     B2B)(B);
typedef B (*    BB2B)(B, B);
typedef B (*   BBB2B)(B, B, B);
typedef B (*  BBBB2B)(B, B, B, B);

typedef B (*M1C3)(Md1*, B, B, B);
typedef B (*M1C4)(Md1*, B, B, B, B);
typedef B (*M2C4)(Md2*, B, B, B, B);
typedef B (*M2C5)(Md2*, B, B, B, B, B);

typedef B (*D1C1)(Md1D*, B);
typedef B (*D1C2)(Md1D*, B, B);
typedef B (*D2C1)(Md2D*, B);
typedef B (*D2C2)(Md2D*, B, B);

#define FOR_TI(F) \
  F(V2v, freeF)  /* expects refc==0, includes mm_free */ \
  F(AS2B, get)   /* increments result, doesn't consume arg; TODO figure out if this should never allocate, so GC wouldn't happen */ \
  F(AS2B, getU)  /* like get, but doesn't increment result (mostly equivalent to `B t=get(…); dec(t); t`) */ \
  F(BB2B,  m1_d) /* consume all args; (m, f)    */ \
  F(BBB2B, m2_d) /* consume all args; (m, f, g) */ \
  F(BSS2A, slice) /* consumes; create slice from a starting position and length; add shape & rank yourself; may not actually be a Slice object; preserves fill */ \
  F(B2B, identity) /* return identity element of this function; doesn't consume */ \
  \
  F(   BBB2B, fn_uc1) /* t,o,      x→r; r≡O⌾(      T    ) x; consumes x   */ \
  F(  BBBB2B, fn_ucw) /* t,o,    w,x→r; r≡O⌾(w⊸    T    ) x; consumes w,x */ \
  F(M1C3, m1_uc1) /* t,o,f,    x→r; r≡O⌾(   F _T    ) x; consumes x   */ \
  F(M1C4, m1_ucw) /* t,o,f,  w,x→r; r≡O⌾(w⊸(F _T   )) x; consumes w,x */ \
  F(M2C4, m2_uc1) /* t,o,f,g,  x→r; r≡O⌾(   F _T_ G ) x; consumes x   */ \
  F(M2C5, m2_ucw) /* t,o,f,g,w,x→r; r≡O⌾(w⊸(F _T_ G)) x; consumes w,x */ \
  \
  F( BB2B, fn_im) /* t,  x; function monadic inverse;   consumes x   */ \
  F( BB2B, fn_is) /* t,  x; function equal-arg inverse; consumes x   */ \
  F(BBB2B, fn_iw) /* t,w,x; function dyadic 𝕨-inverse;  consumes w,x */ \
  F(BBB2B, fn_ix) /* t,w,x; function dyadic 𝕩-inverse;  consumes w,x */ \
  F( D1C1, m1_im) /* d,  x; 1-modifier monadic inverse;  consumes x   */ \
  F( D1C2, m1_iw) /* d,w,x; 1-modifier dyadic 𝕨-inverse; consumes w,x */ \
  F( D1C2, m1_ix) /* d,w,x; 1-modifier dyadic 𝕩-inverse; consumes w,x */ \
  F( D2C1, m2_im) /* d,  x; 2-modifier monadic inverse;  consumes x   */ \
  F( D2C2, m2_iw) /* d,w,x; 2-modifier dyadic 𝕨-inverse; consumes w,x */ \
  F( D2C2, m2_ix) /* d,w,x; 2-modifier dyadic 𝕩-inverse; consumes w,x */ \
  \
  F(B2b, canStore) /* doesn't consume */ \
  F(u8, elType) /* guarantees that the corresponding i32any_ptr/f64any_ptr/c32any_ptr/… always succeeds */ \
  \
  F(FB2v, print) /* doesn't consume */ \
  F(V2v, visit) /* call mm_visit for all referents */ \
  F(V2v, freeO) /* like freeF, but doesn't call mm_free for GC to be able to clear cycles */ \
  F(V2v, freeT) /* freeF, but assumes this is an array which consists of non-heap-allocated elements */ \
  F(B2B, decompose) /* consumes; must return a HArr */ \
  F(bool, isArr) /* whether this type would have an ARR_TAG tag, in cases where the tag is unknown */ \
  F(bool, arrD1) /* is always an array with depth 1 */ \

#define F(TY,N) extern TY ti_##N[t_COUNT];
  FOR_TI(F)
#undef F
#define TIi(X,V) (ti_##V[X])
#define TIv(X,V) (ti_##V[PTY(X)])
#define TI(X,V)  (ti_##V[ TY(X)])

#define SGetU(X) Arr* X##_arrU = a(X); AS2B X##_getU = TIv(X##_arrU,getU);
#define IGetU(X,N) ({ Arr* x_ = a(X); TIv(x_,getU)(x_,N); })
#define GetU(X,N) X##_getU(X##_arrU,N)
#define SGet(X) Arr* X##_arr = a(X); AS2B X##_get = TIv(X##_arr,get);
#define IGet(X,N) ({ Arr* x_ = a(X); TIv(x_,get)(x_,N); })
#define Get(X,N) X##_get(X##_arr,N)


enum Flags {
  fl_squoze=1,
  fl_asc=2, // sorted ascending (non-descending)
  fl_dsc=4, // sorted descending (non-ascending)
};
#define FL_SET(X,F)  ({ B    x_ = (X); v(x_)->flags|= (F); x_; })
#define FLV_SET(X,F) ({ AUTO x_ = (X);    x_->flags|= (F); x_; })
#define FL_KEEP(X,F)  ({ B    x_ = (X); v(x_)->flags&= (F); x_; })
#define FLV_KEEP(X,F) ({ AUTO x_ = (X);    x_->flags&= (F); x_; })
#define FL_HAS(X,F) ((v(X)->flags&(F)) != 0)
#define FLV_HAS(X,F) (((X)->flags&(F)) != 0)

// refcount stuff
static bool reusable(B x) { return v(x)->refc==1; }
#define REUSE(X) ({ B x_ = (X); v(x_)->flags = 0; x_; })
#define DEF_FREE(TY) static inline void TY##_freeO(Value* x); static void TY##_freeF(Value* x) { TY##_freeO(x); mm_free(x); } static inline void TY##_freeO(Value* x)
FORCE_INLINE void value_free(Value* x) { TIv(x,freeF)(x); }
void value_freeF(Value* x);
static void dec(B x) {
  if (!isVal(VALIDATE(x))) return;
  Value* vx = v(x);
  if(!--vx->refc) value_free(vx);
}
static inline void ptr_dec(void* x) { if(!--VALIDATEP((Value*)x)->refc) value_free(x); }
static inline void ptr_decR(void* x) { if(!--VALIDATEP((Value*)x)->refc) value_freeF(x); }
#define tptr_dec(X, F) ({ Value* x_ = (Value*)(X); if (!--VALIDATEP(x_)->refc) F(x_); })
static void decR(B x) {
  if (!isVal(VALIDATE(x))) return;
  Value* vx = v(x);
  if(!--vx->refc) value_freeF(vx);
}
void decA_F(B x);
static void decA(B x) { if (RARE(isVal(x))) decA_F(x); } // decrement what's likely an atom
static inline B inc(B x) {
  if (isVal(VALIDATE(x))) v(x)->refc++;
  return x;
}
static inline void decG(B x) {
  #if DEBUG
  assert(isVal(x));
  #endif
  Value* vx = v(x);
  if(!--vx->refc) value_free(vx);
}
FORCE_INLINE void ptr_decT(Arr* x) { // assumes argument is an array and consists of non-heap-allocated elements
  if (x->refc==1) TIv(x,freeT)((Value*) x);
  else x->refc--;
}
static inline B incG(B x) { // inc for guaranteed heap-allocated objects
  assert(isVal(x));
  v(VALIDATE(x))->refc++;
  return x;
}
static inline B incBy(B x, i64 am) { // you most likely don't want am to be negative as this won't free on refc==0
  if (isVal(VALIDATE(x))) v(x)->refc+= am;
  return x;
}
static inline B incByG(B x, i64 am) { v(x)->refc+= am; return x; }
#define ptr_inc(X) ({ AUTO x_ = (X); VALIDATEP((Value*)x_)->refc++; x_; })



typedef struct Fun {
  struct Value;
  BB2B c1;
  BBB2B c2;
} Fun;

#if USE_VALGRIND
  B vg_validateResult(B x);
  #define VRES(X) vg_validateResult(X)
#else
  #define VRES(X) X
#endif

B c1F(B f, B x);
B c2F(B f, B w, B x);
static B c1(B f, B x) { // BQN-call f monadically; consumes x
  if (isFun(f)) return VALIDATE(VRES(c(Fun,f)->c1(f, VRES(x))));
  return c1F(f, x);
}
static B c2(B f, B w, B x) { // BQN-call f dyadically; consumes w,x
  if (isFun(f)) return VALIDATE(VRES(c(Fun,f)->c2(f, VRES(w), VRES(x))));
  return c2F(f, w, x);
}
static void errMd(B x) { if(RARE(isMd(x))) thrM("Calling a modifier"); }
// like c1/c2, but with less overhead on non-functions
static B c1i(B f, B x) {
  if (isFun(f)) return VALIDATE(c(Fun,f)->c1(f, x));
  dec(x); errMd(f);
  return inc(f);
}
static B c2i(B f, B w, B x) {
  if (isFun(f)) return VALIDATE(c(Fun,f)->c2(f, w, x));
  dec(w); dec(x); errMd(f);
  return inc(f);
}
static B c1iX(B f, B x) { // c1 with inc(x)
  if (isFun(f)) return VALIDATE(c(Fun,f)->c1(f, inc(x)));
  errMd(f);
  return inc(f);
}
static B c2iX(B f, B w, B x) { // c2 with inc(x)
  if (isFun(f)) return VALIDATE(c(Fun,f)->c2(f, w, inc(x)));
  dec(w); errMd(f);
  return inc(f);
}
static B c2iW(B f, B w, B x) { // c2 with inc(w)
  if (isFun(f)) return VALIDATE(c(Fun,f)->c2(f, inc(w), x));
  dec(x); errMd(f);
  return inc(f);
}
static B c2iWX(B f, B w, B x) { // c2 with inc(w), inc(x)
  if (isFun(f)) return VALIDATE(c(Fun,f)->c2(f, inc(w), inc(x)));
  errMd(f);
  return inc(f);
}


struct Md1 {
  struct Value;
  D1C1 c1; // f(md1d{this,f},  x); consumes x
  D1C2 c2; // f(md1d{this,f},w,x); consumes w,x
};
struct Md2 {
  struct Value;
  D2C1 c1; // f(md2d{this,f,g},  x); consumes x
  D2C2 c2; // f(md2d{this,f,g},w,x); consumes w,x
};
static B m1_d(B m, B f     );
static B m2_d(B m, B f, B g);
static B m2_h(B m,      B g);
static B m_md1D(Md1* m, B f     );
static B m_md2D(Md2* m, B f, B g);
static B m_md2H(Md2* m,      B g);
static B m_fork(B f, B g, B h);
static B m_atop(     B g, B h);


