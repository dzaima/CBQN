#include "../core.h"
#include "mut.h"

NOINLINE void mut_to(Mut* m, u8 n) {
  u8 o = m->fns->elType;
  assert(o!=el_B);
  if (o==el_MAX) {
    mut_init(m, n);
  } else {
    m->fns = &mutFns[n];
    sprnk(m->val, 1);
    m->val->sh = &m->val->ia;
    #ifdef USE_VALGRIND
      VALGRIND_MAKE_MEM_DEFINED(m->val, mm_size((Value*)m->val)); // it's incomplete, but it's a typed array so garbage is acceptable
    #endif
    #ifdef DEBUG
      if (n==el_B && o==el_f64) { // hack to make toHArr calling f64arr_get not cry about possible sNaN floats
        usz ia = m->val->ia;
        f64* p = f64arr_ptr(taga(m->val));
        for (usz i = 0; i < ia; i++) if (!isF64(b(p[i]))) p[i] = 1.2217638442043777e161; // 0x6161616161616161
      }
    #endif
    switch(n) { default: UD;
      case el_bit: { BitArr* t=cpyBitArr(taga(m->val)); m->val=(Arr*)t; m->a = t->a; return; }
      case el_i8:  { I8Arr*  t=cpyI8Arr (taga(m->val)); m->val=(Arr*)t; m->a = t->a; return; }
      case el_i16: { I16Arr* t=cpyI16Arr(taga(m->val)); m->val=(Arr*)t; m->a = t->a; return; }
      case el_i32: { I32Arr* t=cpyI32Arr(taga(m->val)); m->val=(Arr*)t; m->a = t->a; return; }
      case el_c8:  { C8Arr*  t=cpyC8Arr (taga(m->val)); m->val=(Arr*)t; m->a = t->a; return; }
      case el_c16: { C16Arr* t=cpyC16Arr(taga(m->val)); m->val=(Arr*)t; m->a = t->a; return; }
      case el_c32: { C32Arr* t=cpyC32Arr(taga(m->val)); m->val=(Arr*)t; m->a = t->a; return; }
      case el_f64: { F64Arr* t=cpyF64Arr(taga(m->val)); m->val=(Arr*)t; m->a = t->a; return; }
      case el_B  : { HArr*   t=cpyHArr  (taga(m->val)); m->val=(Arr*)t; m->a = t->a; return; }
    }
  }
}

NOINLINE B vec_addF(B w, B x) {
  usz wia = IA(w);
  MAKE_MUT(r, wia+1); mut_init(r, el_or(TI(w,elType), selfElType(x)));
  MUTG_INIT(r);
  mut_copyG(r, 0, w, 0, wia);
  mut_setG(r, wia, x);
  dec(w);
  return mut_fv(r);
}
NOINLINE B vec_addN(B w, B x) {
  return vec_add(w, x);
}

NOINLINE void mut_pfree(Mut* m, usz n) { // free the first n elements
  if (m->fns->elType==el_B) harr_pfree(taga(m->val), n);
  else mm_free((Value*) m->val);
}


B vec_join(B w, B x) {
  bool unused;
  return arr_join_inline(w, x, true, &unused);
}



#define CHK(REQ,NEL,N,...) if (!(REQ)) mut_to(m, NEL); m->fns->m_##N##G(m, __VA_ARGS__);
#define CHK_S(REQ,X,N,...) CHK(REQ, el_or(m->fns->elType, selfElType(X)), N, __VA_ARGS__)


#define DEF0(RT, N, TY, INIT, CHK, EL, ARGS, ...) \
  static RT m_##N##G_##TY ARGS;            \
  static RT m_##N##_##TY ARGS { INIT       \
    Mut* m = a;                            \
    if (RARE(!(CHK))) {                    \
      mut_to(m, EL);                       \
      m->fns->m_##N##G(m->a, __VA_ARGS__); \
    } else {                               \
      m_##N##G_##TY(m->a, __VA_ARGS__);    \
    }                                      \
  }

#define DEF(RT, N, TY, INIT, CHK, EL, ARGS, ...) \
  DEF0(RT, N, TY, INIT, CHK, EL, ARGS, __VA_ARGS__) \
  static RT m_##N##G_##TY ARGS

