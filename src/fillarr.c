#include "h.h"

typedef struct FillArr {
  struct Arr;
  B fill;
  B a[];
} FillArr;

B asFill(B x) {
  if (isArr(x)) {
    HArr_p r = m_harrc(x);
    usz ia = r.c->ia;
    BS2B xget = TI(x).get;
    bool noFill = false;
    for (usz i = 0; i < ia; i++) if ((r.a[i]=asFill(xget(x,i))).u == bi_noFill.u) noFill = true;
    dec(x);
    if (noFill) { dec(r.b); return bi_noFill; }
    return r.b;
  }
  if (isF64(x)|isI32(x)) return m_i32(0);
  if (isC32(x)) return m_c32(' ');
  return bi_noFill;
}
B withFill(B x, B fill) {
  B r = m_arr(fsizeof(FillArr,a,B,a(x)->ia), t_fillarr);
  arr_shCopy(r, x);
  c(FillArr,r)->fill = fill;
  B* a = c(FillArr,r)->a;
  usz ia = a(x)->ia;
  BS2B xget = TI(x).get;
  for (usz i = 0; i < ia; i++) a[i] = xget(x,i);
  dec(x);
  return r;
}
B getFill(B x) {
  if (isArr(x)) {
    u8 t = v(x)->type;
    if (t==t_fillarr  ) { B r = inci(c(FillArr,x            )->fill); dec(x); return r; }
    if (t==t_fillslice) { B r = inci(c(FillArr,c(Slice,x)->p)->fill); dec(x); return r; }
    if (t==t_c32arr || t==t_c32slice) return m_c32(' ');
  }
  if (isC32(x)) return m_c32(' ');
  return m_f64(0);
}

typedef struct FillSlice {
  struct Slice;
  B* a;
} FillSlice;
B m_fillslice(B p, B* ptr) {
  FillSlice* r = mm_allocN(sizeof(FillSlice), t_fillslice);
  r->p = p;
  r->a = ptr;
  return tag(r, ARR_TAG);
}

B fillarr_slice  (B x, usz s) {return m_fillslice(x                      , c(FillArr  ,x)->a+s); }
B fillslice_slice(B x, usz s) { B r = m_fillslice(inci(c(FillSlice,x)->p), c(FillSlice,x)->a+s); dec(x); return r; }


B fillarr_get  (B x, usz n) { VT(x,t_fillarr  ); return inci(c(FillArr  ,x)->a[n]); }
B fillslice_get(B x, usz n) { VT(x,t_fillslice); return inci(c(FillSlice,x)->a[n]); }
void fillarr_free(B x) {
  decSh(x);
  B* p = c(FillArr, x)->a;
  usz ia = a(x)->ia;
  for (usz i = 0; i < ia; i++) dec(p[i]);
}

void fillarr_init() {
  ti[t_fillarr].get   = fillarr_get;   ti[t_fillslice].get   = fillslice_get;
  ti[t_fillarr].slice = fillarr_slice; ti[t_fillslice].slice = fillslice_slice;
  ti[t_fillarr].free  = fillarr_free;  ti[t_fillslice].free  =     slice_free;
  ti[t_fillarr].print =     arr_print; ti[t_fillslice].print = arr_print;
}
