#define I8Atom i8
#define I16Atom i16
#define I32Atom i32
#define F64Atom f64

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
static inline u64 bitx(B x) { // repeats the boolean across all 64 bits
  return o2bu(x)? ~(u64)0 : 0;
}

// BitArr
typedef struct BitArr {
  struct Arr;
  u64 a[];
} BitArr;
#define BITARR_SZ(IA) fsizeof(BitArr, a, u64, BIT_N(IA))
static B m_bitarrv(u64** p, usz ia) {
  BitArr* r = m_arr(BITARR_SZ(ia), t_bitarr, ia);
  arr_shVec((Arr*)r);
  *p = r->a;
  return taga(r);
}
static B m_bitarrc(u64** p, B x) { assert(isArr(x));
  BitArr* r = m_arr(BITARR_SZ(a(x)->ia), t_bitarr, a(x)->ia);
  *p = r->a;
  arr_shCopy((Arr*)r, x);
  return taga(r);
}
static Arr* m_bitarrp(u64** p, usz ia) {
  BitArr* r = m_arr(BITARR_SZ(ia), t_bitarr, ia);
  *p = r->a;
  return (Arr*)r;
}
static u64* bitarr_ptr(B x) { VTY(x, t_bitarr); return c(BitArr,x)->a; }


I8Arr*  cpyI8Arr (B x); // consumes
I16Arr* cpyI16Arr(B x); // consumes
I32Arr* cpyI32Arr(B x); // consumes
F64Arr* cpyF64Arr(B x); // consumes
BitArr* cpyBitArr(B x); // consumes

// all consume x
static I8Arr*  toI8Arr (B x) { return v(x)->type==t_i8arr ? c(I8Arr, x) : cpyI8Arr (x); }
static I16Arr* toI16Arr(B x) { return v(x)->type==t_i16arr? c(I16Arr,x) : cpyI16Arr(x); }
static I32Arr* toI32Arr(B x) { return v(x)->type==t_i32arr? c(I32Arr,x) : cpyI32Arr(x); }
static F64Arr* toF64Arr(B x) { return v(x)->type==t_f64arr? c(F64Arr,x) : cpyF64Arr(x); }
static BitArr* toBitArr(B x) { return v(x)->type==t_bitarr? c(BitArr,x) : cpyBitArr(x); }

static B toI8Any (B x) { u8 t=v(x)->type; return t==t_i8arr  || t==t_i8slice ? x : taga(cpyI8Arr (x)); }
static B toI16Any(B x) { u8 t=v(x)->type; return t==t_i16arr || t==t_i16slice? x : taga(cpyI16Arr(x)); }
static B toI32Any(B x) { u8 t=v(x)->type; return t==t_i32arr || t==t_i32slice? x : taga(cpyI32Arr(x)); }
static B toF64Any(B x) { u8 t=v(x)->type; return t==t_f64arr || t==t_f64slice? x : taga(cpyF64Arr(x)); }


B m_cai32(usz ia, i32* a);
B m_caf64(usz sz, f64* a);

static i64 bit_sum(u64* x, u64 am) {
  i64 r = 0;
  for (u64 i = 0; i < (am>>6); i++) r+= POPC(x[i]);
  if (am&63) r+= POPC(x[am>>6]<<(64-am & 63));
  return r;
}

static i64 isum(B x) { // doesn't consume; may error
  assert(isArr(x));
  i64 r = 0;
  usz xia = a(x)->ia;
  u8 xe = TI(x,elType);
  if      (xe==el_bit) return bit_sum(bitarr_ptr(x), xia);
  else if (xe==el_i8 ) { i8*  p = i8any_ptr (x); for (usz i = 0; i < xia; i++) r+= p[i]; }
  else if (xe==el_i16) { i16* p = i16any_ptr(x); for (usz i = 0; i < xia; i++) if (addOn(r,p[i])) goto err; }
  else if (xe==el_i32) { i32* p = i32any_ptr(x); for (usz i = 0; i < xia; i++) if (addOn(r,p[i])) goto err; }
  else if (xe==el_f64) {
    f64* p = f64any_ptr(x);
    for (usz i = 0; i < xia; i++) { if(p[i]!=(i64)p[i] || addOn(r,(i64)p[i])) goto err; }
  } else {
    SGetU(x)
    for (usz i = 0; i < xia; i++) r+= o2i64(GetU(x,i));
  }
  return r;
  err: thrM("Expected integer");
}
