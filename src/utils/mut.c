#include "../core.h"
#include "mut.h"

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
      case el_bit: { BitArr* t=cpyBitArr(taga(m->val)); m->val=(Arr*)t; m->abit=t->a; return; }
      case el_i8:  { I8Arr*  t=cpyI8Arr (taga(m->val)); m->val=(Arr*)t; m->ai8 =t->a; return; }
      case el_i16: { I16Arr* t=cpyI16Arr(taga(m->val)); m->val=(Arr*)t; m->ai16=t->a; return; }
      case el_i32: { I32Arr* t=cpyI32Arr(taga(m->val)); m->val=(Arr*)t; m->ai32=t->a; return; }
      case el_c8:  { C8Arr*  t=cpyC8Arr (taga(m->val)); m->val=(Arr*)t; m->ac8 =t->a; return; }
      case el_c16: { C16Arr* t=cpyC16Arr(taga(m->val)); m->val=(Arr*)t; m->ac16=t->a; return; }
      case el_c32: { C32Arr* t=cpyC32Arr(taga(m->val)); m->val=(Arr*)t; m->ac32=t->a; return; }
      case el_f64: { F64Arr* t=cpyF64Arr(taga(m->val)); m->val=(Arr*)t; m->af64=t->a; return; }
      case el_B  : { HArr*   t=cpyHArr  (taga(m->val)); m->val=(Arr*)t; m->aB  =t->a; return; }
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
