#include "../core.h"
#include "../builtins.h"
#include "../utils/mut.h"
#include "../utils/calls.h"
#include <math.h>

B fne_c1(B, B);
B shape_c1(B, B);
B shape_c2(B, B, B);
B transp_c2(B, B, B);
B take_c2(B, B, B);
B join_c2(B, B, B);
B select_c2(B, B, B);

// from fold.c:
B fold_rows(Md1D* d, B x, usz n, usz m);
B fold_rows_bit(Md1D* d, B x, usz n, usz m);
B insert_cells_join(B x, usz* xsh, ur cr, ur k);
B insert_cells_identity(B x, B f, usz* xsh, ur xr, ur k, u8 rtid);

// from scan.c:
B scan_rows_bit(u8, B x, usz m);
B takedrop_highrank(bool take, B w, B x);
B rotate_highrank(bool inv, B w, B x);

Arr* join_cells(B w, B x, ur k); // from transpose.c

NOINLINE B interleave_cells(B w, B x, ur k) { // consumes w,x; interleave arrays, 𝕨 ≍⎉(-xk) 𝕩; assumes equal-shape args
  ux xr = RNK(x);
  if (xr==0) return C2(join, w, x);
  ShArr* rsh = m_shArr(xr+1); // TODO handle leak if join_cells fails
  usz* xsh = SH(x);
  shcpy(rsh->a, xsh, k);
  rsh->a[k] = 2;
  shcpy(rsh->a+k+1, xsh+k, xr-k);
  return taga(arr_shSetUG(join_cells(w, x, k), xr+1, rsh));
}

// from select.c:
B select_rows_B(B x, ux csz, ux cam, B inds);
B select_rows_direct(B x, ux csz, ux cam, void* inds, ux indn, u8 ie);

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

#define SLICE(X, S) taga(arr_shSetUO(X##_slc(X, S, X##_csz), X##_cr, X##_csh))
#define SLICEI(X) ({ B r = SLICE(X, X##p); X##p+= X##_csz; r; })



B insert_base(B f, B x, bool has_w, B w) { // Used by Insert in fold.c
  assert(isArr(x) && RNK(x)>0);
  usz* xsh = SH(x);
  usz xn = xsh[0];
  assert(!has_w || xn>0);
  S_KSLICES(x, xsh, 1, xn, 0)
  usz p = xn*x_csz;
  B r = w;
  if (!has_w) {
    p -= x_csz; xn--;
    r = SLICE(x, p);
  }
  FC2 fc2 = c2fn(f);
  while (xn--) {
    p-= x_csz;
    r = fc2(f, SLICE(x, p), r);
  }
  return r;
}

B select_cells_base(B inds, B x0, ux csz, ux cam) { // consumes inds,x0; Used by select.c
  assert(cam!=0);
  Arr* xa = customizeShape(x0);
  usz* xsh = arr_shAlloc(xa, 2);
  xsh[0] = cam;
  xsh[1] = csz;
  B x = taga(xa);
  assert(RNK(x)==2);
  S_KSLICES(x, xsh, 1, cam, 0) incBy(inds, cam-1);
  usz shBuf[] = {cam};
  M_APD_SH_N(r, 1, shBuf, cam);
  for (usz i=0,xp=0; i<cam; i++) APDD(r, C2(select, inds, SLICEI(x)));
  return taga(APD_SH_GET(r, '\0'));
}

static B scan_cells_stride1(B f, B x, usz m) {
  // m is cell size == scan axis length
  B xf = getFillR(x);
  HArr_p r = m_harr0c(x);
  SGet(x)
  FC2 fc2 = c2fn(f);
  for (usz i=0, ia=IA(x); i<ia; ) {
    usz e = i + m;
    r.a[i] = Get(x, i); i++;
    for (; i<e; i++) r.a[i] = fc2(f, inc(r.a[i-1]), Get(x, i));
  }
  decG(x);
  return withFill(r.b, xf);
}