#define DEF_S( RT, N, TY, CHK, X, ARGS, ...) DEF(RT, N, TY, , CHK, selfElType(X), ARGS, __VA_ARGS__)
#define DEF_E( RT, N, TY, CHK, X, ARGS, ...) DEF(RT, N, TY, , CHK, TI(X,elType),  ARGS, __VA_ARGS__)
#define DEF_G( RT, N, TY,         ARGS, ...)  \
  static RT m_##N##G_##TY ARGS;               \
  static RT m_##N##_##TY ARGS {               \
    m_##N##G_##TY(((Mut*)a)->a, __VA_ARGS__); \
  }                                           \
  static RT m_##N##G_##TY ARGS

DEF_S(void, set, MAX, false,    x, (void* a, usz ms, B x), ms, x) { err("m_setG_MAX"); }
DEF_S(void, set, bit, q_bit(x), x, (void* a, usz ms, B x), ms, x) { bitp_set((u64*)a, ms, o2bu(x)); }
DEF_S(void, set, i8,  q_i8 (x), x, (void* a, usz ms, B x), ms, x) { (( i8*)a)[ms] = o2iu(x); }
DEF_S(void, set, i16, q_i16(x), x, (void* a, usz ms, B x), ms, x) { ((i16*)a)[ms] = o2iu(x); }
DEF_S(void, set, i32, q_i32(x), x, (void* a, usz ms, B x), ms, x) { ((i32*)a)[ms] = o2iu(x); }
DEF_S(void, set, c8,  q_c8 (x), x, (void* a, usz ms, B x), ms, x) { (( u8*)a)[ms] = o2cu(x); }
DEF_S(void, set, c16, q_c16(x), x, (void* a, usz ms, B x), ms, x) { ((u16*)a)[ms] = o2cu(x); }
DEF_S(void, set, c32, q_c32(x), x, (void* a, usz ms, B x), ms, x) { ((u32*)a)[ms] = o2cu(x); }
DEF_S(void, set, f64, q_f64(x), x, (void* a, usz ms, B x), ms, x) { ((f64*)a)[ms] = o2fu(x); }
DEF_G(void, set, B,                (void* a, usz ms, B x), ms, x) { ((  B*)a)[ms] = x; }

DEF_S(void, fill, MAX, false,    x, (void* a, usz ms, B x, usz l), ms, x, l) { err("m_fillG_MAX"); }
DEF_S(void, fill, bit, q_bit(x), x, (void* a, usz ms, B x, usz l), ms, x, l) { u64* p =    (u64*)a;bool v = o2bu(x); for (usz i = 0; i < l; i++) bitp_set(p, ms+i, v); }
DEF_S(void, fill, i8 , q_i8 (x), x, (void* a, usz ms, B x, usz l), ms, x, l) { i8*  p = ms+( i8*)a; i8  v = o2iu(x); for (usz i = 0; i < l; i++) p[i] = v; }
DEF_S(void, fill, i16, q_i16(x), x, (void* a, usz ms, B x, usz l), ms, x, l) { i16* p = ms+(i16*)a; i16 v = o2iu(x); for (usz i = 0; i < l; i++) p[i] = v; }
DEF_S(void, fill, i32, q_i32(x), x, (void* a, usz ms, B x, usz l), ms, x, l) { i32* p = ms+(i32*)a; i32 v = o2iu(x); for (usz i = 0; i < l; i++) p[i] = v; }
DEF_S(void, fill, c8 , q_c8 (x), x, (void* a, usz ms, B x, usz l), ms, x, l) { u8*  p = ms+( u8*)a; u8  v = o2cu(x); for (usz i = 0; i < l; i++) p[i] = v; }
DEF_S(void, fill, c16, q_c16(x), x, (void* a, usz ms, B x, usz l), ms, x, l) { u16* p = ms+(u16*)a; u16 v = o2cu(x); for (usz i = 0; i < l; i++) p[i] = v; }
DEF_S(void, fill, c32, q_c32(x), x, (void* a, usz ms, B x, usz l), ms, x, l) { u32* p = ms+(u32*)a; u32 v = o2cu(x); for (usz i = 0; i < l; i++) p[i] = v; }
DEF_S(void, fill, f64, isF64(x), x, (void* a, usz ms, B x, usz l), ms, x, l) { f64* p = ms+(f64*)a; f64 v = o2fu(x); for (usz i = 0; i < l; i++) p[i] = v; }
DEF_G(void, fill, B  ,              (void* a, usz ms, B x, usz l), ms, x, l) {
  B* p = ms + (B*)a;
  for (usz i = 0; i < l; i++) p[i] = x;
  if (isVal(x)) incBy(x, l);
  return;
}


