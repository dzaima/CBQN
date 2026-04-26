#pragma once

#ifndef DEBUG
  #define DEBUG 0
#endif

#ifdef __OpenBSD__
  #define __wchar_t __wchar_t2 // horrible hack for BSD
#endif
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#ifndef USE_SETJMP
  #define USE_SETJMP 1
#endif
#if USE_SETJMP
#include <setjmp.h>
#endif

#ifndef MM
  #define MM 1
#endif
#if ALL_R0 || ALL_R1
  #define WRAP_NNBI 1
#endif
#if RT_PERF || RT_VERIFY
  #define RT_WRAP 1
#endif
#if RT_WRAP || WRAP_NNBI
  #define IF_WRAP(X) X
#else
  #define IF_WRAP(X)
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
typedef float    f32;
typedef size_t ux;
#if defined(__SIZEOF_INT128__)
  #define HAS_U128 1
  typedef unsigned __int128 u128;
  typedef signed __int128 i128;
#endif
#define I8_MIN -128
#define I8_MAX  127
#define I16_MIN -32768
#define I16_MAX  32767
#define I32_MIN -2147483648
#define I32_MAX  2147483647
#define I64_MIN ((i64)(1ULL<<63))
#define I64_MAX ((i64)((1ULL<<63)-1))
#define CHR_MAX 1114111
#define U8_MAX  ((u8 )~(u8 )0)
#define U16_MAX ((u16)~(u16)0)
#define U32_MAX ((u32)~(u32)0)
#define U64_MAX ((u64)~(u64)0)
#define MAYBE_UNUSED __attribute__((unused))
#define NOINLINE     __attribute__((noinline))
#define FORCE_INLINE __attribute__((always_inline)) static inline
#define NORETURN     __attribute__((noreturn))
#if DEBUG
  #define SHOULD_INLINE static
#else
  #define SHOULD_INLINE FORCE_INLINE
#endif
#define AUTO __auto_type
#define LIKELY(X) __builtin_expect((X)!=0,1)
#define RARE(X) __builtin_expect((X)!=0,0)
#define ARBITRARY(T) ((T)0) // to be potentially replaced with something better if such arrives
#define GUARANTEED(V) ({ AUTO v_ = (V); __builtin_constant_p(v_) && v_; })
#define fsizeof(T,F,E,N) (offsetof(T, F) + sizeof(E)*(N)) // type, flexible array member name, flexible array member type, item amount
#define RFLD(X,T,F) ((T*)((char*)(X) - offsetof(T,F))) // value, result type, field name; reverse-read field: `T* x = …; E v = x->f; x == RFLD(v, T, f)`
#define CLZ64(X) __builtin_clzll(X)
#define CTZ(X) __builtin_ctzll(X)
#define POPC(X) __builtin_popcountll(X)
#define IMAX(A, B) ({ AUTO a_ = (A); AUTO b_ = (B); a_ > b_? a_ : b_; })
#define IMIN(A, B) ({ AUTO a_ = (A); AUTO b_ = (B); a_ < b_? a_ : b_; })
#define ICMP(A, B) ({ AUTO a_ = (A); AUTO b_ = (B); (a_>b_?1:0)-(a_<b_?1:0); })
#define IABS(R, X) ({ AUTO sgn_ = (X); R uns_ = (R) sgn_; sgn_<0? -uns_ : uns_; })
#define PTR_TO_U64(X) ((u64)(uintptr_t)(X))
#define PTR_FROM_INT(T,X) ((T*)(uintptr_t)(X))
#define PTR_ROUND_UP(P, N) ({ AUTO p_ = (P); u64 n_ = (N); PTR_FROM_INT(typeof(*p_), (PTR_TO_U64(p_)+n_-1) & ~(n_-1)); })
#define PTR_ROUND_UP_TO_ELT(P) ({ AUTO p2_ = (P); PTR_ROUND_UP(p2_, _Alignof(typeof(*p2_))); })
#define ADD_ON(V,X) ({ AUTO v_ = &(V); __builtin_add_overflow(*v_, X, v_); })
#define MUL_ON(V,X) ({ AUTO v_ = &(V); __builtin_mul_overflow(*v_, X, v_); })
static void storeu_u64(void* p, u64 v) { memcpy(p, &v, 8); }  static u64 loadu_u64(void* p) { u64 v; memcpy(&v, p, 8); return v; }
static void storeu_u32(void* p, u32 v) { memcpy(p, &v, 4); }  static u32 loadu_u32(void* p) { u32 v; memcpy(&v, p, 4); return v; }
static void storeu_u16(void* p, u16 v) { memcpy(p, &v, 2); }  static u16 loadu_u16(void* p) { u16 v; memcpy(&v, p, 2); return v; }
#define N64x "%"SCNx64
#define N64d "%"SCNd64
#define N64u "%"SCNu64
#if __clang__
  #define NOUNROLL _Pragma("clang loop unroll(disable)")
  #define NOVECTORIZE _Pragma("clang loop vectorize(disable)")
  #define vfor _Pragma("clang loop vectorize(assume_safety)") for
