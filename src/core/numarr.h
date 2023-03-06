typedef struct TyArr {
  struct Arr;
  char a[];
} TyArr;
typedef struct TySlice {
  struct Slice;
  void* a;
} TySlice;

#define TU I8
#define TP(W,X) W##i8##X
#include "tyarrTemplate.h"
#define TU I16
#define TP(W,X) W##i16##X
#include "tyarrTemplate.h"
#define TU I32
#define TP(W,X) W##i32##X
#include "tyarrTemplate.h"
#define TU F64
#define TP(W,X) W##f64##X
#include "tyarrTemplate.h"
typedef TyArr BitArr;

// bit array stuff
#define BIT_N(IA) (((IA)+63) >> 6) // u64 count needed to store IA bits

static inline void bitp_set(u64* arr, u64 n, bool v) {
  u64 m = ((u64)1)<<(n&63);
  if (v) arr[n>>6]|=  m;
  else   arr[n>>6]&= ~m;
  // arr[n>>6] = (arr[n>>6]&(~m)) | (((u64)v)<<(n&63));
}
static inline bool bitp_get(u64* arr, u64 n) {
  return (arr[n>>6] >> (n&63)) & 1;
}
static inline u64 bitp_l0(u64* arr, u64 ia) { // last u64 of the array, with the tail set to 0s
  return ia&63? arr[ia>>6]&((1ULL<<(ia&63))-1) : 0;
}
static inline u64 bitp_l1(u64* arr, u64 ia) { // last u64 of the array, with the tail set to 1s
  return ia&63? arr[ia>>6]|~((1ULL<<(ia&63))-1) : ~0ULL;
}
static inline u64 bitx(B x) { // repeats the boolean across all 64 bits
  return o2bG(x)? ~(u64)0 : 0;
}
static inline bool bit_has(u64* arr, u64 ia, bool v) {
  u64 w = ~-(u64)v;
  u64 e = ia/64, q = ia%64;
  for (usz i=0; i<e; i++) if (arr[i]^w) return 1;
  return q && ((arr[e]^w) & ((1ULL<<q)-1));
}
static inline u64 bit_find(u64* arr, u64 ia, bool v) {
  u64 w = ~-(u64)v;
  u64 e = ia/64;
  for (u64 i=0; i<e; i++) {
    u64 f = w ^ arr[i];
    if (f) return 64*i + CTZ(f);
  }
  u64 q = ia%64;
  return (ia - q) | CTZ((w^arr[e]) | ~(u64)0<<q);
}

// BitArr
#define BITARR_SZ(IA) fsizeof(BitArr, a, u64, BIT_N(IA))
static B m_bitarrv(u64** p, usz ia) {
  BitArr* r = m_arr(BITARR_SZ(ia), t_bitarr, ia);
  arr_shVec((Arr*)r);
  *p = (u64*)r->a;
  return taga(r);
}
static B m_bitarrc(u64** p, B x) { assert(isArr(x));
  BitArr* r = m_arr(BITARR_SZ(IA(x)), t_bitarr, IA(x));
  *p = (u64*)r->a;
  arr_shCopy((Arr*)r, x);
  return taga(r);
}
static Arr* m_bitarrp(u64** p, usz ia) {
  BitArr* r = m_arr(BITARR_SZ(ia), t_bitarr, ia);
  *p = (u64*)r->a;
  return (Arr*)r;
}
static u64* bitarr_ptr(B x) { VTY(x, t_bitarr); return (u64*)c(BitArr,x)->a; }
static u64* bitarrv_ptr(TyArr* x) { return (u64*)x->a; }


Arr* cpyI8Arr (B x); // consumes
Arr* cpyI16Arr(B x); // consumes
Arr* cpyI32Arr(B x); // consumes
Arr* cpyF64Arr(B x); // consumes
Arr* cpyBitArr(B x); // consumes

// all consume x
static I8Arr*  toI8Arr (B x) { return TY(x)==t_i8arr ? c(I8Arr, x) : (I8Arr*)  cpyI8Arr (x); }
static I16Arr* toI16Arr(B x) { return TY(x)==t_i16arr? c(I16Arr,x) : (I16Arr*) cpyI16Arr(x); }
static I32Arr* toI32Arr(B x) { return TY(x)==t_i32arr? c(I32Arr,x) : (I32Arr*) cpyI32Arr(x); }
static F64Arr* toF64Arr(B x) { return TY(x)==t_f64arr? c(F64Arr,x) : (F64Arr*) cpyF64Arr(x); }
static BitArr* toBitArr(B x) { return TY(x)==t_bitarr? c(BitArr,x) : (BitArr*) cpyBitArr(x); }

static B toI8Any (B x) { u8 t=TY(x); return t==t_i8arr  || t==t_i8slice ? x : taga(cpyI8Arr (x)); }
static B toI16Any(B x) { u8 t=TY(x); return t==t_i16arr || t==t_i16slice? x : taga(cpyI16Arr(x)); }
static B toI32Any(B x) { u8 t=TY(x); return t==t_i32arr || t==t_i32slice? x : taga(cpyI32Arr(x)); }
static B toF64Any(B x) { u8 t=TY(x); return t==t_f64arr || t==t_f64slice? x : taga(cpyF64Arr(x)); }


B m_cai32(usz ia, i32* a);
B m_caf64(usz sz, f64* a);

i64 bit_sum(u64* x, u64 am);
u64 usum(B x); // doesn't consume; error if not natural numbers or overflow