#if SINGELI
  #define DEF_COPY(TY, BODY) DEF0(void, copy, TY, u8 xe=TI(x,elType); u8 ne=el_or(xe,el_##TY);, ne==el_##TY, ne, (void* a, usz ms, B x, usz xs, usz l), ms, x, xs, l)
#else
  #define DEF_COPY(TY, BODY)  DEF(void, copy, TY, u8 xe=TI(x,elType); u8 ne=el_or(xe,el_##TY);, ne==el_##TY, ne, (void* a, usz ms, B x, usz xs, usz l), ms, x, xs, l) { u8 xt=v(x)->type; (void)xt; BODY }
#endif


DEF_COPY(bit, { bit_cpy((u64*)a, ms, bitarr_ptr(x), xs, l); return; })
DEF_COPY(i8 , { i8*  rp = ms+(i8*)a;
  switch (xt) { default: UD;
    case t_bitarr: { u64* xp = bitarr_ptr(x); for (usz i = 0; i < l; i++) rp[i] = bitp_get(xp, xs+i); return; }
    case t_i8arr: case t_i8slice:   { i8*  xp = i8any_ptr(x);  for (usz i = 0; i < l; i++) rp[i] = xp[i+xs]; return; }
  }
})
DEF_COPY(i16, { i16* rp = ms+(i16*)a;
  switch (xt) { default: UD;
    case t_bitarr: { u64* xp = bitarr_ptr(x); for (usz i = 0; i < l; i++) rp[i] = bitp_get(xp, xs+i); return; }
    case t_i8arr:  case t_i8slice:  { i8*  xp = i8any_ptr (x); for (usz i = 0; i < l; i++) rp[i] = xp[i+xs]; return; }
    case t_i16arr: case t_i16slice: { i16* xp = i16any_ptr(x); for (usz i = 0; i < l; i++) rp[i] = xp[i+xs]; return; }
  }
})
DEF_COPY(i32, { i32* rp = ms+(i32*)a;
  switch (xt) { default: UD;
    case t_bitarr: { u64* xp = bitarr_ptr(x); for (usz i = 0; i < l; i++) rp[i] = bitp_get(xp, xs+i); return; }
    case t_i8arr:  case t_i8slice:  { i8*  xp = i8any_ptr (x); for (usz i = 0; i < l; i++) rp[i] = xp[i+xs]; return; }
    case t_i16arr: case t_i16slice: { i16* xp = i16any_ptr(x); for (usz i = 0; i < l; i++) rp[i] = xp[i+xs]; return; }
    case t_i32arr: case t_i32slice: { i32* xp = i32any_ptr(x); for (usz i = 0; i < l; i++) rp[i] = xp[i+xs]; return; }
  }
})
DEF_COPY(f64, { f64* rp = ms+(f64*)a;
  switch (xt) { default: UD;
    case t_bitarr: { u64* xp = bitarr_ptr(x); for (usz i = 0; i < l; i++) rp[i] = bitp_get(xp, xs+i); return; }
    case t_i8arr:  case t_i8slice:  { i8*  xp = i8any_ptr (x); for (usz i = 0; i < l; i++) rp[i] = xp[i+xs]; return; }
    case t_i16arr: case t_i16slice: { i16* xp = i16any_ptr(x); for (usz i = 0; i < l; i++) rp[i] = xp[i+xs]; return; }
    case t_i32arr: case t_i32slice: { i32* xp = i32any_ptr(x); for (usz i = 0; i < l; i++) rp[i] = xp[i+xs]; return; }
    case t_f64arr: case t_f64slice: { f64* xp = f64any_ptr(x); for (usz i = 0; i < l; i++) rp[i] = xp[i+xs]; return; }
  }
})
DEF_COPY(c8 , { u8*  rp = ms+(u8*)a;   u8*  xp = c8any_ptr(x);  for (usz i = 0; i < l; i++) rp[i] = xp[i+xs]; return; })
DEF_COPY(c16, { u16* rp = ms+(u16*)a;
  switch (xt) { default: UD;
    case t_c8arr:  case t_c8slice:  { u8*  xp = c8any_ptr (x); for (usz i = 0; i < l; i++) rp[i] = xp[i+xs]; return; }
    case t_c16arr: case t_c16slice: { u16* xp = c16any_ptr(x); for (usz i = 0; i < l; i++) rp[i] = xp[i+xs]; return; }
  }
})
DEF_COPY(c32, { u32* rp = ms+(u32*)a;
  switch (xt) { default: UD;
    case t_c8arr:  case t_c8slice:  { u8*  xp = c8any_ptr (x); for (usz i = 0; i < l; i++) rp[i] = xp[i+xs]; return; }
    case t_c16arr: case t_c16slice: { u16* xp = c16any_ptr(x); for (usz i = 0; i < l; i++) rp[i] = xp[i+xs]; return; }
    case t_c32arr: case t_c32slice: { u32* xp = c32any_ptr(x); for (usz i = 0; i < l; i++) rp[i] = xp[i+xs]; return; }
  }
})
DEF_E(void, copy, MAX, false, x, (void* a, usz ms, B x, usz xs, usz l), ms, x, xs, l) { err("m_copyG_MAX"); }
NOINLINE void m_copyG_B_generic(void* a, B* mpo, B x, usz xs, usz l) {
  SLOW1("copyG", x);
  SGet(x)
  for (usz i = 0; i < l; i++) mpo[i] = Get(x,i+xs);
}
DEF_G(void, copy, B,             (void* a, usz ms, B x, usz xs, usz l), ms, x, xs, l) { 
  B* mpo = ms+(B*)a;
  switch(v(x)->type) {
    case t_bitarr: { u64* xp = bitarr_ptr(x); for (usz i = 0; i < l; i++) mpo[i] = m_i32(bitp_get(xp, xs+i)); return; }
    case t_i8arr:  case t_i8slice:  { i8*  xp = i8any_ptr (x); for (usz i = 0; i < l; i++) mpo[i] = m_i32(xp[i+xs]); return; }
    case t_i16arr: case t_i16slice: { i16* xp = i16any_ptr(x); for (usz i = 0; i < l; i++) mpo[i] = m_i32(xp[i+xs]); return; }
    case t_i32arr: case t_i32slice: { i32* xp = i32any_ptr(x); for (usz i = 0; i < l; i++) mpo[i] = m_i32(xp[i+xs]); return; }
    case t_c8arr:  case t_c8slice:  { u8*  xp = c8any_ptr (x); for (usz i = 0; i < l; i++) mpo[i] = m_c32(xp[i+xs]); return; }
    case t_c16arr: case t_c16slice: { u16* xp = c16any_ptr(x); for (usz i = 0; i < l; i++) mpo[i] = m_c32(xp[i+xs]); return; }
    case t_c32arr: case t_c32slice: { u32* xp = c32any_ptr(x); for (usz i = 0; i < l; i++) mpo[i] = m_c32(xp[i+xs]); return; }
    case t_harr: case t_hslice: case t_fillarr: case t_fillslice:;
      B* xp = arr_bptr(x)+xs;
      for (usz i = 0; i < l; i++) inc(xp[i]);
      memcpy(mpo, xp, l*sizeof(B));
      return;
    case t_f64arr: case t_f64slice:
      assert(sizeof(B)==sizeof(f64));
      memcpy(mpo, f64any_ptr(x)+xs, l*sizeof(B));
      return;
    default: {
      m_copyG_B_generic(a, mpo, x, xs, l);
      return;
    }
  }
}

