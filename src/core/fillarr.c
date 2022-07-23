#include "../core.h"

B asFill(B x) { // consumes
  if (isArr(x)) {
    u8 xe = TI(x,elType);
    usz ia = a(x)->ia;
    if (elNum(xe)) {
      Arr* r = allZeroes(ia);
      arr_shCopy(r, x);
      decG(x);
      return taga(r);
    }
    if (elChr(xe)) {
      u8* rp; B r = m_c8arrc(&rp, x);
      for (usz i = 0; i < ia; i++) rp[i] = ' ';
      decG(x);
      return r;
    }
    M_HARR(r, a(x)->ia)
    SGet(x)
    for (usz i = 0; i < ia; i++) {
      if (noFill(HARR_ADD(r, i, asFill(Get(x,i))))) { HARR_ABANDON(r); decG(x); return bi_noFill; }
    }
    B xf = getFillQ(x);
    return withFill(HARR_FCD(r, x), xf);
  }
  if (isF64(x)) return m_i32(0);
  if (isC32(x)) return m_c32(' ');
  dec(x);
  return bi_noFill;
}

static Arr* m_fillslice(Arr* p, B* ptr, usz ia, B fill) {
  FillSlice* r = m_arr(sizeof(FillSlice), t_fillslice, ia);
  r->p = p;
  r->a = ptr;
  r->fill = fill;
  return (Arr*)r;
}
static Arr* fillarr_slice  (B x, usz s, usz ia) { FillArr*   a=c(FillArr  ,x); return m_fillslice((Arr*)a,       a->a+s, ia, inc(a->fill)); }
static Arr* fillslice_slice(B x, usz s, usz ia) { FillSlice* a=c(FillSlice,x); Arr* r=m_fillslice(ptr_inc(a->p), a->a+s, ia, inc(a->fill)); decG(x); return r; }

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

static void fillarr_freeT(Value* x) { FillArr* s=(void*)x; dec(s->fill); decSh(x); mm_free(x); }

static void fillslice_visit(Value* x) { FillSlice* s=(void*)x; mm_visitP(s->p); mm_visit(s->fill); }
static void fillslice_freeO(Value* x) { FillSlice* s=(void*)x; ptr_dec(s->p); dec(s->fill); decSh(x); }
static void fillslice_freeF(Value* x) { fillslice_freeO(x); mm_free(x); }

void fillarr_init() {
  TIi(t_fillarr,get)   = fillarr_get;   TIi(t_fillslice,get)   = fillslice_get;
  TIi(t_fillarr,getU)  = fillarr_getU;  TIi(t_fillslice,getU)  = fillslice_getU;
  TIi(t_fillarr,slice) = fillarr_slice; TIi(t_fillslice,slice) = fillslice_slice;
  TIi(t_fillarr,freeO) = fillarr_freeO; TIi(t_fillslice,freeO) = fillslice_freeO;
  TIi(t_fillarr,freeF) = fillarr_freeF; TIi(t_fillslice,freeF) = fillslice_freeF;
  TIi(t_fillarr,freeT) = fillarr_freeT;
  TIi(t_fillarr,visit) = fillarr_visit; TIi(t_fillslice,visit) = fillslice_visit;
  TIi(t_fillarr,print) =    farr_print; TIi(t_fillslice,print) = farr_print;
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

NOINLINE bool fillEqualF(B w, B x) { // doesn't consume; both args must be arrays
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
    case t_f64arr: case t_f64slice: case t_bitarr:
    case t_i32arr: case t_i32slice: case t_i16arr: case t_i16slice: case t_i8arr: case t_i8slice: if(fill.u == m_i32(0  ).u) return x; break;
    case t_c32arr: case t_c32slice: case t_c16arr: case t_c16slice: case t_c8arr: case t_c8slice: if(fill.u == m_c32(' ').u) return x; break;
    case t_fillslice: if (fillEqual(c(FillSlice,x)->fill, fill)) { dec(fill); return x; } break;
    case t_fillarr:   if (fillEqual(c(FillArr,  x)->fill, fill)) { dec(fill); return x; }
      if (reusable(x)) { // keeping flags is fine probably
        dec(c(FillArr, x)->fill);
        c(FillArr, x)->fill = fill;
        return x;
      }
      break;
  }
  usz ia = a(x)->ia;
  if (!FL_HAS(x,fl_squoze)) {
    if (isNum(fill)) {
      x = num_squeeze(x);
      if (elNum(TI(x,elType))) return x;
      FL_KEEP(x, ~fl_squoze);
    } else if (isC32(fill)) {
      x = chr_squeeze(x);
      if (elChr(TI(x,elType))) return x;
      FL_KEEP(x, ~fl_squoze);
    }
  }
  Arr* r;
  B* xbp = arr_bptr(x);
  if (xbp!=NULL) {
    Arr* xa = a(x);
    if (IS_SLICE(xa->type)) xa = ptr_inc(((Slice*)xa)->p);
    else ptr_inc(xa);
    r = m_fillslice(xa, xbp, ia, fill);
  } else {
    FillArr* rf = m_arr(fsizeof(FillArr,a,B,ia), t_fillarr, ia);
    rf->fill = fill;
    
    B* rp = rf->a; SGet(x)
    for (usz i = 0; i < ia; i++) rp[i] = Get(x,i);
    r = (Arr*)rf;
  }
  arr_shCopy(r, x);
  decG(x);
  return taga(r);
}