B scan_arith(B f, B w, B x, usz* xsh) { // Used by scan.c
  bool has_w = w.u != m_f64(0).u;
  assert(isArr(x) && (!has_w || isArr(w)));
  ur xr = RNK(x);
  usz xn = xsh[0];
  assert(xr>1 && (!has_w || xn>0));
  S_KSLICES(x, xsh, 1, xn, 0)
  usz p = 0;
  B c = w;
  M_APD_SH(r, 1, xsh);
  if (!has_w) {
    APDD(r, incG(c = SLICE(x, 0)));
    p+= x_csz;
  }
  FC2 fc2 = c2fn(f);
  for (usz i = !has_w; i < xn; i++) {
    APDD(r, incG(c = fc2(f, c, SLICE(x, p))));
    p+= x_csz;
  }
  decG(c);
  return taga(APD_SH_GET(r, 0));
}


#if TEST_CELL_FILLS
  i32 fullCellFills = 2*SEMANTIC_CATCH;
  i32 cellFillErrored = 0;
  #define DO_CELL_CATCH (fullCellFills==2)
  #define SET_FILL_ERRORED cellFillErrored = 1
#else
  #define DO_CELL_CATCH SEMANTIC_CATCH
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
  if (HEURISTIC(csz<5120>>arrTypeBitsLog(TY(b)))) {
    B s = mr==wr? w : x; // smaller argument
    s = C2(slash, m_usz(csz), taga(arr_shVec(TI(s,slice)(s,0,IA(s)))));
    assert(reusable(s) && RNK(s)==1);
    arr_shCopy(a(s), b);
    if (mr==wr) w=s; else x=s;
    return fc2(m_f64(0), w, x);
  } else {
    M_APD_SH_N(r, mr, bsh, cam);
    S_KSLICES(b, bsh, mr, cam, 1) usz bp=0;
    if (mr==wr) { SGetU(w); for (usz i=0; i<cam; i++) APDD(r, fc2(m_f64(0), GetU(w,i), SLICEI(b))); }
    else        { SGetU(x); for (usz i=0; i<cam; i++) APDD(r, fc2(m_f64(0), SLICEI(b), GetU(x,i))); }
    decG(w); decG(x);
    return taga(APD_SH_GET(r, 0));
  }
}



// fast special-case implementations
B select_cells_single(usz ind, B x, usz cam, usz l, usz csz); // from select.c
static NOINLINE B select_cells(usz ind, B x, ur xr, usz cam, usz k) { // ind ⊏⎉¯k x
  assert(xr == RNK(x) && xr>1 && k<xr);
  usz* xsh = SH(x);
  usz csz = shProd(xsh, k+1, xr);
  usz l = xsh[k];
  assert(ind < l && cam*l*csz == IA(x));
  B r = select_cells_single(ind, x, cam, l, csz);
  usz* rsh = arr_shAlloc(a(r), xr-1);
  if (rsh) {
    shcpy(rsh, xsh, k);
    shcpy(rsh+k, xsh+k+1, xr-1-k);
  }
  decG(x);
  return r;
}
static NOINLINE B pick_cells(usz ind, B x, ur xr, usz cam, usz k) { // ind <∘⊑⎉¯k x
  assert(xr == RNK(x) && xr>1 && k<=xr);
  usz* xsh = SH(x);
  usz l = shProd(xsh, k, xr);
  assert(ind < (k==xr? 1 : xsh[k]) && cam*l == IA(x));
  B r = select_cells_single(ind, x, cam, l, 1);
  usz* rsh = arr_shAlloc(a(r), k);
  if (rsh) shcpy(rsh, xsh, k);
  decG(x);
  return r;
}

