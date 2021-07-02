typedef struct HArr {
  struct Arr;
  B a[];
} HArr;
typedef struct HSlice {
  struct Slice;
  B* a;
} HSlice;


typedef struct HArr_p {
  B b;
  B* a;
  HArr* c;
} HArr_p;
static inline HArr_p harr_parts(B b) {
  HArr* p = c(HArr,b);
  return (HArr_p){.b = b, .a = p->a, .c = p};
}
static inline HArr_p harrP_parts(HArr* p) {
  return (HArr_p){.b = taga(p), .a = p->a, .c = p};
}
NOINLINE void harr_pfree(B x, usz am); // am - item after last written


static HArr_p m_harrs(usz ia, usz* ctr) { // writes just ia
  HArr* r = mm_alloc(fsizeof(HArr,a,B,ia), t_harrPartial);
  r->ia = ia;
  r->sh = ctr;
  HArr_p rp = harrP_parts(r);
  gsAdd(rp.b);
  return rp;
}
static B harr_fv(HArr_p p) { VTY(p.b, t_harrPartial);
  assert(p.c->ia == *p.c->sh);
  p.c->type = t_harr;
  p.c->sh = &p.c->ia;
  srnk(p.b, 1);
  gsPop();
  return p.b;
}
static B harr_fc(HArr_p p, B x) { VTY(p.b, t_harrPartial);
  assert(p.c->ia == *p.c->sh);
  p.c->type = t_harr;
  arr_shCopy((Arr*)p.c, x);
  gsPop();
  return p.b;
}
static B harr_fcd(HArr_p p, B x) { VTY(p.b, t_harrPartial);
  assert(p.c->ia == *p.c->sh);
  p.c->type = t_harr;
  arr_shCopy((Arr*)p.c, x);
  dec(x);
  gsPop();
  return p.b;
}
static usz* harr_fa(HArr_p p, ur r) { VTY(p.b, t_harrPartial);
  p.c->type = t_harr;
  gsPop();
  return arr_shAllocR((Arr*)p.c, r);
}
static void harr_abandon(HArr_p p) { VTY(p.b, t_harrPartial);
  gsPop();
  value_free((Value*)p.c);
}

static HArr_p m_harrUv(usz ia) {
  HArr* r = mm_alloc(fsizeof(HArr,a,B,ia), t_harr);
  arr_shVec((Arr*)r, ia);
  return harrP_parts(r);
}
static HArr_p m_harrUc(B x) { assert(isArr(x));
  HArr* r = mm_alloc(fsizeof(HArr,a,B,a(x)->ia), t_harr);
  arr_shCopy((Arr*)r, x);
  return harrP_parts(r);
}
static HArr_p m_harrUp(usz ia) { // doesn't write shape/rank
  HArr* r = mm_alloc(fsizeof(HArr,a,B,ia), t_harr);
  r->ia = ia;
  return harrP_parts(r);
}

static B m_hunit(B x) {
  HArr_p r = m_harrUp(1);
  arr_shAllocR((Arr*)r.c, 0);
  r.a[0] = x;
  return r.b;
}



static B* harr_ptr(B x) { VTY(x,t_harr); return c(HArr,x)->a; }
HArr* toHArr(B x);
B m_caB(usz ia, B* a);

// consumes all
static B m_v1(B a               ) { HArr_p r = m_harrUv(1); r.a[0] = a;                                     return r.b; }
static B m_v2(B a, B b          ) { HArr_p r = m_harrUv(2); r.a[0] = a; r.a[1] = b;                         return r.b; }
static B m_v3(B a, B b, B c     ) { HArr_p r = m_harrUv(3); r.a[0] = a; r.a[1] = b; r.a[2] = c;             return r.b; }
static B m_v4(B a, B b, B c, B d) { HArr_p r = m_harrUv(4); r.a[0] = a; r.a[1] = b; r.a[2] = c; r.a[3] = d; return r.b; }

