#include "../core.h"
#include "../utils/talloc.h"
#include "../utils/mut.h"
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


NOINLINE B before_c1F(Md2D* d, B x, B f) { errMd(f); return c2(d->g, c(Fun,f)->c1(f,inc(x)), x); }
NOINLINE B after_c1F (Md2D* d, B x, B g) { errMd(g); return c2(d->f, x, c(Fun,g)->c1(g,inc(x))); }
B before_c1(Md2D* d, B x) { B f=d->f; return isCallable(f)? before_c1F(d, x, f) : c2(d->g, inc(f), x); }
B after_c1 (Md2D* d, B x) { B g=d->g; return isCallable(g)? after_c1F (d, x, g) : c2(d->f, x, inc(g)); }
B before_c2(Md2D* d, B w, B x) { return c2(d->g, c1i(d->f, w), x); }
B after_c2 (Md2D* d, B w, B x) { return c2(d->f, w, c1i(d->g, x)); }
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
    B fn = pick_c2(m_f64(0), fr, inc(g));
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
    B fn = pick_c2(m_f64(0), fr, inc(g));
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

static f64 req_whole(f64 f) {
  if (floor(f)!=f) thrM("⎉: 𝕘 was a fractional number");
  return f;
}
static usz check_rank_vec(B g) {
  if (!isArr(g)) thrM("⎉: Invalid 𝔾 result");
  usz gia = IA(g);
  if (!(gia>=1 && gia<=3)) thrM("⎉: 𝔾 result must have 1 to 3 elements");
  SGetU(g)
  if (!elInt(TI(g,elType))) for (i32 i = 0; i < gia; i++) req_whole(o2f(GetU(g,i)));
  return gia;
}
static ur cell_rank(f64 r, f64 k) { // ⎉k over arg rank r
  return k<0? (k+r<0? 0 : k+r) : (k>r? r : k);
}

B to_fill_cell_k(B x, ur k, char* err) { // consumes x
  B xf = getFillQ(x);
  if (noFill(xf)) xf = m_f64(0);
  ur cr = RNK(x)-k;
  usz* sh = SH(x)+k;
  usz csz = 1;
  for (usz i=0; i<cr; i++) if (mulOn(csz, sh[i])) thrF(err, x);
  MAKE_MUT(fc, csz);
  mut_fill(fc, 0, xf, csz); dec(xf);
  Arr* ca = mut_fp(fc);
  usz* csh = arr_shAlloc(ca, cr);
  if (cr>1) shcpy(csh, sh, cr);
  decG(x);
  return taga(ca);
}
static B to_fill_cell(B x, ur k) {
  return to_fill_cell_k(x, k, "⎉: Empty argument too large (%H ≡ ≢𝕩)");
}
static B merge_fill_result(B rc, ur k, usz* sh) {
  u64 rr = k; if (isArr(rc)) rr += RNK(rc);
  if (rr>UR_MAX) thrM("⎉: Result rank too large");
  B rf = getFillQ(rc);
  Arr* r = m_fillarrp(0);
  fillarr_setFill(r, rf);
  usz* rsh = arr_shAlloc(r, rr);
  if (rr>1) {
    shcpy(rsh, sh, k);
    shcpy(rsh+k, SH(rc), rr-k);
  }
  dec(rc);
  return taga(r);
}
static B empty_frame(usz* xsh, ur k) {
  HArr_p f = m_harrUp(0);
  Arr *a = (Arr*)f.c;
  if (k <= 1) arr_shVec(a); else shcpy(arr_shAlloc(a,k), xsh, k);
  return f.b;
}
static B rank2_empty(B f, B w, ur wk, B x, ur xk) {
  B fa = wk>xk?w:x;
  ur k = wk>xk?wk:xk;
  usz* sh = SH(fa);
  usz s0=0; ShArr* s=NULL; ur sho=RNK(fa)>1;
  if (!sho) { s0=sh[0]; sh=&s0; } else { s=ptr_inc(shObj(fa)); }
  if (!isPureFn(f) || !CATCH_ERRORS) { dec(w); dec(x); goto empty; }
  B r;
  if (wk) w = to_fill_cell(w, wk);
  if (xk) x = to_fill_cell(x, xk);
  if (CATCH) { empty:
    r = empty_frame(sh, k);
  } else {
    B rc = c2(f, w, x);
    popCatch();
    r = merge_fill_result(rc, k, sh);
  }
  if (sho) ptr_dec(s);
  return r;
}

