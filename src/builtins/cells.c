#include "../core.h"
#include "../builtins.h"
#include "../utils/mut.h"
#include "../utils/calls.h"
#include <math.h>

B fne_c1(B, B);
B shape_c2(B, B, B);
B transp_c2(B, B, B);
B fold_rows(Md1D* d, B x); // from fold.c
B takedrop_highrank(bool take, B w, B x); // from sfns.c

// X - variable name; XSH - its shape; K - number of leading axes that get iterated over; SLN - number of slices that will be made; DX - additional refcount count to add to x
#define S_KSLICES(X, XSH, K, SLN, DX)\
  usz X##_sn = (SLN);            \
  assert(X##_sn>0);              \
  usz X##_k = (K);               \
  usz X##_cr = RNK(X)-X##_k;     \
  usz X##_csz = 1;               \
  ShArr* X##_csh = NULL;         \
  if (LIKELY(X##_cr >= 1)) {     \
    if (RARE(X##_cr > 1)) {      \
      X##_csh = m_shArr(X##_cr); \
      ptr_incBy(X##_csh, X##_sn-1); \
      PLAINLOOP for (usz i = 0; i < X##_cr; i++) { \
        usz v = XSH[i+X##_k];    \
        X##_csz*= v;             \
        X##_csh->a[i] = v;       \
      }                          \
    } else X##_csz = XSH[X##_k]; \
  }                              \
  BSS2A X##_slc = TI(X,slice);   \
  incByG(X, (i64)X##_sn + ((i64)DX-1));

#define S_SLICES(X, SLN) usz* X##_sh = SH(X); S_KSLICES(X, X##_sh, 1, SLN, 0)
#define SLICE(X, S) taga(arr_shSetUO(X##_slc(X, S, X##_csz), X##_cr, X##_csh))
#define SLICEI(X) ({ B r = SLICE(X, X##p); X##p+= X##_csz; r; })



// Used by Insert in fold.c
B insert_base(B f, B x, usz xia, bool has_w, B w) {
  assert(isArr(x) && RNK(x)>0);
  S_SLICES(x, *x_sh)
  usz p = xia;
  B r = w;
  if (!has_w) {
    p -= x_csz;
    r = SLICE(x, p);
  }
  FC2 fc2 = c2fn(f);
  while(p!=0) {
    p-= x_csz;
    r = fc2(f, SLICE(x, p), r);
  }
  return r;
}


#if TEST_CELL_FILLS
  i32 fullCellFills = 2*CATCH_ERRORS;
  i32 cellFillErrored = 0;
  #define DO_CELL_CATCH (fullCellFills==2)
  #define getFillQ2 (fullCellFills? getFillR : getFillQ)
  #define SET_FILL_ERRORED cellFillErrored = 1
#else
  #define DO_CELL_CATCH CATCH_ERRORS
  #define getFillQ2 getFillQ
  #define SET_FILL_ERRORED
#endif

static NOINLINE B empty_frame(usz* xsh, ur k) {
  if (k==1) return emptyHVec();
  assert(k>1);
  HArr_p f = m_harrUp(0);
  shcpy(arr_shAlloc((Arr*)f.c, k), xsh, k);
  return f.b;
}
NOINLINE B toCells(B x) {
  assert(isArr(x) && RNK(x)>1);
  usz* xsh = SH(x);
  usz cam = xsh[0];
  if (cam==0) { decG(x); return emptyHVec(); }
  if (RNK(x)==2) {
    M_HARR(r, cam)
    incByG(x, cam-1);
    BSS2A slice = TI(x,slice);
    usz csz = arr_csz(x);
    for (usz i=0,p=0; i<cam; i++,p+=csz) HARR_ADD(r, i, taga(arr_shVec(slice(x, p, csz))));
    return HARR_FV(r);
  } else {
    S_KSLICES(x, xsh, 1, cam, 0)
    M_HARR(r, cam)
    assert(x_cr > 1);
    for (usz i=0,xp=0; i<cam; i++) HARR_ADD(r, i, SLICEI(x));
    return HARR_FV(r);
  }
}
NOINLINE B toKCells(B x, ur k) {
  assert(isArr(x) && k<=RNK(x) && k>=0);
  usz* xsh = SH(x);
  usz cam = shProd(xsh, 0, k);
  
  B r;
  if (cam==0) {
    r = empty_frame(xsh, k); 
  } else {
    S_KSLICES(x, xsh, k, cam, 1)
    M_HARR(r, cam)
    for (usz i=0,xp=0; i<cam; i++) HARR_ADD(r, i, SLICEI(x));
    usz* rsh = HARR_FA(r, k);
    if (rsh) shcpy(rsh, xsh, k);
    r = HARR_O(r).b;
  }
  
  decG(x);
  return r;
}

B slash_c2(B, B, B);
NOINLINE B leading_axis_arith(FC2 fc2, B w, B x, usz* wsh, usz* xsh, ur mr) { // assumes non-equal rank non-empty conforming typed array arguments
  assert(isArr(w) && isArr(x) && TI(w,elType)!=el_B && TI(x,elType)!=el_B && IA(w)!=0 && IA(x)!=0);
  ur wr = RNK(w);
  ur xr = RNK(x);
  #if DEBUG
    assert(wr!=xr && (mr==wr || mr==xr) && eqShPart(wsh, xsh, mr));
  #endif
  usz cam = shProd(xsh, 0, mr);
  
  B b = mr==wr? x : w; // bigger argument
  usz* bsh = mr==wr? xsh : wsh;
  ur br = wr>xr? wr : xr;
  
  usz csz = shProd(bsh, mr, br);
  if (csz<200) {
    B s = mr==wr? w : x; // smaller argument
    s = C2(slash, m_usz(csz), taga(arr_shVec(TI(s,slice)(s,0,IA(s)))));
    assert(reusable(s) && RNK(s)==1);
    arr_shCopy(a(s), b);
    if (mr==wr) w=s; else x=s;
    return fc2(m_f64(0), w, x);
  } else {
    M_APD_SH(r, mr, bsh);
    S_KSLICES(b, bsh, mr, cam, 1) usz bp=0;
    if (mr==wr) { SGetU(w); for (usz i=0; i<cam; i++) APDD(r, fc2(m_f64(0), GetU(w,i), SLICEI(b))); }
    else        { SGetU(x); for (usz i=0; i<cam; i++) APDD(r, fc2(m_f64(0), SLICEI(b), GetU(x,i))); }
    decG(w); decG(x);
    return taga(APD_SH_GET(r, 0));
  }
}



// fast special-case implementations
static NOINLINE B select_cells(usz n, B x, usz cam, usz k, bool leaf) { // n {leaf? <∘⊑; ⊏}⎉¯k x; TODO probably can share some parts with takedrop_highrank and/or call ⊏?
  ur xr = RNK(x);
  assert(xr>1 && k<xr);
  usz* xsh = SH(x);
  usz csz = shProd(xsh, k+1, xr);
  usz take = leaf? 1 : csz;
  usz jump = xsh[k] * csz;
  assert(cam*jump == IA(x));
  Arr* ra;
  if (take==jump) {
    ra = cpyWithShape(incG(x));
    arr_shErase(ra, 1);
  } else if (take==1) {
    u8 xe = TI(x,elType);
    if (xe==el_B) {
      SGet(x)
      HArr_p rp = m_harrUv(cam);
      for (usz i = 0; i < cam; i++) rp.a[i] = Get(x, i*jump+n);
      NOGC_E; ra = (Arr*)rp.c;
    } else {
      void* rp = m_tyarrp(&ra, elWidth(xe), cam, el2t(xe));
      void* xp = tyany_ptr(x);
      switch(xe) {
        case el_bit: for (usz i=0; i<cam; i++) bitp_set(rp, i, bitp_get(xp, i*jump+n)); break;
        case el_i8:  case el_c8:  PLAINLOOP for (usz i=0; i<cam; i++) ((u8* )rp)[i] = ((u8* )xp)[i*jump+n]; break;
        case el_i16: case el_c16: PLAINLOOP for (usz i=0; i<cam; i++) ((u16*)rp)[i] = ((u16*)xp)[i*jump+n]; break;
        case el_i32: case el_c32: PLAINLOOP for (usz i=0; i<cam; i++) ((u32*)rp)[i] = ((u32*)xp)[i*jump+n]; break;
        case el_f64:              PLAINLOOP for (usz i=0; i<cam; i++) ((f64*)rp)[i] = ((f64*)xp)[i*jump+n]; break;
      }
    }
  } else {
    MAKE_MUT_INIT(rm, cam*take, TI(x,elType)); MUTG_INIT(rm);
    usz xi = take*n;
    usz ri = 0;
    for (usz i = 0; i < cam; i++) {
      mut_copyG(rm, ri, x, xi, take);
      xi+= jump;
      ri+= take;
    }
    ra = mut_fp(rm);
  }
  usz* rsh = arr_shAlloc(ra, leaf? k : xr-1);
  if (rsh) {
    shcpy(rsh, xsh, k);
    if (!leaf) shcpy(rsh+k, xsh+k+1, xr-1-k);
  }
  decG(x);
  return taga(ra);
}

static NOINLINE B shift_cells(B f, B x, usz cam, usz csz, u8 e, u8 rtid) { // »⎉1 or «⎉1
  assert(cam!=0 && csz!=0);
  MAKE_MUT_INIT(r, IA(x), e); MUTG_INIT(r);
  bool after = rtid==n_shifta;
  usz xi=after, ri=!after, fi=after?csz-1:0;
  incBy(f, cam-1); // cam≠0 → cam-1 ≥ 0
  for (usz i = 0; i < cam; i++) {
    mut_copyG(r, ri, x, xi, csz-1);
    mut_setG(r, fi, f);
    xi+= csz;
    ri+= csz;
    fi+= csz;
  }
  return mut_fcd(r, x);
}

static B allBit(bool b, usz n) {
  return taga(arr_shVec(b ? allOnes(n) : allZeroes(n)));
}
static NOINLINE Arr* match_cells(bool ne, B w, B x, ur wr, ur xr, ur k, usz len) {
  usz* wsh = SH(w);
  if (wr!=xr || (wr>k && !eqShPart(wsh+k, SH(x)+k, wr-k))) {
    return a(allBit(ne, len));
  }
  usz csz = shProd(wsh, k, wr);
  if (csz==0) return a(allBit(!ne, len));
  u8 we = TI(w,elType);
  u8 xe = TI(x,elType);
  if (we>el_c32 || xe>el_c32) return NULL;
  usz ww = csz * elWidth(we); u8* wp = tyany_ptr(w);
  usz xw = csz * elWidth(xe); u8* xp = tyany_ptr(x);
  u64* rp; Arr* r = m_bitarrp(&rp, len);
  if (csz==1 && we==xe) {
    CmpAAFn cmp = ne ? CMP_AA_FN(ne,we) : CMP_AA_FN(eq,we);
    CMP_AA_CALL(cmp, rp, wp, xp, len);
  } else {
    if (we==el_bit || xe==el_bit) { mm_free((Value*)r); return NULL; }
    EqFnObj eqfn = EQFN_GET(we, xe);
    for (usz i = 0; i < len; i++) {
      bitp_set(rp, i, ne^EQFN_CALL(eqfn, wp, xp, csz));
      wp += ww; xp += xw;
    }
  }
  return r;
}

static NOINLINE B transp_cells(ur ax, ur k, B x) {
  assert(k>0 && ax>=k);
  assert(UR_MAX < I16_MAX);
  i16* wp; B w=m_i16arrv(&wp, k+1);
  PLAINLOOP for (usz i=0; i<k; i++) wp[i] = i;
  wp[k]=ax;
  return C2(transp, w, x);
}



// helpers
static NOINLINE B to_fill_cell(B x, ur k, u32 chr) { // consumes x
  B xf = getFillQ2(x);
  if (noFill(xf)) xf = m_f64(0);
  ur cr = RNK(x)-k;
  usz* sh = SH(x)+k;
  usz csz = 1;
  for (usz i=0; i<cr; i++) if (mulOn(csz, sh[i])) thrF("%c: Empty argument too large (%H ≡ ≢𝕩)", chr, x);
  MAKE_MUT(fc, csz);
  mut_fill(fc, 0, xf, csz); dec(xf);
  Arr* ca = mut_fp(fc);
  usz* csh = arr_shAlloc(ca, cr);
  if (cr>1) shcpy(csh, sh, cr);
  decG(x);
  return taga(ca);
}

static NOINLINE B merge_fill_result(B rc, ur k, usz* sh, u32 chr) {
  u64 rr = k; if (isArr(rc)) rr+= RNK(rc);
  if (rr>UR_MAX) thrF("%c: Result rank too large", chr);
  Arr* r = m_fillarrpEmpty(getFillQ2(rc));
  usz* rsh = arr_shAlloc(r, rr);
  if (rr>1) {
    shcpy(rsh, sh, k);
    usz cr = rr-k;
    if (cr) shcpy(rsh+k, SH(rc), cr);
  }
  dec(rc);
  return taga(r);
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


NOINLINE B for_cells_AS(B f, B w, B x, ur wcr, ur wr, u32 chr);
NOINLINE B for_cells_SA(B f, B w, B x, ur xcr, ur xr, u32 chr);
NOINLINE B for_cells_AA(B f, B w, B x, ur wcr, ur xcr, u32 chr);

static NOINLINE B c1wrap(B f,      B x) { B r = c1(f,    x); return isAtm(r)? m_unit(r) : r; }
static NOINLINE B c2wrap(B f, B w, B x) { B r = c2(f, w, x); return isAtm(r)? m_unit(r) : r; }
// monadic ˘ & ⎉
B for_cells_c1(B f, u32 xr, u32 cr, u32 k, B x, u32 chr) { // F⎉cr x, with array x, xr>0, 0≤cr<xr, k≡xr-cr
  assert(isArr(x) && xr>0 && k>0 && cr<xr);
  usz* xsh = SH(x);
  usz cam = shProd(xsh, 0, k); // from k>0 this will always include at least one item of shape; therefore, cam≡0 → IA(x)≡0 and IA(x)≢0 → cam≢0
  if (isFun(f)) {
    u8 rtid = v(f)->flags-1;
    switch(rtid) {
      case n_lt:
        if (cam==0) goto noCells; // toCells/toKCells don't set outer array fill
        return k==1 && RNK(x)>1? toCells(x) : k==0? m_unit(x) : toKCells(x, k);
      case n_select:
        if (IA(x)==0) goto noSpecial;
        if (cr==0) goto base;
        return select_cells(0, x, cam, k, false);
      case n_pick:
        if (IA(x)==0) goto noSpecial;
        if (cr==0 || !TI(x,arrD1)) goto base;
        return select_cells(0, x, cam, k, true);
      case n_couple: {
        Arr* r = cpyWithShape(x); xsh=PSH(r);
        if (xr==UR_MAX) thrF("≍%c: Result rank too large (%i≡=𝕩)", chr, xr);
        ShArr* rsh = m_shArr(xr+1);
        shcpy(rsh->a, xsh, k);
        rsh->a[k] = 1;
        shcpy(rsh->a+k+1, xsh+k, xr-k);
        return taga(arr_shReplace(r, xr+1, rsh));
      }
      case n_shape: {
        if (cr==1) return x;
        Arr* r = cpyWithShape(x); xsh=PSH(r);
        usz csz = shProd(xsh, k, xr);
        ShArr* rsh = m_shArr(k+1);
        shcpy(rsh->a, xsh, k);
        rsh->a[k] = csz;
        return taga(arr_shReplace(r, k+1, rsh));
      }
      case n_shifta: case n_shiftb: {
        if (IA(x)==0) goto noSpecial;
        if (cr!=1) goto base;
        B xf = getFillR(x);
        if (noFill(xf)) goto base;
        return shift_cells(xf, x, cam, xsh[k], TI(x,elType), rtid);
      }
      case n_transp: {
        return cr<=1? x : transp_cells(xr-1, k, x);
      }
    }
    
    if (TY(f) == t_md1D) {
      Md1D* fd = c(Md1D,f);
      u8 rtid = fd->m1->flags-1;
      if (rtid==n_const) { f=fd->f; goto const_f; }
      if ((rtid==n_fold || rtid==n_insert) && TI(x,elType)!=el_B && k==1 && xr==2 && isFun(fd->f) && isPervasiveDyExt(fd->f)) { // TODO extend to any rank x with cr==1
        usz *sh = SH(x); usz m = sh[1];
        if (m == 1) return select_cells(0, x, cam, k, false);
        if (m <= 64 && m < sh[0]) return fold_rows(fd, x);
      }
    } else if (TY(f) == t_md2D) {
      Md2D* fd = c(Md2D,f);
      u8 rtid = fd->m2->flags-1;
      if (rtid==n_before && !isCallable(fd->f)) return for_cells_SA(fd->g, inc(fd->f), x, cr, xr, chr);
      if (rtid==n_after  && !isCallable(fd->g)) return for_cells_AS(fd->f, x, inc(fd->g), cr, xr, chr);
    }
  } else if (!isMd(f)) {
    const_f:; inc(f);
    u32 fr;
    if (isAtm(f) || RNK(f)==0) {
      if (k!=1) { fr = 0; goto const_f_cont; }
      usz cam = xsh[0];
      decG(x);
      return C2(shape, m_usz(cam), f);
    } else {
      fr = RNK(f);
      if (fr+k > UR_MAX) thrF("%c: Result rank too large", chr);
      const_f_cont:;
      f64* shp; B sh = m_f64arrv(&shp, fr+k);
      PLAINLOOP for (usz i=0; i<k; i++) shp[i] = xsh[i];
      if (isArr(f)) {
        usz* fsh = SH(f);
        PLAINLOOP for (usz i=0; i<fr; i++) shp[i+k] = fsh[i];
      }
      decG(x);
      return C2(shape, sh, f);
    }
  }
  
  noSpecial:;
  if (cam == 0) {
    noCells:;
    usz s0=0; ShArr* s=NULL;
    if (xr<=1) { s0=xsh[0]; xsh=&s0; } else { s=ptr_inc(shObj(x)); }
    if (!DO_CELL_CATCH || !isPureFn(f)) { decG(x); goto empty; }
    B cf = to_fill_cell(x, k, chr);
    B r;
    if (CATCH) {
      SET_FILL_ERRORED;
      freeThrown();
      empty:
      r = empty_frame(xsh, k);
    } else {
      B rc = c1(f, cf);
      popCatch();
      r = merge_fill_result(rc, k, xsh, chr);
    }
    if (xr>1) ptr_dec(s);
    return r;
  }
  
  base:;
  
  M_APD_SH(r, k, xsh);
  S_KSLICES(x, xsh, k, cam, 1); FC1 fc1 = c1fn(f);
  for (usz i=0,xp=0; i<cam; i++) APDD(r, fc1(f, SLICEI(x)));
  decG(x);
  
  return taga(APD_SH_GET(r, chr));
}

B cell_c1(Md1D* d, B x) { B f = d->f;
  ur xr;
  if (isAtm(x) || (xr=RNK(x))==0) return c1wrap(f, x);
  return for_cells_c1(f, xr, xr-1, 1, x, U'˘');
}
B rank_c1(Md2D* d, B x) { B f = d->f; B g = d->g;
  f64 kf;
  B gi = m_f64(0);
  if (RARE(isFun(g))) gi = g = c1iX(g, x);
  if (LIKELY(isNum(g))) {
    kf = req_whole(o2fG(g));
  } else {
    usz gia = check_rank_vec(g);
    kf = IGetU(g, gia==2).f;
    decA(gi);
  }
  if (isAtm(x)) return c1wrap(f, x);
  ur xr = RNK(x);
  ur cr = cell_rank(xr, kf);
  if (cr==xr) return c1wrap(f, x);
  return for_cells_c1(f, xr, cr, xr-cr, x, U'⎉');
}


static NOINLINE B rank2_empty(B f, B w, ur wk, B x, ur xk, u32 chr) {
  B fa = wk>xk?w:x;
  ur k = wk>xk?wk:xk;
  usz* sh = SH(fa);
  usz s0=0; ShArr* s=NULL; ur sho=RNK(fa)>1;
  if (!sho) { s0=sh[0]; sh=&s0; } else { s=ptr_inc(shObj(fa)); }
  if (!DO_CELL_CATCH || !isPureFn(f)) { dec(w); dec(x); goto empty; }
  B r;
  if (wk) w = to_fill_cell(w, wk, chr);
  if (xk) x = to_fill_cell(x, xk, chr);
  if (CATCH) {
    SET_FILL_ERRORED;
    freeThrown();
    empty:
    r = empty_frame(sh, k);
  } else {
    B rc = c2(f, w, x);
    popCatch();
    r = merge_fill_result(rc, k, sh, U'⎉');
  }
  if (sho) ptr_dec(s);
  return r;
}

NOINLINE B for_cells_AS(B f, B w, B x, ur wcr, ur wr, u32 chr) {
  assert(isArr(w));
  ur wk = wr-wcr; assert(wk>0 && wcr<wr);
  usz* wsh=SH(w); usz cam=shProd(wsh,0,wk);
  if (cam==0) return rank2_empty(f, w, wk, x, 0, chr);
  if (isFun(f) && IA(w)!=0) {
    if (isPervasiveDy(f)) {
      if (isAtm(x)) return c2(f, w, x);
      if (RNK(x)!=wcr || !eqShPart(SH(x), wsh+wk, wcr)) goto generic;
      if (TI(w,elType)==el_B || TI(x,elType)==el_B || (IA(x)>500 && IA(w)!=IA(x))) goto generic;
      return c2(f, w, C2(shape, C1(fne, incG(w)), x));
    }
  }
  generic:;
  S_KSLICES(w, wsh, wk, cam, 1) incBy(x, cam-1);
  M_APD_SH(r, wk, wsh); FC2 fc2 = c2fn(f);
  for (usz i=0,wp=0; i<cam; i++) APDD(r, fc2(f, SLICEI(w), x));
  decG(w); return taga(APD_SH_GET(r, chr));
}
NOINLINE B for_cells_SA(B f, B w, B x, ur xcr, ur xr, u32 chr) {
  assert(isArr(x));
  ur xk = xr-xcr; assert(xk>0 && xcr<xr);
  usz* xsh=SH(x); usz cam=shProd(xsh,0,xk);
  if (cam==0) return rank2_empty(f, w, 0, x, xk, chr);
  if (isFun(f) && IA(x)!=0) {
    u8 rtid = v(f)->flags-1;
    if (rtid==n_select && xk==1 && isF64(w) && xr==2)              return select_cells(WRAP(o2i64(w), SH(x)[1], thrF("⊏: Indexing out-of-bounds (𝕨≡%R, %s≡≠𝕩)", w, cam)), x, cam, 1, false);
    if (rtid==n_pick && xk==1 && TI(x,arrD1) && xr==2 && isF64(w)) return select_cells(WRAP(o2i64(w), SH(x)[1], thrF("⊑: Indexing out-of-bounds (𝕨≡%R, %s≡≠𝕩)", w, cam)), x, cam, 1, true);
    if ((rtid==n_shifta || rtid==n_shiftb) && xk==1 && xr==2 && isAtm(w)) {
      if (isArr(w)) { B w0=w; w = IGet(w,0); decG(w0); }
      return shift_cells(w, x, SH(x)[0], SH(x)[1], el_or(TI(x,elType), selfElType(w)), rtid);
    }
    if (rtid==n_take || rtid==n_drop) {
      if (xcr==0 || !isF64(w)) goto generic;
      bool take = rtid==n_take;
      f64* ap;
      B a = m_f64arrv(&ap, xk+1);
      if (!take) { FILL_TO(ap, el_f64, 0, m_f64(0), xk); }
      else { usz* xsh=SH(x); PLAINLOOP for (usz i=0; i<xk; i++) ap[i] = xsh[i]; }
      ap[xk] = o2fG(w);
      return takedrop_highrank(take, a, x);
    }
    if (rtid==n_transp && q_usz(w)) { usz a=o2sG(w); if (a<xcr) return transp_cells(a+xk, xk, x); }
    if (isPervasiveDy(f)) {
      if (isAtm(w)) return c2(f, w, x);
      if (RNK(w)!=xcr || !eqShPart(SH(w), xsh+xk, xcr)) goto generic;
      if (TI(w,elType)==el_B || TI(x,elType)==el_B || (IA(w)>500 && IA(w)!=IA(x))) goto generic;
      return c2(f, C2(shape, C1(fne, incG(x)), w), x);
    }
  }
  generic:;
  S_KSLICES(x, xsh, xk, cam, 1) incBy(w, cam-1);
  M_APD_SH(r, xk, xsh); FC2 fc2 = c2fn(f);
  for (usz i=0,xp=0; i<cam; i++) APDD(r, fc2(f, w, SLICEI(x)));
  decG(x); return taga(APD_SH_GET(r, chr));
}
NOINLINE B for_cells_AA(B f, B w, B x, ur wcr, ur xcr, u32 chr) {
  assert(isArr(w) && isArr(x));
  ur wr = RNK(w); ur wk = wr-wcr; usz* wsh = SH(w); assert(wk>0 && wcr<wr);
  ur xr = RNK(x); ur xk = xr-xcr; usz* xsh = SH(x); assert(xk>0 && xcr<xr);
  bool xkM = xk>wk;
  ur k, zk;
  if (xkM) { k=wk; zk=xk; }
  else     { k=xk; zk=wk; }
  usz* zsh = xkM? xsh : wsh;
  
  usz cam0 = 1;
  for (usz i = 0; i < k; i++) {
    usz wl = wsh[i], xl = xsh[i];
    if (wl != xl) thrF("%c: Argument frames don't agree (%H ≡ ≢𝕨, %H ≡ ≢𝕩, common frame of %i axes)", chr, w, x, k);
    cam0*= wsh[i];
  }
  usz ext = shProd(zsh, k, zk);
  usz cam = cam0*ext;
  
  if (cam==0) return rank2_empty(f, w, wk, x, xk, chr);
  
  if (isFun(f)) {
    if (wk==xk) {
      u8 rtid = v(f)->flags-1;
      if (rtid==n_feq || rtid==n_fne) {
        Arr* r = match_cells(rtid!=n_feq, w, x, wr, xr, wk, cam);
        if (r==NULL) goto generic;
        usz* rsh = arr_shAlloc(r, zk);
        if (rsh) shcpy(rsh, zsh, zk);
        decG(w); decG(x); return taga(r);
      }
    }
    if (isPervasiveDy(f)) {
      if (TI(w,elType)==el_B || TI(x,elType)==el_B) goto generic;
      ur mr = xr<wr? xr : wr;
      if ((wk>mr?mr:wk) != (xk>mr?mr:xk) || !eqShPart(wsh, xsh, mr)) goto generic;
      return c2(f, w, x);
    }
  }
  generic:;
  
  M_APD_SH(r, zk, zsh);
  S_KSLICES(w, wsh, wk, xkM? cam0 : cam, 1) usz wp=0;
  S_KSLICES(x, xsh, xk, xkM? cam : cam0, 1) usz xp=0;
  FC2 fc2 = c2fn(f);
  if (ext==1) { for (usz i=0; i<cam; i++) APDD(r, fc2(f, SLICEI(w), SLICEI(x))); }
  else if (xkM) { for (usz i=0; i<cam; ) { B wb=incByG(SLICEI(w), ext-1); for (usz e = i+ext; i < e; i++) APDD(r, fc2(f, wb, SLICEI(x))); } }
  else          { for (usz i=0; i<cam; ) { B xb=incByG(SLICEI(x), ext-1); for (usz e = i+ext; i < e; i++) APDD(r, fc2(f, SLICEI(w), xb)); } }
  
  decG(w); decG(x);
  return taga(APD_SH_GET(r, chr));
}

B rank_c2(Md2D* d, B w, B x) { B f = d->f; B g = d->g;
  f64 wf, xf;
  B gi = m_f64(0);
  if (RARE(isFun(g))) gi = g = c2iWX(g, w, x);
  if (LIKELY(isNum(g))) {
    wf = xf = req_whole(o2fG(g));
  } else {
    usz gia = check_rank_vec(g);
    SGetU(g);
    wf = GetU(g, gia<2?0:gia-2).f;
    xf = GetU(g, gia-1).f;
    dec(gi);
  }
  
  ur wr, wcr;
  ur xr, xcr;
  if     (isAtm(w)) goto r0Wt; else { wr = RNK(w); if ((wcr=cell_rank(wr,wf)) == wr) goto r0Wt; /*else fallthrough*/ }
  if     (isAtm(x)) goto r0X;  else { xr = RNK(x); if ((xcr=cell_rank(xr,xf)) == xr) goto r0X;  else goto neither; }
  r0Wt:if(isAtm(x)) goto r0WX; else { xr = RNK(x); if ((xcr=cell_rank(xr,xf)) == xr) goto r0WX; else goto r0W; }
  
  neither: return for_cells_AA(f, w, x, wcr, xcr, U'⎉');
  r0X: return for_cells_AS(f, w, x, wcr, wr, U'⎉');
  r0W: return for_cells_SA(f, w, x, xcr, xr, U'⎉');
  r0WX: return c2wrap(f, w, x);
}
B cell_c2(Md1D* d, B w, B x) { B f = d->f;
  ur wr, xr;
  if (isAtm(w) || (wr=RNK(w))==0) {
    if (isAtm(x) || (xr=RNK(x))==0) return c2wrap(f, w, x);
    return for_cells_SA(f, w, x, xr-1, xr, U'˘');
  }
  if (isAtm(x) || (xr=RNK(x))==0) return for_cells_AS(f, w, x, wr-1, wr, U'˘');
  return for_cells_AA(f, w, x, wr-1, xr-1, U'˘');
}
