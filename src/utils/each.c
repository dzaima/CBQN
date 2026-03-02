#include "../core.h"
#include "each.h"

static inline B  mv(B*     p, usz n) { B r = p  [n]; p  [n] = m_f64(0); return r; }
static inline B hmv(HArr_p p, usz n) { B r = p.a[n]; p.a[n] = m_f64(0); return r; }

B eachd_fn(B fo, B w, B x, FC2 f) {
  ur wr, xr; // if rank is 0, respective w/x will be disclosed
  if (isArr(w)) { wr=RNK(w); if (wr==0) w = TO_GET(w,0); } else wr=0;
  if (isArr(x)) { xr=RNK(x); if (xr==0) x = TO_GET(x,0); } else xr=0;
  bool wg = wr>xr;
  ur rM = wg? wr : xr;
  ur rm = wg? xr : wr;
  if (rM==0) return squeezed_unit(f(fo, w, x));
  if (rm && !eqShPart(SH(w), SH(x), rm)) thrF("Mapping: Expected equal shape prefix (%H ≡ ≢𝕨, %H ≡ ≢𝕩)", w, x);
  bool rw = rM==wr && reusable(w) && TY(w)==t_harr; // dereferencing is safe as rank>0 from rM==
  bool rx = rM==xr && reusable(x) && TY(x)==t_harr;
  if (rw|rx && (wr==xr | rm==0)) {
    HArr_p r = harr_parts(REUSE(rw? w : x));
    usz ria = r.c->ia;
    if (ria>0) {
      if      (wr==0) { incBy(w, ria-1); for(usz i=0; i<ria; i++) r.a[i] = f(fo, w,   hmv(r,i)); }
      else if (xr==0) { incBy(x, ria-1); for(usz i=0; i<ria; i++) r.a[i] = f(fo, hmv(r,i), x  ); }
      else {
        assert(wr==xr);
        if (rw) { SGet(x) for (usz i = 0; i < ria; i++) r.a[i] = f(fo, hmv(r,i), Get(x,i)); }
        else    { SGet(w) for (usz i = 0; i < ria; i++) r.a[i] = f(fo, Get(w,i), hmv(r,i)); }
        decG(rw? x : w);
      }
    } else {
      dec(rw? x : w);
    }
    return squeeze_any(r.b);
  }
  
  B bo = wg? w : x;
  usz ria = IA(bo);
  B rb;
  if (ria==0) {
    rb = rM==1? emptyHVec() : m_harrUc(bo).b;
  } else {
    M_HARR(r, ria)
    if (wr==xr) {            SGet(x) SGet(w) for(usz ri=0; ri<ria; ri++) HARR_ADD(r, ri, f(fo, Get(w,ri), Get(x,ri))); }
    else if (wr==0) { incBy(w, ria); SGet(x) for(usz ri=0; ri<ria; ri++) HARR_ADD(r, ri, f(fo, w        , Get(x,ri))); }
    else if (xr==0) { incBy(x, ria); SGet(w) for(usz ri=0; ri<ria; ri++) HARR_ADD(r, ri, f(fo, Get(w,ri), x        )); }
    else {
      usz min = wg? IA(x) : IA(w);
      usz ext = ria / min;
      SGet(w) SGet(x)
      if (wg) for (usz i = 0; i < min; i++) { B c=incBy(Get(x,i), ext-1); for (usz j=0; j<ext; j++) HARR_ADDA(r, f(fo, Get(w,HARR_I(r)), c)); }
      else    for (usz i = 0; i < min; i++) { B c=incBy(Get(w,i), ext-1); for (usz j=0; j<ext; j++) HARR_ADDA(r, f(fo, c, Get(x,HARR_I(r)))); }
    }
    rb = squeeze_any(HARR_FC(r, bo));
  }
  dec(w); dec(x);
  return rb;
}

