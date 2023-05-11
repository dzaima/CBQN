#include "../core.h"
#include "../utils/each.h"
#include "../utils/file.h"
#include "../utils/time.h"
#include "../utils/calls.h"
#include "../builtins.h"



static NOINLINE B homFil1(B f, B r, B xf) {
  assert(EACH_FILLS);
  if (isPureFn(f)) {
    if (f.u==bi_eq.u || f.u==bi_ne.u || f.u==bi_feq.u) { dec(xf); return num_squeeze(r); }
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
    if (f.u==bi_feq.u || f.u==bi_fne.u) { dec(wf); dec(xf); return num_squeeze(r); }
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
  
  if (isAtm(x)) r = m_hunit(c1(f, x));
  else if (isFun(f)) r = eachm_fn(f, x, c(Fun,f)->c1);
  else {
    if (isMd(f)) if (isAtm(x) || IA(x)) { decR(x); thrM("Calling a modifier"); }
    usz ia = IA(x);
    MAKE_MUT(rm, ia);
    mut_fill(rm, 0, f, ia);
    r = mut_fcd(rm, x);
  }
  
  if (EACH_FILLS) return homFil1(f, r, xf);
  else return r;
}
B tbl_c1(Md1D* d, B x) {
  return each_c1(d, x);
}

B slash_c2(B t, B w, B x);
B shape_c2(B t, B w, B x);
B tbl_c2(Md1D* d, B w, B x) { B f = d->f;
  if (isAtm(w)) w = m_unit(w);
  if (isAtm(x)) x = m_unit(x);
  ur wr = RNK(w); usz wia = IA(w);
  ur xr = RNK(x); usz xia = IA(x);
  ur rr = wr+xr;  usz ria = uszMul(wia, xia);
  if (rr<xr) thrF("⌜: Result rank too large (%i≡=𝕨, %i≡=𝕩)", wr, xr);
  
  B r;
  usz* rsh;
  
  FC2 fc2 = c2fn(f);
  if (isFun(f) && TI(w,arrD1) && isPervasiveDyExt(f)) {
    if (TI(x,arrD1) && wia>130 && xia<2560>>arrTypeBitsLog(TY(x))) {
      Arr* wd = arr_shVec(TI(w,slice)(incG(w), 0, wia));
      r = fc2(f, C2(slash, m_i32(xia), taga(wd)), C2(shape, m_f64(ria), incG(x)));
      arr_shErase(a(r), 1);
    } else if (xia>7 && wia>0) {
      SGet(w)
      M_APD_TOT(rm, ria)
      incByG(x, wia);
      for (usz wi = 0; wi < wia; wi++) APDD(rm, fc2(f, Get(w,wi), x));
      r = taga(arr_shVec(APD_TOT_GET(rm)));
    } else goto generic;
    rsh = arr_shAlloc(a(r), rr);
  } else {
    generic:;
    SGetU(w) SGet(x)
    
    M_HARR(r, ria)
    for (usz wi = 0; wi < wia; wi++) {
      B cw = incBy(GetU(w,wi), xia);
      for (usz xi = 0; xi < xia; xi++) HARR_ADDA(r, fc2(f, cw, Get(x,xi)));
    }
    rsh = HARR_FA(r, rr);
    r = HARR_O(r).b;
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
  if (isAtm(w) & isAtm(x)) return m_hunit(c2(f, w, x));
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
  if (am<=0) thrM("•_timed: 𝕨 must be an integer greater than 1");
  incBy(x, am);
  u64 sns = nsTime();
  for (i64 i = 0; i < am; i++) dec(c1(f, x));
  u64 ens = nsTime();
  return m_f64((ens-sns)/(1e9*am));
}
B timed_c1(Md1D* d, B x) { B f = d->f;
  u64 sns = nsTime();
  dec(c1(f, x));
  u64 ens = nsTime();
  return m_f64((ens-sns)*1e-9);
}

static void print_md1BI(FILE* f, B x) { fprintf(f, "%s", pm1_repr(c(Md1,x)->extra)); }
static B md1BI_im(Md1D* d,      B x) { return ((BMd1*)d->m1)->im(d,    x); }
static B md1BI_iw(Md1D* d, B w, B x) { return ((BMd1*)d->m1)->iw(d, w, x); }
static B md1BI_ix(Md1D* d, B w, B x) { return ((BMd1*)d->m1)->ix(d, w, x); }

B swap_im(Md1D* d, B x) { return isFun(d->f)? TI(d->f,fn_is)(d->f, x): def_fn_is(d->f, x); }

void md1_init(void) {
  TIi(t_md1BI,print) = print_md1BI;
  TIi(t_md1BI,m1_im) = md1BI_im;
  TIi(t_md1BI,m1_iw) = md1BI_iw;
  TIi(t_md1BI,m1_ix) = md1BI_ix;
  c(BMd1,bi_swap)->im = swap_im;
}