static void set_column_typed(void* rp, B v, u8 e, ux p, ux stride, ux n) { // may write to all elements 0 ≤ i < stride×n, and after that too for masked stores
  assert(p < stride);
  switch(e) { default: UD;
    case el_bit: if (stride<64 && HEURISTIC(n>64)) goto bit_special;
                 NOVECTORIZE for (usz i=0; i<n; i++, p+= stride) bitp_set(rp, p, o2bG(v));return;
    case el_c8 : NOVECTORIZE for (usz i=0; i<n; i++, p+= stride) ((u8 *)rp)[p] = o2cG(v); return;
    case el_c16: NOVECTORIZE for (usz i=0; i<n; i++, p+= stride) ((u16*)rp)[p] = o2cG(v); return;
    case el_c32: NOVECTORIZE for (usz i=0; i<n; i++, p+= stride) ((u32*)rp)[p] = o2cG(v); return;
    case el_i8 : NOVECTORIZE for (usz i=0; i<n; i++, p+= stride) ((i8 *)rp)[p] = o2iG(v); return;
    case el_i16: NOVECTORIZE for (usz i=0; i<n; i++, p+= stride) ((i16*)rp)[p] = o2iG(v); return;
    case el_i32: NOVECTORIZE for (usz i=0; i<n; i++, p+= stride) ((i32*)rp)[p] = o2iG(v); return;
    case el_f64: NOVECTORIZE for (usz i=0; i<n; i++, p+= stride) ((f64*)rp)[p] = o2fG(v); return;
  }
  {
    bit_special:;
    bool b = o2bG(v);
    B m1 = taga(arr_shVec(b? allZeroes(stride) : allOnes(stride)));
    bitp_set(bitarr_ptr(m1), p, b);
    B m = C2(shape, m_f64(stride*n+8), m1); // +8 to make the following loops reading past-the-end read acceptable values
    assert(TI(m,elType)==el_bit);
    u8* mp = (u8*)bitany_ptr(m);
    if (b) for (ux i = 0; i < (stride*n+7)/8; i++) ((u8*)rp)[i]|= mp[i]; // TODO call some general fns for this
    else   for (ux i = 0; i < (stride*n+7)/8; i++) ((u8*)rp)[i]&= mp[i];
    decG(m);
    return;
  }
}

static NOINLINE B shift_cells(B f, B x, usz cam, usz csz, u8 e, u8 rtid) { // »⎉1 or «⎉1
  assert(cam!=0 && csz!=0);
  usz xia = IA(x);
  if (csz==1) {
    Arr* r = arr_shCopy(reshape_one(xia, f), x);
    decG(x);
    return taga(r);
  }
  MAKE_MUT_INIT(r, xia, e); MUTG_INIT(r);
  bool after = rtid==n_shifta;
  usz p = after? csz-1 : 0;
  mut_copyG(r, after? 0 : 1, x, after, xia-1);
  if (e==el_B) {
    mut_setG(r, after? xia-1 : 0, m_f64(0));
    incBy(f, cam-1); // cam≠0 → cam-1 ≥ 0
    for (; p < xia; p+= csz) {
      mut_rm(r, p);
      mut_setG(r, p, f);
    }
  } else {
    set_column_typed(r->a, f, e, p, csz, cam);
  }
  return mut_fcd(r, x);
}

