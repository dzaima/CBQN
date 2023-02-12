#include "../core.h"
#include "../utils/mut.h"

u8 elType2type[] = {
  [el_i8 ] = t_i8arr, [el_c8 ] = t_c8arr,
  [el_i16] = t_i16arr,[el_c16] = t_c16arr,
  [el_i32] = t_i32arr,[el_c32] = t_c32arr,
  [el_bit] = t_bitarr,[el_f64] = t_f64arr,
  [el_B] = t_invalid, [el_MAX] = t_invalid
};
u8 elTypeWidth[] = {
  [el_i8 ] = 1, [el_c8 ] = 1,
  [el_i16] = 2, [el_c16] = 2,
  [el_i32] = 4, [el_c32] = 4,
  [el_bit] = 0, [el_f64] = 8,
  [el_B] = 8
};
u8 elTypeWidthLogBits[] = {
  [el_i8 ] = 3, [el_c8 ] = 3,
  [el_i16] = 4, [el_c16] = 4,
  [el_i32] = 5, [el_c32] = 5,
  [el_bit] = 0, [el_f64] = 6,
  [el_B] = 6
};
u8 arrTypeWidthLog[] = {
  [t_bitarr]=99,
  [t_i8arr ]=0, [t_i8slice ]=0, [t_c8arr ]=0, [t_c8slice ]=0,
  [t_i16arr]=1, [t_i16slice]=1, [t_c16arr]=1, [t_c16slice]=1,
  [t_i32arr]=2, [t_i32slice]=2, [t_c32arr]=2, [t_c32slice]=2,
  [t_f64arr]=3, [t_f64slice]=3
};
u8 arrTypeBitsLog[] = {
  [t_bitarr]=0,
  [t_i8arr ]=3, [t_i8slice ]=3, [t_c8arr ]=3, [t_c8slice ]=3,
  [t_i16arr]=4, [t_i16slice]=4, [t_c16arr]=4, [t_c16slice]=4,
  [t_i32arr]=5, [t_i32slice]=5, [t_c32arr]=5, [t_c32slice]=5,
  [t_f64arr]=6, [t_f64slice]=6,
  [t_harr  ]=6, [t_hslice  ]=6, [t_fillarr]=6,[t_fillslice]=6
};

B m_i8(i8 x) { return m_i32(x); } B m_i16(i16 x) { return m_i32(x); }
B m_c8(u8 x) { return m_c32(x); } B m_c16(u16 x) { return m_c32(x); }
#define TU I8
#define TP(W,X) W##i8##X
#include "tyarrTemplate.c"
#define TU I16
#define TP(W,X) W##i16##X
#include "tyarrTemplate.c"
#define TU I32
#define TP(W,X) W##i32##X
#include "tyarrTemplate.c"
#define TU C8
#define TP(W,X) W##c8##X
#include "tyarrTemplate.c"
#define TU C16
#define TP(W,X) W##c16##X
#include "tyarrTemplate.c"
#define TU C32
#define TP(W,X) W##c32##X
#include "tyarrTemplate.c"
#define TU F64
#define TP(W,X) W##f64##X
#include "tyarrTemplate.c"

NOINLINE B m_caf64(usz sz,  f64* a) { f64* rp; B r = m_f64arrv(&rp, sz); memcpy(rp, a, sz*sizeof( f64)); return r; }
NOINLINE B m_cai32(usz sz,  i32* a) { i32* rp; B r = m_i32arrv(&rp, sz); memcpy(rp, a, sz*sizeof( i32)); return r; }
NOINLINE B m_c8vec(char* a, i64 sz) { u8*  rp; B r = m_c8arrv (&rp, sz); memcpy(rp, a, sz*sizeof(char)); return r; }
NOINLINE B m_c32vec(u32* s, i64 sz) { u32* rp; B r = m_c32arrv(&rp, sz); memcpy(rp, s, sz*sizeof( u32)); return r; }
NOINLINE B m_c8vec_0(char* s) { return m_c8vec(s, strlen(s)); }
NOINLINE B m_c32vec_0(u32* s) { usz sz=0; while(s[sz]) sz++; return m_c32vec(s, sz); }


static Arr* bitarr_slice(B x, usz s, usz ia) {
  u64* rp; Arr* r = m_bitarrp(&rp, ia);
  bit_cpy(rp, 0, bitarr_ptr(x), s, ia);
  ptr_dec(v(x));
  return r;
}

static B bitarr_get(Arr* x, usz n) { assert(PTY(x)==t_bitarr); return bitp_get((u64*)((BitArr*)x)->a, n)? m_f64(1) : m_f64(0); }
static bool bitarr_canStore(B x) { return q_bit(x); }

static void bitarr_init(void) {
  TIi(t_bitarr,get)   = bitarr_get;
  TIi(t_bitarr,getU)  = bitarr_get;
  TIi(t_bitarr,slice) = bitarr_slice;
  TIi(t_bitarr,freeO) =  tyarr_freeO;
  TIi(t_bitarr,freeF) =  tyarr_freeF;
  TIi(t_bitarr,visit) =   noop_visit;
  TIi(t_bitarr,print) =   farr_print;
  TIi(t_bitarr,isArr) = true;
  TIi(t_bitarr,arrD1) = true;
  TIi(t_bitarr,elType) = el_bit;
  TIi(t_bitarr,canStore) = bitarr_canStore;
}

void tyarr_init(void) {
  i8arr_init(); i16arr_init(); i32arr_init(); bitarr_init();
  c8arr_init(); c16arr_init(); c32arr_init(); f64arr_init();
  
  { u64* tmp; bi_emptyIVec = m_bitarrv(&tmp, 0); gc_add(bi_emptyIVec); }
  { u8*  tmp; bi_emptyCVec = m_c8arrv (&tmp, 0); gc_add(bi_emptyCVec); }
  
  Arr* emptySVec = arr_shVec(m_fillarrp(0));
  fillarr_setFill(emptySVec, emptyCVec());
  bi_emptySVec = taga(emptySVec); gc_add(bi_emptySVec);
}
