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

B m_cai32(usz ia, i32* a);
B m_caf64(usz sz, f64* a);


I8Arr*  cpyI8Arr (B x); // consumes
I16Arr* cpyI16Arr(B x); // consumes
I32Arr* cpyI32Arr(B x); // consumes
F64Arr* cpyF64Arr(B x); // consumes

// all consume x
static I8Arr*  toI8Arr (B x) { return v(x)->type==t_i8arr ? c(I8Arr, x) : cpyI8Arr (x); }
static I16Arr* toI16Arr(B x) { return v(x)->type==t_i16arr? c(I16Arr,x) : cpyI16Arr(x); }
static I32Arr* toI32Arr(B x) { return v(x)->type==t_i32arr? c(I32Arr,x) : cpyI32Arr(x); }
static F64Arr* toF64Arr(B x) { return v(x)->type==t_f64arr? c(F64Arr,x) : cpyF64Arr(x); }

static B toI8Any (B x) { u8 t=v(x)->type; return t==t_i8arr  || t==t_i8slice ? x : taga(cpyI8Arr (x)); }
static B toI16Any(B x) { u8 t=v(x)->type; return t==t_i16arr || t==t_i16slice? x : taga(cpyI16Arr(x)); }
static B toI32Any(B x) { u8 t=v(x)->type; return t==t_i32arr || t==t_i32slice? x : taga(cpyI32Arr(x)); }
static B toF64Any(B x) { u8 t=v(x)->type; return t==t_f64arr || t==t_f64slice? x : taga(cpyF64Arr(x)); }


static i64 isum(B x) { // doesn't consume; may error
  assert(isArr(x));
  i64 r = 0;
  usz xia = a(x)->ia;
  u8 xe = TI(x,elType);
  if      (xe==el_i8 ) { i8*  p = i8any_ptr (x); for (usz i = 0; i < xia; i++) r+= p[i]; }
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
