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

NOINLINE B vec_addR(B w, B x) {
  usz wia = a(w)->ia;
  MAKE_MUT(r, wia+1); mut_init(r, el_or(TI(w,elType), selfElType(x)));
  mut_copyG(r, 0, w, 0, wia);
  mut_setG(r, wia, x);
  dec(w);
  return mut_fv(r);
}

NOINLINE void mut_pfree(Mut* m, usz n) { // free the first n elements
  if (m->fns->elType==el_B) harr_pfree(taga(m->val), n);
  else mm_free((Value*) m->val);
}


B vec_join(B w, B x) {
  return vec_join_inline(w, x);
}



#define CHK(REQ,NEL,N,...) if (!(REQ)) mut_to(m, NEL); m->fns->m_##N##G(m, __VA_ARGS__);
#define CHK_S(REQ,X,N,...) CHK(REQ, el_or(m->fns->elType, selfElType(X)), N, __VA_ARGS__)


#define DEF(RT, N, TY, INIT, CHK, EL, ARGS, ...) \
  static RT m_##N##G_##TY ARGS;         \
  static RT m_##N##_##TY ARGS { INIT    \
    if (RARE(!(CHK))) {                 \
      mut_to(m, EL);                    \
      m->fns->m_##N##G(m, __VA_ARGS__); \
    } else {                            \
      m_##N##G_##TY(m, __VA_ARGS__);    \
    }                                   \
  }                                     \
  static RT m_##N##G_##TY ARGS

