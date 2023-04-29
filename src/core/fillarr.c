#include "../core.h"

B asFill(B x) { // consumes
  if (isArr(x)) {
    u8 xe = TI(x,elType);
    usz ia = IA(x);
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
    M_HARR(r, IA(x))
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

static B fillarr_get   (Arr* x, usz n) { assert(PTY(x)==t_fillarr  ); return inc(((FillArr*  )x)->a[n]); }
static B fillslice_get (Arr* x, usz n) { assert(PTY(x)==t_fillslice); return inc(((FillSlice*)x)->a[n]); }
static B fillarr_getU  (Arr* x, usz n) { assert(PTY(x)==t_fillarr  ); return     ((FillArr*  )x)->a[n] ; }
static B fillslice_getU(Arr* x, usz n) { assert(PTY(x)==t_fillslice); return     ((FillSlice*)x)->a[n] ; }
DEF_FREE(fillarr) {
  decSh(x);
  B* p = ((FillArr*)x)->a;
  dec(((FillArr*)x)->fill);
  usz ia = PIA((Arr*)x);
  for (usz i = 0; i < ia; i++) dec(p[i]);
}
static void fillarr_visit(Value* x) { assert(PTY(x) == t_fillarr);
  VISIT_SHAPE(x);
  usz ia = PIA((Arr*)x); B* p = ((FillArr*)x)->a;
  mm_visit(((FillArr*)x)->fill);
  for (usz i = 0; i < ia; i++) mm_visit(p[i]);
}
static bool fillarr_canStore(B x) { return true; }

static void fillarr_freeT(Value* x) { FillArr* s=(void*)x; dec(s->fill); decSh(x); mm_free(x); }

static void fillslice_visit(Value* x) { FillSlice* s=(void*)x; mm_visitP(s->p); mm_visit(s->fill); VISIT_SHAPE(x); }
static void fillslice_freeO(Value* x) { FillSlice* s=(void*)x; ptr_dec(s->p); dec(s->fill); decSh(x); }
static void fillslice_freeF(Value* x) { fillslice_freeO(x); mm_free(x); }

void fillarr_init(void) {
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
    usz ia = IA(x);
    for (usz i = 0; i < ia; i++) validateFill(GetU(x,i));
  } else if (isF64(x)) {
    assert(x.f==0);
  } else if (isC32(x)) {
    assert(' '==(u32)x.u);
  }
}

NOINLINE bool fillEqualF(B w, B x) { // doesn't consume; both args must be arrays
  if (!eqShape(w, x)) return false;
  usz ia = IA(w);
  if (ia==0) return true;
  
  u8 we = TI(w,elType);
  u8 xe = TI(x,elType);
  if (we!=el_B && xe!=el_B) return elNum(we) == elNum(xe);
  
  SGetU(x) SGetU(w)
  for (usz i = 0; i < ia; i++) if(!fillEqual(GetU(w,i),GetU(x,i))) return false;
  return true;
}



B withFill(B x, B fill) { // consumes both
  assert(isArr(x));
  #ifdef DEBUG
  validateFill(fill);
  #endif
  u8 xt = TY(x);
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
  usz ia = IA(x);
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
    if (IS_SLICE(PTY(xa))) xa = ptr_inc(((Slice*)xa)->p);
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


NOINLINE B m_funit(B x) {
  B xf = asFill(inc(x));
  if (noFill(xf)) return m_hunit(x);
  FillArr* r = m_arr(fsizeof(FillArr,a,B,1), t_fillarr, 1);
  arr_shAtm((Arr*)r);
  r->fill = xf;
  r->a[0] = x;
  return taga(r);
}

NOINLINE B m_unit(B x) {
  u64 data; assert(sizeof(f64)<=8);
  u8 t; u64 sz;
  if (isNum(x)) {
    i32 xi = (i32)x.f;
    if (RARE(xi!=x.f))    { f64 v=x.f;     memcpy(&data, &v, sz=sizeof(v)); t=t_f64arr; }
    else if (q_ibit(xi))  { u64 v=bitx(x); memcpy(&data, &v, sz=sizeof(v)); t=t_bitarr; }
    else if (xi==(i8 )xi) { i8  v=xi;      memcpy(&data, &v, sz=sizeof(v)); t=t_i8arr; }
    else if (xi==(i16)xi) { i16 v=xi;      memcpy(&data, &v, sz=sizeof(v)); t=t_i16arr; }
    else                  { i32 v=xi;      memcpy(&data, &v, sz=sizeof(v)); t=t_i32arr; }
  } else if (isC32(x)) {
    u32 xi = o2cG(x);
    if      (xi==(u8 )xi) { u8  v=xi; memcpy(&data, &v, sz=sizeof(v)); t=t_c8arr; }
    else if (xi==(u16)xi) { u16 v=xi; memcpy(&data, &v, sz=sizeof(v)); t=t_c16arr; }
    else                  { u32 v=xi; memcpy(&data, &v, sz=sizeof(v)); t=t_c32arr; }
  } else return m_funit(x);
  TyArr* r = m_arr(offsetof(TyArr,a) + sizeof(u64), t, 1);
  *((u64*)r->a) = data;
  FINISH_OVERALLOC(r, offsetof(TyArr,a)+sz, offsetof(TyArr,a)+sizeof(u64));
  arr_shAtm((Arr*)r);
  return taga(r);
}

NOINLINE B m_vec1(B a) {
  if (isF64(a)) {
    i32 i = (i32)a.f;
    if (RARE(a.f != i))   { f64* rp; B r = m_f64arrv(&rp, 1); rp[0] = a.f; return r; }
    else if (q_ibit(i))   { u64* rp; B r = m_bitarrv(&rp, 1); rp[0] = i; return r; }
    else if (i == (i8 )i) { i8*  rp; B r = m_i8arrv (&rp, 1); rp[0] = i; return r; }
    else if (i == (i16)i) { i16* rp; B r = m_i16arrv(&rp, 1); rp[0] = i; return r; }
    else                  { i32* rp; B r = m_i32arrv(&rp, 1); rp[0] = i; return r; }
  }
  if (isC32(a)) {
    u32 c = o2cG(a);
    if      (LIKELY(c<U8_MAX )) { u8*  rp; B r = m_c8arrv (&rp, 1); rp[0] = c; return r; }
    else if (LIKELY(c<U16_MAX)) { u16* rp; B r = m_c16arrv(&rp, 1); rp[0] = c; return r; }
    else                        { u32* rp; B r = m_c32arrv(&rp, 1); rp[0] = c; return r; }
  }
  Arr* ra = arr_shVec(m_fillarrp(1));
  fillarr_ptr(ra)[0] = a;
  fillarr_setFill(ra, m_f64(0));
  NOGC_E;
  fillarr_setFill(ra, asFill(inc(a)));
  return taga(ra);
}