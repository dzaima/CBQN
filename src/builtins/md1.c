#include "../core.h"
#include "../utils/mut.h"
#include "../utils/each.h"
#include "../utils/time.h"
#include "../builtins.h"



static NOINLINE B homFil1(B f, B r, B xf) {
  assert(EACH_FILLS);
  if (isPureFn(f)) {
    if (f.u==bi_eq.u || f.u==bi_ne.u || f.u==bi_feq.u) { dec(xf); return squeeze_numNew(r); }
    if (f.u==bi_fne.u) { dec(xf); return withFill(r, emptyHVec()); }
    if (!noFill(xf)) {
      if (CATCH) { freeThrown(); return r; }
      B rf = asFill(c1(f, xf));
      popCatch();
      return withFill(r, rf);
    }
  }
  dec(xf);
  return r;
}
static NOINLINE B homFil2(B f, B r, B wf, B xf) {
  assert(EACH_FILLS);
  if (isPureFn(f)) {
    if (f.u==bi_feq.u || f.u==bi_fne.u) { dec(wf); dec(xf); return squeeze_numNew(r); }
    if (!noFill(wf) && !noFill(xf)) {
      if (CATCH) { freeThrown(); return r; }
      B rf = asFill(c2(f, wf, xf));
      popCatch();
      return withFill(r, rf);
    }
  }
  dec(wf); dec(xf);
  return r;
}

B each_c1(Md1D* d, B x) { B f = d->f;
  B r, xf;
  if (EACH_FILLS) xf = getFillR(x);
  
  if (isAtm(x)) r = squeezed_unit(c1(f, x));
  else if (isFun(f)) {
    u8 rtid = RTID(f);
    if (rtid==n_ltack || rtid==n_rtack) {
      if (EACH_FILLS) dec(xf);
      return TI(x,arrD1) || IA(x)==0? x : squeeze_any(EACH_FILLS? x : withFill(x, bi_noFill));
    }
    r = eachm_fn(f, x, c(Fun,f)->c1);
  } else {
    usz ia = IA(x);
    if (isMd(f) && ia>0) { decR(x); thrM("Calling a modifier"); }
    r = taga(arr_shCopy(reshape_one(ia, inc(f)), x));
    decG(x);
  }
  
  if (EACH_FILLS) return homFil1(f, r, xf);
  else return r;
}
B tbl_c1(Md1D* d, B x) {
  return each_c1(d, x);
}

B slash_c2(B t, B w, B x);
B shape_c2(B t, B w, B x);
Arr* reshape_cycle(usz nia, usz xia, B x); // from sfns.c
static B replicate_by(usz rep, usz xia, B x) { return C2(slash, m_usz(rep), taga(arr_shVec(TI(x,slice)(incG(x), 0, xia)))); }
B tbl_c2(Md1D* d, B w, B x) { B f = d->f;
  if (isAtm(w)) w = m_unit(w);
  if (isAtm(x)) x = m_unit(x);
  ur wr = RNK(w); usz wia = IA(w);
  ur xr = RNK(x); usz xia = IA(x);
  ur rr = wr+xr;  usz ria = uszMul(wia, xia);
  if (rr<xr) thrF("𝕨𝔽⌜𝕩: Result rank too large (%i≡=𝕨, %i≡=𝕩)", wr, xr);
  
  B r;
  usz* rsh;
  
  FC2 fc2 = c2fn(f);
  Arr* ra_nosh;
  if (RARE(!isFun(f))) {
    if (isMd(f) && ria>0) thrM("Calling a modifier");
    ra_nosh = reshape_one(ria, f);
    goto nosh;
  } else if (RTID(f) == n_ltack) {
    w = squeeze_any(w);
    r = replicate_by(xia, wia, w);
    goto arith_finish;
  } else if (RTID(f) == n_rtack) {
    x = squeeze_any(x);
    ux xia = IA(x);
    if (ria<=xia) ra_nosh = TI(x,slice)(incG(x), 0, ria); // ria==0 or ria==xia; necessary as reshape_cycle doesn't handle those
    else ra_nosh = reshape_cycle(ria, xia, incG(x));
    goto nosh;
  } else if (isPervasiveDyExt(f)) {
    if (ria == 0) goto arith_empty;
    if (!TI(w,arrD1)) goto generic;
    if (TI(x,arrD1) && wia>=4 && xia<2560>>arrTypeBitsLog(TY(x))) { // arrD1 checks imply that squeeze won't change fill (and the arith call will squeeze anyway)
      B expW, expX;
      if (0) {
        arith_empty:;
        expW = taga(emptyArr(w, 1));
        expX = taga(emptyArr(x, 1));
      } else {
        assert(wia>1); // implies ria > xia, a requirement of reshape_cycle
        expW = replicate_by(xia, wia, w);
        expX = taga(arr_shVec(reshape_cycle(ria, xia, incG(x))));
      }
      r = fc2(f, expW, expX);
      arith_finish:;
      ra_nosh = RARE(!reusable(r))? cpyWithShape(r) : a(r);
      arr_shErase(ra_nosh, 1);
      goto nosh;
    } else if (xia>3) {
      SGet(w)
      M_APD_TOT(rm, ria)
      incByG(x, wia);
      for (usz wi = 0; wi < wia; wi++) APDD(rm, fc2(f, Get(w,wi), x)); // arith call will squeeze
      ra_nosh = APD_TOT_GET(rm);
      goto nosh;
    } else goto generic;
  } else {
    generic:;
    SGetU(w) SGet(x)
    
    M_HARR(r, ria)
    for (usz wi = 0; wi < wia; wi++) {
      B cw = incBy(GetU(w,wi), xia);
      for (usz xi = 0; xi < xia; xi++) HARR_ADDA(r, fc2(f, cw, Get(x,xi)));
    }
    rsh = HARR_FA(r, rr);
    r = squeeze_any(HARR_O(r).b);
  }
  if (0) {
    nosh:
    rsh = arr_shAlloc(ra_nosh, rr);
    r = taga(ra_nosh);
  }
  
  if (rsh) {
    shcpy(rsh   , SH(w), wr);
    shcpy(rsh+wr, SH(x), xr);
  }
  B wf, xf;
  if (EACH_FILLS) {
    assert(isArr(w)); wf=getFillR(w);
    assert(isArr(x)); xf=getFillR(x);
    decG(w); decG(x);
    return homFil2(f, r, wf, xf);
  } else {
    decG(w); decG(x);
    return r;
  }
}