#if SINGELI
  #include <xmmintrin.h>
  #if __GNUC__ && !__clang__ // yay gcc
    __m128i _mm_loadu_si32(void* p) {
      return (__m128i) _mm_load_ss(p);
    }
    void _mm_storeu_si32(void* p, __m128i x) {
      _mm_store_ss(p, _mm_castsi128_ps(x));
    }
  #endif
  
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wunused-variable"
  #include "../singeli/gen/copy.c"
  #pragma GCC diagnostic pop
  typedef void (*copy_fn)(u8*, u8*, u64, u8*);
  
  static void badCopy(u8* xp, u8* rp, u64 len, u8* xRaw) {
    err("Copying wrong array type");
  }
  
  #define COPY_FN(X,R) avx2_copy_##X##_##R
  #define MAKE_CPY(TY, MAKE, GET, WR, XRP, H2T, T, ...) \
  static copy_fn copy##T##Fns[10];                \
  NOINLINE void cpy##T##Arr_BF(u8* xp, u8* rp, u64 ia, Arr* xa) { \
    AS2B fn = TIv(xa,GET);                 \
    for (usz i=0; i<ia; i++) WR(fn(xa,i)); \
  } \
  static void cpy##T##Arr_B(u8* xp, u8* rp, u64 ia, u8* xRaw) { \
    Arr* xa = (Arr*)xRaw; B* bxp = arrV_bptr(xa); \
    if (bxp!=NULL && sizeof(B)==sizeof(f64)) {    \
      H2T;                                        \
    } else cpy##T##Arr_BF(xp, rp, ia, xa);        \
  }                                               \
  static copy_fn copy##T##Fns[] = __VA_ARGS__;    \
  T##Arr* cpy##T##Arr(B x) { \
    usz ia = IA(x);       \
    MAKE; arr_shCopy(r, x);  \
    if (ia>0) {              \
      copy##T##Fns[TI(x,elType)](tyany_ptr(x), (u8*)(XRP), ia, (u8*)a(x)); \
    }                        \
    if (TY) ptr_decT(a(x));  \
    else decG(x);            \
    return (T##Arr*)r;       \
  }
  #define BIT_PUT(V) bitp_set((u64*)rp, i, o2bu(V))
  #define H2T_COPY(T) copy##T##Fns[el_MAX]((u8*)bxp, rp, ia, xRaw)
  #define MAKE_TYCPY(T, E, F, ...) MAKE_CPY(1, T##Atom* rp; Arr* r = m_##E##arrp(&rp, ia), getU, ((T##Atom*)rp)[i] = F, rp, H2T_COPY(T), T, __VA_ARGS__)
  #define MAKE_CCPY(T,E) MAKE_TYCPY(T, E, o2cu, {badCopy,     badCopy,      badCopy,       badCopy,       badCopy,       COPY_FN(c8,E),COPY_FN(c16,E),COPY_FN(c32,E),cpy##T##Arr_B,COPY_FN(B,E)})
  #define MAKE_ICPY(T,E) MAKE_TYCPY(T, E, o2fu, {COPY_FN(1,E),COPY_FN(i8,E),COPY_FN(i16,E),COPY_FN(i32,E),COPY_FN(f64,E),badCopy,      badCopy,       badCopy,       cpy##T##Arr_B,COPY_FN(f64,E)})
  MAKE_CPY(0, HArr_p p = m_harrUp(ia); Arr* r = (Arr*)p.c, get, ((B*)rp)[i] =, p.a, for (usz i=0; i<ia; i++) ((B*)rp)[i] = inc(bxp[i]),
                                    H,          {COPY_FN(1,B),COPY_FN(i8,B),COPY_FN(i16,B),COPY_FN(i32,B),COPY_FN(f64,B),COPY_FN(c8,B),COPY_FN(c16,B),COPY_FN(c32,B),cpyHArr_B,    COPY_FN(f64,B)})
  MAKE_CPY(1, u64* rp; Arr* r = m_bitarrp(&rp, ia), getU, BIT_PUT, rp, H2T_COPY(Bit),
                                  Bit,          {COPY_FN(1,1),COPY_FN(i8,1),COPY_FN(i16,1),COPY_FN(i32,1),COPY_FN(f64,1),badCopy,      badCopy,       badCopy,       cpyBitArr_B,  COPY_FN(f64,1)})
  
  
  static copy_fn tcopy_i8Fns [] = {[t_bitarr]=avx2_copy_1_i8,  [t_i8arr]=avx2_copy_i8_i8 ,[t_i8slice]=avx2_copy_i8_i8};
  static copy_fn tcopy_i16Fns[] = {[t_bitarr]=avx2_copy_1_i16, [t_i8arr]=avx2_copy_i8_i16,[t_i8slice]=avx2_copy_i8_i16, [t_i16arr]=avx2_copy_i16_i16,[t_i16slice]=avx2_copy_i16_i16};
  static copy_fn tcopy_i32Fns[] = {[t_bitarr]=avx2_copy_1_i32, [t_i8arr]=avx2_copy_i8_i32,[t_i8slice]=avx2_copy_i8_i32, [t_i16arr]=avx2_copy_i16_i32,[t_i16slice]=avx2_copy_i16_i32, [t_i32arr]=avx2_copy_i32_i32,[t_i32slice]=avx2_copy_i32_i32};
  static copy_fn tcopy_f64Fns[] = {[t_bitarr]=avx2_copy_1_f64, [t_i8arr]=avx2_copy_i8_f64,[t_i8slice]=avx2_copy_i8_f64, [t_i16arr]=avx2_copy_i16_f64,[t_i16slice]=avx2_copy_i16_f64, [t_i32arr]=avx2_copy_i32_f64,[t_i32slice]=avx2_copy_i32_f64, [t_f64arr]=avx2_copy_f64_f64,[t_f64slice]=avx2_copy_f64_f64};
  static copy_fn tcopy_c8Fns [] = {[t_c8arr]=avx2_copy_c8_c8 ,[t_c8slice]=avx2_copy_c8_c8};
  static copy_fn tcopy_c16Fns[] = {[t_c8arr]=avx2_copy_c8_c16,[t_c8slice]=avx2_copy_c8_c16, [t_c16arr]=avx2_copy_c16_c16,[t_c16slice]=avx2_copy_c16_c16};
  static copy_fn tcopy_c32Fns[] = {[t_c8arr]=avx2_copy_c8_c32,[t_c8slice]=avx2_copy_c8_c32, [t_c16arr]=avx2_copy_c16_c32,[t_c16slice]=avx2_copy_c16_c32, [t_c32arr]=avx2_copy_c32_c32,[t_c32slice]=avx2_copy_c32_c32};
  
  #define TCOPY_FN(T, N) static void m_copyG_##N(void* a, usz ms, B x, usz xs, usz l) { \
    if (l==0) return; \
    u8* xp = tyany_ptr(x); \
    T* rp = ms + (T*)a; \
    u8 xt = v(x)->type; \
    if (xt==t_bitarr) { \
      for (usz i = 0; i < l; i++) rp[i] = bitp_get((u64*)xp, xs+i); \
    } else { \
      tcopy_##N##Fns[xt]((xs << arrTypeWidthLog(xt)) + (u8*)xp, (u8*)rp, l, (u8*)a(x)); \
    } \
  }
  TCOPY_FN(i8,i8)
  TCOPY_FN(i16,i16)
  TCOPY_FN(i32,i32)
  TCOPY_FN(u8,c8)
  TCOPY_FN(u16,c16)
  TCOPY_FN(u32,c32)
  TCOPY_FN(f64,f64)
  static void m_copyG_bit(void* a, usz ms, B x, usz xs, usz l) {
    bit_cpy((u64*)a, ms, bitarr_ptr(x), xs, l);
  }
