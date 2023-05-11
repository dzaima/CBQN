#include "../core.h"
#include "../utils/each.h"
#include "../utils/talloc.h"
#include "../utils/mut.h"
#include "../nfns.h"
#include "../builtins.h"
#include <math.h>

B md2BI_uc1(Md2* t, B o, B f, B g,      B x) { return ((BMd2*)t)->uc1(t, o, f, g,    x); }
B md2BI_ucw(Md2* t, B o, B f, B g, B w, B x) { return ((BMd2*)t)->ucw(t, o, f, g, w, x); }


B val_c1(Md2D* d,      B x) { return c1(d->f,   x); }
B val_c2(Md2D* d, B w, B x) { return c2(d->g, w,x); }

#if CATCH_ERRORS && !BI_CATCH_DISABLED
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
void pushRe(void) {
  ReObj* o = m_customObj(sizeof(ReObj), re_visit, re_freeO);
  o->msg = lastErrMsg;
  gsAdd(tag(o,OBJ_TAG));
  
  lastErrMsg = inc(thrownMsg);
  freeThrown();
}
B catch_c1(Md2D* d,      B x) { if(CATCH) { pushRe(); B r = c1(d->g,   x); dec(gsPop()); return r; } B r = c1(d->f,        inc(x)); popCatch();         dec(x); return r; }
B catch_c2(Md2D* d, B w, B x) { if(CATCH) { pushRe(); B r = c2(d->g, w,x); dec(gsPop()); return r; } B r = c2(d->f, inc(w),inc(x)); popCatch(); dec(w); dec(x); return r; }
#else
B fillBy_c1(Md2D* d,      B x) { return c1(d->f,   x); }
B fillBy_c2(Md2D* d, B w, B x) { return c2(d->f, w,x); }
B catch_c1(Md2D* d,      B x) { return c1(d->f,   x); }
B catch_c2(Md2D* d, B w, B x) { return c2(d->f, w,x); }
#endif

