#include "../core.h"
#include "mut.h"

void mut_copyG(Mut* m, usz ms, B x, usz xs, usz l) { assert(isArr(x));
  // printf("mut_%d[%d…%d] ← %s[%d…%d]\n", m->type, ms, ms+l, type_repr(xt), xs, xs+l); fflush(stdout);
  u8 xt = v(x)->type;
  switch(m->type) { default: UD;
    case el_bit: bit_cpy(m->abit, ms, bitarr_ptr(x), xs, l); return;
    case el_i8:  { i8*  rp = m->ai8+ms;
      switch (xt) { default: UD;
        case t_bitarr: { u64* xp = bitarr_ptr(x); for (usz i = 0; i < l; i++) rp[i] = bitp_get(xp, xs+i); return; }
        case t_i8arr: case t_i8slice:   { i8*  xp = i8any_ptr(x);  for (usz i = 0; i < l; i++) rp[i] = xp[i+xs]; return; }
      }
    }
    case el_i16: { i16* rp = m->ai16+ms;
      switch (xt) { default: UD;
        case t_bitarr: { u64* xp = bitarr_ptr(x); for (usz i = 0; i < l; i++) rp[i] = bitp_get(xp, xs+i); return; }
        case t_i8arr:  case t_i8slice:  { i8*  xp = i8any_ptr (x); for (usz i = 0; i < l; i++) rp[i] = xp[i+xs]; return; }
        case t_i16arr: case t_i16slice: { i16* xp = i16any_ptr(x); for (usz i = 0; i < l; i++) rp[i] = xp[i+xs]; return; }
      }
    }
    case el_i32: { i32* rp = m->ai32+ms;
      switch (xt) { default: UD;
        case t_bitarr: { u64* xp = bitarr_ptr(x); for (usz i = 0; i < l; i++) rp[i] = bitp_get(xp, xs+i); return; }
        case t_i8arr:  case t_i8slice:  { i8*  xp = i8any_ptr (x); for (usz i = 0; i < l; i++) rp[i] = xp[i+xs]; return; }
        case t_i16arr: case t_i16slice: { i16* xp = i16any_ptr(x); for (usz i = 0; i < l; i++) rp[i] = xp[i+xs]; return; }
        case t_i32arr: case t_i32slice: { i32* xp = i32any_ptr(x); for (usz i = 0; i < l; i++) rp[i] = xp[i+xs]; return; }
      }
    }
    case el_f64: {
      f64* rp = m->af64+ms;
      switch (xt) { default: UD;
        case t_bitarr: { u64* xp = bitarr_ptr(x); for (usz i = 0; i < l; i++) rp[i] = bitp_get(xp, xs+i); return; }
        case t_i8arr:  case t_i8slice:  { i8*  xp = i8any_ptr (x); for (usz i = 0; i < l; i++) rp[i] = xp[i+xs]; return; }
        case t_i16arr: case t_i16slice: { i16* xp = i16any_ptr(x); for (usz i = 0; i < l; i++) rp[i] = xp[i+xs]; return; }
        case t_i32arr: case t_i32slice: { i32* xp = i32any_ptr(x); for (usz i = 0; i < l; i++) rp[i] = xp[i+xs]; return; }
        case t_f64arr: case t_f64slice: { f64* xp = f64any_ptr(x); for (usz i = 0; i < l; i++) rp[i] = xp[i+xs]; return; }
      }
    }
    case el_c8:  { u8*  rp = m->ac8+ms;   u8*  xp = c8any_ptr(x);  for (usz i = 0; i < l; i++) rp[i] = xp[i+xs]; return; }
    case el_c16: { u16* rp = m->ac16+ms;
      switch (xt) { default: UD;
        case t_c8arr:  case t_c8slice:  { u8*  xp = c8any_ptr (x); for (usz i = 0; i < l; i++) rp[i] = xp[i+xs]; return; }
        case t_c16arr: case t_c16slice: { u16* xp = c16any_ptr(x); for (usz i = 0; i < l; i++) rp[i] = xp[i+xs]; return; }
      }
    }
    case el_c32: { u32* rp = m->ac32+ms;
      switch (xt) { default: UD;
        case t_c8arr:  case t_c8slice:  { u8*  xp = c8any_ptr (x); for (usz i = 0; i < l; i++) rp[i] = xp[i+xs]; return; }
        case t_c16arr: case t_c16slice: { u16* xp = c16any_ptr(x); for (usz i = 0; i < l; i++) rp[i] = xp[i+xs]; return; }
        case t_c32arr: case t_c32slice: { u32* xp = c32any_ptr(x); for (usz i = 0; i < l; i++) rp[i] = xp[i+xs]; return; }
      }
    }
    case el_B: {
      B* mpo = m->aB+ms;
      B* xp = arr_bptr(x);
      if (xp!=NULL) {
        memcpy(mpo, xp+xs, l*sizeof(B));
        for (usz i = 0; i < l; i++) inc(mpo[i]);
        return;
      }
      SGet(x)
      for (usz i = 0; i < l; i++) mpo[i] = Get(x,i+xs);
      return;
    }
  }
}

NOINLINE void mut_to(Mut* m, u8 n) {
  u8 o = m->type;
  assert(o!=el_B);
  if (o==el_MAX) {
    mut_init(m, n);
  } else {
    m->type = n;
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
  if (m->type==el_B) harr_pfree(taga(m->val), n);
  else mm_free((Value*) m->val);
}

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
}


B vec_join(B w, B x) {
  return vec_join_inline(w, x);
}