B rank_c1(Md2D* d, B x) { B f = d->f; B g = d->g;
  f64 kf;
  bool gf = isFun(g);
  if (RARE(gf)) g = c1(g, inc(x));
  if (LIKELY(isNum(g))) {
    kf = req_whole(o2fG(g));
  } else {
    usz gia = check_rank_vec(g);
    SGetU(g); kf = GetU(g, gia==2).f;
  }
  if (gf) dec(g);
  
  if (isAtm(x) || RNK(x)==0) {
    B r = c1(f, x);
    return isAtm(r)? m_atomUnit(r) : r;
  }
  i32 xr = RNK(x);
  ur cr = cell_rank(xr, kf);
  i32 k = xr - cr;
  if (Q_BI(f,lt) && IA(x)!=0 && RNK(x)>1) return toKCells(x, k);
  
  usz* xsh = SH(x);
  usz cam = shProd(xsh, 0, k);
  if (cam == 0) {
    usz s0=0; ShArr* s=NULL;
    if (xr<=1) { s0=xsh[0]; xsh=&s0; } else { s=ptr_inc(shObj(x)); }
    if (!isPureFn(f) || !CATCH_ERRORS) { decG(x); goto empty; }
    B cf = to_fill_cell(x, k);
    B r;
    if (CATCH) { empty:
      r = empty_frame(xsh, k);
    } else {
      B rc = c1(f, cf);
      popCatch();
      r = merge_fill_result(rc, k, xsh);
    }
    if (xr>1) ptr_dec(s);
    return r;
  }
  usz csz = shProd(xsh, k, xr);
  ShArr* csh ONLY_GCC(= NULL);
  if (cr>1) {
    csh = m_shArr(cr);
    shcpy(csh->a, xsh+k, cr);
  }
  
  
  BSS2A slice = TI(x,slice);
  M_HARR(r, cam);
  usz p = 0;
  for (usz i = 0; i < cam; i++) {
    Arr* s = arr_shSetI(slice(incG(x), p, csz), cr, csh);
    HARR_ADD(r, i, c1(f, taga(s)));
    p+= csz;
  }
  
  if (cr>1) ptr_dec(csh);
  usz* rsh = HARR_FA(r, k);
  if (k>1) shcpy(rsh, xsh, k);
  
  decG(x);
  return bqn_merge(HARR_O(r).b);
}
B rank_c2(Md2D* d, B w, B x) { B f = d->f; B g = d->g;
  f64 wf, xf;
  bool gf = isFun(g);
  if (RARE(gf)) g = c2(g, inc(w), inc(x));
  if (LIKELY(isNum(g))) {
    wf = xf = req_whole(o2fG(g));
  } else {
    usz gia = check_rank_vec(g);
    SGetU(g);
    wf = GetU(g, gia<2?0:gia-2).f;
    xf = GetU(g, gia-1).f;
  }

  ur wr = isAtm(w) ? 0 : RNK(w); ur wc = cell_rank(wr, wf);
  ur xr = isAtm(x) ? 0 : RNK(x); ur xc = cell_rank(xr, xf);

  B r;
  if (wr == wc) {
    if (xr == xc) {
      if (gf) dec(g);
      r = c2(f, w, x);
      return isAtm(r)? m_atomUnit(r) : r;
    } else {
      i32 k = xr - xc;
      usz* xsh = SH(x);
      usz cam = shProd(xsh, 0, k);
      if (cam == 0) return rank2_empty(f, w, 0, x, k);
      usz csz = shProd(xsh, k, xr);
      ShArr* csh;
      if (xc>1) { csh=m_shArr(xc); shcpy(csh->a, xsh+k, xc); }

      BSS2A slice = TI(x,slice);
      M_HARR(r, cam);
      usz p = 0;
      for (usz i = 0; i < cam; i++) {
        Arr* s = arr_shSetI(slice(incG(x), p, csz), xc, csh);
        HARR_ADD(r, i, c2(f, inc(w), taga(s)));
        p+= csz;
      }

      if (xc>1) ptr_dec(csh);
      usz* rsh = HARR_FA(r, k);
      if (k>1) shcpy(rsh, xsh, k);

      dec(w); decG(x); r = HARR_O(r).b;
    }
  } else if (xr == xc) {
    i32 k = wr - wc;
    usz* wsh = SH(w);
    usz cam = shProd(wsh, 0, k);
    if (cam == 0) return rank2_empty(f, w, k, x, 0);
    usz csz = shProd(wsh, k, wr);
    ShArr* csh;
    if (wc>1) { csh=m_shArr(wc); shcpy(csh->a, wsh+k, wc); }

    BSS2A slice = TI(w,slice);
    M_HARR(r, cam);
    usz p = 0;
    for (usz i = 0; i < cam; i++) {
      Arr* s = arr_shSetI(slice(incG(w), p, csz), wc, csh);
      HARR_ADD(r, i, c2(f, taga(s), inc(x)));
      p+= csz;
    }

    if (wc>1) ptr_dec(csh);
    usz* rsh = HARR_FA(r, k);
    if (k>1) shcpy(rsh, wsh, k);

    decG(w); dec(x); r = HARR_O(r).b;
  } else {
    i32 wk = wr - wc; usz* wsh = SH(w);
    i32 xk = xr - xc; usz* xsh = SH(x);
    i32 k=wk, zk=xk; if (k>zk) { i32 t=k; k=zk; zk=t; }
    usz* zsh = wk>xk? wsh : xsh;

    usz cam = 1; for (usz i =  0; i <  k; i++) {
      usz wl = wsh[i], xl = xsh[i];
      if (wl != xl) thrF("⎉: Argument frames don't agree (%H ≡ ≢𝕨, %H ≡ ≢𝕩, common frame of %i axes)", w, x, k);
      cam*= wsh[i];
    }
    usz ext = shProd(zsh,  k, zk);
    cam *= ext;
    if (cam == 0) return rank2_empty(f, w, wk, x, xk);
    usz wsz = shProd(wsh, wk, wr);
    usz xsz = shProd(xsh, xk, xr);

    ShArr* wcs; if (wc>1) { wcs=m_shArr(wc); shcpy(wcs->a, wsh+wk, wc); }
    ShArr* xcs; if (xc>1) { xcs=m_shArr(xc); shcpy(xcs->a, xsh+xk, xc); }

    BSS2A wslice = TI(w,slice);
    BSS2A xslice = TI(x,slice);
    M_HARR(r, cam);
    usz wp = 0, xp = 0;
    #define CELL(wx) \
      Arr* wx##s = arr_shSetI(wx##slice(incG(wx), wx##p, wx##sz), wx##c, wx##cs); \
      wx##p+= wx##sz
    #define F(W,X) HARR_ADD(r, i, c2(f, W, X))
    if (ext == 1) {
      for (usz i = 0; i < cam; i++) {
        CELL(w); CELL(x); F(taga(ws), taga(xs));
      }
    } else if (wk < xk) {
      for (usz i = 0; i < cam; ) {
        CELL(w); B wb=taga(ws);
        for (usz e = i+ext; i < e; i++) { CELL(x); F(incG(wb), taga(xs)); }
        dec(wb);
      }
    } else {
      for (usz i = 0; i < cam; ) {
        CELL(x); B xb=taga(xs);
        for (usz e = i+ext; i < e; i++) { CELL(w); F(taga(ws), incG(xb)); }
        dec(xb);
      }
    }
    #undef CELL
    #undef F

    if (wc>1) ptr_dec(wcs);
    if (xc>1) ptr_dec(xcs);
    usz* rsh = HARR_FA(r, zk);
    if (zk>1) shcpy(rsh, zsh, zk);

    decG(w); decG(x); r = HARR_O(r).b;
  }
  if (gf) dec(g);
  return bqn_merge(r);
}




extern B rt_depth;
B depth_c1(Md2D* d,      B x) { SLOW3("!F⚇𝕨 𝕩", d->g, x, d->f); return m2c1(rt_depth, d->f, d->g, x); }
B depth_c2(Md2D* d, B w, B x) { SLOW3("!𝕨 𝔽⚇f 𝕩", w, x, d->g);  return m2c2(rt_depth, d->f, d->g, w, x); }


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
  c(BMd2,bi_before)->uc1 = before_uc1;
}
