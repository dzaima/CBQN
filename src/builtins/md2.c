#include "core.h"
#include "utils/each.h"
#include "utils/talloc.h"
#include "utils/calls.h"
#include "utils/nfns.h"
#include "builtins.h"

B val_c1(Md2D* d,      B x) { return c1(d->f,   x); }
B val_c2(Md2D* d, B w, B x) { return c2(d->g, w,x); }

#if SEMANTIC_CATCH
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
#else
B fillBy_c1(Md2D* d,      B x) { return c1(d->f,   x); }
B fillBy_c2(Md2D* d, B w, B x) { return c2(d->f, w,x); }
#endif

#if defined(SEMANTIC_CATCH_BI)? SEMANTIC_CATCH_BI : (SEMANTIC_CATCH && USE_SETJMP)
extern GLOBAL B lastErrMsg; // sysfn.c
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
B catch_c1(Md2D* d,      B x) { return c1(d->f,   x); }
B catch_c2(Md2D* d, B w, B x) { return c2(d->f, w,x); }
#endif

static NOINLINE NORETURN void repeat_bad_num() {
  thrM("⍟: 𝔾 contained non-integer or integer was out of range");
}

extern GLOBAL B rt_undo;
void repeat_bounds(i64* bound, B g) { // doesn't consume
  #define UPD_BOUNDS(B,I) ({ i64 i_ = (I); if (i_<bound[0]) bound[0] = i_; if (i_>bound[1]) bound[1] = i_; })
  if (isArr(g)) {
    usz ia = IA(g);
    if (ia == 0) return;
    u8 ge = TI(g,elType);
    if (elNum(ge)) {
      i64 bres[2];
      if (!getRange_fns[ge](tyany_ptr(g), bres, ia)) repeat_bad_num();
      if (bres[0]<bound[0]) bound[0] = bres[0];
      if (bres[1]>bound[1]) bound[1] = bres[1];
    } else {
      SGetU(g)
      for (usz i = 0; i < ia; i++) repeat_bounds(bound, GetU(g, i));
    }
  } else if (isNum(g)) {
    i64 c;
    if (!q_i64(&c, g)) repeat_bad_num();
    if (c<bound[0]) bound[0] = c;
    if (c>bound[1]) bound[1] = c;
  } else thrM("⍟: 𝔾 contained non-number");
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
  return squeeze_any(HARR_FC(r, g));
}
#define REPEAT_GEN(CALL, END)                      \
  i64 bound[2] = {0,0};                            \
  repeat_bounds(bound, g);                         \
  i64 min=-(u64)bound[0]; i64 max=bound[1];        \
  if ((min|max) >> 48 != 0) repeat_bad_num();      \
  TALLOC(B, all, min+max+1);                       \
  B* q = all+min;                                  \
  q[0] = inc(x);                                   \
  if (min) {                                       \
    B x2 = inc(x);                                 \
    B fi = m1_d(incG(bi_undo), inc(f));            \
    for (i64 i = 0; i < min; i++) q[-1-i] = inc(x2 = CALL(fi, x2)); \
    dec(x2);                                       \
    dec(fi);                                       \
  }                                                \
  for (i64 i = 0; i < max; i++) q[i+1] = inc(x = CALL(f, x)); \
  dec(x);                                          \
  B r = repeat_replace(g, q);                      \
  dec(g);                                          \
  for (i64 i = 0; i < min+max+1; i++) dec(all[i]); \
  END; TFREE(all);                                 \
  return r;

extern B couple_powm(i64, B);
extern B select_powm(i64, B);
extern B drop_powd(i64, B, B);
extern B reverse_powd(i64, B, B);
extern B transp_powm(i64, B);
extern B transp_powd(i64, B, B);
extern B shift_powm(bool, i64, B);
extern B shift_powd(bool, i64, B, B);
extern B join_powd(bool, i64, B, B);
B repeat_c2(Md2D* d, B w, B x);

