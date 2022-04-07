#include <math.h>
#include "../core.h"
#include "../utils/talloc.h"
#include "../builtins.h"

B md2BI_uc1(Md2* t, B o, B f, B g,      B x) { return ((BMd2*)t)->uc1(t, o, f, g,    x); }
B md2BI_ucw(Md2* t, B o, B f, B g, B w, B x) { return ((BMd2*)t)->ucw(t, o, f, g, w, x); }


B val_c1(Md2D* d,      B x) { return c1(d->f,   x); }
B val_c2(Md2D* d, B w, B x) { return c2(d->g, w,x); }


typedef struct CustomObj {
  struct Value;
  V2v visit;
  V2v freeO;
} CustomObj;
void* customObj(u64 size, V2v visit, V2v freeO) {
  CustomObj* r = mm_alloc(size, t_customObj);
  r->visit = visit;
  r->freeO = freeO;
  return r;
}
void customObj_visit(Value* v) { ((CustomObj*)v)->visit(v); }
void customObj_freeO(Value* v) { ((CustomObj*)v)->freeO(v); }
void customObj_freeF(Value* v) { ((CustomObj*)v)->freeO(v); mm_free(v); }


#if CATCH_ERRORS
extern B lastErrMsg; // sysfn.c

B fillBy_c1(Md2D* d, B x) {
  B xf=getFillQ(x);
  B r = c1(d->f, x);
  if(isAtm(r) || noFill(xf)) { dec(xf); return r; }
  if (CATCH) { freeThrown(); return r; }
  B fill = asFill(c1(d->g, xf));
  popCatch();
  return withFill(r, fill);
}
B fillBy_c2(Md2D* d, B w, B x) {
  B wf=getFillQ(w); B xf=getFillQ(x);
  B r = c2(d->f, w,x);
  if(isAtm(r) || noFill(xf)) { dec(xf); dec(wf); return r; }
  if (CATCH) { freeThrown(); return r; }
  if (noFill(wf)) wf = incG(bi_asrt);
  B fill = asFill(c2(d->g, wf, xf));
  popCatch();
  return withFill(r, fill);
}

typedef struct ReObj {
  struct CustomObj;
  B msg;
} ReObj;
void re_visit(Value* v) { mm_visit(((ReObj*)v)->msg); }
void re_freeO(Value* v) { dec(lastErrMsg); lastErrMsg = ((ReObj*)v)->msg; }
void pushRe() {
  ReObj* o = customObj(sizeof(ReObj), re_visit, re_freeO);
  o->msg = lastErrMsg;
  gsAdd(tag(o,OBJ_TAG));
  
  lastErrMsg = inc(thrownMsg);
  freeThrown();
}
B catch_c1(Md2D* d,      B x) { if(CATCH) { pushRe(); B r = c1(d->g,   x); dec(gsPop()); return r; }         inc(x); B r = c1(d->f,   x); popCatch();         dec(x); return r; }
B catch_c2(Md2D* d, B w, B x) { if(CATCH) { pushRe(); B r = c2(d->g, w,x); dec(gsPop()); return r; } inc(w); inc(x); B r = c2(d->f, w,x); popCatch(); dec(w); dec(x); return r; }
#else
B fillBy_c1(Md2D* d,      B x) { return c1(d->f,   x); }
B fillBy_c2(Md2D* d, B w, B x) { return c2(d->f, w,x); }
B catch_c1(Md2D* d,      B x) { return c1(d->f,   x); }
B catch_c2(Md2D* d, B w, B x) { return c2(d->f, w,x); }
#endif

extern B rt_undo;
void repeat_bounds(i64* bound, B g) { // doesn't consume
  if (isArr(g)) {
    SGetU(g)
    usz ia = a(g)->ia;
    for (usz i = 0; i < ia; i++) repeat_bounds(bound, GetU(g, i));
  } else if (isNum(g)) {
    i64 i = o2i64(g);
    if (i<bound[0]) bound[0] = i;
    if (i>bound[1]) bound[1] = i;
  } else thrM("⍟: 𝔾 contained a non-number atom");
}
B repeat_replace(B g, B* q) { // doesn't consume
  if (isArr(g)) {
    SGetU(g)
    usz ia = a(g)->ia;
    M_HARR(r, ia);
    for (usz i = 0; i < ia; i++) HARR_ADD(r, i, repeat_replace(GetU(g,i), q));
    return HARR_FC(r, g);
  } else {
    return inc(q[o2i64u(g)]);
  }
}
#define REPEAT_T(CN, END, ...)                     \
  B g = CN(d->g, __VA_ARGS__ inc(x));              \
  B f = d->f;                                      \
  if (isNum(g)) {                                  \
    i64 am = o2i64(g);                             \
    if (am>=0) {                                   \
      for (i64 i = 0; i < am; i++) x = CN(f, __VA_ARGS__ x); \
      END;                                         \
      return x;                                    \
    }                                              \
  }                                                \
  i64 bound[2] = {0,0};                            \
  repeat_bounds(bound, g);                         \
  i64 min=(u64)-bound[0]; i64 max=(u64)bound[1];   \
  TALLOC(B, all, min+max+1);                       \
  B* q = all+min;                                  \
  q[0] = inc(x);                                   \
  if (min) {                                       \
    B x2 = inc(x);                                 \
    B fi = m1_d(incG(rt_undo), inc(f));            \
    for (i64 i = 0; i < min; i++) q[-1-i] = inc(x2 = CN(fi, __VA_ARGS__ x2)); \
    dec(x2);                                       \
    dec(fi);                                       \
  }                                                \
  for (i64 i = 0; i < max; i++) q[i+1] = inc(x = CN(f, __VA_ARGS__ x)); \
  dec(x);                                          \
  B r = repeat_replace(g, q);                      \
  dec(g);                                          \
  for (i64 i = 0; i < min+max+1; i++) dec(all[i]); \
  END; TFREE(all);                                 \
  return r;

