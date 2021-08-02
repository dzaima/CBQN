#include "../core.h"

B asFill(B x) { // consumes
  if (isArr(x)) {
    u8 xe = TI(x,elType);
    usz ia = a(x)->ia;
    if (xe<=el_f64) {
      i32* rp; B r = m_i32arrc(&rp, x);
      for (usz i = 0; i < ia; i++) rp[i] = 0;
      dec(x);
      return r;
    }
    if (xe==el_c32) {
      u32* rp; B r = m_c32arrc(&rp, x);
      for (usz i = 0; i < ia; i++) rp[i] = ' ';
      dec(x);
      return r;
    }
    HArr_p r = m_harrUc(x);
    BS2B xget = TI(x,get);
    bool noFill = false;
    for (usz i = 0; i < ia; i++) if ((r.a[i]=asFill(xget(x,i))).u == bi_noFill.u) noFill = true;
    B xf = getFillQ(x);
    dec(x);
    if (noFill) { ptr_dec(r.c); return bi_noFill; }
    return withFill(r.b, xf);
  }
  if (isF64(x)|isI32(x)) return m_i32(0);
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
static Arr* fillslice_slice(B x, usz s, usz ia) { Arr* p=c(Slice,x)->p; ptr_inc(p); Arr* r = m_fillslice(p, c(FillSlice,x)->a+s, ia); dec(x); return r; }

static B fillarr_get   (B x, usz n) { VTY(x,t_fillarr  ); return inc(c(FillArr  ,x)->a[n]); }
static B fillslice_get (B x, usz n) { VTY(x,t_fillslice); return inc(c(FillSlice,x)->a[n]); }
static B fillarr_getU  (B x, usz n) { VTY(x,t_fillarr  ); return     c(FillArr  ,x)->a[n] ; }
static B fillslice_getU(B x, usz n) { VTY(x,t_fillslice); return     c(FillSlice,x)->a[n] ; }
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
    BS2B xgetU = TI(x,getU);
    usz ia = a(x)->ia;
    for (usz i = 0; i < ia; i++) validateFill(xgetU(x,i));
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
    if (we==el_c32 ^ xe==el_c32) return false;
    assert(we==el_c32 & xe==el_c32  ||  we<=el_f64 & xe<=el_f64);
    return true;
  }
  BS2B xgetU = TI(x,getU);
  BS2B wgetU = TI(w,getU);
  for (usz i = 0; i < ia; i++) if(!fillEqual(wgetU(w,i),xgetU(x,i))) return false;
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
    case t_i32arr: case t_i32slice: if(fill.u == m_i32(0  ).u) return x; break;
    case t_c32arr: case t_c32slice: if(fill.u == m_c32(' ').u) return x; break;
    case t_fillslice: if (fillEqual(((FillArr*)c(Slice,x)->p)->fill, fill)) { dec(fill); return x; } break;
    case t_fillarr: if (fillEqual(c(FillArr,x)->fill, fill)) { dec(fill); return x; }
      if (reusable(x)) {
        dec(c(FillArr, x)->fill);
        c(FillArr, x)->fill = fill;
        return x;
      }
      break;
  }
  usz ia = a(x)->ia;
  if (isNum(fill)) {
    if (xt==t_harr) {
      B* xp = harr_ptr(x);
      {
        i32* rp; B r = m_i32arrc(&rp, x);
        for (usz i = 0; i < ia; i++) {
          B c = xp[i];
          if (!q_i32(c)) { dec(r); goto h_f64; }
          rp[i] = o2iu(c);
        }
        dec(x);
        return r;
      }
      h_f64: {
        f64* rp; B r = m_f64arrc(&rp, x);
        for (usz i = 0; i < ia; i++) {
          B c = xp[i];
          if (!q_f64(c)) { dec(r); goto base; }
          rp[i] = o2fu(c);
        }
        dec(x);
        return r;
      }
    } else {
      BS2B xgetU = TI(x,getU);
      {
        i32* rp; B r = m_i32arrc(&rp, x);
        for (usz i = 0; i < ia; i++) {
          B c = xgetU(x, i);
          if (!q_i32(c)) { dec(r); goto g_f64; }
          rp[i] = o2iu(c);
        }
        dec(x);
        return r;
      }
      g_f64: {
        f64* rp; B r = m_f64arrc(&rp, x);
        for (usz i = 0; i < ia; i++) {
          B c = xgetU(x, i);
          if (!q_f64(c)) { dec(r); goto base; }
          rp[i] = o2fu(c);
        }
        dec(x);
        return r;
      }
      
      // bool ints = true;
      // for (usz i = 0; i < ia; i++) {
      //   B c = xgetU(x, i);
      //   if (!isNum(c)) goto base;
      //   if (!q_i32(c)) ints = false;
      // }
      // if (ints) {
      //   B r = m_i32arrc(x); i32* rp = i32arr_ptr(r);
      //   for (usz i = 0; i < ia; i++) rp[i] = o2iu(xgetU(x, i));
      //   dec(x);
      //   return r;
      // } else {
      //   B r = m_f64arrc(x); f64* rp = f64arr_ptr(r);
      //   for (usz i = 0; i < ia; i++) rp[i] = o2fu(xgetU(x, i));
      //   dec(x);
      //   return r;
      // }
    }
  } else if (isC32(fill)) {
    u32* rp; B r = m_c32arrc(&rp, x);
    BS2B xgetU = TI(x,getU);
    for (usz i = 0; i < ia; i++) {
      B c = xgetU(x, i);
      if (!isC32(c)) { dec(r); goto base; }
      rp[i] = o2c(c);
    }
    dec(x);
    return r;
  }
  base:;
  FillArr* r = m_arr(fsizeof(FillArr,a,B,ia), t_fillarr, ia);
  arr_shCopy((Arr*)r, x);
  r->fill = fill;
  if (xt==t_harr | xt==t_hslice) {
    if (xt==t_harr && v(x)->refc==1) {
      B* xp = harr_ptr(x);
      B* rp = r->a;
      memcpy(rp, xp, ia*sizeof(B));
      decSh(v(x)); mm_free(v(x)); // manually free x so that refcounting is skipped
    } else {
      B* xp = hany_ptr(x);
      B* rp = r->a;
      memcpy(rp, xp, ia*sizeof(B));
      for (usz i = 0; i < ia; i++) inc(rp[i]);
      dec(x);
    }
    return taga(r);
  } else {
    B* rp = r->a;
    BS2B xget = TI(x,get);
    for (usz i = 0; i < ia; i++) rp[i] = xget(x,i);
    dec(x);
    return taga(r);
  }
}
