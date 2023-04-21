#include "../core.h"
#include "../builtins.h"
#include "../utils/mut.h"
#include "../utils/calls.h"
#include <math.h>

B shape_c1(B, B);
B shape_c2(B, B, B);
B transp_c2(B, B, B);
B fold_rows(Md1D* d, B x); // from fold.c
B takedrop_highrank(bool take, B w, B x); // from sfns.c

#define S_KSLICES(X, XSH, K)     \
  usz X##_k = (K);               \
  usz X##_cr = RNK(X)-X##_k;     \
  usz X##_csz = 1;               \
  ShArr* X##_csh = NULL;         \
  if (LIKELY(X##_cr >= 1)) {     \
    if (RARE(X##_cr > 1)) {      \
      X##_csh = m_shArr(X##_cr); \
      PLAINLOOP for (usz i = 0; i < X##_cr; i++) { \
        usz v = XSH[i+X##_k];    \
        X##_csz*= v;             \
        X##_csh->a[i] = v;       \
      }                          \
    } else X##_csz = XSH[X##_k]; \
  }                              \
  BSS2A X##_slc = TI(X,slice);

#define S_SLICES(X) usz* X##_sh = SH(X); S_KSLICES(X, X##_sh, 1)
#define SLICE(X, S) taga(arr_shSetI(X##_slc(incG(X), S, X##_csz), X##_cr, X##_csh))
#define E_SLICES(X) if (X##_cr>1) ptr_dec(X##_csh); decG(X);



// Used by Insert in fold.c
B insert_base(B f, B x, usz xia, bool has_w, B w) {
  S_SLICES(x)
  usz p = xia;
  B r = w;
  if (!has_w) {
    p -= x_csz;
    r = SLICE(x, p);
  }
  BBB2B fc2 = c2fn(f);
  while(p!=0) {
    p-= x_csz;
    r = fc2(f, SLICE(x, p), r);
  }
  E_SLICES(x)
  return r;
}



// helpers for ˘ & ⎉
static NOINLINE B to_fill_cell_impl(B x, ur k, char* err) { // consumes x
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
static B to_fill_cell_k(B x, ur k) { // consumes x
  return to_fill_cell_impl(x, k, "⎉: Empty argument too large (%H ≡ ≢𝕩)");
}
static B to_fill_cell_1(B x) { // consumes x
  return to_fill_cell_impl(x, 1, "˘: Empty argument too large (%H ≡ ≢𝕩)");
}

FORCE_INLINE B merge_fill_result_impl(u32 chr, B rc, ur k, usz* sh) {
  u64 rr = k; if (isArr(rc)) rr += RNK(rc);
  if (rr>UR_MAX) thrF("%c: Result rank too large", chr);
  Arr* r = m_fillarrpEmpty(getFillQ(rc));
  usz* rsh = arr_shAlloc(r, rr);
  if (rr>1) {
    shcpy(rsh, sh, k);
    shcpy(rsh+k, SH(rc), rr-k);
  }
  dec(rc);
  return taga(r);
}
static NOINLINE B merge_fill_result_k(B rc, ur k, usz* sh) {
  return merge_fill_result_impl(U'⎉', rc, k, sh);
}
static NOINLINE B merge_fill_result_1(B rc) {
  return merge_fill_result_impl(U'˘', rc, 1, (usz[]){0});
}



// fast special-case implementations
static NOINLINE B select_cells(usz n, B x, ur xr) {
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
      for (usz i = 0; i < cam; i++) rp.a[i] = Get(x, i*csz+n);
      NOGC_E; r=rp.b;
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
      MAKE_MUT_INIT(rm, cam*rs, TI(x,elType)); MUTG_INIT(rm);
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

static NOINLINE B shift_cells(B f, B x, u8 e, u8 rtid) {
  MAKE_MUT_INIT(r, IA(x), e); MUTG_INIT(r);
  usz cam = SH(x)[0];
  usz csz = SH(x)[1];
  assert(cam!=0 && csz!=0);
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
static NOINLINE B match_cells(bool ne, B w, B x, ur wr, ur xr, usz len) {
  usz* wsh = SH(w);
  if (wr != xr || (wr>1 && !eqShPart(wsh+1, SH(x)+1, wr-1))) {
    return allBit(ne, len);
  }
  usz csz = shProd(wsh, 1, wr);
  if (csz == 0) return allBit(!ne, len);
  u8 we = TI(w,elType);
  u8 xe = TI(x,elType);
  if (we>el_c32 || xe>el_c32) return bi_N;
  usz ww = csz * elWidth(we); u8* wp = tyany_ptr(w);
  usz xw = csz * elWidth(xe); u8* xp = tyany_ptr(x);
  u64* rp; B r = m_bitarrv(&rp, len);
  if (csz == 1 && we == xe) {
    CmpAAFn cmp = ne ? CMP_AA_FN(ne,we) : CMP_AA_FN(eq,we);
    CMP_AA_CALL(cmp, rp, wp, xp, len);
  } else {
    if (we==el_bit || xe==el_bit) return bi_N;
    EqFnObj eqfn = EQFN_GET(we, xe);
    for (usz i = 0; i < len; i++) {
      bitp_set(rp, i, ne^EQFN_CALL(eqfn, wp, xp, csz));
      wp += ww; xp += xw;
    }
  }
  return r;
}

static B transp_cells(ur ax, B x) {
  i8* wp; B w=m_i8arrv(&wp, 2); wp[0]=0; wp[1]=ax;
  return C2(transp, w, x);
}



// ˘ helpers
static NOINLINE B cell2_empty(B f, B w, B x, ur wr, ur xr) {
  if (!isPureFn(f) || !CATCH_ERRORS) { dec(w); dec(x); return emptyHVec(); }
  if (wr) w = to_fill_cell_1(w);
  if (xr) x = to_fill_cell_1(x);
  if (CATCH) { freeThrown(); return emptyHVec(); }
  B rc = c2(f, w, x);
  popCatch();
  return merge_fill_result_1(rc);
}

// ˘
B cell_c1(Md1D* d, B x) { B f = d->f;
  if (isAtm(x) || RNK(x)==0) {
    B r = c1(f, x);
    return isAtm(r)? m_atomUnit(r) : r;
  }
  
  if (isFun(f)) {
    if (IA(x)!=0) {
      u8 rtid = v(f)->flags-1;
      ur xr = RNK(x);
      if (rtid==n_lt && xr>1) return toCells(x);
      if (rtid==n_select && xr>1) return select_cells(0, x, xr);
      if (rtid==n_pick && xr>1 && TI(x,arrD1)) return select_cells(0, x, xr);
      if (rtid==n_couple) {
        if (xr==0) return C1(shape, x);
        Arr* r = cpyWithShape(x);
        usz* xsh = PSH(r);
        if (xr==UR_MAX) thrF("≍˘: Result rank too large (%i≡=𝕩)", xr);
        ShArr* rsh = m_shArr(xr+1);
        rsh->a[0] = xsh[0];
        rsh->a[1] = 1;
        shcpy(rsh->a+2, xsh+1, xr-1);
        return taga(arr_shReplace(r, xr+1, rsh));
      }
      if (rtid==n_shape) {
        if (xr==2) return x;
        Arr* r = cpyWithShape(x);
        usz cam = PSH(r)[0];
        usz csz = shProd(PSH(r), 1, xr);
        ShArr* rsh = m_shArr(2);
        rsh->a[0] = cam;
        rsh->a[1] = csz;
        return taga(arr_shReplace(r, 2, rsh));
      }
      if ((rtid==n_shifta || rtid==n_shiftb) && xr==2) {
        B xf = getFillR(x);
        if (!noFill(xf)) return shift_cells(xf, x, TI(x,elType), rtid);
      }
      if (rtid==n_transp) return xr<=2? x : transp_cells(xr-1, x);
      if (TY(f) == t_md1D) {
        Md1D* fd = c(Md1D,f);
        u8 rtid = fd->m1->flags-1;
        if (rtid==n_const) { f=fd->f; goto const_f; }
        if ((rtid==n_fold || rtid==n_insert) && TI(x,elType)!=el_B && isPervasiveDyExt(fd->f) && RNK(x)==2) {
          usz *sh = SH(x); usz m = sh[1];
          if (m == 1) return select_cells(0, x, 2);
          if (m <= 64 && m < sh[0]) return fold_rows(fd, x);
        }
      }
    }
  } else if (!isMd(f)) {
    const_f:;
    usz cam = SH(x)[0];
    decG(x);
    B fv = inc(f);
    if (isAtm(fv)) return C2(shape, m_f64(cam), fv);
    usz vr = RNK(fv);
    if (vr==UR_MAX) thrM("˘: Result rank too large");
    f64* shp; B sh = m_f64arrv(&shp, vr+1);
    shp[0] = cam;
    usz* fsh = SH(fv);
    PLAINLOOP for (usz i = 0; i < vr; i++) shp[i+1] = fsh[i];
    return C2(shape, sh, fv);
  }
  
  usz cam = SH(x)[0];
  if (cam==0) {
    if (!isPureFn(f) || !CATCH_ERRORS) { decG(x); return emptyHVec(); }
    B cf = to_fill_cell_1(x);
    if (CATCH) { freeThrown(); return emptyHVec(); }
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
      if (rtid==n_select && isF64(w) && xr>1)              return select_cells(WRAP(o2i64(w), SH(x)[1], thrF("⊏: Indexing out-of-bounds (𝕨≡%R, %s≡≠𝕩)", w, cam)), x, xr);
      if (rtid==n_pick && TI(x,arrD1) && xr>1 && isF64(w)) return select_cells(WRAP(o2i64(w), SH(x)[1], thrF("⊑: Indexing out-of-bounds (𝕨≡%R, %s≡≠𝕩)", w, cam)), x, xr);
      if ((rtid==n_shifta || rtid==n_shiftb) && xr==2) {
        if (isArr(w)) { B w0=w; w = IGet(w,0); decG(w0); }
        return shift_cells(w, x, el_or(TI(x,elType), selfElType(w)), rtid);
      }
      if (rtid==n_take && xr>1 && isF64(w)) return takedrop_highrank(1, m_hVec2(m_f64(SH(x)[0]), w), x);
      if (rtid==n_drop && xr>1 && isF64(w)) return takedrop_highrank(0, m_hVec2(m_f64(0),        w), x);
      if (rtid==n_transp && q_usz(w)) { usz a=o2sG(w); if (a<xr-1) return transp_cells(a+1, x); }
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
    if (isFun(f)) {
      u8 rtid = v(f)->flags-1;
      if (rtid==n_feq || rtid==n_fne) {
        B r = match_cells(rtid!=n_feq, w, x, wr, xr, cam);
        if (!q_N(r)) { decG(w); decG(x); return r; }
      }
    }
    S_SLICES(w) S_SLICES(x)
    M_HARR(r, cam);
    for (usz i=0,wp=0,xp=0; i<cam; i++,wp+=w_csz,xp+=x_csz) HARR_ADD(r, i, c2(f, SLICE(w, wp), SLICE(x, xp)));
    E_SLICES(w) E_SLICES(x)
    r = HARR_FV(r);
  }
  return bqn_merge(r);
}

// ⎉ helpers
static NOINLINE B empty_frame(usz* xsh, ur k) {
  HArr_p f = m_harrUp(0);
  Arr *a = (Arr*)f.c;
  if (k <= 1) arr_shVec(a); else shcpy(arr_shAlloc(a,k), xsh, k);
  return f.b;
}
static NOINLINE B rank2_empty(B f, B w, ur wk, B x, ur xk) {
  B fa = wk>xk?w:x;
  ur k = wk>xk?wk:xk;
  usz* sh = SH(fa);
  usz s0=0; ShArr* s=NULL; ur sho=RNK(fa)>1;
  if (!sho) { s0=sh[0]; sh=&s0; } else { s=ptr_inc(shObj(fa)); }
  if (!isPureFn(f) || !CATCH_ERRORS) { dec(w); dec(x); goto empty; }
  B r;
  if (wk) w = to_fill_cell_k(w, wk);
  if (xk) x = to_fill_cell_k(x, xk);
  if (CATCH) { empty:
    freeThrown();
    r = empty_frame(sh, k);
  } else {
    B rc = c2(f, w, x);
    popCatch();
    r = merge_fill_result_k(rc, k, sh);
  }
  if (sho) ptr_dec(s);
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

// ⎉
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
    B cf = to_fill_cell_k(x, k);
    B r;
    if (CATCH) { empty:
      freeThrown();
      r = empty_frame(xsh, k);
    } else {
      B rc = c1(f, cf);
      popCatch();
      r = merge_fill_result_k(rc, k, xsh);
    }
    if (xr>1) ptr_dec(s);
    return r;
  }
  
  M_HARR(r, cam);
  S_KSLICES(x, xsh, k);
  for (usz i=0,p=0; i<cam; i++,p+=x_csz) HARR_ADD(r, i, c1(f, SLICE(x, p)));
  usz* rsh = HARR_FA(r, k);
  if (k>1) shcpy(rsh, xsh, k);
  E_SLICES(x);
  
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
      ShArr* csh ONLY_GCC(=0);
      if (xc>1) { csh=m_shArr(xc); shcpy(csh->a, xsh+k, xc); }

      BSS2A slice = TI(x,slice);
      M_HARR(r, cam);
      usz p = 0;
      incBy(w, cam);
      incByG(x, cam);
      for (usz i = 0; i < cam; i++) {
        Arr* s = arr_shSetI(slice(x, p, csz), xc, csh);
        HARR_ADD(r, i, c2(f, w, taga(s)));
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
    ShArr* csh ONLY_GCC(=0);
    if (wc>1) { csh=m_shArr(wc); shcpy(csh->a, wsh+k, wc); }

    BSS2A slice = TI(w,slice);
    M_HARR(r, cam);
    usz p = 0;
    incByG(w, cam);
    incBy(x, cam);
    for (usz i = 0; i < cam; i++) {
      Arr* s = arr_shSetI(slice(w, p, csz), wc, csh);
      HARR_ADD(r, i, c2(f, taga(s), x));
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

    ShArr* wcs ONLY_GCC(=0); if (wc>1) { wcs=m_shArr(wc); shcpy(wcs->a, wsh+wk, wc); }
    ShArr* xcs ONLY_GCC(=0); if (xc>1) { xcs=m_shArr(xc); shcpy(xcs->a, xsh+xk, xc); }

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
        CELL(w); B wb=taga(ptr_incBy(ws, ext));
        for (usz e = i+ext; i < e; i++) { CELL(x); F(wb, taga(xs)); }
        dec(wb);
      }
    } else {
      for (usz i = 0; i < cam; ) {
        CELL(x); B xb=taga(ptr_incBy(xs, ext));
        for (usz e = i+ext; i < e; i++) { CELL(w); F(taga(ws), xb); }
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