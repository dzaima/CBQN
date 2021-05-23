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
NOINLINE void harr_pfree(B x, usz am); // am - item after last written


static HArr_p m_harrs(usz ia, usz* ctr) { // writes just ia
  B r = m_arr(fsizeof(HArr,a,B,ia), t_harrPartial);
  a(r)->ia = ia;
  a(r)->sh = ctr;
  gsAdd(r);
  return harr_parts(r);
}
static B harr_fv(HArr_p p) { VTY(p.b, t_harrPartial);
  p.c->type = t_harr;
  p.c->sh = &p.c->ia;
  srnk(p.b, 1);
  gsPop();
  return p.b;
}
static B harr_fc(HArr_p p, B x) { VTY(p.b, t_harrPartial);
  p.c->type = t_harr;
  arr_shCopy(p.b, x);
  gsPop();
  return p.b;
}
static B harr_fcd(HArr_p p, B x) { VTY(p.b, t_harrPartial);
  p.c->type = t_harr;
  arr_shCopy(p.b, x);
  dec(x);
  gsPop();
  return p.b;
}
static usz* harr_fa(HArr_p p, ur r) { VTY(p.b, t_harrPartial);
  p.c->type = t_harr;
  gsPop();
  return arr_shAllocR(p.b, r);
}

static HArr_p m_harrUv(usz ia) {
  B r = m_arr(fsizeof(HArr,a,B,ia), t_harr);
  arr_shVec(r, ia);
  return harr_parts(r);
}
static HArr_p m_harrUc(B x) { assert(isArr(x));
  B r = m_arr(fsizeof(HArr,a,B,a(x)->ia), t_harr);
  arr_shCopy(r, x);
  return harr_parts(r);
}
static HArr_p m_harrUp(usz ia) { // doesn't write shape/rank
  B r = m_arr(fsizeof(HArr,a,B,ia), t_harr);
  a(r)->ia = ia;
  return harr_parts(r);
}

static B m_hunit(B x) {
  HArr_p r = m_harrUp(1);
  arr_shAllocR(r.b, 0);
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

