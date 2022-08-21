#define c8 u8
#define c16 u16
#define c32 u32

#define TU C8
#define TP(W,X) W##c8##X
#include "tyarrTemplate.h"
#define TU C16
#define TP(W,X) W##c16##X
#include "tyarrTemplate.h"
#define TU C32
#define TP(W,X) W##c32##X
#include "tyarrTemplate.h"

#undef c8 // still not sure if i want these
#undef c16
#undef c32


B m_c32vec_0(u32* s);
B m_c32vec(u32* s, i64 sz);
B m_c8vec_0(char* s);
B m_c8vec(char* s, i64 sz);

B utf8Decode0(const char* x);
B utf8Decode(const char* x, i64 sz);
B utf8DecodeA(I8Arr* x);

C8Arr*  cpyC8Arr (B x); // consumes
C16Arr* cpyC16Arr(B x); // consumes
C32Arr* cpyC32Arr(B x); // consumes

// all consume x
static C8Arr*  toC8Arr (B x) { return TY(x)==t_c8arr ? c(C8Arr, x) : cpyC8Arr (x); }
static C16Arr* toC16Arr(B x) { return TY(x)==t_c16arr? c(C16Arr,x) : cpyC16Arr(x); }
static C32Arr* toC32Arr(B x) { return TY(x)==t_c32arr? c(C32Arr,x) : cpyC32Arr(x); }

static B toC8Any (B x) { u8 t=TY(x); return t==t_c8arr  || t==t_c8slice ? x : taga(cpyC8Arr (x)); }
static B toC16Any(B x) { u8 t=TY(x); return t==t_c16arr || t==t_c16slice? x : taga(cpyC16Arr(x)); }
static B toC32Any(B x) { u8 t=TY(x); return t==t_c32arr || t==t_c32slice? x : taga(cpyC32Arr(x)); }