#elif __GNUC__
  #define EXACTLY_GCC 1
  #define NOUNROLL _Pragma("GCC unroll 1")
  #define vfor _Pragma("GCC ivdep") for
  #if __GNUC__ >= 14
    #define NOVECTORIZE _Pragma("GCC novector")
  #else
    #define NOVECTORIZE
  #endif
#else
  #define NOUNROLL
  #define NOVECTORIZE
  #define vfor for
#endif
#define PLAINLOOP NOUNROLL NOVECTORIZE
#if EXACTLY_GCC
  #define ONLY_GCC(X) X
#else
  #define ONLY_GCC(X)
#endif
#if __clang__ && __has_attribute(noescape) // workaround for clang not realizing that stack-returned structs aren't captured
  #define NOESCAPE __attribute__((noescape))
  #define SRET_DEF(RET, NAME, ...) void NAME( RET* ret_val_, __VA_ARGS__)
  #define SRET_CALL(RET, NAME, ...) ({ RET sret_; NAME(&sret_, __VA_ARGS__); sret_; })
  #define SRET_RET(VAL) do { *ret_val_ = VAL; return; } while(0)
#else
  #define NOESCAPE
  #define SRET_DEF(RET, NAME, ...) RET NAME(__VA_ARGS__)
  #define SRET_CALL(RET, NAME, ...) NAME(__VA_ARGS__)
  #define SRET_RET(VAL) return VAL
#endif

#define JOIN0(A,B) A##B
#define JOIN(A,B) JOIN0(A,B)
#define STR0(X) #X
#define STR1(X) STR0(X)
#define INIT_GLOBAL __attribute__((visibility("hidden"))) // global variable set once during initialization, to the same value always
#define GLOBAL __attribute__((visibility("hidden"))) // global variable mutated potentially multiple times, or set to a value referencing the heap
#define STATIC_GLOBAL static // GLOBAL (i.e. may be state-specific) but static

#if USE_REPLXX_IO
  #include <replxx.h>
  extern GLOBAL Replxx* global_replxx;
  #define printf(...) replxx_print(global_replxx, __VA_ARGS__)
  #define fprintf(f, ...) replxx_print(global_replxx, __VA_ARGS__)
#endif

#if USZ_64
  typedef u64 usz;
  #define USZ_MAX ((u64)(1ULL<<48))
  #define CHECK_IA(IA,W) if((IA) >= USZ_MAX) thrOOM()
#else
  typedef u32 usz;
  #define USZ_MAX ((u32)((1LL<<32)-1))
  #define CHECK_IA(IA,W) if ((IA) > ((1LL<<31)/W - 1000)) thrOOM()
#endif

#if UNSAFE_SIZES
  #undef CHECK_IA
  #define CHECK_IA(IA,W)
#endif
typedef u8 ur;
#define UR_MAX 255

                                               // .FF0 .111111111110000000000000000000000000000000000000000000000000000 infinity
                                               // .FF8 .111111111111000000000000000000000000000000000000000000000000000 qNaN
                                               // .FF. .111111111110nnn................................................ sNaN aka tagged aka not f64, if nnn≠0
                                               // 7FF. 0111111111110................................................... direct value with no need of refcounting
