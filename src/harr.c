#include "h.h"

typedef struct HArr {
  struct Arr;
  B a[];
} HArr;

typedef struct HArr_p {
  B b;
  B* a;
  HArr* c;
} HArr_p;
HArr_p harr_parts(B b) {
  HArr* p = c(HArr,b);
  return (HArr_p){.b = b, .a = p->a, .c = p};
}


HArr_p m_harrs(usz ia, usz* ctr) { // writes just ia
  B r = m_arr(fsizeof(HArr,a,B,ia), t_harrPartial);
  a(r)->ia = ia;
  a(r)->sh = ctr;
  gsAdd(r);
  return harr_parts(r);
}
B harr_fv(HArr_p p) { VT(p.b, t_harrPartial);
  p.c->type = t_harr;
  p.c->sh = &p.c->ia;
  srnk(p.b, 1);
  gsPop();
  return p.b;
}
B harr_fc(HArr_p p, B x) { VT(p.b, t_harrPartial);
  p.c->type = t_harr;
  arr_shCopy(p.b, x);
  gsPop();
  return p.b;
}
B harr_fcd(HArr_p p, B x) { VT(p.b, t_harrPartial);
  p.c->type = t_harr;
  arr_shCopy(p.b, x);
  dec(x);
  gsPop();
  return p.b;
}
usz* harr_fa(HArr_p p, ur r) { VT(p.b, t_harrPartial);
  p.c->type = t_harr;
  gsPop();
  return arr_shAllocR(p.b, r);
}

HArr_p m_harrUv(usz ia) {
  B r = m_arr(fsizeof(HArr,a,B,ia), t_harr);
  arr_shVec(r, ia);
  return harr_parts(r);
}
HArr_p m_harrUc(B x) { assert(isArr(x));
  B r = m_arr(fsizeof(HArr,a,B,a(x)->ia), t_harr);
  arr_shCopy(r, x);
  return harr_parts(r);
}
HArr_p m_harrUp(usz ia) { // doesn't write shape/rank
  B r = m_arr(fsizeof(HArr,a,B,ia), t_harr);
  a(r)->ia = ia;
  return harr_parts(r);
}

B m_hunit(B x) {
  HArr_p r = m_harrUp(1);
  arr_shAllocR(r.b, 0);
  r.a[0] = x;
  return r.b;
}


B* harr_ptr(B x) { VT(x,t_harr); return c(HArr,x)->a; }

HArr* toHArr(B x) {
  if (v(x)->type==t_harr) return c(HArr,x);
  HArr_p r = m_harrUc(x);
  usz ia = r.c->ia;
  BS2B xget = TI(x).get;
  for (usz i = 0; i < ia; i++) r.a[i] = xget(x,i);
  dec(x);
  return r.c;
}


B m_caB(usz ia, B* a) {
  HArr_p r = m_harrUv(ia);
  for (usz i = 0; i < ia; i++) r.a[i] = a[i];
  return r.b;
}

// consumes all
B m_v1(B a               ) { HArr_p r = m_harrUv(1); r.a[0] = a;                                     return r.b; }
B m_v2(B a, B b          ) { HArr_p r = m_harrUv(2); r.a[0] = a; r.a[1] = b;                         return r.b; }
B m_v3(B a, B b, B c     ) { HArr_p r = m_harrUv(3); r.a[0] = a; r.a[1] = b; r.a[2] = c;             return r.b; }
B m_v4(B a, B b, B c, B d) { HArr_p r = m_harrUv(4); r.a[0] = a; r.a[1] = b; r.a[2] = c; r.a[3] = d; return r.b; }


typedef struct HSlice {
  struct Slice;
  B* a;
} HSlice;
B m_hslice(B p, B* ptr) {
  HSlice* r = mm_allocN(sizeof(HSlice), t_hslice);
  r->p = p;
  r->a = ptr;
  return tag(r, ARR_TAG);
}

B harr_slice  (B x, usz s) {return m_hslice(x                 , c(HArr  ,x)->a+s); }
B hslice_slice(B x, usz s) { B r = m_hslice(inc(c(Slice,x)->p), c(HSlice,x)->a+s); dec(x); return r; }


B harr_get   (B x, usz n) { VT(x,t_harr  ); return inc(c(HArr  ,x)->a[n]); }
B hslice_get (B x, usz n) { VT(x,t_hslice); return inc(c(HSlice,x)->a[n]); }
B harr_getU  (B x, usz n) { VT(x,t_harr  ); return     c(HArr  ,x)->a[n] ; }
B hslice_getU(B x, usz n) { VT(x,t_hslice); return     c(HSlice,x)->a[n] ; }
void harr_free(B x) {
  decSh(x);
  B* p = c(HArr,x)->a; // don't use harr_ptr so type isn't checked
  usz ia = a(x)->ia;
  for (usz i = 0; i < ia; i++) dec(p[i]);
}
void harr_visit(B x) {
  usz ia = a(x)->ia; B* p = harr_ptr(x);
  for (usz i = 0; i < ia; i++) mm_visit(p[i]);
}
bool harr_canStore(B x) { return true; }



NOINLINE void harr_pfree(B x, usz am) { // am - item after last written
  assert(v(x)->type==t_harr);
  B* p = harr_ptr(x);
  for (usz i = 0; i < am; i++) dec(p[i]);
  mm_free(v(x));
}
void harrP_free(B x) { assert(v(x)->type==t_harrPartial|v(x)->type==t_freed);
  assert(rnk(x)>1? true : a(x)->sh!=&a(x)->ia);
  B* p   =  c(HArr,x)->a;
  usz am = *c(HArr,x)->sh;
  // printf("partfree %d/%d %p\n", am, a(x)->ia, (void*)x.u);
  for (usz i = 0; i < am; i++) dec(p[i]);
}
void harrP_visit(B x) { VT(x, t_harrPartial);
  assert(rnk(x)>1? true : a(x)->sh!=&a(x)->ia);
  B* p   =  c(HArr,x)->a;
  usz am = *c(HArr,x)->sh;
  for (usz i = 0; i < am; i++) mm_visit(p[i]);
}
B harrP_get(B x, usz n) { return err("getting item from t_harrPartial"); }
void harrP_print(B x) {
  B* p = c(HArr,x)->a;
  usz am = *c(HArr,x)->sh;
  usz ia = a(x)->ia;
  printf("(partial HArr %d/%d %p %p: ?⥊⟨", am, ia, c(HArr,x)->sh, &a(x)->ia);
  for (usz i = 0; i < ia; i++) {
    if (i) printf(", ");
    if (i>=am) printf("(…)\n");
    else print(p[i]);
  }
  printf("⟩)");
}

static inline void harr_init() {
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
