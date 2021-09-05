#include "../core.h"

B m_i8(i8 x) { return m_i32(x); } B m_i16(i16 x) { return m_i32(x); }
B m_c8(i8 x) { return m_c32(x); } B m_c16(i16 x) { return m_c32(x); }
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

NOINLINE B m_caf64(usz sz, f64* a) { f64* rp; B r = m_f64arrv(&rp, sz); for (usz i = 0; i < sz; i++) rp[i] =      a[i]; return r; }
NOINLINE B m_cai32(usz sz, i32* a) { i32* rp; B r = m_i32arrv(&rp, sz); for (usz i = 0; i < sz; i++) rp[i] =      a[i]; return r; }
NOINLINE B m_str8(usz sz, char* s) { u32* rp; B r = m_c32arrv(&rp, sz); for (u64 i = 0; i < sz; i++) rp[i] = (u32)s[i]; return r; }
NOINLINE B m_str8l(char* s) { return m_str8(strlen(s), s); }
NOINLINE B m_str32(u32* s) {
  usz sz = 0; while(s[sz]) sz++;
  u32* rp; B r = m_c32arrv(&rp, sz);
  for (usz i = 0; i < sz; i++) rp[i] = s[i];
  return r;
}

void tyarr_init() {
  i8arr_init(); i16arr_init(); i32arr_init();
  c8arr_init(); c16arr_init(); c32arr_init(); f64arr_init();
  
  { i32* tmp; bi_emptyIVec = m_i32arrv(&tmp, 0); gc_add(bi_emptyIVec); }
  { u32* tmp; bi_emptyCVec = m_c32arrv(&tmp, 0); gc_add(bi_emptyCVec); }
  
  Arr* emptySVec = m_fillarrp(0); arr_shVec(emptySVec);
  fillarr_setFill(emptySVec, emptyCVec());
  bi_emptySVec = taga(emptySVec); gc_add(bi_emptySVec);
}