static const u16 C32_TAG = 0b0111111111110001; // 7FF1 0111111111110001................00000000000ccccccccccccccccccccc char
static const u16 TAG_TAG = 0b0111111111110010; // 7FF2 0111111111110010nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn special value (see bi_N ect definitions further below)
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
#define tagu64(V, T) r_uB((u64)(V) | ftag(T))
#define c(T,X) PTR_FROM_INT(T, (X).u&0xFFFFFFFFFFFFull)
#define tag(V, T) ({ void* tagv_ = (V); r_uB(PTR_TO_U64(tagv_) | ftag(T)); })
#define taga(V) tag(V,ARR_TAG)

typedef union B {
  u64 u;
  f64 f;
} B;

FORCE_INLINE u64 r_f64u(f64 x) { u64 r; memcpy(&r,&x,8); return r; }
FORCE_INLINE f64 r_u64f(u64 x) { f64 r; memcpy(&r,&x,8); return r; }
FORCE_INLINE u32 r_f32u(f32 x) { u32 r; memcpy(&r,&x,4); return r; }
FORCE_INLINE f32 r_u32f(u32 x) { f32 r; memcpy(&r,&x,4); return r; }
FORCE_INLINE u64 r_Bu(B x) { return x.u; }
FORCE_INLINE f64 r_Bf(B x) { return x.f; }
FORCE_INLINE B r_uB(u64 x) { return (B){.u=x}; }
FORCE_INLINE B r_fB(f64 x) { return (B){.f=x}; }

#if defined(OBJ_TRACK)
  #define OBJ_COUNTER 1
#endif
#if DEBUG && !defined(VERIFY_TAIL) && MM==1
  #define VERIFY_TAIL 64
#endif
#if ALLOC_STAT || VERIFY_TAIL
  #define ALLOC_NOINLINE 1
#endif
#if ALLOC_NOINLINE
  #if MM_C
    #define ALLOC_IMPL 1
  #endif
  #define ALLOC_FN
#else
  #define ALLOC_FN static
#endif

#define FOR_TYPE(F) \
  /* 0*/ F(empty) \
  /* 1*/ F(funBI) F(funBl) \
  /* 3*/ F(md1BI) F(md1Bl) \
  /* 5*/ F(md2BI) F(md2Bl) \
  /* 7*/ F(shape) /* doesn't get visited in arrays, won't be freed by gc */ \
  \
  /* 8*/ F(fork) F(atop) F(md1D) F(md2D) \
  \
  /*12*/ F(hslice) F(fillslice) F(i8slice) F(i16slice) F(i32slice) F(c8slice) F(c16slice) F(c32slice) F(f64slice) \
  /*21*/ F(harr  ) F(fillarr  ) F(i8arr  ) F(i16arr  ) F(i32arr  ) F(c8arr  ) F(c16arr  ) F(c32arr  ) F(f64arr  ) \
  /*30*/ F(bitarr) \
  \
  /*31*/ F(comp) F(block) F(body) F(scope) F(scopeExt) F(blBlocks) F(arbObj) F(ffiType) F(bvwArena) \
  /*40*/ F(ns) F(nsDesc) F(fldAlias) F(arrMerge) F(vfyObj) F(hashmap) F(temp) F(talloc) F(nfn) F(nfnDesc) \
  /*50*/ F(freed) F(invalid) F(customObj) F(mmapH) \
  \
  /*54*/ IF_WRAP(F(funWrap) F(md1Wrap) F(md2Wrap))

enum Type {
  #define F(X) t_##X,
  FOR_TYPE(F)
  #undef F
  t_COUNT
};
#define IS_ANY_ARR(T) ({ u8 ct_=(T); ct_>=t_hslice & ct_<=t_bitarr; })
#define IS_DIRECT_TYARR(T) ({ u8 ct_=(T); ct_>=t_i8arr & ct_<=t_bitarr; })
#define ARR_IS_SLICE(T) ((T)<=t_f64slice)
#define IS_TYSLICE(T) ({ u8 ct_=(T); ct_>=t_i8slice & ct_<=t_f64slice; })
#define ARR_TO_SLICE(T) ((T) + t_hslice - t_harr) // requires T!=t_bitarr
#define SLICE_TO_ARR(T) ((T) + t_harr - t_hslice)

