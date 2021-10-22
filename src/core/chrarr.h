#define C8Atom u8
#define C16Atom u16
#define C32Atom u32

#define TU C8
#define TP(W,X) W##c8##X
#include "tyarrTemplate.h"
#define TU C16
#define TP(W,X) W##c16##X
#include "tyarrTemplate.h"
#define TU C32
#define TP(W,X) W##c32##X
#include "tyarrTemplate.h"
#define C8Arr TyArr
#define C16Arr TyArr
#define C32Arr TyArr


B m_str8(usz sz, char* s);
B m_str8l(char* s);
B m_str32(u32* s);
B fromUTF8l(char* x);
B fromUTF8a(I8Arr* x);

C8Arr*  cpyC8Arr (B x); // consumes
C16Arr* cpyC16Arr(B x); // consumes
C32Arr* cpyC32Arr(B x); // consumes

// all consume x
static C8Arr*  toC8Arr (B x) { return v(x)->type==t_c8arr ? c(C8Arr, x) : cpyC8Arr (x); }
static C16Arr* toC16Arr(B x) { return v(x)->type==t_c16arr? c(C16Arr,x) : cpyC16Arr(x); }
static C32Arr* toC32Arr(B x) { return v(x)->type==t_c32arr? c(C32Arr,x) : cpyC32Arr(x); }

static B toC8Any (B x) { u8 t=v(x)->type; return t==t_c8arr  || t==t_c8slice ? x : taga(cpyC8Arr (x)); }
static B toC16Any(B x) { u8 t=v(x)->type; return t==t_c16arr || t==t_c16slice? x : taga(cpyC16Arr(x)); }
static B toC32Any(B x) { u8 t=v(x)->type; return t==t_c32arr || t==t_c32slice? x : taga(cpyC32Arr(x)); }