#else
  #define MAKE_ICPY(T,E) T##Arr* cpy##T##Arr(B x) { \
    usz ia = IA(x);     \
    E* rp; Arr* r = m_##E##arrp(&rp, ia); \
    arr_shCopy(r, x);      \
    u8 xe = TI(x,elType);  \
    if      (xe==el_bit) { u64* xp = bitarr_ptr(x); for(usz i=0; i<ia; i++) rp[i]=bitp_get(xp,i); } \
    else if (xe==el_i8 ) { i8*  xp = i8any_ptr (x); for(usz i=0; i<ia; i++) rp[i]=xp[i]; } \
    else if (xe==el_i16) { i16* xp = i16any_ptr(x); for(usz i=0; i<ia; i++) rp[i]=xp[i]; } \
    else if (xe==el_i32) { i32* xp = i32any_ptr(x); for(usz i=0; i<ia; i++) rp[i]=xp[i]; } \
    else if (xe==el_f64) { f64* xp = f64any_ptr(x); for(usz i=0; i<ia; i++) rp[i]=xp[i]; } \
    else {                 \
      B* xp = arr_bptr(x); \
      if (xp!=NULL) { for (usz i=0; i<ia; i++) rp[i]=o2fu(xp[i]    ); } \
      else { SGetU(x) for (usz i=0; i<ia; i++) rp[i]=o2fu(GetU(x,i)); } \
    }                      \
    ptr_decT(a(x));        \
    return (T##Arr*)r;     \
  }

  #define MAKE_CCPY(T,E)     \
  T##Arr* cpy##T##Arr(B x) { \
    usz ia = IA(x);       \
    T##Atom* rp; Arr* r = m_##E##arrp(&rp, ia); \
    arr_shCopy(r, x);        \
    u8 xe = TI(x,elType);    \
    if      (xe==el_c8 ) { u8*  xp = c8any_ptr (x); for(usz i=0; i<ia; i++) rp[i]=xp[i]; } \
    else if (xe==el_c16) { u16* xp = c16any_ptr(x); for(usz i=0; i<ia; i++) rp[i]=xp[i]; } \
    else if (xe==el_c32) { u32* xp = c32any_ptr(x); for(usz i=0; i<ia; i++) rp[i]=xp[i]; } \
    else {                   \
      B* xp = arr_bptr(x);   \
      if (xp!=NULL) { for (usz i=0; i<ia; i++) rp[i]=o2cu(xp[i]    ); } \
      else { SGetU(x) for (usz i=0; i<ia; i++) rp[i]=o2cu(GetU(x,i)); } \
    }                        \
    ptr_decT(a(x));          \
    return (T##Arr*)r;       \
  }

  HArr* cpyHArr(B x) {
    usz ia = IA(x);
    HArr_p r = m_harrUc(x);
    u8 xe = TI(x,elType);
    if      (xe==el_bit) { u64* xp = bitarr_ptr(x); for(usz i=0; i<ia; i++) r.a[i]=m_f64(bitp_get(xp, i)); }
    else if (xe==el_i8 ) { i8*  xp = i8any_ptr (x); for(usz i=0; i<ia; i++) r.a[i]=m_f64(xp[i]); }
    else if (xe==el_i16) { i16* xp = i16any_ptr(x); for(usz i=0; i<ia; i++) r.a[i]=m_f64(xp[i]); }
    else if (xe==el_i32) { i32* xp = i32any_ptr(x); for(usz i=0; i<ia; i++) r.a[i]=m_f64(xp[i]); }
    else if (xe==el_f64) { f64* xp = f64any_ptr(x); for(usz i=0; i<ia; i++) r.a[i]=m_f64(xp[i]); }
    else if (xe==el_c8 ) { u8*  xp = c8any_ptr (x); for(usz i=0; i<ia; i++) r.a[i]=m_c32(xp[i]); }
    else if (xe==el_c16) { u16* xp = c16any_ptr(x); for(usz i=0; i<ia; i++) r.a[i]=m_c32(xp[i]); }
    else if (xe==el_c32) { u32* xp = c32any_ptr(x); for(usz i=0; i<ia; i++) r.a[i]=m_c32(xp[i]); }
    else {
      B* xp = arr_bptr(x);
      if (xp!=NULL) { for (usz i=0; i<ia; i++) r.a[i] = inc(xp[i]); }
      else { SGet(x)  for (usz i=0; i<ia; i++) r.a[i] = Get(x, i);  }
    }
    decG(x);
    return r.c;
  }
  BitArr* cpyBitArr(B x) {
    usz ia = IA(x);
    u64* rp; Arr* r = m_bitarrp(&rp, ia);
    arr_shCopy(r, x);
    u8 xe = TI(x,elType);
    if      (xe==el_bit) { u64* xp = bitarr_ptr(x); for(usz i=0; i<BIT_N(ia); i++) rp[i] = xp[i]; }
    else if (xe==el_i8 ) { i8*  xp = i8any_ptr (x); for(usz i=0; i<ia; i++) bitp_set(rp,i,xp[i]); }
    else if (xe==el_i16) { i16* xp = i16any_ptr(x); for(usz i=0; i<ia; i++) bitp_set(rp,i,xp[i]); }
    else if (xe==el_i32) { i32* xp = i32any_ptr(x); for(usz i=0; i<ia; i++) bitp_set(rp,i,xp[i]); }
    else if (xe==el_f64) { f64* xp = f64any_ptr(x); for(usz i=0; i<ia; i++) bitp_set(rp,i,xp[i]); }
    else {
      B* xp = arr_bptr(x);
      if (xp!=NULL) { for (usz i=0; i<ia; i++) bitp_set(rp,i,o2fu(xp[i]    )); }
      else { SGetU(x) for (usz i=0; i<ia; i++) bitp_set(rp,i,o2fu(GetU(x,i))); }
    }
    ptr_decT(a(x));
    return (BitArr*)r;
  }