enum ElType { // if X can store a superset of elements of Y, X > Y
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
  #if OBJ_COUNTER
  u64 uid;
  #endif
} Value;
typedef struct Arr {
  struct Value;
  usz ia;
  usz* sh;
} Arr;

#if DEBUG
  NOINLINE NORETURN void assert_fail(char* expr, char* file, int line, const char fn[]);
  #define assert_impl(M, X) do {if (!(X)) assert_fail(M, __FILE__, __LINE__, __PRETTY_FUNCTION__);} while(0)
  #define assert(X) assert_impl(#X, X)
  #define debug_assert(X) assert_impl(#X, X)
  B VALIDATE(B x);
  Value* VALIDATEP(Value* x);
  #define UD assert(false)
  extern GLOBAL bool cbqn_noAlloc;
  NOINLINE void cbqn_NOGC_start(); // function to allow breakpointing
  #define NOGC_CHECK(M) do { if (cbqn_noAlloc && !gc_depth) fatal(M); } while (0)
  #define NOGC_S cbqn_NOGC_start()
  #define NOGC_E cbqn_noAlloc=false
  #define HEURISTIC_BOUNDED(X, TRUE_REQ, FALSE_REQ) ({ bool hb_ = (X); assert(hb_? (TRUE_REQ) : (FALSE_REQ)); hb_; })
#else
  #define assert(X) do {if (!(X)) __builtin_unreachable();} while(0)
  #define debug_assert(X)
  #define VALIDATE(X) (X)
  #define VALIDATEP(X) (X)
  #define UD __builtin_unreachable()
  #define NOGC_S
  #define NOGC_E
  #define NOGC_CHECK(M)
  #define HEURISTIC_BOUNDED(X, TRUE_REQ, FALSE_REQ) (X)
#endif
#if RANDOMIZE_HEURISTICS
  bool heuristic_rand(bool heuristic, bool true_req, bool false_req);
  #undef HEURISTIC_BOUNDED
  #define HEURISTIC_BOUNDED(X, TRUE_REQ, FALSE_REQ) heuristic_rand(X, TRUE_REQ, FALSE_REQ)
#else
  #define RANDOMIZE_HEURISTICS 0
#endif
#define HEURISTIC(X) HEURISTIC_BOUNDED(X, true, true)
#define MAY_T(X) ({ bool hw_=(X); HEURISTIC_BOUNDED(hw_, true, !hw_); }) // returns X, or, with randomized heuristics enabled, possibly true
#define MAY_F(X) ({ bool hw_=(X); HEURISTIC_BOUNDED(hw_, hw_,  true); }) // returns X, or, with randomized heuristics enabled, possibly false
#if WARN_SLOW
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
void gc_add_ref(B* x); // add x as a root reference
bool gc_maybeGC(bool toplevel); // gc if that seems necessary; returns if did gc
void gc_forceGC(bool toplevel); // force a gc; who knows what happens if gc is disabled (probably should error)
u64 tot_heapUsed(void);

// some primitive actions
static const B bi_N      = (B) {.u = (u64)0x7FF2000000000000ull };
static const B bi_noVar  = (B) {.u = (u64)0x7FF2C00000000001ull };
static const B bi_okHdr  = (B) {.u = (u64)0x7FF2000000000002ull };
static const B bi_optOut = (B) {.u = (u64)0x7FF2800000000003ull };
static const B bi_noFill = (B) {.u = (u64)0x7FF2000000000005ull };
static const B bi_z      = (B) {.u = 0}; // non-heap-allocated object placeholder
extern GLOBAL B bi_emptyHVec, bi_emptyIVec, bi_emptyCVec, bi_emptySVec;
#define emptyHVec() incG(bi_emptyHVec)
#define emptyIVec() incG(bi_emptyIVec)
#define emptyCVec() incG(bi_emptyCVec)
#define emptySVec() incG(bi_emptySVec)
ALLOC_FN void* mm_alloc(u64 sz, u8 type);
ALLOC_FN void  mm_free(Value* x);
static void mm_visit(B x);
static void mm_visitP(void* x);
static u64  mm_size(Value* x);
#if !VERIFY_TAIL
#define mm_sizeUsable mm_size
#endif
static void dec(B x);
static B    inc(B x);
static void ptr_dec(void* x);