B repeat_c1(Md2D* d, B x) {
  B g = c1(d->g, inc(x));
  B f = d->f;
  if (isNum(g)) {
    i64 am = o2i64(g);
    if ((u64)am <= 1) goto basic;
    if (isFun(f)) { u8 rtid = RTID(f); switch (rtid) {
      case n_ltack: case n_rtack:
        am = 0; break;
      case n_stile: case n_shape:
      case n_indexOf: case n_find: case n_and: case n_or:
        am = IMIN(1, am); break;
      case n_eq: case n_ne: case n_feq:
        am = IMIN(2, am); break;
      case n_fne:
        am = IMIN(3, am); break;
      case n_reverse:
        am &= 1;
        if (am==0 && (isAtm(x) || RNK(x)==0)) thrM("⌽𝕩: 𝕩 cannot be a unit");
        break;
      case n_sub: case n_add: { // 1 or 2 times needed in general to account for fills
        i64 b = am & 1;
        am = isArr(x)&&elNum(TI(x,elType)) ? b : 2-b;
      } break;
      case n_not: { // 2 or 3 times needed for floats (e.g. 9.007199254740994e15)
        i64 b = am & 1;
        am = isArr(x)&&elInt(TI(x,elType)) ? b : am==-1 ? 1 : 2+b;
      } break;
      case n_couple: return couple_powm(am, x);
      case n_select: if (am<0) goto gen; return select_powm(am, x);
      case n_transp: return transp_powm(am, x);
      case n_shiftb: case n_shifta:
        if (am<0) goto gen;
        return shift_powm(rtid==n_shifta, am, x);
      case (u8)RTID_NONE:
        if (TY(f)==t_md2D) {
          Md2D* fd = c(Md2D,f);
          if (PRTID(fd->m2)==n_before && !isCallable(fd->f)) {
            Md2D dm; dm.m2=d->m2; dm.f=fd->g; dm.g=g;
            return repeat_c2(&dm, inc(fd->f), x);
          }
          if (PRTID(fd->m2)==n_after && isFun(fd->f) && RTID(fd->f)==n_join && !isCallable(fd->g)) {
            if (am<0) goto gen;
            return join_powd(1, am, inc(fd->g), x);
          }
        }
    }}
    if (am < 0) goto gen;
    basic:;
    for (i64 i = 0; i < am; i++) x = c1(f, x);
    return x;
  }
  gen:;
  REPEAT_GEN(c1, {});
}

B repeat_c2(Md2D* d, B w, B x) {
  B g = c2(d->g, inc(w), inc(x));
  B f = d->f;
  if (isNum(g)) {
    i64 am = o2i64(g);
    if ((u64)am <= 1) goto basic;
    if (isFun(f)) { u8 rtid = RTID(f); switch (rtid) {
      case n_rtack: am = 0; break;
      case n_ltack: am = am<0 ? -1 : 1; break;
      case n_floor: case n_ceil: case n_shape: case n_take:
        am = IMIN(1, am); break;
      // For comparisons, first application returns a boolean
      // Then for atom e in w, e⊸Cmp is a monadic boolean function 01⊢¬
      // All these repeat at 2 iterations; all but ¬ are idempotent
      // For < and ≤, ¬ is not possible because e.g. (e<0) ≤ (e<1)
      case n_le: case n_lt:
        am = IMIN(2, am); break;
      case n_gt: case n_ge: case n_eq: case n_ne: case n_feq: case n_fne:
        if (am<0) goto gen; // else fallthrough
      case n_sub: // 2 or 3 times needed, checked by bqn-smt
        am = am==-1 ? -1 : 2+(am&1); break;
      case n_drop: if (am<0) goto gen; return drop_powd(am, w, x);
      case n_reverse: return reverse_powd(am, w, x);
      case n_transp: return transp_powd(am, w, x);
      case n_join: if (am<0) goto gen; return join_powd(0, am, w, x);
      case n_shiftb: case n_shifta:
        if (am<0) goto gen;
        return shift_powd(rtid==n_shifta, am, w, x);
      case (u8)RTID_NONE:
        if (TY(f)==t_md1D) {
          Md1D* fd = c(Md1D,f); B ff = fd->f;
          if (PRTID(fd->m1)==n_swap && isFun(ff) && RTID(ff)==n_join) {
            if (am<0) goto gen;
            return join_powd(1, am, w, x);
          }
        } else if (TY(f)==t_md2D) {
          Md2D* fd = c(Md2D,f); B fg = fd->g;
          if (PRTID(fd->m2)==n_atop && isFun(fg) && RTID(fg)==n_rtack) {
            Md2D dm; dm.m2=d->m2; dm.f=fd->f; dm.g=g;
            dec(w); return repeat_c1(&dm, x);
          }
        }
    }}
    if (am < 0) goto gen;
    basic:;
    for (i64 i = 0; i < am; i++) x = c2(f, inc(w), x);
    dec(w);
    return x;
  }
  gen:;
  #define CALL(F,X) c2(F, inc(w), X)
  REPEAT_GEN(CALL, dec(w));
  #undef CALL
}

