#include "../core.h"
#include "../utils/mut.h"

u8 elType2type[] = {
  [el_i8 ] = t_i8arr, [el_c8 ] = t_c8arr,
  [el_i16] = t_i16arr,[el_c16] = t_c16arr,
  [el_i32] = t_i32arr,[el_c32] = t_c32arr,
  [el_bit] = t_bitarr,[el_f64] = t_f64arr
};
u8 elTypeWidth[] = {
  [el_i8 ] = 1, [el_c8 ] = 1,
  [el_i16] = 2, [el_c16] = 2,
  [el_i32] = 4, [el_c32] = 4,
  [el_bit] = 0, [el_f64] = 8
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

NOINLINE B m_caf64(usz sz,  f64* a) { f64* rp; B r = m_f64arrv(&rp, sz); for (usz i = 0; i < sz; i++) rp[i] = a[i]; return r; }
NOINLINE B m_cai32(usz sz,  i32* a) { i32* rp; B r = m_i32arrv(&rp, sz); for (usz i = 0; i < sz; i++) rp[i] = a[i]; return r; }
NOINLINE B m_ascii(char* s, i64 sz) { u8*  rp; B r = m_c8arrv (&rp, sz); for (u64 i = 0; i < sz; i++) rp[i] = s[i]; return r; }
NOINLINE B m_ascii0(char* s) { return m_ascii(s, strlen(s)); }
NOINLINE B m_str32(u32* s) {
  usz sz = 0; while(s[sz]) sz++;
  u32* rp; B r = m_c32arrv(&rp, sz);
  for (usz i = 0; i < sz; i++) rp[i] = s[i];
  return r;
}

static Arr* bitarr_slice(B x, usz s, usz ia) {
  u64* rp; Arr* r = m_bitarrp(&rp, ia);
  bit_cpy(rp, 0, bitarr_ptr(x), s, ia);
  ptr_dec(v(x));
  return r;
}

static B bitarr_get(Arr* x, usz n) { assert(x->type==t_bitarr); return bitp_get((u64*)((BitArr*)x)->a, n)? m_f64(1) : m_f64(0); }
static bool bitarr_canStore(B x) { return q_bit(x); }

static void bitarr_init() {
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

void tyarr_init() {
  i8arr_init(); i16arr_init(); i32arr_init(); bitarr_init();
  c8arr_init(); c16arr_init(); c32arr_init(); f64arr_init();
  
  { u64* tmp; bi_emptyIVec = m_bitarrv(&tmp, 0); gc_add(bi_emptyIVec); }
  { u8*  tmp; bi_emptyCVec = m_c8arrv (&tmp, 0); gc_add(bi_emptyCVec); }
  
  Arr* emptySVec = arr_shVec(m_fillarrp(0));
  fillarr_setFill(emptySVec, emptyCVec());
  bi_emptySVec = taga(emptySVec); gc_add(bi_emptySVec);
}