NORETURN NOINLINE void fatal(char* s);
void fprintI(FILE* f, B x); // doesn't consume
void  printI(         B x); // doesn't consume
void fprintsB(FILE* f, B x); // doesn't consume
void  printsB(         B x); // doesn't consume
void farr_print(FILE* f, B x); // doesn't consume
void  arr_print(         B x); // doesn't consume

NOINLINE NORETURN void thr(B b);
NOINLINE NORETURN void rethrow(void);
NOINLINE NORETURN void thrM(char* s);
NOINLINE NORETURN void thrF(char* s, ...);
NOINLINE NORETURN void thrOOM(void);
#if USE_SETJMP
  jmp_buf* prepareCatch(void);
  #define CATCH setjmp(*prepareCatch()) // use as `if (CATCH) { /*handle error*/ freeThrown(); return; } /*potentially erroring thing*/ popCatch(); /*no errors yay*/`
  void popCatch(void);                  // note: popCatch() must always be called if no error is thrown, so no returns before it!
#else
  #define CATCH false
  #define popCatch()
#endif
extern GLOBAL B thrownMsg;
void freeThrown(void);



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

#define RTID_NONE -1
#define PRTID(X) ((X)->flags-1)
#define RTID(X) PRTID(v(X))
#define NID(X) ((X)->nid)

#define VTY(X,T) assert(isVal(X) && TY(X)==(T))

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
// FORCE_INLINE bool isF64(B x) { return ((x.u>>51&0xFFF) != 0xFFE)  |  ((x.u<<1)==(r_Bu(m_f64(1.0/0.0))<<1)); }
FORCE_INLINE bool isVal(B x) { return (x.u - (((u64)VAL_TAG<<51) + 1)) < ((1ull<<51) - 1); } // ((x.u>>51) == VAL_TAG)  &  ((x.u<<13) != 0);
FORCE_INLINE bool isF64(B x) { return (x.u<<1) - ((0xFFEull<<52) + 2) >= (1ull<<52) - 2; }
FORCE_INLINE bool isNum(B x) { return isF64(x); }

FORCE_INLINE bool isAtm(B x) { return !isArr(x); }
FORCE_INLINE bool isCallable(B x) { u16 tag = x.u>>48; return tag>=MD1_TAG && tag<=FUN_TAG; }
FORCE_INLINE bool isPrim(B x) { return isCallable(x) && RTID(x)!=RTID_NONE; }


// make objects
static B m_f64(f64 n) { assert(isF64(r_fB(n))); return r_fB(n); } // assert just to make sure we're actually creating a float
static B m_c32(u32 n) { return tagu64(n,C32_TAG); } // TODO check validity?
static B m_i32(i32 n) { return m_f64(n); }
static B m_usz(usz n) { return m_f64(n); }

// very funky workarounds for out-of-range float-to-int casts being UB, but it being desirable here to have them work anyway
// this still does UB, but hopefully in such a way that any sane optimizations can't really make it produce wrong results
#if defined(FLOAT_CAST_FENCE_ASM) ? FLOAT_CAST_FENCE_ASM : __clang__
  #define FLOAT_TO_INT(T, X) ({ __auto_type x_ = (T)(X); if (!__builtin_constant_p(x_)) __asm__("" : "+r"(x_)); x_; })
#else
  #define FLOAT_TO_INT(T, X) ((T)(X))