B repeat_c1(Md2D* d,      B x) { REPEAT_T(c1,{}              ); }
B repeat_c2(Md2D* d, B w, B x) { REPEAT_T(c2,dec(w), inc(w), ); }
#undef REPEAT_T


B before_c1(Md2D* d,      B x) { return c2(d->g, c1iX(d->f, x), x); }
B before_c2(Md2D* d, B w, B x) { return c2(d->g, c1i (d->f, w), x); }
B after_c1(Md2D* d,      B x) { return c2(d->f, x, c1iX(d->g, x)); }
B after_c2(Md2D* d, B w, B x) { return c2(d->f, w, c1i (d->g, x)); }
B atop_c1(Md2D* d,      B x) { return c1(d->f, c1(d->g,    x)); }
B atop_c2(Md2D* d, B w, B x) { return c1(d->f, c2(d->g, w, x)); }
B over_c1(Md2D* d,      B x) { return c1(d->f, c1(d->g,    x)); }
B over_c2(Md2D* d, B w, B x) { B xr=c1(d->g, x); return c2(d->f, c1(d->g, w), xr); }

B pick_c2(B t, B w, B x);

B cond_c1(Md2D* d, B x) { B f=d->f; B g=d->g;
  B fr = c1iX(f, x);
  if (isNum(fr)) {
    if (isAtm(g)||rnk(g)!=1) thrM("◶: 𝕘 must have rank 1");
    usz fri = WRAP(o2i64(fr), a(g)->ia, thrM("◶: 𝔽 out of bounds of 𝕘"));
    return c1(IGetU(g, fri), x);
  } else {
    B fn = pick_c2(m_f64(0), fr, inc(g));
    B r = c1(fn, x);
    dec(fn);
    return r;
  }
}
B cond_c2(Md2D* d, B w, B x) { B g=d->g;
  B fr = c2iWX(d->f, w, x);
  if (isNum(fr)) {
    if (isAtm(g)||rnk(g)!=1) thrM("◶: 𝕘 must have rank 1");
    usz fri = WRAP(o2i64(fr), a(g)->ia, thrM("◶: 𝔽 out of bounds of 𝕘"));
    return c2(IGetU(g, fri), w, x);
  } else {
    B fn = pick_c2(m_f64(0), fr, inc(g));
    B r = c2(fn, w, x);
    dec(fn);
    return r;
  }
}

extern B rt_under, bi_before;
B under_c1(Md2D* d, B x) { B f=d->f; B g=d->g;
  if (!isVal(g)) { // ugh idk
    B fn = m2_d(incG(rt_under), inc(f), inc(g));
    B r = c1(fn, x);
    dec(fn);
    return r;
  }
  return TI(g,fn_uc1)(g, f, x);
}
B under_c2(Md2D* d, B w, B x) { B f=d->f; B g=d->g;
  if (!isVal(g)) {
    B fn = m2_d(incG(rt_under), inc(f), inc(g));
    B r = c2(fn, w, x);
    dec(fn);
    return r;
  }
  B f2 = m2_d(incG(bi_before), c1(g, w), inc(f));
  B r = TI(g,fn_uc1)(g, f2, x);
  dec(f2);
  return r;
}

B before_uc1(Md2* t, B o, B f, B g, B x) {
  if (!isFun(g)) return def_m2_uc1(t, o, f, g, x);
  return TI(g,fn_ucw)(g, o, inc(f), x);
}