static B eachd(B f, B w, B x) {
  if (isAtm(w) & isAtm(x)) return squeezed_unit(c2(f, w, x));
  return eachd_fn(f, w, x, c2fn(f));
}

B each_c2(Md1D* d, B w, B x) { B f = d->f;
  if (!EACH_FILLS) return eachd(f, w, x);
  B wf = getFillR(w);
  B xf = getFillR(x);
  return homFil2(f, eachd(f, w, x), wf, xf);
}

B const_c1(Md1D* d,      B x) {         dec(x); return inc(d->f); }
B const_c2(Md1D* d, B w, B x) { dec(w); dec(x); return inc(d->f); }

B swap_c1(Md1D* d,      B x) { return c2(d->f, inc(x), x); }
B swap_c2(Md1D* d, B w, B x) { return c2(d->f,     x , w); }


B timed_c2(Md1D* d, B w, B x) { B f = d->f;
  i64 am = o2i64(w);
  if (am<=0) thrM("𝕨 𝔽•_timed 𝕩: 𝕨 must be an integer greater than 0");
  incBy(x, am-1);
  FC1 fc1 = c1fn(f);
  u64 sns = nsTime();
  for (i64 i = 0; i < am; i++) dec(fc1(f, x));
  u64 ens = nsTime();
  return m_f64((ens-sns)/(1e9*am));
}
B timed_c1(Md1D* d, B x) { B f = d->f;
  u64 sns = nsTime();
  dec(c1(f, x));
  u64 ens = nsTime();
  return m_f64((ens-sns)*1e-9);
}

static void print_md1BI(FILE* f, B x) { fprintf(f, "%s", pm1_repr(NID(c(BMd1,x)))); }
static B md1BI_im(Md1D* d,      B x) { return ((BMd1*)d->m1)->im(d,    x); }
static B md1BI_iw(Md1D* d, B w, B x) { return ((BMd1*)d->m1)->iw(d, w, x); }
static B md1BI_ix(Md1D* d, B w, B x) { return ((BMd1*)d->m1)->ix(d, w, x); }
static B swap_im(Md1D* d,      B x) { return isFun(d->f)? TI(d->f,fn_is)(d->f,    x) : def_fn_is(d->f,    x); }
static B swap_iw(Md1D* d, B w, B x) { return isFun(d->f)? TI(d->f,fn_ix)(d->f, w, x) : def_fn_ix(d->f, w, x); }
static B swap_ix(Md1D* d, B w, B x) { return isFun(d->f)? TI(d->f,fn_iw)(d->f, w, x) : def_fn_iw(d->f, w, x); }

void md1_init(void) {
  TIi(t_md1BI,print) = print_md1BI;
  TIi(t_md1BI,m1_im) = md1BI_im;
  TIi(t_md1BI,m1_iw) = md1BI_iw;
  TIi(t_md1BI,m1_ix) = md1BI_ix;
  c(BMd1,bi_swap)->im = swap_im;
  c(BMd1,bi_swap)->iw = swap_iw;
  c(BMd1,bi_swap)->ix = swap_ix;
}
