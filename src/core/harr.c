#include "../core.h"
#include "gstack.h"



B toCells(B x) {
  assert(isArr(x) && rnk(x)>1);
  usz cam = a(x)->sh[0];
  usz csz = arr_csz(x);
  usz i = 0;
  BS2A slice = TI(x).slice;
  usz p = 0;
  HArr_p r = m_harrs(cam, &i);
  if (rnk(x)==2) {
    for (; i < cam; i++) {
      Arr* s = slice(inc(x), p);
      arr_shVec(s, csz);
      r.a[i] = taga(s);
      p+= csz;
    }
  } else {
    usz cr = rnk(x)-1;
    ShArr* csh = m_shArr(cr);
    usz* xsh = a(x)->sh;
    for (i32 i = 0; i < cr; i++) csh->a[i] = xsh[i+1];
    for (; i < cam; i++) {
      Arr* s = slice(inc(x), p);
      arr_shSetI(s, csz, cr, csh);
      r.a[i] = taga(s);
      p+= csz;
    }
    ptr_dec(csh);
  }
  dec(x);
  return harr_fv(r);
}
B toKCells(B x, ur k) {
  assert(isArr(x) && k<=rnk(x) && k>=0);
  ur xr = rnk(x); usz* xsh = a(x)->sh;
  ur cr = xr-k;
  usz cam = 1; for (i32 i = 0; i < k ; i++) cam*= xsh[i];
  usz csz = 1; for (i32 i = k; i < xr; i++) csz*= xsh[i];
  
  ShArr* csh;
  if (cr>1) {
    csh = m_shArr(cr);
    for (i32 i = 0; i < cr; i++) csh->a[i] = xsh[i+k];
  }
  
  usz i = 0;
  usz p = 0;
  HArr_p r = m_harrs(cam, &i);
  BS2A slice = TI(x).slice;
  for (; i < cam; i++) {
    Arr* s = slice(inc(x), p);
    arr_shSetI(s, csz, cr, csh);
    r.a[i] = taga(s);
    p+= csz;
  }
  if (cr>1) ptr_dec(csh);
  usz* rsh = harr_fa(r, k);
  if (rsh) for (i32 i = 0; i < k; i++) rsh[i] = xsh[i];
  dec(x);
  return r.b;
}


HArr* toHArr(B x) {
  if (v(x)->type==t_harr) return c(HArr,x);
  HArr_p r = m_harrUc(x);
  usz ia = r.c->ia;
  BS2B xget = TI(x).get;
  for (usz i = 0; i < ia; i++) r.a[i] = xget(x,i);
  dec(x);
  return r.c;
}

NOINLINE B m_caB(usz ia, B* a) {
  HArr_p r = m_harrUv(ia);
  for (usz i = 0; i < ia; i++) r.a[i] = a[i];
  return r.b;
}

NOINLINE void harr_pfree(B x, usz am) { // am - item after last written
  assert(v(x)->type==t_harr);
  B* p = harr_ptr(x);
  for (usz i = 0; i < am; i++) dec(p[i]);
  if (rnk(x)>1) ptr_dec(shObj(x));
  mm_free(v(x));
}



static Arr* m_hslice(B p, B* ptr) {
  HSlice* r = mm_alloc(sizeof(HSlice), t_hslice);
  r->p = p;
  r->a = ptr;
  return (Arr*)r;
}
static Arr* harr_slice  (B x, usz s) { return   m_hslice(x                 , c(HArr  ,x)->a+s); }
static Arr* hslice_slice(B x, usz s) { Arr* r = m_hslice(inc(c(Slice,x)->p), c(HSlice,x)->a+s); dec(x); return r; }

static B harr_get   (B x, usz n) { VTY(x,t_harr  ); return inc(c(HArr  ,x)->a[n]); }
static B hslice_get (B x, usz n) { VTY(x,t_hslice); return inc(c(HSlice,x)->a[n]); }
static B harr_getU  (B x, usz n) { VTY(x,t_harr  ); return     c(HArr  ,x)->a[n] ; }
static B hslice_getU(B x, usz n) { VTY(x,t_hslice); return     c(HSlice,x)->a[n] ; }
static void harr_free(Value* x) {
  decSh(x);
  B* p = ((HArr*)x)->a; // don't use harr_ptr so type isn't checked
  usz ia = ((Arr*)x)->ia;
  for (usz i = 0; i < ia; i++) dec(p[i]);
}
static void harr_visit(Value* x) {
  usz ia = ((Arr*)x)->ia; B* p = ((HArr*)x)->a;
  for (usz i = 0; i < ia; i++) mm_visit(p[i]);
}
static bool harr_canStore(B x) { return true; }



static void harrP_free(Value* x) { assert(x->type==t_harrPartial|x->type==t_freed);
  assert(prnk(x)>1? true : ((Arr*)x)->sh!=&((Arr*)x)->ia);
  B* p   =  ((HArr*)x)->a;
  usz am = *((HArr*)x)->sh;
  // printf("partfree %d/%d %p\n", am, a(x)->ia, (void*)x.u);
  for (usz i = 0; i < am; i++) dec(p[i]);
}
static void harrP_visit(Value* x) { assert(x->type==t_harrPartial);
  assert(prnk(x)>1? true : ((Arr*)x)->sh!=&((Arr*)x)->ia);
  B* p   =  ((HArr*)x)->a;
  usz am = *((HArr*)x)->sh;
  for (usz i = 0; i < am; i++) mm_visit(p[i]);
}
static B harrP_get(B x, usz n) { err("getting item from t_harrPartial"); }
static void harrP_print(B x) {
  B* p = c(HArr,x)->a;
  usz am = *c(HArr,x)->sh;
  usz ia = a(x)->ia;
  printf("(partial HArr %d/%d: ⟨", am, ia);
  for (usz i = 0; i < ia; i++) {
    if (i) printf(", ");
    if (i>=am) printf("?");
    else print(p[i]);
  }
  printf("⟩)");
}

void harr_init() {
  ti[t_harr].get   = harr_get;   ti[t_hslice].get   = hslice_get;   ti[t_harrPartial].get   = harrP_get;
  ti[t_harr].getU  = harr_getU;  ti[t_hslice].getU  = hslice_getU;  ti[t_harrPartial].getU  = harrP_get;
  ti[t_harr].slice = harr_slice; ti[t_hslice].slice = hslice_slice;
  ti[t_harr].free  = harr_free;  ti[t_hslice].free  =  slice_free;  ti[t_harrPartial].free  = harrP_free;
  ti[t_harr].visit = harr_visit; ti[t_hslice].visit =  slice_visit; ti[t_harrPartial].visit = harrP_visit;
  ti[t_harr].print =  arr_print; ti[t_hslice].print = arr_print;    ti[t_harrPartial].print = harrP_print;
  ti[t_harr].isArr = true;       ti[t_hslice].isArr = true;
  ti[t_harr].canStore = harr_canStore;
  bi_emptyHVec = m_harrUv(0).b;
  gc_add(bi_emptyHVec);
}