B beforeConst_c1(Md2D* d, B x) { return c2(d->g, inc(d->f), x); }

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
    if (isAtm(g)||RNK(g)!=1) thrM("𝔽◶𝕘𝕩: 𝕘 must have rank 1 when index is a number");
    usz fri = WRAP(o2i64(fr), IA(g), thrM("𝔽◶𝕘𝕩: Index out of bounds of 𝕘"));
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
    if (isAtm(g)||RNK(g)!=1) thrM("𝕨𝔽◶𝕘𝕩: 𝕘 must have rank 1 when index is a number");
    usz fri = WRAP(o2i64(fr), IA(g), thrM("𝕨𝔽◶𝕘𝕩: Index out of bounds of 𝕘"));
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
#if SINGELI_SIMD
B arr_blend(B m, B w, B x, ux ia);
#endif
B under_c2(Md2D* d, B w, B x) { B f=d->f; B g=d->g;
  #if SINGELI_SIMD
  if (d->f.u == bi_ltack.u && isFun(d->g) && TY(d->g) == t_md2D) {
    Md2D* g = c(Md2D, d->g);
    if (PRTID(g->m2) == n_before && g->g.u == bi_slash.u) {
      B mask = g->f;
      if (isAtm(mask) || RNK(mask)!=1 || TI(mask,elType)!=el_bit) goto not_blend;
      usz ia = IA(mask);
      if (isAtm(w) || RNK(w)!=1 || IA(w)!=ia) goto not_blend;
      if (isAtm(x) || RNK(x)!=1 || IA(x)!=ia) goto not_blend;
      return arr_blend(mask, w, x, ia);
    }
    not_blend:;
  }
  #endif
  
  B f2 = m2_d(incG(bi_beforeConst), c1(g, w), inc(f));
  B r = (LIKELY(isVal(g))? TI(g,fn_uc1) : def_fn_uc1)(g, f2, x);
  dec(f2);
  return r;
}

B before_uc1(Md2* t, B o, B f, B g, B x) {
  if (!isFun(g) || isCallable(f)) return def_m2_uc1(t, o, f, g, x);
  return TI(g,fn_ucw)(g, o, inc(f), x);
}
B before_im(Md2D* d, B x) { return isFun(d->g) && !isCallable(d->f)? TI(d->g,fn_ix)(d->g, inc(d->f), x) : def_m2_im(d, x); }
B after_im (Md2D* d, B x) { return isFun(d->f) && !isCallable(d->g)? TI(d->f,fn_iw)(d->f, inc(d->g), x) : def_m2_im(d, x); }


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
extern GLOBAL B rt_depth;
B depth_c1(Md2D* d, B x) {
  if (isF64(d->g) && o2fG(d->g)==0) return depthf_c1(d->f, x);
  SLOW3("!𝔽⚇𝕘 𝕩", d->g, x, d->f);
  return m2c1(rt_depth, d->f, d->g, x);
}
B depth_c2(Md2D* d, B w, B x) {
  if (isF64(d->g) && o2fG(d->g)==0) return depthf_c2(d->f, w, x);
  SLOW3("!𝕨 𝔽⚇𝕘 𝕩", w, x, d->g);
  return m2c2(rt_depth, d->f, d->g, w, x);
}


static void print_md2BI(FILE* f, B x) { fprintf(f, "%s", pm2_repr(NID(c(BMd2,x)))); }
static B md2BI_im(Md2D* d,      B x) { return ((BMd2*)d->m2)->im(d,    x); }
static B md2BI_iw(Md2D* d, B w, B x) { return ((BMd2*)d->m2)->iw(d, w, x); }
static B md2BI_ix(Md2D* d, B w, B x) { return ((BMd2*)d->m2)->ix(d, w, x); }
static B md2BI_uc1(Md2* t, B o, B f, B g,      B x) { return ((BMd2*)t)->uc1(t, o, f, g,    x); }
static B md2BI_ucw(Md2* t, B o, B f, B g, B w, B x) { return ((BMd2*)t)->ucw(t, o, f, g, w, x); }

void md2_init(void) {
  TIi(t_md2BI,print) = print_md2BI;
  TIi(t_md2BI,m2_im) = md2BI_im;
  TIi(t_md2BI,m2_iw) = md2BI_iw;
  TIi(t_md2BI,m2_ix) = md2BI_ix;
  TIi(t_md2BI,m2_uc1) = md2BI_uc1;
  TIi(t_md2BI,m2_ucw) = md2BI_ucw;
  c(BMd2,bi_before)->uc1 = before_uc1;
  c(BMd2,bi_after)->im = after_im;
  c(BMd2,bi_before)->im = before_im;
}
