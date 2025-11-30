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

SHOULD_INLINE HArr_p harr_parts(B b) {
  HArr* p = c(HArr,b);
  return (HArr_p){.b = b, .a = p->a, .c = p};
}
SHOULD_INLINE HArr_p harrP_parts(HArr* p) {
  return (HArr_p){.b = taga(p), .a = p->a, .c = p};
}
SHOULD_INLINE HArr_p harr_untagged(UntaggedArr a) {
  return (HArr_p){.b=taga(a.obj), .a=a.data, .c=(HArr*)a.obj};
}

NOINLINE void barr_pfree(B x, usz am); // either harr or fillarr; frees first am elements


#define M_HARR(N, IA) usz N##_len = (IA); HArr_p N##_v = m_harr_impl(N##_len); usz* N##_ia = &N##_v.c->ia; usz N##_i = 0;
#define HARR_ADD(N, I, V) ({ B v_ = (V); usz i_ = (I); assert(N##_i==i_); N##_v.a[i_] = v_; *N##_ia = i_+1; N##_i++; v_; })
#define HARR_ADDA(N, V)   ({ B v_ = (V); N##_v.a[N##_i] = v_; *N##_ia = ++N##_i; v_; })
#define HARR_O(N) N##_v
#define HARR_I(N) N##_i
SHOULD_INLINE HArr_p m_harr_impl(usz ia) {
  CHECK_IA(ia, sizeof(B));
  ux sz = fsizeof(HArr,a,B,ia);
  arr_check_size(sz, t_harr, ia);
  return harrP_parts(m_arr(sz, t_harr, 0));
}

#define HARR_FV(N) ({ assert(N##_v.c->ia == N##_len); harr_fv_impl(N##_v); })
#define HARR_FC(N, X) ({ assert(N##_v.c->ia == N##_len); harr_fc_impl(N##_v, X); })
#define HARR_FCD(N, X) ({ assert(N##_v.c->ia == N##_len); harr_fcd_impl(N##_v, X); })
#define HARR_FA(N, R) ({ assert(N##_v.c->ia == N##_len); harr_fa_impl(N##_v, R); })
#define HARR_FP(N, R) ({ assert(N##_v.c->ia == N##_len); harr_fp_impl(N##_v, R); })
#define HARR_ABANDON(N) harr_abandon_impl(N##_v.c)
SHOULD_INLINE B harr_fv_impl(HArr_p p) { VTY(p.b, t_harr);
  p.c->type = t_harr;
  arr_shVec((Arr*) p.c);
  return p.b;
}
SHOULD_INLINE B harr_fc_impl(HArr_p p, B x) { VTY(p.b, t_harr);
  p.c->type = t_harr;
  arr_shCopy((Arr*)p.c, x);
  return p.b;
}
SHOULD_INLINE B harr_fcd_impl(HArr_p p, B x) { VTY(p.b, t_harr);
  p.c->type = t_harr;
  arr_shCopy((Arr*)p.c, x);
  decG(x);
  return p.b;
}
SHOULD_INLINE Arr* harr_fp_impl(HArr_p p, ur r) { VTY(p.b, t_harr);
  p.c->type = t_harr;
  return (Arr*)p.c;
}
SHOULD_INLINE usz* harr_fa_impl(HArr_p p, ur r) { VTY(p.b, t_harr);
  p.c->type = t_harr;
  return arr_shAlloc((Arr*)p.c, r);
}
void harr_abandon_impl(HArr* p);



// see src/README.md for what these all are

SHOULD_INLINE UntaggedArr m_barrUpCore(ux ia, u8 fillarr, B fill);
SHOULD_INLINE UntaggedArr m_barrUvCore(ux ia, u8 fillarr, B fill);
SHOULD_INLINE UntaggedArr m_barrUcCore(B x,   u8 fillarr, B fill);

SHOULD_INLINE HArr_p m_harrUp(usz ia) { return harr_untagged(m_barrUpCore(ia, false, bi_noFill)); }
SHOULD_INLINE HArr_p m_harrUv(usz ia) { return harr_untagged(m_barrUvCore(ia, false, bi_noFill)); }
SHOULD_INLINE HArr_p m_harrUc(B x)    { return harr_untagged(m_barrUcCore(x,  false, bi_noFill)); }

HArr* m_harr0pN(usz ia);
HArr* m_harr0vN(usz ia);
HArr* m_harr0cN(B x);
SHOULD_INLINE HArr_p m_harr0p(usz ia) { return harrP_parts(m_harr0pN(ia)); }
SHOULD_INLINE HArr_p m_harr0v(usz ia) { return harrP_parts(m_harr0vN(ia)); }
SHOULD_INLINE HArr_p m_harr0c(B x   ) { return harrP_parts(m_harr0cN(x));  }

static B m_hunit(B x) { // consumes
  HArr_p r = m_harrUp(1);
  arr_shAtm((Arr*)r.c);
  r.a[0] = x;
  NOGC_E;
  return r.b;
}

static B* harrv_ptr(void* x) { VTY(taga(x),t_harr); return ((HArr*)x)->a; }
static B* hslicev_ptr(void* x) { VTY(taga(x),t_hslice); return ((HSlice*)x)->a; }
static B* harr_ptr(B x) { return harrv_ptr(a(x)); }
static B* hslice_ptr(B x) { return hslicev_ptr(a(x)); }

Arr* cpyHArr(B x); // consumes
static HArr* toHArr(B x) { return TY(x)==t_harr? c(HArr,x) : (HArr*) cpyHArr(x); }
#define TO_BPTR_RUN(X, F) ({ B* bp_ = arr_bptr(X); if (bp_==NULL) { HArr* nha_ = (HArr*)cpyHArr(X); X=taga(nha_); bp_=nha_->a; F; }; bp_; })
#define TO_BPTR(X) TO_BPTR_RUN(X, )

B m_caB(usz ia, B* a);

// consumes all
static B m_hvec1(B a               ) { HArr_p r = m_harrUv(1); r.a[0] = a;                                     NOGC_E; return r.b; }
static B m_hvec2(B a, B b          ) { HArr_p r = m_harrUv(2); r.a[0] = a; r.a[1] = b;                         NOGC_E; return r.b; }
static B m_hvec3(B a, B b, B c     ) { HArr_p r = m_harrUv(3); r.a[0] = a; r.a[1] = b; r.a[2] = c;             NOGC_E; return r.b; }
static B m_hvec4(B a, B b, B c, B d) { HArr_p r = m_harrUv(4); r.a[0] = a; r.a[1] = b; r.a[2] = c; r.a[3] = d; NOGC_E; return r.b; }