B eachm_fn(B fo, B x, FC1 f) {
  usz ia = IA(x);
  usz i = 0;
  if (ia==0) return x;
  if (reusable(x)) {
    B* xp;
    re_reuse:
    switch (v(x)->type) {
      case t_fillarr: {
        dec(c(FillArr,x)->fill);
        c(FillArr,x)->fill = bi_noFill;
        xp = fillarrv_ptr(a(x));
        break;
      }
      case t_harr: {
        xp = harr_ptr(x);
        break;
      }
      case t_fillslice: {
        FillSlice* s = c(FillSlice,x);
        Arr* p = s->p;
        if (p->refc==1 && ((p->type==t_fillarr && ((FillArr*)p)->a==s->a) || (p->type==t_harr && harrv_ptr(p)==s->a)) && PIA(p)==ia) {
          ur sr = PRNK(s);
          if (sr<=1) {
            arr_shErase(p, sr);
          } else {
            usz* ssh = PSH(s);
            if (ssh != PSH(p)) arr_shReplace(p, sr, ptr_inc(shObjS(ssh)));
          }
          
          x = taga(ptr_inc(p));
          value_free((Value*)s);
          goto re_reuse;
        } else goto base;
      }
      default: goto base;
    }
    REUSE(x);
    for (; i < ia; i++) xp[i] = f(fo, mv(xp, i));
    return squeeze_any(x);
  }
  
  base:;
  M_HARR(r, ia)
  void* xp = tyany_ptr(x);
  switch(TI(x,elType)) { default: UD;
    case el_B: { SGet(x);
               { for (; i<ia; i++) HARR_ADD(r, i, f(fo, Get(x,i))); } break; }
    case el_bit: for (; i<ia; i++) HARR_ADD(r, i, f(fo, m_i32(bitp_get(xp,i)))); break;
    case el_i8:  for (; i<ia; i++) HARR_ADD(r, i, f(fo, m_i32(((i8* )xp)[i]))); break;
    case el_i16: for (; i<ia; i++) HARR_ADD(r, i, f(fo, m_i32(((i16*)xp)[i]))); break;
    case el_i32: for (; i<ia; i++) HARR_ADD(r, i, f(fo, m_i32(((i32*)xp)[i]))); break;
    case el_f64: for (; i<ia; i++) HARR_ADD(r, i, f(fo, m_f64(((f64*)xp)[i]))); break;
    case el_c8:  for (; i<ia; i++) HARR_ADD(r, i, f(fo, m_c32(((u8* )xp)[i]))); break;
    case el_c16: for (; i<ia; i++) HARR_ADD(r, i, f(fo, m_c32(((u16*)xp)[i]))); break;
    case el_c32: for (; i<ia; i++) HARR_ADD(r, i, f(fo, m_c32(((u32*)xp)[i]))); break;
  }
  return squeeze_any(HARR_FCD(r, x));
}



#define FI_REF 1 // whether incG is needed to get an owned copy
#define FI_NEW 2 // whether decG is needed in case the result is unneeded; implies FI_UPD
#define FI_UPD 4 // whether this is different from the input
typedef struct {
  B r;
  u8 fl; // if neither FI_REF nor FI_NEW are set, !isVal(r)
} FillRes;
#define RET_FILL(R,FL) return (FillRes){R,FL}

static B fill_own(FillRes r) {
  return r.fl & FI_REF ? incG(r.r) : r.r;
}

static UntaggedArr copyFillarr(B x, ux ia) {
  Arr* r = m_fillarrp(ia);
  B* rp = fillarrv_ptr(r);
  fillarr_setFill(r, m_f64(0));
  COPY_TO(rp, el_B, 0, x, 0, ia);
  NOGC_E;
  arr_shCopy(r, x);
  return (UntaggedArr){r, rp};
}

static NOINLINE FillRes asChrFill(B x) {
  RET_FILL(taga(arr_shCopy(reshape_one(IA(x), m_c32(' ')), x)), FI_NEW|FI_UPD);
}
static NOINLINE FillRes asNumFill(B x) {
  RET_FILL(taga(arr_shCopy(allZeroesFl(IA(x)), x)), FI_NEW|FI_UPD);
}
static NOINLINE FillRes asNoFill(B x) { // semantically, replace fill and elements of x with bi_noFill
  if (IA(x) == 0) RET_FILL(RNK(x)==1? emptyHVec() : m_harrUc(x).b, FI_NEW|FI_UPD);
  RET_FILL(bi_noFill, FI_UPD);
}

static FillRes fill_updateCore(B x, FillRes (*fn)(B x)) {
  debug_assert(TI(x,elType)==el_B); // not strictly required by any code here, but if this weren't the case the squeeze_any's would benefit from an elType quick path
  SGetU(x)
  Arr* r = NULL; B* rp;
  ux ia = IA(x);
  for (ux i = 0; i < ia; i++) {
    FillRes c = fn(GetU(x,i));
    if (c.fl & FI_NEW) assert(c.fl & FI_UPD);
    
    if (c.fl & FI_UPD) {
      if (RARE(noFill(c.r))) {
        if (r) ptr_dec(r);
        RET_FILL(bi_noFill, FI_UPD);
      }
      if (RARE(!r)) {
        UntaggedArr u = copyFillarr(x, ia);
        r = u.obj;
        rp = u.data;
      }
      dec(rp[i]);
      rp[i] = fill_own(c);
    }
  }
  
  FillRes f = fn(getFillN(x));
  if (r) {
    fillarr_setFill(r, fill_own(f));
    RET_FILL(squeeze_any(taga(r)), FI_NEW|FI_UPD);
  } else if (f.fl & FI_UPD) {
    RET_FILL(withFill(squeeze_any(incG(x)), f.r), FI_NEW|FI_UPD);
  } else {
    B sq = squeeze_any(incG(x));
    if (sq.u == x.u) {
      decG(sq);
      RET_FILL(x, FI_REF);
    } else {
      RET_FILL(sq, FI_NEW|FI_UPD);
    }
  }
}