#endif
#define FLOAT_UB __attribute__((no_sanitize("float-cast-overflow")))
FLOAT_UB FORCE_INLINE bool q_fbit(f64 x) { return x==0 | x==1; }
FLOAT_UB FORCE_INLINE bool q_fi8 (f64 x) { return x==(f64)(i8 )(i32)x; }
FLOAT_UB FORCE_INLINE bool q_fi16(f64 x) { return x==(f64)(i16)(i32)x; }
FLOAT_UB FORCE_INLINE bool q_fi32(f64 x) { return x==(f64)FLOAT_TO_INT(i32, x); }
FLOAT_UB FORCE_INLINE bool q_fu8 (f64 x) { return x==(f64)(u8 )(i32)x; }
FLOAT_UB FORCE_INLINE bool q_fu16(f64 x) { return x==(f64)(u16)(i32)x; }
FLOAT_UB FORCE_INLINE bool q_fu32(f64 x) { return x==(f64)FLOAT_TO_INT(u32, x); }
FLOAT_UB FORCE_INLINE bool q_fi64(i64* out, f64 x) { return x == (f64) (*out = FLOAT_TO_INT(i64, x)); }
FLOAT_UB FORCE_INLINE bool q_fu64(u64* out, f64 x) { return x == (f64) (*out = FLOAT_TO_INT(u64, x)); }
FLOAT_UB FORCE_INLINE bool q_fusz(usz* out, f64 x) { return x == (f64) (*out = FLOAT_TO_INT(usz, x)); }
FLOAT_UB FORCE_INLINE bool q_fi8o(i8*  out, f64 x) { return x == (f64) (*out = (i8) (i32)x); }
FLOAT_UB FORCE_INLINE bool q_fi16o(i16*out, f64 x) { return x == (f64) (*out = (i16)(i32)x); }
FLOAT_UB FORCE_INLINE bool q_fi32o(i32*out, f64 x) { return x == (f64) (*out = FLOAT_TO_INT(i32, x)); }

FORCE_INLINE bool q_bit(B x) { return isF64(x) & (x.f==0 | x.f==1); }
FORCE_INLINE bool q_i8 (B x) { return q_fi8 (x.f); }
FORCE_INLINE bool q_i16(B x) { return q_fi16(x.f); }
FORCE_INLINE bool q_i32(B x) { return q_fi32(x.f); }
FORCE_INLINE bool q_f64(B x) { return isF64(x); }
FORCE_INLINE bool q_i64(i64* out, B x) { return q_fi64(out, x.f); }
FORCE_INLINE bool q_u64(u64* out, B x) { return q_fu64(out, x.f); }
FORCE_INLINE bool q_usz(usz* out, B x) { return q_fusz(out, x.f); }
FORCE_INLINE bool q_i32o(i32*out, B x) { return q_fi32o(out, x.f); }

FORCE_INLINE bool q_ibit(i64 x) { return x==0 | x==1; }
FORCE_INLINE bool q_ubit(u64 x) { return x==0 | x==1; }
FORCE_INLINE bool q_c8 (B x) { return x.u>>8  == ((u64)C32_TAG)<<40; }
FORCE_INLINE bool q_c16(B x) { return x.u>>16 == ((u64)C32_TAG)<<32; }
FORCE_INLINE bool q_c32(B x) { return isC32(x); }
FORCE_INLINE bool q_N   (B x) { return x.u==bi_N.u; } // is ·
FORCE_INLINE bool noFill(B x) { return x.u==bi_noFill.u; }
FORCE_INLINE bool q_z(B x) { return x.u==bi_z.u; }


NORETURN void expI_f64(f64 what); NORETURN void expI_B(B what);
NORETURN void expU_f64(f64 what); NORETURN void expU_B(B what);
FORCE_INLINE bool  o2bG(B x) { return(x.u<<1)!=0;}
FORCE_INLINE i32   o2iG(B x) { return (i32)x.f; }
FORCE_INLINE u32   o2cG(B x) { return (u32)x.u; }
FORCE_INLINE usz   o2sG(B x) { return (usz)x.f; }
FORCE_INLINE f64   o2fG(B x) { return      x.f; }
FORCE_INLINE i64 o2i64G(B x) { return (i64)x.f; }
FORCE_INLINE u64 o2u64G(B x) { return (u64)x.f; }