extern B rt_undo;
void repeat_bounds(i64* bound, B g) { // doesn't consume
  #define UPD_BOUNDS(I) ({ i64 i_ = (I); if (i_<bound[0]) bound[0] = i_; if (i_>bound[1]) bound[1] = i_; })
  if (isArr(g)) {
    usz ia = IA(g);
    u8 xe = TI(g,elType);
    if (elNum(xe)) {
      incG(g);
      if (xe<el_i32) g = taga(cpyI32Arr(g));
      if (xe==el_f64) { f64* gp = f64any_ptr(g); NOUNROLL for (usz i=0; i<ia; i++) UPD_BOUNDS(o2i64(b(gp[i]))); }
      else            { i32* gp = i32any_ptr(g); NOUNROLL for (usz i=0; i<ia; i++) UPD_BOUNDS(        gp[i]  ); }
      decG(g);
    } else {
      SGetU(g)
      for (usz i = 0; i < ia; i++) repeat_bounds(bound, GetU(g, i));
    }
  } else if (isNum(g)) {
    UPD_BOUNDS(o2i64(g));
  } else thrM("⍟: 𝔾 contained a non-number atom");
}
B repeat_replaceR(B g, B* q);
FORCE_INLINE B repeat_replace(B g, B* q) { // doesn't consume
  if (isArr(g)) return repeat_replaceR(g, q);
  else return inc(q[o2i64G(g)]);
}
NOINLINE B repeat_replaceR(B g, B* q) {
  SGetU(g)
  usz ia = IA(g);
  M_HARR(r, ia);
  for (usz i = 0; i < ia; i++) HARR_ADD(r, i, repeat_replace(GetU(g,i), q));
  return HARR_FC(r, g);
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
    B fi = m1_d(incG(bi_undo), inc(f));            \
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


NOINLINE B before_c1F(Md2D* d, B x, B f) { errMd(f); return c2(d->g, c1G(f,inc(x)), x); }
NOINLINE B after_c1F (Md2D* d, B x, B g) { errMd(g); return c2(d->f, x, c1G(g,inc(x))); }
B before_c1(Md2D* d, B x) { B f=d->f; return isCallable(f)? before_c1F(d, x, f) : c2(d->g, inc(f), x); }
B after_c1 (Md2D* d, B x) { B g=d->g; return isCallable(g)? after_c1F (d, x, g) : c2(d->f, x, inc(g)); }
B before_c2(Md2D* d, B w, B x) { return c2(d->g, c1I(d->f, w), x); }
B after_c2 (Md2D* d, B w, B x) { return c2(d->f, w, c1I(d->g, x)); }
B atop_c1(Md2D* d,      B x) { return c1(d->f, c1(d->g,    x)); }
B atop_c2(Md2D* d, B w, B x) { return c1(d->f, c2(d->g, w, x)); }
B over_c1(Md2D* d,      B x) { return c1(d->f, c1(d->g,    x)); }
B over_c2(Md2D* d, B w, B x) { B xr=c1(d->g, x); return c2(d->f, c1(d->g, w), xr); }

B pick_c2(B t, B w, B x);

B cond_c1(Md2D* d, B x) { B f=d->f; B g=d->g;
  B fr = c1iX(f, x);
  if (isNum(fr)) {
    if (isAtm(g)||RNK(g)!=1) thrM("◶: 𝕘 must have rank 1");
    usz fri = WRAP(o2i64(fr), IA(g), thrM("◶: 𝔽 out of bounds of 𝕘"));
    return c1(IGetU(g, fri), x);
  } else {
    B fn = C2(pick, fr, inc(g));
    B r = c1(fn, x);
    dec(fn);
    return r;
  }
}
B cond_c2(Md2D* d, B w, B x) { B g=d->g;
  B fr = c2iWX(d->f, w, x);
  if (isNum(fr)) {
    if (isAtm(g)||RNK(g)!=1) thrM("◶: 𝕘 must have rank 1");
    usz fri = WRAP(o2i64(fr), IA(g), thrM("◶: 𝔽 out of bounds of 𝕘"));
    return c2(IGetU(g, fri), w, x);
  } else {
    B fn = C2(pick, fr, inc(g));
    B r = c2(fn, w, x);
    dec(fn);
    return r;
  }
}

B under_c1(Md2D* d, B x) { B f=d->f; B g=d->g;
  return (LIKELY(isVal(g))? TI(g,fn_uc1) : def_fn_uc1)(g, f, x);
}
B under_c2(Md2D* d, B w, B x) { B f=d->f; B g=d->g;
  B f2 = m2_d(incG(bi_before), c1(g, w), inc(f));
  B r = (LIKELY(isVal(g))? TI(g,fn_uc1) : def_fn_uc1)(g, f2, x);
  dec(f2);
  return r;
}

B before_uc1(Md2* t, B o, B f, B g, B x) {
  if (!isFun(g)) return def_m2_uc1(t, o, f, g, x);
  return TI(g,fn_ucw)(g, o, inc(f), x);
}


B while_c1(Md2D* d, B x) { B f=d->f; B g=d->g;
  FC1 ff = c1fn(f);
  FC1 gf = c1fn(g);
  while (o2b(gf(g,inc(x)))) x = ff(f, x);
  return x;
}
B while_c2(Md2D* d, B w, B x) { B f=d->f; B g=d->g;
  FC2 ff = c2fn(f);
  FC2 gf = c2fn(g);
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



// TODO fills on EACH_FILLS
B depthf_c1(B t, B x) {
  if (isArr(x)) return eachm_fn(t, x, depthf_c1);
  else return c1(t, x);
}
B depthf_c2(B t, B w, B x) {
  if (isArr(w) || isArr(x)) return eachd_fn(t, w, x, depthf_c2);
  else return c2(t, w, x);
}
extern B rt_depth;
B depth_c1(Md2D* d, B x) {
  if (isF64(d->g) && o2fG(d->g)==0) {
    if (isArr(x)) return eachm_fn(d->f, x, depthf_c1);
    else return c1(d->f, x);
  }
  SLOW3("!F⚇𝕨 𝕩", d->g, x, d->f);
  return m2c1(rt_depth, d->f, d->g, x);
}
B depth_c2(Md2D* d, B w, B x) {
  if (isF64(d->g) && o2fG(d->g)==0) {
    if (isArr(w) || isArr(x)) return eachd_fn(d->f, w, x, depthf_c2);
    else return c2(d->f, w, x);
  }
  SLOW3("!𝕨 𝔽⚇f 𝕩", w, x, d->g);
  return m2c2(rt_depth, d->f, d->g, w, x);
}


static void print_md2BI(FILE* f, B x) { fprintf(f, "%s", pm2_repr(c(Md1,x)->extra)); }
static B md2BI_im(Md2D* d,      B x) { return ((BMd2*)d->m2)->im(d,    x); }
static B md2BI_iw(Md2D* d, B w, B x) { return ((BMd2*)d->m2)->iw(d, w, x); }
static B md2BI_ix(Md2D* d, B w, B x) { return ((BMd2*)d->m2)->ix(d, w, x); }
void md2_init(void) {
  TIi(t_md2BI,print) = print_md2BI;
  TIi(t_md2BI,m2_uc1) = md2BI_uc1;
  TIi(t_md2BI,m2_ucw) = md2BI_ucw;
  TIi(t_md2BI,m2_im) = md2BI_im;
  TIi(t_md2BI,m2_iw) = md2BI_iw;
  TIi(t_md2BI,m2_ix) = md2BI_ix;
  c(BMd2,bi_before)->uc1 = before_uc1;
}