B while_c1(Md2D* d, B x) { B f=d->f; B g=d->g;
  BB2B ff = c1fn(f);
  BB2B gf = c1fn(g);
  while (o2b(gf(g,inc(x)))) x = ff(f, x);
  return x;
}
B while_c2(Md2D* d, B w, B x) { B f=d->f; B g=d->g;
  BBB2B ff = c2fn(f);
  BBB2B gf = c2fn(g);
  while (o2b(gf(g,inc(w),inc(x)))) x = ff(f, inc(w), x);
  dec(w);
  return x;
}

static B m2c1(B t, B f, B g, B x) { // consumes x
  B fn = m2_d(inc(t), inc(f), inc(g));
  B r = c1(fn, x);
  decG(fn);
  return r;
}
static B m2c2(B t, B f, B g, B w, B x) { // consumes w,x
  B fn = m2_d(inc(t), inc(f), inc(g));
  B r = c2(fn, w, x);
  decG(fn);
  return r;
}

B rank_c1(Md2D* d, B x) { B f = d->f; B g = d->g;
  f64 kf;
  bool gf = isFun(g);
  if (RARE(gf)) g = c1(g, inc(x));
  if (LIKELY(isNum(g))) {
    kf = o2fu(g);
  } else if (isArr(g)) {
    usz gia = a(g)->ia;
    if (!(gia>=1 && gia<=3)) thrM("⎉: 𝔾 result must have 1 to 3 elements");
    SGetU(g)
    if (!elNum(TI(g,elType))) for (i32 i = 0; i < gia; i++) o2f(GetU(g,i));
    kf = GetU(g, gia==2).f;
  } else thrM("⎉: Invalid 𝔾 result");
  if (gf) dec(g);
  i32 k = kf;
  
  if (isAtm(x) || rnk(x)==0) {
    if (floor(kf)!=kf) thrM("⎉: 𝕘 was a fractional number");
    B r = c1(f, x);
    return isAtm(r)? m_atomUnit(r) : r;
  }
  i32 xr = rnk(x);
  usz* xsh = a(x)->sh;
  if (k!=kf) {
    if (floor(kf)!=kf) thrM("⎉: 𝕘 was a fractional number");
    k = kf>0? 0 : xr;
  } else {
    k = k<0? (k+xr<0? xr : xr-(k+xr)) : (k>xr? 0 : xr-k);
  }
  if (Q_BI(f,lt) && a(x)->ia!=0 && rnk(x)>1) return toKCells(x, k);
  
  usz cam = 1; for (usz i = 0; i <  k; i++) cam*= xsh[i];
  usz csz = 1; for (usz i = k; i < xr; i++) csz*= xsh[i];
  ur cr = xr-k;
  ShArr* csh;
  if (cr>1) {
    csh = m_shArr(cr);
    memcpy(csh->a, xsh+k, cr*sizeof(usz));
  }
  
  
  BSS2A slice = TI(x,slice);
  M_HARR(r, cam);
  usz p = 0;
  for (usz i = 0; i < cam; i++) {
    Arr* s = slice(inc(x), p, csz); arr_shSetI(s, cr, csh);
    HARR_ADD(r, i, c1(f, taga(s)));
    p+= csz;
  }
  
  if (cr>1) ptr_dec(csh);
  usz* rsh = HARR_FA(r, k);
  if (k>1) memcpy(rsh, xsh, k*sizeof(usz));
  
  decG(x);
  return bqn_merge(HARR_O(r).b);
}
extern B rt_rank;
B rank_c2(Md2D* d, B w, B x) { B f = d->f; B g = d->g; // TODO
  return m2c2(rt_rank, f, g, w, x);
}




extern B rt_depth;
B depth_c1(Md2D* d,      B x) { return m2c1(rt_depth, d->f, d->g, x); }
B depth_c2(Md2D* d, B w, B x) { return m2c2(rt_depth, d->f, d->g, w, x); }


static void print_md2BI(FILE* f, B x) { fprintf(f, "%s", pm2_repr(c(Md1,x)->extra)); }
static B md2BI_im(Md2D* d,      B x) { return ((BMd2*)d->m2)->im(d,    x); }
static B md2BI_iw(Md2D* d, B w, B x) { return ((BMd2*)d->m2)->iw(d, w, x); }
static B md2BI_ix(Md2D* d, B w, B x) { return ((BMd2*)d->m2)->ix(d, w, x); }
void md2_init() {
  TIi(t_md2BI,print) = print_md2BI;
  TIi(t_md2BI,m2_uc1) = md2BI_uc1;
  TIi(t_md2BI,m2_ucw) = md2BI_ucw;
  TIi(t_md2BI,m2_im) = md2BI_im;
  TIi(t_md2BI,m2_iw) = md2BI_iw;
  TIi(t_md2BI,m2_ix) = md2BI_ix;
  TIi(t_customObj,freeO) = customObj_freeO;
  TIi(t_customObj,freeF) = customObj_freeF;
  TIi(t_customObj,visit) = customObj_visit;
  c(BMd2,bi_before)->uc1 = before_uc1;
}