FORCE_INLINE bool  o2b(B x) { i32 t; if(!q_i32o(&t,x) || (u32)t >= 2) thrM("Expected boolean"); return t; }
FORCE_INLINE f64   o2f(B x) { if (!q_f64(x)) thrM("Expected number");    return o2fG(x); }
FORCE_INLINE u32   o2c(B x) { if (!isC32(x)) thrM("Expected character"); return o2cG(x); }
FORCE_INLINE i32   o2i(B x) { if (!q_i32(x)) expI_B(x);                  return o2iG(x); }
FORCE_INLINE usz   o2s(B x) { usz v; if (!q_usz(&v, x)) expU_B(x);       return v; }
FORCE_INLINE i64 o2i64(B x) { i64 v; if (!q_i64(&v, x)) expI_B(x);       return v; }
FORCE_INLINE u64 o2u64(B x) { u64 v; if (!q_u64(&v, x)) expU_B(x);       return v; }

// some aliases for macro-generated code
typedef u8 c8; typedef u16 c16; typedef u32 c32;
static B m_i8(i8 x) { return m_i32(x); } static B m_i16(i16 x) { return m_i32(x); }
static B m_c8(u8 x) { return m_c32(x); } static B m_c16(u16 x) { return m_c32(x); }
static B m_B(B x) { return x; } static bool q_B(B x) { return true; }

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
typedef BB2B FC1;
typedef BBB2B FC2;


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
  F( BBB2B, fn_uc1) /* t,o,      x→r; r≡O⌾(      T    ) x; consumes x   */ \
  F(BBBB2B, fn_ucw) /* t,o,    w,x→r; r≡O⌾(w⊸    T    ) x; consumes w,x */ \
  F(M1C3,   m1_uc1) /* t,o,f,    x→r; r≡O⌾(   F _T    ) x; consumes x   */ \
  F(M1C4,   m1_ucw) /* t,o,f,  w,x→r; r≡O⌾(w⊸(F _T   )) x; consumes w,x */ \
  F(M2C4,   m2_uc1) /* t,o,f,g,  x→r; r≡O⌾(   F _T_ G ) x; consumes x   */ \
  F(M2C5,   m2_ucw) /* t,o,f,g,w,x→r; r≡O⌾(w⊸(F _T_ G)) x; consumes w,x */ \
  \
  F(FC1,  fn_im) /* t,  x; function monadic inverse;   consumes x   */ \
  F(FC1,  fn_is) /* t,  x; function equal-arg inverse; consumes x   */ \
  F(FC2,  fn_iw) /* t,w,x; function dyadic 𝕨-inverse;  consumes w,x */ \
  F(FC2,  fn_ix) /* t,w,x; function dyadic 𝕩-inverse;  consumes w,x */ \
  F(D1C1, m1_im) /* d,  x; 1-modifier monadic inverse;  consumes x   */ \
  F(D1C2, m1_iw) /* d,w,x; 1-modifier dyadic 𝕨-inverse; consumes w,x */ \
  F(D1C2, m1_ix) /* d,w,x; 1-modifier dyadic 𝕩-inverse; consumes w,x */ \
  F(D2C1, m2_im) /* d,  x; 2-modifier monadic inverse;  consumes x   */ \
  F(D2C2, m2_iw) /* d,w,x; 2-modifier dyadic 𝕨-inverse; consumes w,x */ \
  F(D2C2, m2_ix) /* d,w,x; 2-modifier dyadic 𝕩-inverse; consumes w,x */ \
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
  F(bool, byRef) /* is always compared by reference */ \

#define F(TY,N) extern GLOBAL TY ti_##N[t_COUNT];
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
#define TO_GET(X,N) ({ B x_2 = (X); B r = IGet(x_2,N); decG(x_2); r; })


