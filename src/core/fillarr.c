#include "../core.h"

B asFill(B x) { // consumes
  if (isArr(x)) {
    u8 xe = TI(x,elType);
    usz ia = a(x)->ia;
    if (elNum(xe)) {
      u64* rp; B r = m_bitarrc(&rp, x);
      for (usz i = 0; i < BIT_N(ia); i++) rp[i] = 0;
      dec(x);
      return r;
    }
    if (elChr(xe)) {
      u8* rp; B r = m_c8arrc(&rp, x);
      for (usz i = 0; i < ia; i++) rp[i] = ' ';
      dec(x);
      return r;
    }
    HArr_p r = m_harrUc(x);
    SGet(x)
    bool noFill = false;
    for (usz i = 0; i < ia; i++) if ((r.a[i]=asFill(Get(x,i))).u == bi_noFill.u) noFill = true;
    B xf = getFillQ(x);
    dec(x);
    if (noFill) { ptr_dec(r.c); return bi_noFill; }
    return withFill(r.b, xf);
  }
  if (isF64(x)) return m_i32(0);
  if (isC32(x)) return m_c32(' ');
  dec(x);
  return bi_noFill;
}

static Arr* m_fillslice(Arr* p, B* ptr, usz ia) {
  FillSlice* r = m_arr(sizeof(FillSlice), t_fillslice, ia);
  r->p = p;
  r->a = ptr;
  return (Arr*)r;
}
static Arr* fillarr_slice  (B x, usz s, usz ia) { return m_fillslice(a(x), c(FillArr,x)->a+s, ia); }
static Arr* fillslice_slice(B x, usz s, usz ia) { Arr* p = ptr_inc(c(Slice,x)->p); Arr* r = m_fillslice(p, c(FillSlice,x)->a+s, ia); dec(x); return r; }

static B fillarr_get   (Arr* x, usz n) { assert(x->type==t_fillarr  ); return inc(((FillArr*  )x)->a[n]); }
static B fillslice_get (Arr* x, usz n) { assert(x->type==t_fillslice); return inc(((FillSlice*)x)->a[n]); }
static B fillarr_getU  (Arr* x, usz n) { assert(x->type==t_fillarr  ); return     ((FillArr*  )x)->a[n] ; }
static B fillslice_getU(Arr* x, usz n) { assert(x->type==t_fillslice); return     ((FillSlice*)x)->a[n] ; }
DEF_FREE(fillarr) {
  decSh(x);
  B* p = ((FillArr*)x)->a;
  dec(((FillArr*)x)->fill);
  usz ia = ((Arr*)x)->ia;
  for (usz i = 0; i < ia; i++) dec(p[i]);
}
static void fillarr_visit(Value* x) { assert(x->type == t_fillarr);
  usz ia = ((Arr*)x)->ia; B* p = ((FillArr*)x)->a;
  mm_visit(((FillArr*)x)->fill);
  for (usz i = 0; i < ia; i++) mm_visit(p[i]);
}
static bool fillarr_canStore(B x) { return true; }

void fillarr_init() {
  TIi(t_fillarr,get)   = fillarr_get;   TIi(t_fillslice,get)   = fillslice_get;
  TIi(t_fillarr,getU)  = fillarr_getU;  TIi(t_fillslice,getU)  = fillslice_getU;
  TIi(t_fillarr,slice) = fillarr_slice; TIi(t_fillslice,slice) = fillslice_slice;
  TIi(t_fillarr,freeO) = fillarr_freeO; TIi(t_fillslice,freeO) =     slice_freeO;
  TIi(t_fillarr,freeF) = fillarr_freeF; TIi(t_fillslice,freeF) =     slice_freeF;
  TIi(t_fillarr,visit) = fillarr_visit; TIi(t_fillslice,visit) =     slice_visit;
  TIi(t_fillarr,print) =     arr_print; TIi(t_fillslice,print) = arr_print;
  TIi(t_fillarr,isArr) = true;          TIi(t_fillslice,isArr) = true;
  TIi(t_fillarr,canStore) = fillarr_canStore;
}



void validateFill(B x) {
  if (isArr(x)) {
    SGetU(x)
    usz ia = a(x)->ia;
    for (usz i = 0; i < ia; i++) validateFill(GetU(x,i));
  } else if (isF64(x)) {
    assert(x.f==0);
  } else if (isC32(x)) {
    assert(' '==(u32)x.u);
  }
}

NOINLINE bool fillEqualR(B w, B x) { // doesn't consume; both args must be arrays
  if (!eqShape(w, x)) return false;
  usz ia = a(w)->ia;
  if (ia==0) return true;
  
  u8 we = TI(w,elType);
  u8 xe = TI(x,elType);
  if (we!=el_B && xe!=el_B) {
    return elChr(we) == elChr(xe);
  }
  SGetU(x)
  SGetU(w)
  for (usz i = 0; i < ia; i++) if(!fillEqual(GetU(w,i),GetU(x,i))) return false;
  return true;
}



B withFill(B x, B fill) { // consumes both
  assert(isArr(x));
  #ifdef DEBUG
  validateFill(fill);
  #endif
  u8 xt = v(x)->type;
  if (noFill(fill) && xt!=t_fillarr && xt!=t_fillslice) return x;
  switch(xt) {
    case t_f64arr: case t_f64slice:
    case t_i32arr: case t_i32slice: case t_i16arr: case t_i16slice: case t_i8arr: case t_i8slice: if(fill.u == m_i32(0  ).u) return x; break;
    case t_c32arr: case t_c32slice: case t_c16arr: case t_c16slice: case t_c8arr: case t_c8slice: if(fill.u == m_c32(' ').u) return x; break;
    case t_fillslice: if (fillEqual(((FillArr*)c(Slice,x)->p)->fill, fill)) { dec(fill); return x; } break;
    case t_fillarr: if (fillEqual(c(FillArr,x)->fill, fill)) { dec(fill); return x; }
      if (reusable(x)) { // keeping flags is fine probably
        dec(c(FillArr, x)->fill);
        c(FillArr, x)->fill = fill;
        return x;
      }
      break;
  }
  usz ia = a(x)->ia;
  if (isNum(fill)) {
    x = num_squeezeChk(x);
    if (elNum(TI(x,elType))) return x;
    FL_KEEP(x, ~fl_squoze);
  } else if (isC32(fill)) {
    x = chr_squeezeChk(x);
    if (elChr(TI(x,elType))) return x;
    FL_KEEP(x, ~fl_squoze);
  }
  FillArr* r = m_arr(fsizeof(FillArr,a,B,ia), t_fillarr, ia);
  arr_shCopy((Arr*)r, x);
  r->fill = fill;
  if (xt==t_harr | xt==t_hslice) {
    if (xt==t_harr && v(x)->refc==1) {
      B* xp = harr_ptr(x);
      B* rp = r->a;
      memcpy(rp, xp, ia*sizeof(B));
      tyarr_freeF(v(x)); // manually free x so that decreasing its refcounts is skipped
    } else {
      B* xp = hany_ptr(x);
      B* rp = r->a;
      memcpy(rp, xp, ia*sizeof(B));
      for (usz i = 0; i < ia; i++) inc(rp[i]);
      dec(x);
    }
  } else {
    B* rp = r->a;
    SGet(x)
    for (usz i = 0; i < ia; i++) rp[i] = Get(x,i);
    dec(x);
  }
  return taga(r);
}