// the following functions assume a valid fill input, and recursively, incl. through fills, does the specified transformation

static FillRes fill_charToErr(B x) { // doesn't consume; num→num, char→bi_noFill
  if (noFill(x) || numFill(x)) RET_FILL(x, 0);
  if (chrFill(x)) RET_FILL(bi_noFill, FI_UPD);
  assert(isArr(x));
  
  u8 xe = TI(x,elType);
  if (elNum(xe)) RET_FILL(x, FI_REF);
  if (elChr(xe)) return asNoFill(x);
  
  return fill_updateCore(x, fill_charToErr);
}

static FillRes fill_charToNum(B x) { // doesn't consume; num→num, char→num
  if (!isArr(x)) {
    if (noFill(x) || numFill(x)) RET_FILL(x, 0);
    if (chrFill(x)) RET_FILL(m_f64(0), FI_UPD);
    UD;
  }
  u8 xe = TI(x,elType);
  if (elNum(xe)) RET_FILL(x, FI_REF);
  if (elChr(xe)) return asNumFill(x);
  
  return fill_updateCore(x, fill_charToNum);
}

static FillRes fill_charOnlyToNum(B x) { // doesn't consume; num→bi_noFill, char→num
  if (!isArr(x)) {
    if (noFill(x)) RET_FILL(x, 0);
    if (chrFill(x)) RET_FILL(m_f64(0), FI_UPD);
    if (numFill(x)) RET_FILL(bi_noFill, FI_UPD);
    UD;
  }
  
  u8 xe = TI(x,elType);
  if (elChr(xe)) return asNumFill(x);
  if (elNum(xe)) return asNoFill(x);
  
  return fill_updateCore(x, fill_charOnlyToNum);
}

static FillRes fill_numOnlyToChar(B x) { // doesn't consume; num→char, char→bi_noFill
  if (!isArr(x)) {
    if (noFill(x)) RET_FILL(x, 0);
    if (numFill(x)) RET_FILL(m_c32(' '), FI_UPD);
    if (chrFill(x)) RET_FILL(bi_noFill, FI_UPD);
    UD;
  }
  u8 xe = TI(x,elType);
  if (elNum(xe)) return asChrFill(x);
  if (elChr(xe)) return asNoFill(x);
  
  return fill_updateCore(x, fill_numOnlyToChar);
}

static FillRes fill_swapNumChar(B x) { // doesn't consume; num→char, char→num
  if (!isArr(x)) {
    if (noFill(x)) RET_FILL(x, 0);
    if (numFill(x)) RET_FILL(m_c32(' '), FI_UPD);
    if (chrFill(x)) RET_FILL(m_f64(0), FI_UPD);
    UD;
  }
  u8 xe = TI(x,elType);
  if (elNum(xe)) return asChrFill(x);
  if (elChr(xe)) return asNumFill(x);
  
  return fill_updateCore(x, fill_swapNumChar);
}

static FillRes fill_allToNone(B x) { // doesn't consume; changes both nums and chars to bi_noFill, but preserves the structure of empty arrays
  if (!isArr(x)) RET_FILL(bi_noFill, FI_UPD);
  u8 xe = TI(x,elType);
  if (xe != el_B) return asNoFill(x);
  return fill_updateCore(x, fill_allToNone);
}

static NOINLINE FillRes fill_fixedNum(ux mixed, B arr, u32 cfg) {
  // printf("fill_fixedNum: %zu\n", mixed&3);
  switch (mixed & 3) { default: UD;
    case 0:     return fill_charToErr(arr);
    case A_NUM: return fill_charToNum(arr);
    case A_CHR: RET_FILL(arr, FI_REF);
  }
}
static NOINLINE FillRes fill_fixedChr(ux mixed, B arr, u32 cfg) {
  // printf("fill_fixedChr: %zu %d\n", mixed&3, (cfg>>6) & 3);
  switch ((mixed&3) | ((cfg>>6) & 3)<<2) { default: UD;
    //   numarr|chrarr<<2
    case 0               : return fill_allToNone(arr);
    case A_CHR           : return fill_numOnlyToChar(arr);
    case         A_NUM<<2: return fill_charOnlyToNum(arr);
    case A_NUM | A_NUM<<2: return fill_charToNum(arr);
    case A_CHR | A_NUM<<2: return fill_swapNumChar(arr);
  }
}
static FillRes fill_fixed(bool fixedNum, B arr, u32 cfg, ux mixed_chr, ux mixed_num) {
  assert(isArr(arr));
  return fixedNum? fill_fixedNum(mixed_num, arr, cfg) : fill_fixedChr(mixed_chr, arr, cfg);
}