enum Flags {
  fl_squoze=1,
  fl_asc=2, // sorted ascending (non-descending)
  fl_dsc=4, // sorted descending (non-ascending)
};
#define FL_GET(X) (v(X)->flags)
#define FLV_GET(X) ((X)->flags)
#define FL_SET(X,F)  ({ B    x_ = (X); v(x_)->flags|= (F); x_; })
#define FLV_SET(X,F) ({ AUTO x_ = (X);    x_->flags|= (F); x_; })
#define FL_KEEP(X,F)  ({ B    x_ = (X); v(x_)->flags&= (F); x_; })
#define FLV_KEEP(X,F) ({ AUTO x_ = (X);    x_->flags&= (F); x_; })
#define FL_HAS(X,F) ((v(X)->flags&(F)) != 0)
#define FLV_HAS(X,F) (((X)->flags&(F)) != 0)
#define FL_HAS_ALL(X,F) ((v(X)->flags&(F)) == (F))
#define FLV_HAS_ALL(X,F) (((X)->flags&(F)) == (F))

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
static void decA(B x) { if (RARE(isVal(x))) decA_F(x); } // decrement a value which is likely to not be heap-allocated
static inline B inc(B x) {
  if (isVal(VALIDATE(x))) v(x)->refc++;
  return x;
}
static inline void decG(B x) {
  if (DEBUG) assert(isVal(x) && v(x)->refc>0);
  Value* vx = v(x);
  if(!--vx->refc) value_free(vx);
}
FORCE_INLINE void ptr_decT(Arr* x) { // assumes argument is an array and consists of non-heap-allocated elements
  if (DEBUG) assert(x->refc>0);
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
#define ptr_incBy(X, AM) ({ AUTO x_ = (X); VALIDATEP((Value*)x_)->refc+= (i64)(AM); x_; })



typedef struct Fun {
  struct Value;
  FC1 c1;
  FC2 c2;
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
NORETURN void callMd(B x);
static B errMd(B x) { if (RARE(isMd(x))) callMd(x); return x; }
// like c1/c2, but with less overhead on non-functions
SHOULD_INLINE B c1I(B f, B x) {
  if (isFun(f)) return VALIDATE(c(Fun,f)->c1(f, x));
  dec(x);
  return inc(errMd(f));
}
SHOULD_INLINE B c2I(B f, B w, B x) {
  if (isFun(f)) return VALIDATE(c(Fun,f)->c2(f, w, x));
  dec(w); dec(x);
  return inc(errMd(f));
}
static B c1iX(B f, B x) { // c1 with inc(x)
  if (isFun(f)) return VALIDATE(c(Fun,f)->c1(f, inc(x)));
  return inc(errMd(f));
}
static B c2iWX(B f, B w, B x) { // c2 with inc(w), inc(x)
  if (isFun(f)) return VALIDATE(c(Fun,f)->c2(f, inc(w), inc(x)));
  return inc(errMd(f));
}

static B c1G(B f,      B x) { assert(isFun(f)); return c(Fun,f)->c1(f,    x); }
static B c2G(B f, B w, B x) { assert(isFun(f)); return c(Fun,f)->c2(f, w, x); }
#define c1rt(N,    X) ({           B x_=(X); SLOW1("!rt_" #N,   x_); c1G(rt_##N,     x_); })
#define c2rt(N, W, X) ({ B w_=(W); B x_=(X); SLOW2("!rt_" #N,w_,x_); c2G(rt_##N, w_, x_); })

B md_c1(B t,      B x);
B md_c2(B t, B w, B x);
B arr_c1(B t,      B x);
B arr_c2(B t, B w, B x);
static FC1 c1fn(B f) {
  if (isFun(f)) return c(Fun,f)->c1;
  if (isMd(f)) return md_c1;
  return arr_c1;
}
static FC2 c2fn(B f) {
  if (isFun(f)) return c(Fun,f)->c2;
  if (isMd(f)) return md_c2;
  return arr_c2;
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

typedef struct ShArr {
  struct Value;
  usz a[];
} ShArr;
static ShArr* shObjS(usz* x) { return RFLD(x, ShArr, a); }
static ShArr* shObj (B x) { return RFLD(SH(x), ShArr, a); }
static ShArr* shObjP(Value* x) { return RFLD(PSH((Arr*)x), ShArr, a); }
static void decShObj(ShArr* x) { tptr_dec(x, mm_free); }
static void decSh(Value* x) { if (RARE(PRNK(x)>1)) decShObj(shObjP(x)); }

static u64 bit_reverse64(u64 x) {
  u64 c = __builtin_bswap64(x);
  c = (c&0x0f0f0f0f0f0f0f0f)<<4 | (c&0xf0f0f0f0f0f0f0f0)>>4;
  c = (c&0x3333333333333333)<<2 | (c&0xcccccccccccccccc)>>2;
  c = (c&0x5555555555555555)<<1 | (c&0xaaaaaaaaaaaaaaaa)>>1;
  return c;
}