#endif

MAKE_ICPY(I8, i8)
MAKE_ICPY(I16, i16)
MAKE_ICPY(I32, i32)
MAKE_ICPY(F64, f64)

MAKE_CCPY(C8, c8)
MAKE_CCPY(C16, c16)
MAKE_CCPY(C32, c32)
#undef BIT_PUT
#undef MAKE_CCPY
#undef MAKE_ICPY
#undef MAKE_CPY
#undef COPY_FN




static B m_getU_MAX(Mut* m, usz ms) { err("m_setG_MAX"); }
static B m_getU_bit(Mut* m, usz ms) { return m_i32(bitp_get(m->abit, ms)); }
static B m_getU_i8 (Mut* m, usz ms) { return m_i32(m->ai8 [ms]); }
static B m_getU_i16(Mut* m, usz ms) { return m_i32(m->ai16[ms]); }
static B m_getU_i32(Mut* m, usz ms) { return m_i32(m->ai32[ms]); }
static B m_getU_c8 (Mut* m, usz ms) { return m_c32(m->ac8 [ms]); }
static B m_getU_c16(Mut* m, usz ms) { return m_c32(m->ac16[ms]); }
static B m_getU_c32(Mut* m, usz ms) { return m_c32(m->ac32[ms]); }
static B m_getU_f64(Mut* m, usz ms) { return m_f64(m->af64[ms]); }
static B m_getU_B  (Mut* m, usz ms) { return m->aB[ms]; }