static Arr* allBit(bool b, usz n) {
  return b ? allOnes(n) : allZeroes(n);
}
static NOINLINE Arr* match_cells(bool ne, B w, B x, ur wr, ur xr, ur k, usz len) {
  usz* wsh = SH(w);
  if (wr!=xr || (wr>k && !eqShPart(wsh+k, SH(x)+k, wr-k))) {
    return allBit(ne, len);
  }
  usz csz = shProd(wsh, k, wr);
  if (csz==0) return allBit(!ne, len);
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
    MatchFnObj match = MATCH_GET(we, xe);
    for (usz i = 0; i < len; i++) {
      bitp_set(rp, i, ne^MATCH_CALL(match, wp, xp, csz));
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
  B xf = getFillR(x);
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
  Arr* r = m_fillarrpEmpty(getFillR(rc));
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

// v˙⎉(-k) x
static B const_cells(B x, ur k, usz* xsh, B v, u32 chr) { // consumes v, x
  u32 vr;
  if (isAtm(v) || RNK(v)==0) {
    if (k!=1) { vr = 0; goto rank0; }
    usz cam = xsh[0];
    decG(x);
    return C2(shape, m_usz(cam), v);
  } else {
    vr = RNK(v);
    if (vr+k > UR_MAX) thrF("%c: Result rank too large", chr);
    rank0:;
    f64* shp; B sh = m_f64arrv(&shp, k+vr);
    PLAINLOOP for (usz i=0; i<k; i++) shp[i] = xsh[i];
    if (vr) {
      usz* vsh = SH(v);
      PLAINLOOP for (usz i=0; i<vr; i++) shp[k+i] = vsh[i];
    }
    decG(x);
    return C2(shape, sh, v);
  }
}


NOINLINE B for_cells_AS(B f, B w, B x, ur wcr, ur wr, u32 chr);
NOINLINE B for_cells_SA(B f, B w, B x, ur xcr, ur xr, u32 chr); // referenced in fns.c
NOINLINE B for_cells_AA(B f, B w, B x, ur wcr, ur xcr, u32 chr);

static NOINLINE B c1wrap(B f,      B x) { B r = c1(f,    x); return isAtm(r)? m_unit(r) : r; }
static NOINLINE B c2wrap(B f, B w, B x) { B r = c2(f, w, x); return isAtm(r)? m_unit(r) : r; }
static u8 reverse_inds_64[128] = {63,62,61,60,59,58,57,56,55,54,53,52,51,50,49,48,47,46,45,44,43,42,41,40,39,38,37,36,35,34,33,32,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0}; // 64 trailing elements for overreading loads
// monadic ˘ & ⎉
B for_cells_c1(B f, u32 xr, u32 cr, u32 k, B x, u32 chr) { // F⎉cr x; array x, xr>0, 0≤cr<xr, k≡xr-cr
  assert(isArr(x) && xr>0 && k>0 && cr<xr && k == xr-cr);
  usz* xsh = SH(x);
  usz cam = shProd(xsh, 0, k); // from k>0 this will always include at least one item of shape; therefore, cam≡0 → IA(x)≡0 and IA(x)≢0 → cam≢0
  if (isFun(f)) {
    u8 rtid = RTID(f);
    switch(rtid) {
      case n_ltack: case n_rtack:
        return x;
      case n_lt:
        if (cam==0) goto noCells; // toCells/toKCells don't set outer array fill
        return k==1 && RNK(x)>1? toCells(x) : k==0? m_unit(x) : toKCells(x, k);
      case n_select:
        if (IA(x)==0) goto noSpecial;
        if (cr==0) goto base;
        return select_cells(0, x, xr, cam, k);
      case n_pick:
        if (IA(x)==0) goto noSpecial;
        if (!TI(x,arrD1)) goto base;
        return pick_cells(0, x, xr, cam, k);
      case n_couple: {
        Arr* r = cpyWithShape(x); xsh=PSH(r);
        if (xr==UR_MAX) thrF("≍%U 𝕩: Result rank too large (%i≡=𝕩)", chr==U'˘'? "˘" : "⎉𝕘", xr);
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
        if (IA(x)==0) return x;
        if (cr!=1) {
          if (cr==0) goto base;
          if (!(xsh[k]==1 // handled by fill case
             || shProd(xsh, k+1, xr)==1)) goto base;
        }
        B xf = getFillR(x);
        if (noFill(xf)) goto base;
        return shift_cells(xf, x, cam, xsh[k], TI(x,elType), rtid);
      }
      case n_transp: {
        return cr<=1? x : transp_cells(xr-1, k, x);
      }
      case n_reverse: {
        if (cr == 0) break;
        ux csz = xsh[k];
        if (csz<=1 || IA(x)==0) return x;
        u8 xe = TI(x,elType);
        if (cr==1 && csz<=64 && xe!=el_bit && csz <= (128*8 >> arrTypeBitsLog(TY(x)))) {
          incG(x); // TODO proper shape moving
          Arr* r = customizeShape(select_rows_direct(x, csz, cam, reverse_inds_64+64-csz, csz, el_i8));
          arr_shCopy(r, x);
          decG(x);
          return taga(r);
        }
        break;
      }
      // trivial cases for unhandled functions
      case n_and: case n_or: case n_find:
        if (cr == 0) break;
        if (xsh[k] <= 1) return x;
        break;
      case n_gradeUp: case n_gradeDown: case n_indexOf: case n_memberOf: case n_count: {
        if (cr == 0) break;
        usz l = xsh[k];
        if (l <= 1) {
          usz ia = l*cam;
          Arr* r = allBit(rtid==n_memberOf, ia);
          usz* rsh = arr_shAlloc(r, k+1);
          shcpy(rsh, xsh, k+1);
          decG(x); return taga(r);
        }
        break;
      }
    }
    
    if (TY(f) == t_md1D) {
      Md1D* fd = c(Md1D,f);
      u8 rtid = PRTID(fd->m1);
      switch(rtid) {
        case n_const: f=fd->f; goto const_f;
        case n_cell: cr-= cr>0; return for_cells_c1(fd->f, xr, cr, xr-cr, x, U'˘');
        case n_fold: if (cr != 1) break; // else fall through
        case n_insert: if (cr>0 && isFun(fd->f)) {
          u8 frtid = RTID(fd->f);
          if (frtid==n_join && rtid==n_insert) return insert_cells_join(x, xsh, cr, k);
          usz m = xsh[k];
          if (m==0) return insert_cells_identity(x, fd->f, xsh, xr, k, rtid);
          if (TI(x,elType)==el_B) break;
          if (m==1 || frtid==n_ltack) return select_cells(0  , x, xr, cam, k);
          if (        frtid==n_rtack) return select_cells(m-1, x, xr, cam, k);
          if (isPervasiveDyExt(fd->f) && 1==shProd(xsh, k+1, xr)) {
            B r;
            // special cases always return rank 1
            // incG(x) preserves the shape to restore afterwards if needed
            if (TI(x,elType)==el_bit) {
              incG(x); r = fold_rows_bit(fd, x, cam, m);
              if (q_N(r)) decG(x); // will try fold_rows
              else goto finish_fold;
            }
            if (m<=64 && m<cam) {
              incG(x); r = fold_rows    (fd, x, cam, m);
            }
            else break;
            finish_fold:
            if (xr > 2) {
              usz* rsh = arr_shAlloc(a(r), xr-1);
              shcpy(rsh, xsh, k);
              shcpy(rsh+k, xsh+k+1, xr-1-k);
            }
            decG(x); return r;
          }
        } break;
        case n_scan: {
          if (cr==0) break;
          usz m = xsh[k];
          if (m<=1 || IA(x)==0) return x;
          B f = fd->f;
          if (!isFun(f)) break;
          u8 frtid = RTID(f);
          if (frtid==n_rtack) return x;
          if (cr==1 || cam*m == IA(x)) {
            if (TI(x,elType)==el_bit && (isPervasiveDy(f) || frtid==n_ltack)) {
              B r = scan_rows_bit(frtid, x, m); if (!q_N(r)) return r;
            }
            if (m <= 6) return scan_cells_stride1(f, x, m);
          }
          break;
        }
        case n_undo: if (isFun(fd->f)) {
          u8 frtid = RTID(fd->f);
          if (frtid==n_couple && cr!=0 && xsh[k]==1) {
            assert(xr>=2);
            if (xr==2) return C1(shape, x);
            Arr* r = cpyWithShape(x); xsh=PSH(r);
            ShArr* rsh = m_shArr(xr-1);
            shcpy(rsh->a, xsh, k);
            shcpy(rsh->a+k, xsh+k+1, xr-k-1);
            return taga(arr_shReplace(r, xr-1, rsh));
          }
        } break;
      }
    } else if (TY(f) == t_md2D) {
      Md2D* fd = c(Md2D,f);
      u8 rtid = PRTID(fd->m2);
      if (rtid==n_before && !isCallable(fd->f)) return for_cells_SA(fd->g, inc(fd->f), x, cr, xr, chr);
      if (rtid==n_after  && !isCallable(fd->g)) return for_cells_AS(fd->f, x, inc(fd->g), cr, xr, chr);
    }
  } else if (!isMd(f)) {
    const_f:;
    return const_cells(x, k, xsh, inc(f), chr);
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
  
  M_APD_SH_N(r, k, xsh, cam);
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
    kf = o2fG(IGetU(g, gia==2));
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

SHOULD_INLINE bool unpack_unit(B* r) {
  B x = *r;
  if (isAtm(x)) return true;
  if (RNK(x)!=0) return false;
  *r = TO_GET(x,0);
  return true;
}

NOINLINE B for_cells_AS(B f, B w, B x, ur wcr, ur wr, u32 chr) { // F⟜x⎉wcr w; array w, wr>0, 0≤wcr<wr, k≡wr-wcr
  assert(isArr(w));
  ur wk = wr-wcr; assert(wk>0 && wcr<wr);
  usz* wsh=SH(w); usz cam=shProd(wsh,0,wk);
  if (cam==0) return rank2_empty(f, w, wk, x, 0, chr);
  if (isFun(f)) {
    u8 rtid = RTID(f);
    switch (rtid) {
      case n_ltack: dec(x); return w;
      case n_rtack: return const_cells(w, wk, wsh, x, chr);
      case n_couple: if (RNK(w)==1) {
        if (!unpack_unit(&x)) break;
        x = taga(arr_shVec(reshape_one(IA(w), x)));
        return interleave_cells(w, x, 1);
      } break;
    }
    if (IA(w)!=0 && isPervasiveDy(f)) {
      if (isAtm(x)) return c2(f, w, x);
      if (RNK(x)!=wcr || !eqShPart(SH(x), wsh+wk, wcr)) goto generic;
      if (TI(w,elType)==el_B || TI(x,elType)==el_B || HEURISTIC(IA(x)>(2048*8)>>arrTypeBitsLog(TY(x)) && IA(w)!=IA(x))) goto generic;
      return c2(f, w, C2(shape, C1(fne, incG(w)), x));
    }
  } else if (!isMd(f)) {
    dec(x);
    return const_cells(w, wk, wsh, inc(f), chr);
  }
  generic:;
  S_KSLICES(w, wsh, wk, cam, 1) incBy(x, cam-1);
  M_APD_SH_N(r, wk, wsh, cam); FC2 fc2 = c2fn(f);
  for (usz i=0,wp=0; i<cam; i++) APDD(r, fc2(f, SLICEI(w), x));
  decG(w); return taga(APD_SH_GET(r, chr));
}
NOINLINE B for_cells_SA(B f, B w, B x, ur xcr, ur xr, u32 chr) { // w⊸F⎉xcr x; array x, xr>0, 0≤xcr<xr, k≡xr-xcr
  assert(isArr(x));
  ur xk = xr-xcr; assert(xk>0 && xcr<xr);
  usz* xsh=SH(x); usz cam=shProd(xsh,0,xk);
  if (cam==0) return rank2_empty(f, w, 0, x, xk, chr);
  if (isFun(f)) {
    u8 rtid = RTID(f);
    switch(rtid) {
      case n_rtack: dec(w); return x;
      case n_ltack: return const_cells(x, xk, xsh, w, chr);
      case n_select:
        if (isArr(w) && xcr==1) {
          if (!TI(w,arrD1)) {
            u8 xe;
            w = squeeze_numTry(w, &xe);
            if (xe==el_B) break;
          }
          assert(xr > 1);
          ur wr = RNK(w);
          if (wr == 0) {
            usz ind = WRAP(o2i64(IGetU(w,0)), xsh[xk], break);
            decG(w);
            return select_cells(ind, x, xr, cam, xk);
          }
          ur rr = xk+wr;
          ShArr* rsh = m_shArr(rr);
          shcpy(rsh->a, xsh, xk);
          shcpy(rsh->a+xk, SH(w), wr);
          Arr* r = customizeShape(select_rows_B(x, shProd(xsh,xk,xr), cam, w));
          return taga(arr_shSetUG(r, rr, rsh));
        }
        if (isF64(w) && xcr>=1) {
          usz l = xsh[xk];
          return select_cells(WRAP(o2i64(w), l, thrF("𝕨⊏𝕩: Indexing out-of-bounds (𝕨≡%R, %s≡≠𝕩)", w, l)), x, xr, cam, xk);
        }
        break;
      case n_couple: if (RNK(x)==1) {
        if (!unpack_unit(&w)) break;
        w = taga(arr_shVec(reshape_one(IA(x), w)));
        return interleave_cells(w, x, 1);
      } break;
      case n_pick: if (isF64(w) && xcr==1 && TI(x,arrD1)) {
        usz l = xsh[xk];
        return pick_cells(WRAP(o2i64(w), l, thrF("𝕨⊑𝕩: Indexing out-of-bounds (𝕨≡%R, %s≡≠𝕩)", w, l)), x, xr, cam, xk);
      } break;
      case n_shifta: case n_shiftb: if (isAtm(w)) {
        if (IA(x)==0) return x;
        if (xcr!=1) {
          if (xcr==0) break;
          if (!(xsh[xk]==1 || shProd(xsh, xk+1, xr)==1)) break;
        }
        if (isArr(w)) w = TO_GET(w, 0);
        return shift_cells(w, x, cam, xsh[xk], el_or(TI(x,elType), selfElType(w)), rtid);
      } break;
      case n_take: case n_drop: {
        bool take = rtid==n_take;
        B a;
        if (isF64(w)) {
          if (xcr < 1) break;
          f64* ap;
          a = m_f64arrv(&ap, xk+1);
          if (!take) { FILL_TO(ap, el_f64, 0, m_f64(0), xk); }
          else { PLAINLOOP for (usz i=0; i<xk; i++) ap[i] = xsh[i]; }
          ap[xk] = o2fG(w);
        } else {
          if (!isArr(w) || RNK(w)>1) break;
          usz wia = IA(w);
          if (xcr < wia) break; // needs rank extension in the middle of x's shape
          if (!take) a = C2(take, m_f64(-(i32)(xk+wia)), w);
          else a = C2(join, C2(take, m_f64(xk), C1(fne, incG(x))), w); // (k↑≢𝕩)∾𝕨
        }
        return takedrop_highrank(take, a, x);
      } break;
      case n_reverse: {
        B a;
        if (isF64(w)) {
          if (xcr < 1) break;
          f64* ap;
          a = m_f64arrv(&ap, xk+1);
          FILL_TO(ap, el_f64, 0, m_f64(0), xk);
          ap[xk] = o2fG(w);
        } else {
          if (!isArr(w) || RNK(w)>1) break;
          usz wia = IA(w);
          if (xcr < wia) break;
          a = C2(take, m_f64(-(i32)(xk+wia)), w);
        }
        return rotate_highrank(0, a, x);
      }
      case n_transp:
        if (q_usz(w)) { usz a=o2sG(w); if (a<xcr) return transp_cells(a+xk, xk, x); }
        break;
      default: if (isPervasiveDy(f)) {
        if (isAtm(w)) return c2(f, w, x);
        if (IA(x)==0) break;
        if (RNK(w)!=xcr || !eqShPart(SH(w), xsh+xk, xcr)) break;
        if (TI(w,elType)==el_B || TI(x,elType)==el_B || HEURISTIC(IA(w)>(2048*8)>>arrTypeBitsLog(TY(w)) && IA(w)!=IA(x))) break;
        return c2(f, C2(shape, C1(fne, incG(x)), w), x);
      }
    }
  } else if (!isMd(f)) {
    dec(w);
    return const_cells(x, xk, xsh, inc(f), chr);
  }
  S_KSLICES(x, xsh, xk, cam, 1) incBy(w, cam-1);
  M_APD_SH_N(r, xk, xsh, cam); FC2 fc2 = c2fn(f);
  for (usz i=0,xp=0; i<cam; i++) APDD(r, fc2(f, w, SLICEI(x)));
  decG(x); return taga(APD_SH_GET(r, chr));
}
NOINLINE B for_cells_AA(B f, B w, B x, ur wcr, ur xcr, u32 chr) { // w F⎉wcr‿xcr x; array w & x, wr>0, 0≤wcr<wr, xr>0, 0≤xcr<xr
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
    if (wl != xl) thrF("𝕨%c𝕩: Argument frames don't agree (%H ≡ ≢𝕨, %H ≡ ≢𝕩, common frame of %i axes)", chr, w, x, k);
    cam0*= wsh[i];
  }
  usz ext = shProd(zsh, k, zk);
  usz cam = cam0*ext;
  
  if (cam==0) return rank2_empty(f, w, wk, x, xk, chr);
  
  if (isFun(f)) {
    if (wk==xk) {
      u8 rtid = RTID(f);
      if (rtid==n_rtack) { decG(w); return x; }
      if (rtid==n_ltack) { decG(x); return w; }
      if (rtid==n_feq || rtid==n_fne) {
        Arr* r = match_cells(rtid!=n_feq, w, x, wr, xr, wk, cam);
        if (r==NULL) goto generic;
        usz* rsh = arr_shAlloc(r, zk);
        if (rsh) shcpy(rsh, zsh, zk);
        decG(w); decG(x); return taga(r);
      }
      if (rtid==n_couple && wr==xr && eqShPart(wsh+wk, xsh+wk, wcr)) {
        return interleave_cells(w, x, wk);
      }
    }
    if (isPervasiveDy(f)) {
      if (TI(w,elType)==el_B || TI(x,elType)==el_B) goto generic;
      ur mr = xr<wr? xr : wr;
      if ((wk>mr?mr:wk) != (xk>mr?mr:xk) || !eqShPart(wsh, xsh, mr)) goto generic;
      return c2(f, w, x);
    }
  } else if (!isMd(f)) {
    decG(xkM? w : x);
    return const_cells(xkM? x : w, zk, zsh, inc(f), chr);
  }
  generic:;
  
  M_APD_SH_N(r, zk, zsh, cam);
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
