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
      case el_i32: { I32Arr* t=toI32Arr(taga(m->val)); m->val=(Arr*)t; m->ai32=t->a; return; }
      case el_f64: { F64Arr* t=toF64Arr(taga(m->val)); m->val=(Arr*)t; m->af64=t->a; return; }
      case el_c32: { C32Arr* t=toC32Arr(taga(m->val)); m->val=(Arr*)t; m->ac32=t->a; return; }
      case el_B  : { HArr*   t=toHArr  (taga(m->val)); m->val=(Arr*)t; m->aB  =t->a; return; }
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