static u8 fill_case(u32 cfg, bool wnum, bool xnum) {
  return (cfg >> (A_SHW*!wnum + A_SHX*!xnum)) & 3;
}

static FillRes fill_evalDy(B w, B x, u32 cfg) { // doesn't consume; result FI_UPD is unspecified
  assert(!noFill(w) && !noFill(x) && (cfg&A_NN(A_NUM))!=0);
  bool fixedNum;
  if (0) { fixedW: return fill_fixed(fixedNum, x, cfg, cfg>>A_SHW, cfg>>A_SHX); }
  if (0) { fixedX: return fill_fixed(fixedNum, w, cfg, cfg>>A_SHX, cfg>>A_SHW); }
  
  // printf("fill_evalDy: %08b\n", cfg);
  if (isAtm(w)) {
    if (isAtm(x)) {
      switch (fill_case(cfg, numFill(w), numFill(x))) { default: UD;
        case 0:     RET_FILL(bi_noFill,  FI_UPD);
        case A_NUM: RET_FILL(m_f64(0),   FI_UPD);
        case A_CHR: RET_FILL(m_c32(' '), FI_UPD);
      }
    }
    fixedNum=numFill(w); goto fixedW;
  } else if (isAtm(x)) {
    fixedNum=numFill(x); goto fixedX;
  }
  
  ur wr = RNK(w);
  ur xr = RNK(x);
  bool wg = wr>xr;
  ur rm = wg? xr : wr;
  if (!eqShPart(SH(w), SH(x), rm)) RET_FILL(bi_noFill, FI_UPD);
  
  u8 we = TI(w,elType);
  u8 xe = TI(x,elType);
  if (MAY_F(wr == xr)) {
    if (we!=el_B) {
      if (xe!=el_B) {
        switch (fill_case(cfg, elNum(we), elNum(xe))) { default: UD;
          case 0: return asNoFill(w);
          case A_NUM: if (elNum(we)) RET_FILL(w, FI_REF); if (elNum(xe)) RET_FILL(x, FI_REF); return asNumFill(w);
          case A_CHR: if (elChr(we)) RET_FILL(w, FI_REF); if (elChr(xe)) RET_FILL(x, FI_REF); UD;
        }
      }
      fixedNum=elNum(we); goto fixedW;
    } else if (xe!=el_B) {
      fixedNum=elNum(xe); goto fixedX;
    }
  }
  
  ux wia = IA(w);
  ux xia = IA(x);
  ux ria = IMAX(wia, xia);
  Arr* r = m_fillarr0p(ria);
  B oM = wg? w : x;
  arr_shCopy(r, oM);
  B* rp = fillarrv_ptr(r);
  
  if (wia == xia) {
    SGetU(w) SGetU(x)
    for (ux i = 0; i < ria; i++) {
      FillRes c = fill_evalDy(GetU(w,i), GetU(x,i), cfg);
      if (RARE(noFill(c.r))) { arr_nope:
        ptr_dec(r);
        RET_FILL(bi_noFill, FI_UPD);
      }
      rp[i] = fill_own(c);
    }
  } else {
    B om = wg? x : w;
    SGetU(om) SGetU(oM)
    ux mia = wg? xia : wia;
    ux ext = ria / mia;
    ux iM = 0;
    for (ux im = 0; im < mia; im++) {
      B cm = GetU(om, im);
      for (ux j = 0; j < ext; j++) {
        B cM = GetU(oM, iM);
        FillRes c = fill_evalDy(wg? cM : cm, wg? cm : cM, cfg);
        if (RARE(noFill(c.r))) goto arr_nope;
        rp[iM] = fill_own(c);
        iM++;
      }
    }
  }
  
  B wf = getFillN(w);
  B xf = getFillN(x);
  B fill = noFill(wf) || noFill(xf) ? bi_noFill : fill_own(fill_evalDy(wf, xf, cfg));
  fillarr_setFill(r, fill);
  RET_FILL(squeeze_any(taga(r)), FI_NEW|FI_UPD);
}

B arith_recd(FC2 f, B w, B x, u32 cfg) {
  B fx = getFillN(x); if (DEBUG) validateFill(fx); if (noFill(fx)) return eachd_fn(bi_z, w, x, f);
  B fw = getFillN(w); if (DEBUG) validateFill(fw); if (noFill(fw)) return eachd_fn(bi_z, w, x, f);
  
  B fill = fill_own(fill_evalDy(fw, fx, cfg));
  B r = eachd_fn(bi_z, w, x, f);
  return withFill(r, fill);
}

B fill_charToErrOwned(B x) {
  assert(!noFill(x));
  return fill_own(fill_charToErr(x));
}
