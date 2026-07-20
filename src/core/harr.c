#include "core.h"

NOINLINE B m_caB(usz ia, B* a) {
  HArr_p r = m_harrUv(ia);
  vfor (usz i = 0; i < ia; i++) r.a[i] = a[i];
  NOGC_E;
  return r.b;
}

NOINLINE void barr_pfree(B x, usz am) {
  B* p;
  if (TY(x)==t_harr) {
    p = harr_ptr(x);
  } else if (TY(x)==t_fillarr) {
    dec(((FillArr*)a(x))->fill);
    p = fillarrv_ptr(a(x));
  } else UD;
  
  for (ux i = 0; i < am; i++) dec(p[i]);
  decSh(v(x));
  mm_free(v(x));
}

HArr* m_harr0pN(usz ia) {
  HArr_p r = m_harrUp(ia);
  FILL_TO(r.a, el_B, 0, m_f64(0), ia);
  NOGC_E;
  return r.c;
}
HArr* m_harr0vN(usz ia) {
  HArr_p r = m_harrUv(ia);
  FILL_TO(r.a, el_B, 0, m_f64(0), ia);
  NOGC_E;
  return r.c;
}
HArr* m_harr0cN(B x) {
  usz ia = IA(x);
  HArr_p r = m_harrUc(x);
  FILL_TO(r.a, el_B, 0, m_f64(0), ia);
  NOGC_E;
  return r.c;
}

Arr* m_fillarr0p(usz ia) {
  Arr* r = arr_shVec(m_fillarrp(ia));
  fillarr_setFill(r, m_f64(0));
  FILL_TO(fillarrv_ptr(r), el_B, 0, m_f64(0), ia);
  NOGC_E;
  return r;
}

NOINLINE B m_hvec1N(B a               ) { return m_hvec1(a); }
NOINLINE B m_hvec2N(B a, B b          ) { return m_hvec2(a,b); }
NOINLINE B m_hvec3N(B a, B b, B c     ) { return m_hvec3(a,b,c); }
NOINLINE B m_hvec4N(B a, B b, B c, B d) { return m_hvec4(a,b,c,d); }



static Arr* m_hslice(Arr* p, B* ptr, usz ia) {
  HSlice* r = m_arr(sizeof(HSlice), t_hslice, ia);
  r->p = p;
  r->a = ptr;
  return (Arr*)r;
}
static Arr* harr_slice  (B x, usz s, usz ia) { return m_hslice(a(x), c(HArr,x)->a+s, ia); }
static Arr* hslice_slice(B x, usz s, usz ia) { Arr* p = ptr_inc(c(Slice,x)->p); Arr* r = m_hslice(p, c(HSlice,x)->a+s, ia); decG(x); return r; }

static B harr_get   (Arr* x, usz n) { assert(PTY(x)==t_harr   && n<PIA(x)); return inc(harrv_ptr  (x)[n]); }
static B hslice_get (Arr* x, usz n) { assert(PTY(x)==t_hslice && n<PIA(x)); return inc(hslicev_ptr(x)[n]); }
static B harr_getU  (Arr* x, usz n) { assert(PTY(x)==t_harr   && n<PIA(x)); return     harrv_ptr  (x)[n] ; }
static B hslice_getU(Arr* x, usz n) { assert(PTY(x)==t_hslice && n<PIA(x)); return     hslicev_ptr(x)[n] ; }
DEF_FREE(harr) {
  decSh(x);
  B* p = ((HArr*)x)->a; // don't use harrv_ptr so type isn't checked
  usz ia = PIA((Arr*)x);
  for (usz i = 0; i < ia; i++) dec(p[i]);
}
static void harr_visit(Value* x) {
  VISIT_SHAPE(x);
  usz ia = PIA((Arr*)x); B* p = harrv_ptr(x);
  for (usz i = 0; i < ia; i++) mm_visit(p[i]);
}
static bool harr_canStore(B x) { return true; }



void harr_abandon_impl(HArr* p) {
  assert(PTY(p) == t_harr && p->refc==1);
  harr_freeF((Value*) p);
}

#if DEBUG
  static void harr_freeT(Value* x) {
    B* p = harrv_ptr(x);
    usz ia = PIA((Arr*)x);
    for (usz i = 0; i < ia; i++) assert(!isVal(p[i]));
    tyarr_freeF(x);
  }
#endif

void harr_init(void) {
  TIi(t_harr,get)   = harr_get;    TIi(t_hslice,get)   = hslice_get;
  TIi(t_harr,getU)  = harr_getU;   TIi(t_hslice,getU)  = hslice_getU;
  TIi(t_harr,slice) = harr_slice;  TIi(t_hslice,slice) = hslice_slice;
  TIi(t_harr,freeO) = harr_freeO;  TIi(t_hslice,freeO) =  slice_freeO;
  TIi(t_harr,freeF) = harr_freeF;  TIi(t_hslice,freeF) =  slice_freeF;
  #if DEBUG
  TIi(t_harr,freeT) = harr_freeT;
  #else
  TIi(t_harr,freeT) = tyarr_freeF;
  #endif
  TIi(t_harr,visit) = harr_visit;  TIi(t_hslice,visit) =  slice_visit;
  TIi(t_harr,print) = farr_print;  TIi(t_hslice,print) = farr_print;
  TIi(t_harr,isArr) = true;        TIi(t_hslice,isArr) = true;
  TIi(t_harr,canStore) = harr_canStore;
  bi_emptyHVec = gc_add(m_harrUv(0).b);
}
