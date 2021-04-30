#include "h.h"

typedef struct FillArr {
  struct Arr;
  B fill;
  B a[];
} FillArr;

B asFill(B x) { // consumes
  if (isArr(x)) {
    HArr_p r = m_harrUc(x);
    usz ia = r.c->ia;
    BS2B xget = TI(x).get;
    bool noFill = false;
    for (usz i = 0; i < ia; i++) if ((r.a[i]=asFill(xget(x,i))).u == bi_noFill.u) noFill = true;
    dec(x);
    if (noFill) { ptr_dec(r.c); return bi_noFill; }
    return r.b;
  }
  if (isF64(x)|isI32(x)) return m_i32(0);
  if (isC32(x)) return m_c32(' ');
  dec(x);
  return bi_noFill;
}
B getFill(B x) { // consumes; can return bi_noFill
  bool defZero = true;
  #ifdef CATCH_ERRORS
  defZero = false;
  #endif
  if (isArr(x)) {
    u8 t = v(x)->type;
    if (t==t_fillarr  ) { B r = inc(c(FillArr,x            )->fill); dec(x); return r; }
    if (t==t_fillslice) { B r = inc(c(FillArr,c(Slice,x)->p)->fill); dec(x); return r; }
    if (t==t_c32arr || t==t_c32slice) { dec(x); return m_c32(' '); }
    if (t==t_i32arr || t==t_i32slice) { dec(x); return m_f64(0  ); }
    if (t==t_f64arr || t==t_f64slice) { dec(x); return m_f64(0  ); }
    dec(x);
    return defZero? m_f64(0) : bi_noFill;
  }
  if (isF64(x)|isI32(x)) return m_i32(0);
  if (isC32(x)) return m_c32(' ');
  dec(x);
  return defZero? m_f64(0) : bi_noFill;
}
bool noFill(B x) { return x.u == bi_noFill.u; }

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

B* fillarr_ptr(B x) { VT(x,t_fillarr); return c(FillArr,x)->a; }
B fillarr_slice  (B x, usz s) {return m_fillslice(x                 , c(FillArr  ,x)->a+s); }
B fillslice_slice(B x, usz s) { B r = m_fillslice(inc(c(Slice,x)->p), c(FillSlice,x)->a+s); dec(x); return r; }


B fillarr_get   (B x, usz n) { VT(x,t_fillarr  ); return inc(c(FillArr  ,x)->a[n]); }
B fillslice_get (B x, usz n) { VT(x,t_fillslice); return inc(c(FillSlice,x)->a[n]); }
B fillarr_getU  (B x, usz n) { VT(x,t_fillarr  ); return     c(FillArr  ,x)->a[n] ; }
B fillslice_getU(B x, usz n) { VT(x,t_fillslice); return     c(FillSlice,x)->a[n] ; }
void fillarr_free(B x) {
  decSh(x);
  B* p = c(FillArr, x)->a;
  dec(c(FillArr, x)->fill);
  usz ia = a(x)->ia;
  for (usz i = 0; i < ia; i++) dec(p[i]);
}
void fillarr_visit(B x) {
  usz ia = a(x)->ia; B* p = fillarr_ptr(x);
  mm_visit(c(FillArr,x)->fill);
  for (usz i = 0; i < ia; i++) mm_visit(p[i]);
}
bool fillarr_canStore(B x) { return true; }

static inline void fillarr_init() {
  ti[t_fillarr].get   = fillarr_get;   ti[t_fillslice].get   = fillslice_get;
  ti[t_fillarr].getU  = fillarr_getU;  ti[t_fillslice].getU  = fillslice_getU;
  ti[t_fillarr].slice = fillarr_slice; ti[t_fillslice].slice = fillslice_slice;
  ti[t_fillarr].free  = fillarr_free;  ti[t_fillslice].free  =     slice_free;
  ti[t_fillarr].visit = fillarr_visit; ti[t_fillslice].visit =     slice_visit;
  ti[t_fillarr].print =     arr_print; ti[t_fillslice].print = arr_print;
  ti[t_fillarr].isArr = true;          ti[t_fillslice].isArr = true;
  ti[t_fillarr].canStore = fillarr_canStore;
}

B m_unit(B x) {
  B xf = asFill(inc(x));
  if (noFill(xf)) {
    HArr_p r = m_harrUp(1);
    arr_shAllocR(r.b, 0);
    r.a[0] = x;
    return r.b;
  }
  B r = m_arr(fsizeof(FillArr,a,B,1), t_fillarr);
  arr_shAllocI(r, 1, 0);
  c(FillArr,r)->fill = xf;
  c(FillArr,r)->a[0] = x;
  return r;
}

void validateFill(B x) {
  if (isArr(x)) {
    BS2B xgetU = TI(x).getU;
    usz ia = a(x)->ia;
    for (usz i = 0; i < ia; i++) validateFill(xgetU(x,i));
  } else if (isF64(x)) {
    assert(x.f==0);
  } else if (isC32(x)) {
    assert(' '==(u32)x.u);
  }
}

B fill_both(B w, B x) { // doesn't consume
  B fw = getFill(inc(w));
  if (noFill(fw)) return bi_noFill;
  B fx = getFill(inc(x));
  if (!equal(fw, fx)) {
    dec(fw); dec(fx);
    return bi_noFill;
  }
  dec(fw);
  return fx;
}

B withFill(B x, B fill) { // consumes both
  assert(isArr(x));
  #ifdef DEBUG
  validateFill(fill);
  #endif
  if (noFill(fill) && v(x)->type!=t_fillarr && v(x)->type!=t_fillslice) return x;
  switch(v(x)->type) {
    case t_f64arr : case t_f64slice:
    case t_i32arr : case t_i32slice: if(fill.u == m_i32(0  ).u) return x; break;
    case t_c32arr : case t_c32slice: if(fill.u == m_c32(' ').u) return x; break;
    case t_fillslice: if (equal(c(FillArr,c(Slice,x)->p)->fill, fill)) { dec(fill); return x; } break;
    case t_fillarr: if (equal(c(FillArr,x)->fill, fill)) { dec(fill); return x; }
      if (reusable(x)) {
        dec(c(FillArr, x)->fill);
        c(FillArr, x)->fill = fill;
        return x;
      }
      break;
  }
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
B qWithFill(B x, B fill) { // consumes both
  if (noFill(fill)) return x;
  return withFill(x, fill);
}