#define DEF_S( RT, N, TY, CHK, X, ARGS, ...) DEF(RT, N, TY, , CHK, selfElType(X), ARGS, __VA_ARGS__)
#define DEF_E( RT, N, TY, CHK, X, ARGS, ...) DEF(RT, N, TY, , CHK, TI(X,elType),  ARGS, __VA_ARGS__)
#define DEF_G( RT, N, TY,         ARGS     ) \
  static RT m_##N##G_##TY ARGS; \
  static RT (*m_##N##_##TY) ARGS = m_##N##G_##TY; \
  static RT m_##N##G_##TY ARGS

DEF_S(void, set, MAX, false,    x, (Mut* m, usz ms, B x), ms, x) { err("m_setG_MAX"); }
DEF_S(void, set, bit, q_bit(x), x, (Mut* m, usz ms, B x), ms, x) { bitp_set(m->abit, ms, o2bu(x)); }
DEF_S(void, set, i8,  q_i8 (x), x, (Mut* m, usz ms, B x), ms, x) { m->ai8 [ms] = o2iu(x); }
DEF_S(void, set, i16, q_i16(x), x, (Mut* m, usz ms, B x), ms, x) { m->ai16[ms] = o2iu(x); }
DEF_S(void, set, i32, q_i32(x), x, (Mut* m, usz ms, B x), ms, x) { m->ai32[ms] = o2iu(x); }
DEF_S(void, set, c8,  q_c8 (x), x, (Mut* m, usz ms, B x), ms, x) { m->ac8 [ms] = o2cu(x); }
DEF_S(void, set, c16, q_c16(x), x, (Mut* m, usz ms, B x), ms, x) { m->ac16[ms] = o2cu(x); }
DEF_S(void, set, c32, q_c32(x), x, (Mut* m, usz ms, B x), ms, x) { m->ac32[ms] = o2cu(x); }
DEF_S(void, set, f64, q_f64(x), x, (Mut* m, usz ms, B x), ms, x) { m->af64[ms] = o2fu(x); }
DEF_G(void, set, B,                (Mut* m, usz ms, B x)       ) { m->aB[ms] = x; }

DEF_S(void, fill, MAX, false,    x, (Mut* m, usz ms, B x, usz l), ms, x, l) { err("m_fillG_MAX"); }
DEF_S(void, fill, bit, q_bit(x), x, (Mut* m, usz ms, B x, usz l), ms, x, l) { u64* p = m->abit;   bool v = o2bu(x); for (usz i = 0; i < l; i++) bitp_set(p, ms+i, v); }
DEF_S(void, fill, i8 , q_i8 (x), x, (Mut* m, usz ms, B x, usz l), ms, x, l) { i8*  p = m->ai8 +ms; i8  v = o2iu(x); for (usz i = 0; i < l; i++) p[i] = v; }
DEF_S(void, fill, i16, q_i16(x), x, (Mut* m, usz ms, B x, usz l), ms, x, l) { i16* p = m->ai16+ms; i16 v = o2iu(x); for (usz i = 0; i < l; i++) p[i] = v; }
DEF_S(void, fill, i32, q_i32(x), x, (Mut* m, usz ms, B x, usz l), ms, x, l) { i32* p = m->ai32+ms; i32 v = o2iu(x); for (usz i = 0; i < l; i++) p[i] = v; }
DEF_S(void, fill, c8 , q_c8 (x), x, (Mut* m, usz ms, B x, usz l), ms, x, l) { u8*  p = m->ac8 +ms; u8  v = o2cu(x); for (usz i = 0; i < l; i++) p[i] = v; }
DEF_S(void, fill, c16, q_c16(x), x, (Mut* m, usz ms, B x, usz l), ms, x, l) { u16* p = m->ac16+ms; u16 v = o2cu(x); for (usz i = 0; i < l; i++) p[i] = v; }
DEF_S(void, fill, c32, q_c32(x), x, (Mut* m, usz ms, B x, usz l), ms, x, l) { u32* p = m->ac32+ms; u32 v = o2cu(x); for (usz i = 0; i < l; i++) p[i] = v; }
DEF_S(void, fill, f64, isF64(x), x, (Mut* m, usz ms, B x, usz l), ms, x, l) { f64* p = m->af64+ms; f64 v = o2fu(x); for (usz i = 0; i < l; i++) p[i] = v; }
DEF_G(void, fill, B  ,              (Mut* m, usz ms, B x, usz l)          ) {
  B* p = m->aB+ms;
  for (usz i = 0; i < l; i++) p[i] = x;
  if (isVal(x)) for (usz i = 0; i < l; i++) inc(x);
  return;
}

#define DEF_COPY(TY, BODY) DEF(void, copy, TY, u8 xe=TI(x,elType); u8 ne=el_or(xe,el_##TY);, ne==el_##TY, ne, (Mut* m, usz ms, B x, usz xs, usz l), ms, x, xs, l) { u8 xt=v(x)->type; (void)xt; BODY }
DEF_COPY(bit, { bit_cpy(m->abit, ms, bitarr_ptr(x), xs, l); return; })
DEF_COPY(i8 , { i8*  rp = m->ai8+ms;
  switch (xt) { default: UD;
    case t_bitarr: { u64* xp = bitarr_ptr(x); for (usz i = 0; i < l; i++) rp[i] = bitp_get(xp, xs+i); return; }
    case t_i8arr: case t_i8slice:   { i8*  xp = i8any_ptr(x);  for (usz i = 0; i < l; i++) rp[i] = xp[i+xs]; return; }
  }
})
DEF_COPY(i16, { i16* rp = m->ai16+ms;
  switch (xt) { default: UD;
    case t_bitarr: { u64* xp = bitarr_ptr(x); for (usz i = 0; i < l; i++) rp[i] = bitp_get(xp, xs+i); return; }
    case t_i8arr:  case t_i8slice:  { i8*  xp = i8any_ptr (x); for (usz i = 0; i < l; i++) rp[i] = xp[i+xs]; return; }
    case t_i16arr: case t_i16slice: { i16* xp = i16any_ptr(x); for (usz i = 0; i < l; i++) rp[i] = xp[i+xs]; return; }
  }
})
DEF_COPY(i32, { i32* rp = m->ai32+ms;
  switch (xt) { default: UD;
    case t_bitarr: { u64* xp = bitarr_ptr(x); for (usz i = 0; i < l; i++) rp[i] = bitp_get(xp, xs+i); return; }
    case t_i8arr:  case t_i8slice:  { i8*  xp = i8any_ptr (x); for (usz i = 0; i < l; i++) rp[i] = xp[i+xs]; return; }
    case t_i16arr: case t_i16slice: { i16* xp = i16any_ptr(x); for (usz i = 0; i < l; i++) rp[i] = xp[i+xs]; return; }
    case t_i32arr: case t_i32slice: { i32* xp = i32any_ptr(x); for (usz i = 0; i < l; i++) rp[i] = xp[i+xs]; return; }
  }
})
DEF_COPY(f64, { f64* rp = m->af64+ms;
  switch (xt) { default: UD;
    case t_bitarr: { u64* xp = bitarr_ptr(x); for (usz i = 0; i < l; i++) rp[i] = bitp_get(xp, xs+i); return; }
    case t_i8arr:  case t_i8slice:  { i8*  xp = i8any_ptr (x); for (usz i = 0; i < l; i++) rp[i] = xp[i+xs]; return; }
    case t_i16arr: case t_i16slice: { i16* xp = i16any_ptr(x); for (usz i = 0; i < l; i++) rp[i] = xp[i+xs]; return; }
    case t_i32arr: case t_i32slice: { i32* xp = i32any_ptr(x); for (usz i = 0; i < l; i++) rp[i] = xp[i+xs]; return; }
    case t_f64arr: case t_f64slice: { f64* xp = f64any_ptr(x); for (usz i = 0; i < l; i++) rp[i] = xp[i+xs]; return; }
  }
})
DEF_COPY(c8 , { u8*  rp = m->ac8+ms;   u8*  xp = c8any_ptr(x);  for (usz i = 0; i < l; i++) rp[i] = xp[i+xs]; return; })
DEF_COPY(c16, { u16* rp = m->ac16+ms;
  switch (xt) { default: UD;
    case t_c8arr:  case t_c8slice:  { u8*  xp = c8any_ptr (x); for (usz i = 0; i < l; i++) rp[i] = xp[i+xs]; return; }
    case t_c16arr: case t_c16slice: { u16* xp = c16any_ptr(x); for (usz i = 0; i < l; i++) rp[i] = xp[i+xs]; return; }
  }
})
DEF_COPY(c32, { u32* rp = m->ac32+ms;
  switch (xt) { default: UD;
    case t_c8arr:  case t_c8slice:  { u8*  xp = c8any_ptr (x); for (usz i = 0; i < l; i++) rp[i] = xp[i+xs]; return; }
    case t_c16arr: case t_c16slice: { u16* xp = c16any_ptr(x); for (usz i = 0; i < l; i++) rp[i] = xp[i+xs]; return; }
    case t_c32arr: case t_c32slice: { u32* xp = c32any_ptr(x); for (usz i = 0; i < l; i++) rp[i] = xp[i+xs]; return; }
  }
})
DEF_E(void, copy, MAX, false, x, (Mut* m, usz ms, B x, usz xs, usz l), ms, x, xs, l) { err("m_copyG_MAX"); }
DEF_G(void, copy, B,             (Mut* m, usz ms, B x, usz xs, usz l)) { 
  B* mpo = m->aB+ms;
  switch(v(x)->type) {
    case t_harr: case t_hslice: case t_fillarr: case t_fillslice:
      memcpy(mpo, arr_bptr(x)+xs, l*sizeof(B));
      for (usz i = 0; i < l; i++) inc(mpo[i]);
      return;
    case t_f64arr: case t_f64slice:
      memcpy(mpo, f64any_ptr(x)+xs, l*sizeof(B));
      return;
    default:
      SGet(x)
      for (usz i = 0; i < l; i++) mpo[i] = Get(x,i+xs);
      return;
  }
}

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
