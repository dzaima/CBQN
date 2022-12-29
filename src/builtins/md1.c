#include "../core.h"
#include "../utils/each.h"
#include "../utils/file.h"
#include "../utils/time.h"
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
  if (EACH_FILLS) xf = getFillQ(x);
  
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

B slash_c2(B f, B w, B x);
B shape_c2(B f, B w, B x);
B tbl_c2(Md1D* d, B w, B x) { B f = d->f;
  if (isAtm(w)) w = m_atomUnit(w);
  if (isAtm(x)) x = m_atomUnit(x);
  ur wr = RNK(w); usz wia = IA(w);
  ur xr = RNK(x); usz xia = IA(x);
  ur rr = wr+xr;  usz ria = uszMul(wia, xia);
  if (rr<xr) thrF("⌜: Result rank too large (%i≡=𝕨, %i≡=𝕩)", wr, xr);
  
  B r;
  usz* rsh;
  
  BBB2B fc2 = c2fn(f);
  if (isFun(f) && isPervasiveDy(f) && TI(w,arrD1)) {
    if (TI(x,arrD1) && wia>130 && xia<2560>>arrTypeBitsLog(TY(x))) {
      Arr* wd = arr_shVec(TI(w,slice)(incG(w), 0, wia));
      r = fc2(f, slash_c2(f, m_i32(xia), taga(wd)), shape_c2(f, m_f64(ria), incG(x)));
    } else if (xia>7) {
      SGet(w)
      M_HARR(r, wia)
      for (usz wi = 0; wi < wia; wi++) HARR_ADD(r, wi, fc2(f, Get(w,wi), incG(x)));
      r = bqn_merge(HARR_FV(r));
    } else goto generic;
    if (RNK(r)>1) {
      SRNK(r, 0); // otherwise the following arr_shAlloc failing will result in r->sh dangling
      ptr_dec(shObj(r));
    }
    rsh = arr_shAlloc(a(r), rr);
  } else {
    generic:;
    SGetU(w) SGet(x)
    
    M_HARR(r, ria)
    for (usz wi = 0; wi < wia; wi++) {
      B cw = GetU(w,wi);
      for (usz xi = 0; xi < xia; xi++) HARR_ADDA(r, fc2(f, inc(cw), Get(x,xi)));
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
    assert(isArr(w)); wf=getFillQ(w);
    assert(isArr(x)); xf=getFillQ(x);
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
  B wf = getFillQ(w);
  B xf = getFillQ(x);
  return homFil2(f, eachd(f, w, x), wf, xf);
}

B const_c1(Md1D* d,      B x) {         dec(x); return inc(d->f); }
B const_c2(Md1D* d, B w, B x) { dec(w); dec(x); return inc(d->f); }

B swap_c1(Md1D* d,      B x) { return c2(d->f, inc(x), x); }
B swap_c2(Md1D* d, B w, B x) { return c2(d->f,     x , w); }


B timed_c2(Md1D* d, B w, B x) { B f = d->f;
  i64 am = o2i64(w);
  incBy(x, am);
  dec(x);
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


static B m1c1(B t, B f, B x) { // consumes x
  B fn = m1_d(inc(t), inc(f));
  B r = c1(fn, x);
  decG(fn);
  return r;
}
static B m1c2(B t, B f, B w, B x) { // consumes w,x
  B fn = m1_d(inc(t), inc(f));
  B r = c2(fn, w, x);
  decG(fn);
  return r;
}

#pragma GCC diagnostic push
#ifdef __clang__
  #pragma GCC diagnostic ignored "-Wsometimes-uninitialized"
  // no gcc case because there's no way to do it specifically for this segment of code; X##_csh is just initialized with an unused null pointer
#endif
#define S_SLICES(X)            \
  BSS2A X##_slc = TI(X,slice); \
  usz X##_csz = 1;             \
  usz X##_cr = RNK(X)-1;       \
  ShArr* X##_csh ONLY_GCC(=0); \
  if (X##_cr>1) {              \
    X##_csh = m_shArr(X##_cr); \
    PLAINLOOP for (usz i = 0; i < X##_cr; i++) { \
      usz v = SH(X)[i+1];      \
      X##_csz*= v;             \
      X##_csh->a[i] = v;       \
    }                          \
  } else if (X##_cr!=0) X##_csz*= SH(X)[1];

#define SLICE(X, S) taga(arr_shSetI(X##_slc(incG(X), S, X##_csz), X##_cr, X##_csh))

#define E_SLICES(X) if (X##_cr>1) ptr_dec(X##_csh); decG(X);


extern B to_fill_cell_k(B x, ur k, char* err); // from md2.c
static B to_fill_cell_1(B x) { // consumes x
  return to_fill_cell_k(x, 1, "˘: Empty argument too large (%H ≡ ≢𝕩)");
}
static B merge_fill_result_1(B rc) {
  u64 rr = isArr(rc)? RNK(rc)+1ULL : 1;
  if (rr>UR_MAX) thrM("˘: Result rank too large");
  B rf = getFillQ(rc);
  Arr* r = m_fillarrp(0);
  fillarr_setFill(r, rf);
  usz* rsh = arr_shAlloc(r, rr);
  if (rr>1) {
    rsh[0] = 0;
    shcpy(rsh+1, SH(rc), rr-1);
  }
  dec(rc);
  return taga(r);
}
B cell2_empty(B f, B w, B x, ur wr, ur xr) {
  if (!isPureFn(f) || !CATCH_ERRORS) { dec(w); dec(x); return emptyHVec(); }
  if (wr) w = to_fill_cell_1(w);
  if (xr) x = to_fill_cell_1(x);
  if (CATCH) return emptyHVec();
  B rc = c2(f, w, x);
  popCatch();
  return merge_fill_result_1(rc);
}

B select_cells(usz n, B x, ur xr) {
  usz* xsh = SH(x);
  B r;
  usz cam = xsh[0];
  if (xr==2) {
    usz csz = xsh[1];
    if (csz==1) return taga(arr_shVec(TI(x,slice)(x,0,IA(x))));
    u8 xe = TI(x,elType);
    if (xe==el_B) {
      SGet(x)
      HArr_p rp = m_harrUv(cam);
      for (usz i = 0; i < cam; i++) rp.a[i] = Get(x, i);
      r = rp.b;
    } else {
      void* rp = m_tyarrv(&r, elWidth(xe), cam, el2t(xe));
      void* xp = tyany_ptr(x);
      switch(xe) {
        case el_bit: for (usz i=0; i<cam; i++) bitp_set(rp, i, bitp_get(xp, i*csz+n)); break;
        case el_i8:  case el_c8:  PLAINLOOP for (usz i=0; i<cam; i++) ((u8* )rp)[i] = ((u8* )xp)[i*csz+n]; break;
        case el_i16: case el_c16: PLAINLOOP for (usz i=0; i<cam; i++) ((u16*)rp)[i] = ((u16*)xp)[i*csz+n]; break;
        case el_i32: case el_c32: PLAINLOOP for (usz i=0; i<cam; i++) ((u32*)rp)[i] = ((u32*)xp)[i*csz+n]; break;
        case el_f64:              PLAINLOOP for (usz i=0; i<cam; i++) ((f64*)rp)[i] = ((f64*)xp)[i*csz+n]; break;
      }
    }
  } else {
    Arr* ra;
    if (xsh[1]==1) {
      ra = TI(x,slice)(incG(x), 0, IA(x));
    } else {
      usz rs = shProd(xsh, 2, xr);
      usz xs = rs*xsh[1]; // aka csz
      MAKE_MUT(rm, cam*rs); mut_init(rm, TI(x,elType)); MUTG_INIT(rm);
      usz xi = rs*n;
      usz ri = 0;
      for (usz i = 0; i < cam; i++) {
        mut_copyG(rm, ri, x, xi, rs);
        xi+= xs;
        ri+= rs;
      }
      ra = mut_fp(rm);
    }
    usz* rsh = arr_shAlloc(ra, xr-1);
    shcpy(rsh+1, xsh+2, xr-2);
    rsh[0] = cam;
    r = taga(ra);
  }
  decG(x);
  return r;
}

B cell_c1(Md1D* d, B x) { B f = d->f;
  if (isAtm(x) || RNK(x)==0) {
    B r = c1(f, x);
    return isAtm(r)? m_atomUnit(r) : r;
  }
  
  if (IA(x)!=0 && isFun(f)) {
    u8 rtid = v(f)->flags-1;
    ur xr = RNK(x);
    if (rtid==n_lt && xr>1) return toCells(x);
    if (rtid==n_select && xr>1) return select_cells(0, x, xr);
    if (rtid==n_couple && xr>0) {
      ShArr* rsh = m_shArr(xr+1);
      usz* xsh = SH(x);
      rsh->a[0] = xsh[0];
      rsh->a[1] = 1;
      shcpy(rsh->a+2, xsh+1, xr-1);
      Arr* r = TI(x,slice)(x, 0, IA(x));
      return taga(arr_shSetU(r, xr+1, rsh));
    }
  }
  
  usz cam = SH(x)[0];
  if (cam==0) {
    if (!isPureFn(f) || !CATCH_ERRORS) { decG(x); return emptyHVec(); }
    B cf = to_fill_cell_1(x);
    if (CATCH) return emptyHVec();
    B rc = c1(f, cf);
    popCatch();
    return merge_fill_result_1(rc);
  }
  S_SLICES(x)
  M_HARR(r, cam);
  for (usz i=0,p=0; i<cam; i++,p+=x_csz) HARR_ADD(r, i, c1(f, SLICE(x, p)));
  E_SLICES(x)
  
  return bqn_merge(HARR_FV(r));
}

B cell_c2(Md1D* d, B w, B x) { B f = d->f;
  ur wr = isAtm(w)? 0 : RNK(w);
  ur xr = isAtm(x)? 0 : RNK(x);
  B r;
  if (wr==0 && xr==0) return isAtm(r = c2(f, w, x))? m_atomUnit(r) : r;
  if (wr==0) {
    usz cam = SH(x)[0];
    if (cam==0) return cell2_empty(f, w, x, wr, xr);
    if (isFun(f)) {
      u8 rtid = v(f)->flags-1;
      if (rtid==n_select && isF64(w)) {
        return select_cells(WRAP(o2i64(w), SH(x)[1], thrF("⊏: Indexing out-of-bounds (𝕨≡%R, %s≡≠𝕩)", w, cam)), x, xr);
      }
    }
    S_SLICES(x)
    M_HARR(r, cam);
    for (usz i=0,p=0; i<cam; i++,p+=x_csz) HARR_ADD(r, i, c2iW(f, w, SLICE(x, p)));
    E_SLICES(x) dec(w);
    r = HARR_FV(r);
  } else if (xr==0) {
    usz cam = SH(w)[0];
    if (cam==0) return cell2_empty(f, w, x, wr, xr);
    S_SLICES(w)
    M_HARR(r, cam);
    for (usz i=0,p=0; i<cam; i++,p+=w_csz) HARR_ADD(r, i, c2iX(f, SLICE(w, p), x));
    E_SLICES(w) dec(x);
    r = HARR_FV(r);
  } else {
    usz cam = SH(w)[0];
    if (cam==0) return cell2_empty(f, w, x, wr, xr);
    if (cam != SH(x)[0]) thrF("˘: Leading axis of arguments not equal (%H ≡ ≢𝕨, %H ≡ ≢𝕩)", w, x);
    S_SLICES(w) S_SLICES(x)
    M_HARR(r, cam);
    for (usz i=0,wp=0,xp=0; i<cam; i++,wp+=w_csz,xp+=x_csz) HARR_ADD(r, i, c2(f, SLICE(w, wp), SLICE(x, xp)));
    E_SLICES(w) E_SLICES(x)
    r = HARR_FV(r);
  }
  return bqn_merge(r);
}

B fold_c1(Md1D* d, B x);
B fold_c2(Md1D* d, B w, B x);

extern B rt_insert;
B insert_c1(Md1D* d, B x) { B f = d->f;
  if (isAtm(x) || RNK(x)==0) thrM("˝: 𝕩 must have rank at least 1");
  usz xia = IA(x);
  if (xia==0) { SLOW2("!𝕎˝𝕩", f, x); return m1c1(rt_insert, f, x); }
  if (isFun(f)) {
    u8 rtid = v(f)->flags-1;
    if (RNK(x)==1 && isPervasiveDy(f)) return m_atomUnit(fold_c1(d, x));
    if (rtid == n_join) {
      ur xr = RNK(x);
      if (xr==1) return x;
      ShArr* rsh;
      if (xr>2) {
        rsh = m_shArr(xr-1);
        usz* xsh = SH(x);
        shcpy(rsh->a+1, xsh+2, xr-2);
        rsh->a[0] = xsh[0] * xsh[1];
      }
      Arr* r = TI(x,slice)(x, 0, IA(x));
      if (xr>2) arr_shSetU(r, xr-1, rsh);
      else arr_shVec(r);
      return taga(r);
    }
  }
  
  S_SLICES(x)
  usz p = xia-x_csz;
  B r = SLICE(x, p);
  while(p!=0) {
    p-= x_csz;
    r = c2(f, SLICE(x, p), r);
  }
  E_SLICES(x)
  return r;
}
B insert_c2(Md1D* d, B w, B x) { B f = d->f;
  if (isAtm(x) || RNK(x)==0) thrM("˝: 𝕩 must have rank at least 1");
  usz xia = IA(x);
  B r = w;
  if (xia==0) return r;
  
  if (isFun(f)) {
    if (RNK(x)==1 && isPervasiveDy(f)) {
      if (isAtm(w)) {
        to_fold: return m_atomUnit(fold_c2(d, w, x));
      }
      if (RNK(w)==0) {
        B w0=w; w = IGet(w,0); decG(w0);
        goto to_fold;
      }
    }
  }
  
  S_SLICES(x)
  usz p = xia;
  while(p!=0) {
    p-= x_csz;
    r = c2(f, SLICE(x, p), r);
  }
  E_SLICES(x)
  return r;
}

#pragma GCC diagnostic pop

static void print_md1BI(FILE* f, B x) { fprintf(f, "%s", pm1_repr(c(Md1,x)->extra)); }
static B md1BI_im(Md1D* d,      B x) { return ((BMd1*)d->m1)->im(d,    x); }
static B md1BI_iw(Md1D* d, B w, B x) { return ((BMd1*)d->m1)->iw(d, w, x); }
static B md1BI_ix(Md1D* d, B w, B x) { return ((BMd1*)d->m1)->ix(d, w, x); }
void md1_init() {
  TIi(t_md1BI,print) = print_md1BI;
  TIi(t_md1BI,m1_im) = md1BI_im;
  TIi(t_md1BI,m1_iw) = md1BI_iw;
  TIi(t_md1BI,m1_ix) = md1BI_ix;
}
