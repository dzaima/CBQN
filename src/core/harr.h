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


#define M_HARR(N, IA) usz N##_len = (IA); HArr_p N##_v = m_harr_impl(N##_len); usz* N##_ia = &N##_v.c->ia; usz N##_i = 0;
#define HARR_ADD(N, I, V) ({ B v_ = (V); usz i_ = (I); assert(N##_i==i_); N##_v.a[i_] = v_; *N##_ia = i_+1; N##_i++; v_; })
#define HARR_ADDA(N, V)   ({ B v_ = (V); N##_v.a[N##_i] = v_; *N##_ia = ++N##_i; v_; })
#define HARR_O(N) N##_v
#define HARR_I(N) N##_i
static HArr_p m_harr_impl(usz ia) {
  CHECK_IA(ia, sizeof(B));
  HArr* r = m_arr(fsizeof(HArr,a,B,ia), t_harrPartial, ia);
  r->ia = 0;
  // don't need to initialize r->sh or rank at all i guess
  HArr_p rp = harrP_parts(r);
  gsAdd(rp.b);
  return rp;
}

#define HARR_FV(N) ({ assert(N##_v.c->ia == N##_len); harr_fv_impl(N##_v); })
#define HARR_FC(N, X) ({ assert(N##_v.c->ia == N##_len); harr_fc_impl(N##_v, X); })
#define HARR_FCD(N, X) ({ assert(N##_v.c->ia == N##_len); harr_fcd_impl(N##_v, X); })
#define HARR_FA(N, R) ({ assert(N##_v.c->ia == N##_len); harr_fa_impl(N##_v, R); })
#define HARR_ABANDON(N) harr_abandon_impl(N##_v.c)
static B harr_fv_impl(HArr_p p) { VTY(p.b, t_harrPartial);
  p.c->type = t_harr;
  p.c->sh = &p.c->ia;
  SRNK(p.b, 1);
  gsPop();
  return p.b;
}
static B harr_fc_impl(HArr_p p, B x) { VTY(p.b, t_harrPartial);
  p.c->type = t_harr;
  arr_shCopy((Arr*)p.c, x);
  gsPop();
  return p.b;
}
static B harr_fcd_impl(HArr_p p, B x) { VTY(p.b, t_harrPartial);
  p.c->type = t_harr;
  arr_shCopy((Arr*)p.c, x);
  decG(x);
  gsPop();
  return p.b;
}
static usz* harr_fa_impl(HArr_p p, ur r) { VTY(p.b, t_harrPartial);
  p.c->type = t_harr;
  gsPop();
  return arr_shAlloc((Arr*)p.c, r);
}
void harr_abandon_impl(HArr* p);

// unsafe-ish things - don't allocate/GC anything before having written to all items

#define m_harr0v(N) ({ usz n_ = (N); HArr_p r_ = m_harrUv(n_); for(usz i=0;i<n_;i++)r_.a[i]=m_f64(0); r_; })
#define m_harr0c(X) ({ B x_ = (X); usz n_ = IA(x_); HArr_p r_ = m_harrUc(x_); for(usz i=0;i<n_;i++)r_.a[i]=m_f64(0); r_; })
#define m_harr0p(N) ({ usz n_ = (N); HArr_p r_ = m_harrUp(n_); for(usz i=0;i<n_;i++)r_.a[i]=m_f64(0); r_; })
static HArr_p m_harrUv(usz ia) {
  CHECK_IA(ia, sizeof(B));
  HArr* r = m_arr(fsizeof(HArr,a,B,ia), t_harr, ia);
  arr_shVec((Arr*)r);
  return harrP_parts(r);
}
static HArr_p m_harrUc(B x) { assert(isArr(x));
  usz ia = IA(x);
  CHECK_IA(ia, sizeof(B));
  HArr* r = m_arr(fsizeof(HArr,a,B,ia), t_harr, ia);
  arr_shCopy((Arr*)r, x);
  return harrP_parts(r);
}
static HArr_p m_harrUp(usz ia) {
  CHECK_IA(ia, sizeof(B));
  HArr* r = m_arr(fsizeof(HArr,a,B,ia), t_harr, ia);
  return harrP_parts(r);
}

static B m_hunit(B x) {
  HArr_p r = m_harrUp(1);
  arr_shAlloc((Arr*)r.c, 0);
  r.a[0] = x;
  return r.b;
}

static B* harr_ptr(B x) { VTY(x,t_harr); return c(HArr,x)->a; }
static B* hany_ptr(B x) { return v(x)->type==t_hslice? c(HSlice,x)->a : harr_ptr(x); }

HArr* cpyHArr(B x); // consumes
static HArr* toHArr(B x) { return v(x)->type==t_harr? c(HArr,x) : cpyHArr(x); }
B m_caB(usz ia, B* a);

// consumes all
static B m_hVec1(B a               ) { HArr_p r = m_harrUv(1); r.a[0] = a;                                     return r.b; }
static B m_hVec2(B a, B b          ) { HArr_p r = m_harrUv(2); r.a[0] = a; r.a[1] = b;                         return r.b; }
static B m_hVec3(B a, B b, B c     ) { HArr_p r = m_harrUv(3); r.a[0] = a; r.a[1] = b; r.a[2] = c;             return r.b; }
static B m_hVec4(B a, B b, B c, B d) { HArr_p r = m_harrUv(4); r.a[0] = a; r.a[1] = b; r.a[2] = c; r.a[3] = d; return r.b; }