MutFns mutFns[el_MAX+1];
u8 el_orArr[el_MAX*16 + el_MAX+1];
void mutF_init() {
  for (u8 i = 0; i <= el_MAX; i++) {
    for (u8 j = 0; j <= el_MAX; j++) {
      u8 el;
      if (i==el_MAX|j==el_MAX) el = i>j?j:i;
      else if (elNum(i) && elNum(j)) el = i>j?i:j;
      else if (elChr(i) && elChr(j)) el = i>j?i:j;
      else el = el_B;
      el_orArr[i*16 + j] = el;
    }
  }
  #define FOR_TY(A) A(MAX) A(bit) A(i8) A(i16) A(i32) A(c8) A(c16) A(c32) A(f64) A(B)
  #define F(T,N) mutFns[el_##T].m_##N = m_##N##_##T;
  #define G(T) F(T,set) F(T,setG) F(T,getU) F(T,fill) F(T,fillG) F(T,copy) F(T,copyG)
  FOR_TY(G)
  #undef G
  #undef F
  mutFns[el_bit].elType = el_bit; mutFns[el_bit].valType = t_bitarr;
  mutFns[el_i8 ].elType = el_i8 ; mutFns[el_i8 ].valType = t_i8arr;
  mutFns[el_i16].elType = el_i16; mutFns[el_i16].valType = t_i16arr;
  mutFns[el_i32].elType = el_i32; mutFns[el_i32].valType = t_i32arr;
  mutFns[el_c8 ].elType = el_c8 ; mutFns[el_c8 ].valType = t_c8arr;
  mutFns[el_c16].elType = el_c16; mutFns[el_c16].valType = t_c16arr;
  mutFns[el_c32].elType = el_c32; mutFns[el_c32].valType = t_c32arr;
  mutFns[el_f64].elType = el_f64; mutFns[el_f64].valType = t_f64arr;
  mutFns[el_B  ].elType = el_B  ; mutFns[el_B  ].valType = t_harr;
  mutFns[el_MAX].elType = el_MAX; mutFns[el_MAX].valType = t_COUNT;
}
