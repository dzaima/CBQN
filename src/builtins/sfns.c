#include "../core.h"
#include "../utils/mut.h"
#include "../utils/talloc.h"
#include "../builtins.h"

static u8 el_orFill(u8 el) { return el; } // currently arrays always fit their fill element

Arr* customizeShape(B x) {
  if (reusable(x)) {
    FL_KEEP(x, fl_squoze);
    if (RNK(x)<=1) return a(x);
    return arr_shErase(a(x), 0);
  }
  return TI(x,slice)(x,0,IA(x));
}

Arr* cpyWithShape(B x) {
  Arr* xv = a(x);
  if (reusable(x)) return FLV_KEEP(xv, fl_squoze);
  ur xr = PRNK(xv);
  Arr* r;
  if (xr<=1) {
    r = TIv(xv,slice)(x, 0, PIA(xv));
    arr_rnk01(r, xr);
  } else {
    usz* sh = PSH(xv);
    ptr_inc(shObjS(sh)); // TODO handle leak when slice allocation errors
    r = TIv(xv,slice)(x, 0, PIA(xv));
    r->sh = sh;
  }
  SPRNK(r, xr);
  return r;
}

FORCE_INLINE B m_vec2Base(B a, B b, bool fills) {
  if (isAtm(a)&isAtm(b)) {
    if (LIKELY(isNum(a)&isNum(b))) {
      i32 ai; i32 bi;
      if(RARE(!q_i32o(&ai,a)|!q_i32o(&bi,b))){f64* rp; B r = m_f64arrv(&rp, 2); rp[0]=o2fG(a); rp[1]=o2fG(b); return r; }
      else if (q_ibit(ai)   &   q_ibit(bi)) { u64* rp; B r = m_bitarrv(&rp, 2); rp[0]=ai | (bi<<1);           return r; }
      else if (ai==(i8 )ai  &  bi==(i8 )bi) { i8*  rp; B r = m_i8arrv (&rp, 2); rp[0]=ai;      rp[1]=bi;      return r; }
      else if (ai==(i16)ai  &  bi==(i16)bi) { i16* rp; B r = m_i16arrv(&rp, 2); rp[0]=ai;      rp[1]=bi;      return r; }
      else                                  { i32* rp; B r = m_i32arrv(&rp, 2); rp[0]=ai;      rp[1]=bi;      return r; }
    }
    if (isC32(b)&isC32(a)) {
      u32 ac=o2cG(a); u32 bc=o2cG(b);
      if      (ac==(u8 )ac & bc==(u8 )bc) { u8*  rp; B r = m_c8arrv (&rp, 2); rp[0]=ac; rp[1]=bc; return r; }
      else if (ac==(u16)ac & bc==(u16)bc) { u16* rp; B r = m_c16arrv(&rp, 2); rp[0]=ac; rp[1]=bc; return r; }
      else                                { u32* rp; B r = m_c32arrv(&rp, 2); rp[0]=ac; rp[1]=bc; return r; }
    }
  }
  if (fills) {
    if (isAtm(a) || isAtm(b)) goto noFills;
    B af = asFill(incG(a));
    if (noFill(af)) goto noFills;
    B bf = asFill(incG(b));
    if (noFill(bf)) { dec(af); goto noFills; }
    if (!fillEqual(af,bf)) { dec(bf); dec(af); goto noFills; }
    dec(bf);
    Arr* ra = arr_shVec(m_fillarrp(2));
    fillarr_setFill(ra, af);
    fillarrv_ptr(ra)[0] = a;
    fillarrv_ptr(ra)[1] = b;
    NOGC_E;
    return taga(ra);
  }
  noFills:
  return m_hvec2(a,b);
}

static Arr* take_head(usz ria, B x) { // consumes; returns ria↑x with unset shape, assumes ria≤≠⥊x
  if (reusable(x)) {
    usz xia = IA(x);
    u64 tgt = mm_sizeUsable(v(x)) / 4; // ÷4 to, even at minimum initial bucket utilization, require at least halving size before stopping in-place truncation
    u64 used;
    u8 xt = TY(x);
    switch (xt) {
      default: goto base;
      case t_bitarr: used = BITARR_SZ(   ria); goto try_tyarr;
      case t_i8arr:  used = TYARR_SZ(I8, ria); goto try_tyarr;
      case t_i16arr: used = TYARR_SZ(I16,ria); goto try_tyarr;
      case t_i32arr: used = TYARR_SZ(I32,ria); goto try_tyarr;
      case t_c8arr:  used = TYARR_SZ(C8, ria); goto try_tyarr;
      case t_c16arr: used = TYARR_SZ(C16,ria); goto try_tyarr;
      case t_c32arr: used = TYARR_SZ(C32,ria); goto try_tyarr;
      case t_f64arr: used = TYARR_SZ(F64,ria); goto try_tyarr;
      case t_harr:    used = fsizeof(HArr,   a,B,ria); goto try_b;
      case t_fillarr: used = fsizeof(FillArr,a,B,ria); goto try_b;
    }
    
    try_b:;
    if (used < tgt) goto try_copy;
    B* xp = arr_bptrG(x);
    for (ux i = ria; i < xia; i++) dec(xp[i]);
    goto inplace_ok;
    
    try_tyarr:;
    if (used < tgt) goto try_copy;
    
    inplace_ok:;
    reinit_portion(a(x), ria, xia);
    arr_shErase(a(x), 1);
    a(REUSE(x))->ia = ria;
    return a(x);
    
    try_copy:;
    // if (used > 64) goto base;
    UntaggedArr r = m_arrp_copyFill(x, ria);
    COPY_TO(r.data, TI(x,elType), 0, x, 0, ria);
    NOGC_E;
    decG(x);
    return r.obj;
  }
  base:;
  return TI(x,slice)(x,0,ria);
}

static Arr* take_impl(usz ria, B x, char* msg) { // consumes x; returns v↑⥊𝕩 with unset shape; v is non-negative
  assert(isArr(x));
  usz xia = IA(x);
  if (ria>xia) {
    B xf = getFillE(x, msg);
    MAKE_MUT_INIT_WITHFILL(r, ria, TI(x,elType), xf); MUTG_INIT(r);
    mut_copyG(r, 0, x, 0, xia);
    mut_fillG(r, xia, xf, ria-xia);
    decG(x);
    return mut_fp(r);
  } else {
    return take_head(ria, x);
  }
}

B m_vec2(B a, B b) { return m_vec2Base(a, b, false); }

static B truncReshape(B x, usz xia, usz nia, ur nr, ShArr* sh) { // consumes x, and sh if nr>1
  return taga(arr_shSetUO(take_head(nia, x), nr, sh));
}
static void fill_words(void* rp, u64 v, u64 bytes) {
  usz wds = bytes/8;
  usz ext = bytes%8;
  u64* p = rp;
  for (usz i=0; i<wds; i++) p[i] = v;
  if (ext) memcpy(p+wds, &v, ext);
}

NOINLINE B i64EachDec(i64 v, B x) {
  B r = taga(arr_shCopy(reshape_one(IA(x), m_f64(v)), x));
  decG(x);
  return r;
}

NOINLINE Arr* reshape_one(usz nia, B x) {
  Arr* r;
  #define FILL(E,T,V) T* rp; r = m_##E##arrp(&rp,nia); fill_words(rp, V, (u64)nia*sizeof(T));
  if (isF64(x)) {
    f64 xf = o2fG(x);
    i32 n;
    if (RARE(!q_fi32o(&n, xf))) {
      FILL(f64,f64,x.u)
    } else if (n==(i8)n) { // memset can be faster than writing words
      u8 b = n;
      void* rp; u64 nb;
      if (b <= 1) { u64* rp0; r = m_bitarrp(&rp0,nia); rp=rp0; nb = 8*BIT_N(nia); b=-n; }
      else        { i8*  rp0; r = m_i8arrp (&rp0,nia); rp=rp0; nb = nia; }
      memset(rp, b, nb);
    } else {
      if(n==(i16)n)  { FILL(i16,i16,(u16)n*0x0001000100010001U) }
      else           { FILL(i32,i32,(u32)n*0x0000000100000001U) }
    }
  } else if (isC32(x)) {
    u32 c = o2cG(x);
    if      (c==(u8 )c) { u8* rp; r = m_c8arrp(&rp,nia); memset(rp, c, nia); }
    else if (c==(u16)c) { FILL(c16,u16,c*0x0001000100010001U) }
    else                { FILL(c32,u32,c*0x0000000100000001U) }
  } else {
    incBy(x, nia); // in addition with the existing reference, this covers the filled amount & asFill
    UntaggedArr ra = m_barrp_withFill(nia, asFill(x));
    if (sizeof(B)==8) fill_words(ra.data, x.u, (u64)nia*8);
    else for (usz i = 0; i < nia; i++) ((B*)ra.data)[i] = x;
    r = ra.obj;
    NOGC_E;
  }
  #undef FILL
  return FLV_SET(r, fl_asc|fl_dsc|fl_squoze);
}

B pair_c1(B t,      B x) { return m_vec1(x); }
B pair_c2(B t, B w, B x) { return m_vec2Base(w, x, true); }

B shape_c1(B t, B x) {
  if (isAtm(x)) return m_vec1(x);
  if (RNK(x)==1) return x;
  usz ia = IA(x);
  if (ia==1 && TI(x,elType)<el_B) {
    return m_vec1(TO_GET(x,0));
  }
  if (reusable(x)) { FL_KEEP(x, fl_squoze);
    decSh(v(x)); arr_shVec(a(x));
    return x;
  }
  return taga(arr_shVec(TI(x,slice)(x, 0, ia)));
}

static void shape_c2_prim0(B c) {
  if (RARE(!isPrim(c))) {
    if (isF64(c)) thrF("𝕨⥊𝕩: 𝕨 must consist of natural numbers or ∘ ⌊ ⌽ ↑ (contained %B)", c);
    else          thrF("𝕨⥊𝕩: 𝕨 must consist of natural numbers or ∘ ⌊ ⌽ ↑ (contained %S)", genericDesc(c));
  }
}
#define SHAPE_C2_PRIM1(ID, GET) if (ID!=n_atop & ID!=n_floor & ID!=n_reverse & ID!=n_take) thrF("𝕨⥊𝕩: 𝕨 must consist of natural numbers or ∘ ⌊ ⌽ ↑ (contained %B)", GET)

Arr* reshape_cycle(usz nia, usz xia, B x);
SHOULD_INLINE Arr* reshape_unshaped(usz nia, B x) {
  if (isArr(x)) {
    usz xia = IA(x);
    if (nia <= xia) return take_head(nia, x);
    else return reshape_cycle(nia, xia, x);
  } else {
    return reshape_one(nia, x);
  }
}

B shape_c2_01(usz wia, B w, B x) {
  switch (wia) { default: UD;
    case 0: // ⟨⟩⥊x
      decG(w);
      if (isAtm(x)) return m_unit(x);
      if (RARE(IA(x) == 0)) thrM("𝕨⥊𝕩: Empty 𝕩 and non-empty result");
      return taga(arr_rnk01(take_head(1, x), 0));
    
    case 1: // ⟨x⟩⥊1
      w = TO_GET(w,0);
      // fallthrough
      usz wu;
      if (q_usz(&wu, w)) {
        return taga(arr_shVec(reshape_unshaped(wu, x)));
      }
    case 2: // atom
      shape_c2_prim0(w);
      u8 id = RTID(w);
      SHAPE_C2_PRIM1(id, w);
      decG(w);
      return C1(shape, x);
  }
}

B shape_c2_listw(B t, B w, B x);
B shape_c2(B t, B w, B x) {
  usz wu;
  if (q_usz(&wu, w)) {
    return taga(arr_shVec(reshape_unshaped(wu, x)));
  } else {
    return shape_c2_listw(t, w, x);
  }
}
NOINLINE B shape_c2_listw(B t, B w, B x) {
  if (RARE(isAtm(w))) return shape_c2_01(2, w, x);
  if (RNK(w) > 1) thrF("𝕨⥊𝕩: 𝕨 must be a list or unit (%i ≡ =𝕩)", RNK(w));
  usz wia = IA(w);
  if (wia <= 1) return shape_c2_01(wia, w, x);
  if (wia > UR_MAX) thrF("𝕨⥊𝕩: Result rank too large (%i ≡ ≠𝕨)", wia);
  
  usz nia = 1;
  ur nr = wia;
  ShArr* sh = m_shArr(nr);
  
  SGetU(w)
  i32 unkPos = -1;
  i32 unkID ONLY_GCC(=0);
  usz xia ONLY_GCC(=0);
  bool bad=false, good=false;
  for (i32 i = 0; i < nr; i++) {
    B c = GetU(w, i);
    usz cu;
    if (q_usz(&cu, c)) {
      sh->a[i] = cu;
      if (RARE(mulOn(nia, cu))) bad = true;
      good|= cu==0;
    } else {
      shape_c2_prim0(c);
      if (unkPos!=-1) thrM("𝕨⥊𝕩: 𝕨 contained multiple computed axes");
      unkPos = i;
      unkID = RTID(c);
      xia = isArr(x)? IA(x) : 1;
      good|= xia==0 | unkID==n_floor;
    }
  }
  if (bad && !good) thrM("𝕨⥊𝕩: 𝕨 too large");
  
  if (unkPos!=-1) {
    SHAPE_C2_PRIM1(unkID, GetU(w,unkPos));
    if (nia==0) thrM("𝕨⥊𝕩: Can't compute axis when the rest of the shape is empty");
    usz div = xia/nia;
    usz mod = xia%nia;
    usz item;
    bool fill = false;
    if (unkID == n_atop) {
      if (mod!=0) thrF("𝕨⥊𝕩: Shape must be exact when reshaping with ∘ (%H ≡ ≢𝕩, %s is the product of non-computed axis)", x, nia);
      item = div;
    } else if (unkID == n_floor) {
      item = div;
    } else if (unkID == n_reverse) {
      item = mod? div+1 : div;
    } else if (unkID == n_take) {
      item = mod? div+1 : div;
      fill = true;
    } else UD;
    sh->a[unkPos] = item;
    nia = uszMul(nia, item);
    if (fill) {
      if (!isArr(x)) x = m_unit(x);
      x = taga(arr_shVec(take_impl(nia, x, "𝕨⥊𝕩: Fill element of 𝕩 needed for overtaking")));
      decG(w);
      return truncReshape(x, nia, nia, nr, sh); // could be improved
    }
  }
  decG(w);
  return taga(arr_shSetUO(reshape_unshaped(nia, x), nr, sh));
}

Arr* reshape_cycle(usz nia, usz xia, B x) { // used directly by tbl_c2
  assert(nia > xia);
  Arr* r;
  if (xia <= 1) {
    if (RARE(xia == 0)) thrM("𝕨⥊𝕩: Empty 𝕩 and non-empty result");
    x = TO_GET(x, 0);
    return reshape_one(nia, x);
  }
  if (xia <= nia/2) x = squeeze_any(x);
  
  u8 xl = arrTypeBitsLog(TY(x));
  u8 xt = arrNewType(TY(x));
  u8* rp;
  u64 bi, bf; // Bytes present, bytes wanted
  if (xl == 0) { // Bits
    u64* rq; r = m_bitarrp(&rq, nia);
    rp = (u8*)rq;
    usz nw = BIT_N(nia);
    u64* xp = bitany_ptr(x);
    u64 b = xia;
    if (b % 8) {
      if (b < 64) {
        // Need to avoid calling bit_cpy with arguments <64 bits apart
        u64 v = xp[0] & (~(u64)0 >> (64-b));
        do { v |= v<<b; b*=2; } while (b%8 && b<64);
        rq[0] = v;
        if (b>64 && nia>64) rq[1] = v>>(64-b/2);
      } else {
        memcpy(rq, xp, (b+7)/8);
      }
      for (; b%8; b*=2) {
        if (b>nw*32) {
          if (b<nia) bit_cpyN(rq, b, rq, 0, nia-b);
          b = 64*nw; // Ensure bi>=bf since bf is rounded up
          break;
        }
        bit_cpyN(rq, b, rq, 0, b);
      }
    } else {
      memcpy(rp, xp, b/8);
    }
    bi = b/8;
    bf = 8*nw;
    if (bi == 1) { memset(rp, rp[0], bf); bi=bf; }
  } else {
    if (TI(x,elType) == el_B) {
      UntaggedArr r = m_barrp_copyFill(x, nia);
      i64 div = nia/xia;
      i64 mod = nia%xia;
      for (i64 i = 0; i < div; i++) COPY_TO(r.data, el_B, i*xia, x, 0, xia);
      COPY_TO(r.data, el_B, div*xia, x, 0, mod);
      NOGC_E;
      decG(x);
      return r.obj;
    }
    u8 xk = xl - 3;
    if (nia >= USZ_MAX) thrOOM();
    rp = m_tyarrp(&r, 1<<xk, nia, xt);
    bi = (u64)xia<<xk;
    bf = (u64)nia<<xk;
    memcpy(rp, tyany_ptr(x), bi);
  }
  
  decG(x);
  if (bi<=8 && !(bi & (bi-1))) {
    // Divisor of 8: write words
    usz b = bi*8;
    u64 v = *(u64*)rp & (~(u64)0 >> (64-b));
    while (b<64) { v |= v<<b; b*=2; }
    fill_words(rp, v, bf);
  } else {
    // Double up to length l, then copy in blocks
    u64 l = 1<<15; if (l>bf) l=bf;
    for (; bi<=l/2; bi+=bi) memcpy(rp+bi, rp, bi);
    u64 e=bi; for (; e+bi<=bf; e+=bi) memcpy(rp+e, rp, bi);
    if (e<bf) memcpy(rp+e, rp, bf-e);
  }
  return r;
}

B pick_c1(B t, B x) {
  if (isAtm(x)) return x;
  if (RARE(IA(x)==0)) {
    thrM("⊑𝕩: 𝕩 cannot be empty");
    // B r = getFillE(x);
    // dec(x);
    // return r;
  }
  return TO_GET(x, 0);
}

static NOINLINE void checkIndexList(B w, ur xr) {
  SGetU(w)
  usz ia = IA(w);
  for (usz i = 0; i < ia; i++) if (!isNum(GetU(w,i))) thrM("𝕨⊑𝕩: 𝕨 contained list with mixed-type elements");
  if (ia>xr+xr+10) {
    if (RNK(w)!=1) thrF("𝕨⊑𝕩: Leaf arrays in 𝕨 must have rank 1 (element in 𝕨 has shape %H)", w);
    thrF("𝕨⊑𝕩: Leaf array in 𝕨 too large (has shape %H)", w);
  }
}

// calculate index into variable RES; reads xsh, defines i for GET
#define PICK_IDX(RES, GET, RNK, OOB)        \
  usz RES = 0;                              \
  for (ux i=0, rnk_=(RNK); i < rnk_; i++) { \
    c = c*xsh[i] + WRAP(GET, xsh[i], OOB);  \
  }

static i64 pick_convFloat(f64 f) {
  i64 fi;
  if (LIKELY(q_fi64(&fi, f))) return fi;
  thrM("𝕨⊑𝕩: 𝕨 contained a non-integer");
}

static B recPick(B w, B x) { // doesn't consume
  assert(isArr(w) && isArr(x));
  usz ia = IA(w);
  ur xr = RNK(x);
  usz* xsh = SH(x);
  switch(TI(w,elType)) { default: UD;
    case el_bit: { u64* wp = bitany_ptr(w); if(RNK(w)!=1)goto wrr; if (ia!=xr)goto wrl; PICK_IDX(c, bitp_get(wp,i),        ia, goto oob) return IGet(x,c); }
    case el_i8:  { i8*  wp = i8any_ptr (w); if(RNK(w)!=1)goto wrr; if (ia!=xr)goto wrl; PICK_IDX(c, wp[i],                 ia, goto oob) return IGet(x,c); }
    case el_i16: { i16* wp = i16any_ptr(w); if(RNK(w)!=1)goto wrr; if (ia!=xr)goto wrl; PICK_IDX(c, wp[i],                 ia, goto oob) return IGet(x,c); }
    case el_i32: { i32* wp = i32any_ptr(w); if(RNK(w)!=1)goto wrr; if (ia!=xr)goto wrl; PICK_IDX(c, wp[i],                 ia, goto oob) return IGet(x,c); }
    case el_f64: { f64* wp = f64any_ptr(w); if(RNK(w)!=1)goto wrr; if (ia!=xr)goto wrl; PICK_IDX(c, pick_convFloat(wp[i]), ia, goto oob) return IGet(x,c); }
    case el_c8: case el_c16: case el_c32:
    case el_B: {
      if (ia==0) {
        if (xr!=0) thrM("𝕨⊑𝕩: 𝕩 must be a unit if 𝕨 contains an empty array");
        return IGet(x,0);
      }
      SGetU(w)
      if (isNum(GetU(w,0))) {
        if(RNK(w)!=1) goto wrr;
        if (ia!=xr) goto wrl;
        PICK_IDX(c, ({
          B cw = GetU(w,i);
          if (!isNum(cw)) thrM("𝕨⊑𝕩: 𝕨 contained list with mixed-type elements");
          o2i64(cw);
        }), ia, goto oob);
        return IGet(x,c);
      } else {
        M_HARR(r, ia);
        for (usz i=0; i<ia; i++) {
          B c = GetU(w, i);
          if (isAtm(c)) thrM("𝕨⊑𝕩: 𝕨 contained list with mixed-type elements");
          HARR_ADD(r, i, recPick(c, x));
        }
        return squeeze_any(HARR_FC(r, w));
      }
    }
  }
  #undef PICK
  
  wrr: checkIndexList(w, xr); thrF("𝕨⊑𝕩: Leaf arrays in 𝕨 must have rank 1 (element: %B)", w); // wrong index rank
  wrl: checkIndexList(w, xr); thrF("𝕨⊑𝕩: Picking item at wrong rank (index %B in array of shape %H)", w, x); // wrong index length
  oob: checkIndexList(w, xr); thrF("𝕨⊑𝕩: Indexing out-of-bounds (index %B in array of shape %H)", w, x);
}

B pick_c2(B t, B w, B x) {
  if (RARE(isAtm(x))) {
    if (isArr(w) && RNK(w)==1 && IA(w)==0) { decG(w); return x; }
    x = m_unit(x);
  }
  if (isNum(w)) {
    if (RNK(x)!=1) thrF("𝕨⊑𝕩: 𝕩 must be a list when 𝕨 is a number (%H ≡ ≢𝕩)", x);
    usz p = WRAP(o2i64(w), IA(x), thrF("𝕨⊑𝕩: indexing out-of-bounds (𝕨≡%R, %s≡≠𝕩)", w, iaW));
    return TO_GET(x, p);
  }
  if (!isArr(w)) thrM("𝕨⊑𝕩: 𝕨 must be a numeric array");
  B r = recPick(w, x);
  decG(w); decG(x);
  return r;
}

FORCE_INLINE B affixes(B x, i32 post) {
  if (!isArr(x) || RNK(x)==0) thrM(post? "↓𝕩: 𝕩 must have rank at least 1" : "↑𝕩: 𝕩 must have rank at least 1");
  ur xr = RNK(x);
  usz* xsh = SH(x);
  u64 cam = *xsh;
  u64 ria = cam+1;
  M_HARR(r, ria);
  BSS2A slice = TI(x,slice);
  if (xr==1) {
    incByG(x, cam);
    for (usz i = 0; i < ria; i++) HARR_ADD(r, i, taga(arr_shVec(slice(x, post?i:0, post?cam-i:i))));
  } else {
    incByG(x, cam+1);
    assert(xr>=2);
    usz csz = arr_csz(x);
    usz* csh = xsh+1;
    for (usz i = 0; i < ria; i++) {
      usz len = post? cam-i : i;
      Arr* c = slice(x, post? i*csz : 0, len*csz);
      usz* sh = arr_shAlloc(c, xr);
      *(sh++) = len;
      shcpy(sh, csh, xr-1);
      HARR_ADD(r, i, taga(c));
    }
    decG(x);
  }
  B rf = incG(HARR_O(r).a[post? cam : 0]);
  return withFill(HARR_FV(r), rf);
}

B take_c1(B t, B x) { return affixes(x, 0); }
B drop_c1(B t, B x) { return affixes(x, 1); }

B take_c2(B, B, B);
B drop_c2(B, B, B);
NOINLINE B takedrop_highrank(bool take, B w, B x) {
  #define SYMB (take? "↑" : "↓")
  if (!isArr(w)) goto nonint;
  ur wr = RNK(w);
  if (wr>1) thrF("𝕨%U𝕩: 𝕨 must have rank at most 1 (%H ≡ ≢𝕨)", SYMB, w);
  usz wia = IA(w);
  if (wia >= UR_MAX) thrF("𝕨%U𝕩: Result rank too large", SYMB);
  B r, w0;
  if (wia<=1) {
    if (wia==0) { r = x; goto decW_ret; }
    w0 = IGetU(w,0);
    if (!isF64(w0)) goto nonint;
    basicTake:;
    r = take? C2(take, w0, x) : C2(drop, w0, x);
    goto decW_ret;
  } else {
    // return take? c2rt(take, w, x) : c2rt(drop, w, x);
    
    ur xr = RNK(x);
    ur rr = IMAX(xr, wia);
    assert(rr>=2);
    ShArr* rsh = m_shArr(rr);
    
    TALLOC(usz, tmp, rr*5);
    usz* ltv = tmp+rr*0; // total counters
    usz* lcv = tmp+rr*1; // current counters
    usz* xcv = tmp+rr*2; // sizes to skip by in x
    usz* rcv = tmp+rr*3; // sizes to skip by in r
    usz* wn  = tmp+rr*4; // 𝕨<0
    usz* xsh = SH(x);
    SGetU(w)
    
    usz ria = 1;
    bool anyFill = false;
    i64 cellStart = -1; // axis from which whole cells can be copied
    bool bad=false, good=false;
    for (usz i = 0; i < rr; i++) {
      i64 cw = i<wia? o2i64(GetU(w, i)) : take? xsh[i] : 0;
      u64 cwa = cw<0? -cw : cw;
      wn[i] = cw<0;
      usz xshc = i<rr-xr? 1 : xsh[i-(rr-xr)];
      
      u64 c = take? cwa : cwa>=xshc? 0 : xshc-cwa;
      if (c!=xshc) cellStart = i;
      anyFill|= c>xshc;
      rsh->a[i] = c;
      good|= c==0;
      bad|= mulOn(ria, c);
    }
    if (bad && !good) thrOOM();
    
    CHECK_IA(ria, 1);
    
    if (cellStart<=0) {
      if (xr==rr) {
        decShObj(rsh);
      } else {
        Arr* xa = customizeShape(x);
        PLAINLOOP for (usz i = 0; i < rr-xr; i++) rsh->a[i] = 1;
        x = VALIDATE(taga(arr_shSetUG(xa, rr, rsh)));
      }
      if (cellStart==-1) { // printf("equal shape\n");
        r = x;
        goto decW_tfree;
      } else { // printf("last axis\n");
        w0 = GetU(w, 0);
        TFREE(tmp);
        goto basicTake;
      }
    } else if (ria==0) { // printf("empty result\n");
      r = taga(arr_shSetUG(emptyArr(x, rr), rr, rsh));
    } else { // printf("generic\n");
      B xf = getFillR(x);
      if (anyFill && noFill(xf)) {
        #if PROPER_FILLS
          thrM("𝕨↑𝕩: Fill element of 𝕩 needed for overtaking");
        #else
          xf = m_f64(0);
        #endif
      }
      
      MAKE_MUT_INIT(rm, ria, TI(x,elType)); MUTG_INIT(rm);
      if (IA(x)==0) {
        mut_fillG(rm, 0, xf, ria);
      } else { // actual generic copying code
        
        usz xcs=1, rcs=1; // current cell size
        usz xs=0, rs=0; // cumulative sum
        usz ri=0, xi=0; // index of first copy
        usz xSkip, rSkip; // amount to skip forward by
        usz cellWrite = 0; // batch write cell size
        ONLY_GCC(xSkip=rSkip=USZ_MAX/2;)
        for (usz i=rr; i-->0; ) {
          usz xshc = i<rr-xr? 1 : xsh[i-(rr-xr)];
          usz rshc = rsh->a[i];
          
          i64 diff = xshc-(i64)rshc;
          i64 off = take^wn[i]? 0 : diff; // take? (wn[i]? diff : 0) : wn[i]? 0 : diff
          if (off>0) xi+= off*xcs;
          if (off<0) ri-= off*rcs;
          
          bool pad = diff<0;
          
          // ltv[i] = lcv[i] = pad? xshc : rshc;
          // rcv[i] = rs; if ( pad) rs-= rcs*diff; rcs*= rshc;
          // xcv[i] = xs; if (!pad) xs+= xcs*diff; xcs*= xshc;
          rcv[i]=rs;
          xcv[i]=xs;
          if (pad) { rs-= rcs*diff; ltv[i]=lcv[i]=xshc; }
          else     { xs+= xcs*diff; ltv[i]=lcv[i]=rshc; }
          usz rcs0 = rcs;
          rcs*= rshc;
          xcs*= xshc;
          
          if (i==cellStart) {
            xSkip=xcs;
            rSkip=rcs;
            cellWrite = (pad? rcs0*xshc : rcs);
            xs+= cellWrite;
            rs+= cellWrite;
          }
        }
        
        usz pri = 0;
        while (true) {
          if (anyFill) { // TODO if cellWrite is a small enough number of bytes (limit possibly higher for el_bit) & elType<el_B, write all the fills at the start at once
            if (ri!=pri) mut_fillG(rm, pri, xf, ri-pri);
            pri = ri+cellWrite;
          }
          mut_copyG(rm, ri, x, xi, cellWrite); // TODO could use cf_
          usz cr = cellStart-1;
          if (0 == --lcv[cr]) {
            do {
              if (cr==0) goto endCopy;
              lcv[cr] = ltv[cr];
              cr--;
            } while (0 == --lcv[cr]);
            ri+= rcv[cr];
            xi+= xcv[cr];
          } else {
            ri+= rSkip;
            xi+= xSkip;
          }
        }
        endCopy:;
        if (anyFill && pri!=ria) mut_fillG(rm, pri, xf, ria-pri);
        
      } // end of actual generic copying code
      
      r = withFill(taga(arr_shSetUG(mut_fp(rm), rr, rsh)), xf);
    }
    decG(x);
    decW_tfree: TFREE(tmp);
    goto decW_ret;
  }
  UD;
  
  decW_ret: decG(w);
  return r;
  
  nonint: thrF("𝕨%U𝕩: 𝕨 must consist of integers", SYMB);
  #undef SYMB
}

#define TAKEDROP_INIT(TAKE) \
  if (!isArr(x)) { x = m_unit(x); } assert(isArr(x)); \
  if (!isNum(w)) return takedrop_highrank(TAKE, w, x); \
  Arr* a;                 \
  i64 wv = o2i64(w);      \
  i64 n = wv;             \
  ur xr = RNK(x);         \
  usz csz=1; usz* xsh ONLY_GCC(=0); \
  u8 fl = FL_GET(x) & (fl_asc|fl_dsc); \
  if (xr>1) {             \
    csz = arr_csz(x);     \
    xsh = SH(x);          \
    if (mulOn(n, csz)) thrOOM(); \
    ptr_inc(shObjS(xsh)); /* TODO handle leak if further allocation fails */ \
  } else xr=1;            \

#define TAKEDROP_SHAPE(SH0)     \
  FLV_SET(a, fl);               \
  if (xr>1) {                   \
    usz* rsh=arr_shAlloc(a,xr); \
    u64 wva = IABS(u64,wv);     \
    rsh[0] = SH0;               \
    shcpy(rsh+1, xsh+1, xr-1);  \
    ptr_dec(shObjS(xsh));       \
  }                             \
  return taga(a);

B take_c2(B t, B w, B x) {
  TAKEDROP_INIT(1);
  
  if (n>=0) {
    if (n != (usz)n) thrOOM();
    a = take_impl(n, x, "𝕨↑𝕩: Fill element of 𝕩 needed for overtaking");
    maybe_rank1:
    if (xr==1) return taga(arr_shVec(FLV_SET(a, fl)));
  } else {
    n = -n;
    if (n != (usz)n) thrOOM();
    usz xia = IA(x);
    if (n>xia) {
      B xf = getFillE(x, "𝕨↑𝕩: Fill element of 𝕩 needed for overtaking");
      MAKE_MUT_INIT_WITHFILL(r, n, el_orFill(TI(x,elType)), xf); MUTG_INIT(r);
      mut_fillG(r, 0, xf, n-xia);
      mut_copyG(r, n-xia, x, 0, xia);
      decG(x);
      a = a(mut_fv(r));
    } else {
      a = TI(x,slice)(x,xia-n,n);
      goto maybe_rank1;
    }
  }
  TAKEDROP_SHAPE(wva);
}
B drop_c2(B t, B w, B x) {
  TAKEDROP_INIT(0);
  
  usz xia = IA(x);
  if (n<0) {
    if (RARE(-(u64)n >= xia)) { empty:;
      a = emptyArr(x, xr);
      decG(x);
      goto res;
    }
    a = take_head(xia+n, x);
  } else {
    if (RARE(n >= xia)) goto empty;
    a = TI(x,slice)(x, n, xia-n);
  }
  
  res:;
  if (xr==1) return taga(arr_shVec(FLV_SET(a, fl)));
  TAKEDROP_SHAPE(wva>=*xsh? 0 : *xsh-wva);
}



B join_c1(B t, B x) {
  if (isAtm(x)) thrM("∾𝕩: 𝕩 must be an array");

  ur xr = RNK(x);
  usz xia = IA(x);
  if (xia==0) {
    B xf = getFillR(x);
    if (noFill(xf)) return x;
    if (isAtm(xf)) {
      decA(xf); decG(x);
      if (!PROPER_FILLS && xr==1) return emptyHVec();
      thrM("∾𝕩: Empty array 𝕩 cannot have an atom fill element");
    }
    ur ir = RNK(xf);
    if (ir<xr) thrF("∾𝕩: Empty array 𝕩 fill rank must be at least rank of 𝕩 (shape %H and fill shape %H)", x, xf);
    HArr_p r = m_harrUp(0);
    usz* sh = arr_shAlloc((Arr*)r.c, ir);
    if (sh) {
      sh[0] = 0;
      usz* fsh = SH(xf);
      if (xr>1) {
        usz* xsh = SH(x);
        PLAINLOOP for (usz i = 0; i < xr; i++) sh[i] = xsh[i]*fsh[i];
      }
      shcpy(sh+xr, fsh+xr, ir-xr);
    }
    B xff = getFillR(xf);
    decG(xf); decG(x);
    return withFill(r.b, xff);

  } else if (xr==1) {
    SGetU(x)
    B x0 = GetU(x,0);
    B rf; if(SFNS_FILLS) rf = getFillR(x0);
    ur rm = isAtm(x0) ? 0 : RNK(x0); // Maximum element rank seen
    ur rr = rm;      // Result rank, or minimum possible so far
    ur rd = 0;       // Difference of max and min lengths (0 or 1)
    usz* esh = NULL;
    usz cam = 1;     // Result length
    if (rm) {
      esh = SH(x0);
      cam = *esh++;
    } else {
      rr++;
    }

    for (usz i = 1; i < xia; i++) {
      B c = GetU(x, i);
      ur cr = isAtm(c) ? 0 : RNK(c);
      char* const badRanks = "∾𝕩: Item ranks in a list can differ by at most one (contained ranks %i and %i)";
      if (cr == 0) {
        if (rm > 1) thrF(badRanks, rm, 0);
        rd=rm; cam++;
      } else {
        usz* csh = SH(c);
        ur cd = rm - cr;
        if (RARE(cd > rd)) {
          if (cr <= rm) {
            if (cd > 1) thrF(badRanks, rm, cr);
          } else {
            if (cr-1 > rm-rd) thrF(badRanks, rm-rd, cr);
            rm = cr;
            if (cr > rr) { // Previous elements were cells
              assert(rd==0 && rr>0);
              esh--;
              usz l = *esh;
              for (usz j=1; j<i; j++) {
                B xj = GetU(x,j);
                if (l != *SH(xj)) thrF("∾𝕩: Item trailing shapes must be equal (contained arrays with shapes %H and %H and later higher-rank array)", x0, xj);
              }
              rr=cr; cam=i;
            }
          }
          rd = 1;
        }
        cam += cr < rm ? 1 : *csh++;
        if (!eqShPart(csh, esh, rm-1)) thrF("∾𝕩: Item trailing shapes must be equal (contained arrays with shapes %H and %H)", x0, c);
      }
      if (SFNS_FILLS && !noFill(rf) && !fillEqualsGetFill(rf, c)) { dec(rf); rf = bi_noFill; }
    }
    if (rm==0) thrM("∾𝕩: Some item rank must be equal or greater than rank of argument");
    
    usz csz = shProd(esh, 0, rr-1);
    M_APD_TOT(r, cam*csz);
    for (usz i = 0; i < xia; i++) APD(r, GetU(x, i));
    Arr* ra = APD_TOT_GET(r);
    usz* sh = arr_shAlloc(ra, rr);
    if (sh) {
      sh[0] = cam;
      shcpy(sh+1, esh, rr-1);
    }
    decG(x);
    return SFNS_FILLS? qWithFill(taga(ra), rf) : taga(ra);
  } else if (xr==0) {
    return bqn_merge(x, 0);
  } else {
    SGetU(x)
    B x0 = GetU(x,0);
    B rf; if(SFNS_FILLS) rf = getFillR(x0);
    ur r0 = isAtm(x0) ? 0 : RNK(x0);

    usz xia = IA(x);
    usz* xsh = SH(x);
    usz tlen = 4*xr+2*r0; PLAINLOOP for (usz a=0; a<xr; a++) tlen+=xsh[a];
    TALLOC(usz, st, tlen); // Temp buffer
    st[xr-1]=1; PLAINLOOP for (ur a=xr; a-->1; ) st[a-1] = st[a]*xsh[a]; // Stride
    usz* tsh0 = st+xr; usz* tsh = tsh0+xr+r0; // Test shapes
    // Length buffer i is lp+lp[i]
    usz* lp = tsh+xr+r0; lp[0]=xr; PLAINLOOP for (usz a=1; a<xr; a++) lp[a] = lp[a-1]+xsh[a-1];

    // Expand checked region from the root ⊑𝕩 along each axis in order,
    // so that a non-root element is checked when the axis of the first
    // nonzero in its index is reached.
    ur tr = r0; // Number of root axes remaining
    for (ur a = 0; a < xr; a++) {
      // Check the axis starting at the root, getting axis lengths
      usz n = xsh[a];
      usz *ll = lp+lp[a];
      if (n == 1) {
        if (!tr) thrM("∾𝕩: Ranks of argument items too small");
        st[a] = ll[0] = SH(x0)[r0-tr];
        tr--; continue;
      }
      usz step = st[a];
      ll[0] = r0;
      for (usz i=1; i<n; i++) {
        B c = GetU(x, i*step);
        ll[i] = LIKELY(isArr(c)) ? RNK(c) : 0;
      }
      usz r1s=r0; for (usz i=1; i<n; i++) if (ll[i]>r1s) r1s=ll[i];
      ur r1 = r1s;
      ur a0 = r1==r0;  // Root has axis a
      if (tr < a0) thrM("∾𝕩: Ranks of argument items too small");
      for (usz i=0; i<n; i++) {
        ur rd = r1 - ll[i];
        if (rd) {
          if (rd>1) thrF("∾𝕩: Item ranks along an axis can differ by at most one (contained ranks %s and %i along axis %i)", ll[i], r1, a);
          ll[i] = -1;
        } else {
          B c = GetU(x, i*step);
          ll[i] = SH(c)[r0-tr];
        }
      }

      // Check shapes
      for (usz j=0; j<xia; j+=n*step) {
        B base = GetU(x, j);
        ur r = isAtm(base) ? 0 : RNK(base);
        ur r1 = r+1-a0;
        ur lr = 0;
        if (r) {
          usz* sh=SH(base);
          lr = r - tr;
          shcpy(tsh,sh,r); shcpy(tsh0,sh,r);
          if (!a0) shcpy(tsh +lr+1, tsh +lr  , tr  );
          else     shcpy(tsh0+lr  , tsh0+lr+1, tr-1);
        }
        for (usz i=1; i<n; i++) {
          B c = GetU(x, j+i*step);
          bool rd = ll[i]==-1;
          tsh[lr] = ll[i];
          ur cr=0; usz* sh=NULL; if (!isAtm(c)) { cr=RNK(c); sh=SH(c); }
          if (cr != r1-rd) thrF("∾𝕩: Incompatible item ranks", base, c);
          if (!eqShPart(rd?tsh0:tsh, sh, cr)) thrF("∾𝕩: Incompatible item shapes (contained arrays with shapes %H and %H along axis %i)", base, c, a);
          if (SFNS_FILLS && !noFill(rf) && !fillEqualsGetFill(rf, c)) { dec(rf); rf = bi_noFill; }
        }
      }
      tr -= a0;

      // Transform to lengths by changing -1 to 1, and get total
      usz len = 0;
      for (usz i=0; i<n; i++) {
        len += ll[i] &= 1 | -(usz)(ll[i]!=-1);
      }
      st[a] = len;
    }
    
    // Shapes known correct; move the data
    usz* csh = tr ? SH(x0) + r0-tr : NULL;  // Trailing shape
    usz csz = shProd(csh, 0, tr);
    usz ria = shProd(st, 0, xr);
    if (mulOn(ria, csz)) thrOOM();
    Arr* ra;
    if (ria>0) {
      MAKE_MUT(r, ria);
      // Element index and effective shape, updated progressively
      usz *ei =tsh;  for (usz i=0; i<xr; i++) ei [i]=0;
      usz ri = 0;
      usz *ll = lp+lp[xr-1];
      for (usz i = 0;;) {
        B e = GetU(x, i);
        usz l = ll[ei[xr-1]] * csz;
        if (RARE(isAtm(e))) {
          assert(l==1);
          mut_set(r, ri, inc(e));
        } else {
          usz eia = IA(e);
          if (eia) {
            usz rj = ri;
            usz *ii=tsh0; for (usz k=0; k<xr-1; k++) ii[k]=0;
            usz str0 = st[xr-1]*csz;
            for (usz j=0;;) {
              mut_copy(r, rj, e, j, l);
              j+=l; if (j==eia) break;
              usz str = str0;
              rj += str;
              for (usz a = xr-2; RARE(++ii[a] == lp[lp[a]+ei[a]]); a--) {
                rj -= ii[a]*str;
                ii[a] = 0;
                str *= st[a];
                rj += str;
              }
            }
          }
        }
        if (++i == xia) break;
        ri += l;
        usz str = csz;
        for (usz a = xr-1; RARE(++ei[a] == xsh[a]); ) {
          ei[a] = 0;
          str *= st[a];
          a--;
          ri += (lp[lp[a]+ei[a]]-1) * str;
        }
      }
      ra = mut_fp(r);
    } else {
      ra = (Arr*) m_harrUp(0).c;
    }
    
    usz* sh = arr_shAlloc(ra, xr+tr);
    shcpy(sh   , st , xr);
    shcpy(sh+xr, csh, tr);
    TFREE(st);
    decG(x);
    return SFNS_FILLS? qWithFill(taga(ra), rf) : taga(ra);
  }
}
B join_c2(B t, B w, B x) {
  if (isAtm(w)) {
    if (isAtm(x)) return pair_c2(t, w, x);
    w = m_unit(w);
  }
  ur wr = RNK(w);
  if (isAtm(x)) {
    if (wr==1 && inplace_add(w, x)) return w;
    x = m_unit(x);
  }
  ur xr = RNK(x);
  B f = fill_both(w, x);
  
  ur c = IMAX(wr, xr);
  if (c==0) {
    HArr_p r = m_harrUv(2);
    r.a[0] = TO_GET(w, 0);
    r.a[1] = TO_GET(x, 0);
    NOGC_E;
    return qWithFill(r.b, f);
  }
  if (c-wr > 1 || c-xr > 1) thrF("𝕨∾𝕩: Argument ranks must differ by 1 or less (%i≡=𝕨, %i≡=𝕩)", wr, xr);
  
  bool usedW;
  usz wia0 = IA(w);
  B r = arr_join_inline(w, x, false, &usedW);
  if (c==1) {
    if (RNK(r)==0) SRNK(r,1);
  } else {
    assert(c>1);
    ur rnk0 = RNK(r);
    ShArr* sh0 = shObj(r);
    usz* wsh;
    if (wr<=1) {
      wsh = &wia0;
    } else {
      wsh = usedW? sh0->a : SH(w); // if usedW, arr_join_inline guarantees returning an array with shape of w if wr>1; otherwise, can just use w
    }
    usz* xsh = SH(x);
    SRNK(r, 0); // otherwise shape allocation failing may break things; leaves shape owned only here
    usz* rsh = arr_shAlloc(a(r), c);
    #if PRINT_JOIN_REUSE
    printf(usedW? "reuse:1;" : "reuse:0;");
    #endif
    for (i32 i = 1; i < c; i++) {
      usz s = xsh[i+xr-c];
      if (RARE(wsh[i+wr-c] != s)) {
        B msg = make_fmt("𝕨∾𝕩: Lengths not matchable (%2H ≡ ≢𝕨, %H ≡ ≢𝕩)", wr, wsh, x);
        if (rnk0>1) decShObj(sh0);
        mm_free((Value*)shObjS(rsh));
        arr_shVec(a(r));
        thr(msg);
      }
      rsh[i] = s;
    }
    rsh[0] = (wr==c? wsh[0] : 1) + (xr==c? xsh[0] : 1);
    if (rnk0>1) decShObj(sh0);
  }
  
  decG(x);
  if (!usedW) decG(w);
  return qWithFill(r, f);
}


B couple_c1(B t, B x) {
  if (isAtm(x)) return m_vec1(x);
  ur xr = RNK(x);
  if (xr==UR_MAX) thrF("≍𝕩: Result rank too large (%i≡=𝕩)", xr);
  Arr* r = cpyWithShape(x);
  if (xr==0) {
    arr_shVec(r);
  } else {
    ShArr* sh = m_shArr(xr+1);
    sh->a[0] = 1;
    shcpy(sh->a+1, PSH(r), xr);
    arr_shReplace(r, xr+1, sh);
  }
  return taga(r);
}
B couple_c2(B t, B w, B x) {
  if (isAtm(w)&isAtm(x)) return m_vec2(w, x);
  if (isAtm(w)) w = m_unit(w);
  if (isAtm(x)) x = m_unit(x);
  if (!eqShape(w, x)) thrF("𝕨≍𝕩: 𝕨 and 𝕩 must have equal shapes (%H ≡ ≢𝕨, %H ≡ ≢𝕩)", w, x);
  usz ia = IA(w);
  ur wr = RNK(w);
  if (wr==UR_MAX) thrM("𝕨≍𝕩: Result rank too large");
  MAKE_MUT_INIT_WITHFILL(r, ia*2, el_or(TI(w,elType), TI(x,elType)), fill_both(w, x)); MUTG_INIT(r);
  mut_copyG(r, 0,  w, 0, ia);
  mut_copyG(r, ia, x, 0, ia);
  Arr* ra = mut_fp(r);
  usz* sh = arr_shAlloc(ra, wr+1);
  if (sh) { sh[0]=2; shcpy(sh+1, SH(w), wr); }
  decG(w); decG(x);
  return taga(ra);
}


static inline void shift_check(B w, B x, u32 chr) {
  ur wr = RNK(w); usz* wsh = SH(w);
  ur xr = RNK(x); usz* xsh = SH(x);
  if (wr+1!=xr & wr!=xr) thrF("𝕨%c𝕩: =𝕨 must be =𝕩 or ¯1+=𝕩 (%i≡=𝕨, %i≡=𝕩)", chr, wr, xr);
  for (i32 i = 1; i < xr; i++) if (wsh[i+wr-xr] != xsh[i]) thrF("𝕨%c𝕩: Lengths not matchable (%H ≡ ≢𝕨, %H ≡ ≢𝕩)", chr, w, x);
}

B shiftb_c1(B t, B x) {
  if (isAtm(x) || RNK(x)==0) thrM("»𝕩: 𝕩 cannot be a unit");
  usz ia = IA(x);
  if (ia==0) return x;
  B xf = getFillE(x, "»𝕩: Fill element of 𝕩 not known");
  usz csz = arr_csz(x);
  if (ia==csz) { // length 1
    Arr* r = arr_shCopy(reshape_one(ia, xf), x);
    decG(x); return taga(r);
  }
  MAKE_MUT_INIT_WITHFILL(r, ia, el_orFill(TI(x,elType)), xf); MUTG_INIT(r);
  mut_copyG(r, csz, x, 0, ia-csz);
  mut_fillG(r, 0, xf, csz);
  return mut_fcd(r, x);
}
B shiftb_c2(B t, B w, B x) {
  if (isAtm(x) || RNK(x)==0) thrM("𝕨»𝕩: 𝕩 cannot be a unit");
  if (isAtm(w)) w = m_unit(w);
  shift_check(w, x, U'»');
  usz xia = IA(x);
  MAKE_MUT_INIT_WITHFILL(r, xia, el_or(TI(w,elType), TI(x,elType)), fill_both(w, x)); MUTG_INIT(r);
  usz wia = IA(w);
  int mid = IMIN(wia, xia);
  mut_copyG(r, 0  , w, 0, mid);
  mut_copyG(r, mid, x, 0, xia-mid);
  decG(w);
  return mut_fcd(r, x);
}

B shifta_c1(B t, B x) {
  if (isAtm(x) || RNK(x)==0) thrM("«𝕩: 𝕩 cannot be a unit");
  usz ia = IA(x);
  if (ia==0) return x;
  B xf = getFillE(x, "«𝕩: Fill element of 𝕩 not known");
  usz csz = arr_csz(x);
  if (ia==csz) { // length 1
    Arr* r = arr_shCopy(reshape_one(ia, xf), x);
    decG(x); return taga(r);
  }
  MAKE_MUT_INIT_WITHFILL(r, ia, el_orFill(TI(x,elType)), xf); MUTG_INIT(r);
  mut_copyG(r, 0, x, csz, ia-csz);
  mut_fillG(r, ia-csz, xf, csz);
  return mut_fcd(r, x);
}
B shifta_c2(B t, B w, B x) {
  if (isAtm(x) || RNK(x)==0) thrM("𝕨«𝕩: 𝕩 cannot be a unit");
  if (isAtm(w)) w = m_unit(w);
  shift_check(w, x, U'«');
  usz xia = IA(x);
  MAKE_MUT_INIT_WITHFILL(r, xia, el_or(TI(w,elType), TI(x,elType)), fill_both(w, x)); MUTG_INIT(r);
  usz wia = IA(w);
  if (wia < xia) {
    usz m = xia-wia;
    mut_copyG(r, 0, x, wia, m);
    mut_copyG(r, m, w, 0, wia);
  } else {
    mut_copyG(r, 0, w, wia-xia, xia);
  }
  decG(w);
  return mut_fcd(r, x);
}

B reverse_c1(B t, B x) {
  if (isAtm(x) || RNK(x)==0) thrM("⌽𝕩: 𝕩 cannot be a unit");
  usz n = *SH(x);
  if (n<=1) return x;
  u8 xl = cellWidthLog(x);
  u8 xt = arrNewType(TY(x));
  if (xl<=6 && (xl>=3 || xl==0)) {
    void* xv = tyany_ptr(x);
    B r;
    switch(xl) { default: UD; break;
      case 0: {
        u64* rp; r = m_bitarrc(&rp, x);
        u64* xp=xv; usz g = BIT_N(n); usz e = g-1;
        vfor (usz i = 0; i < g; i++) rp[i] = bit_reverse64(xp[e-i]);
        if (n&63) {
          u64 sh=(-n)&63;
          vfor (usz i=0; i<e; i++) rp[i]=rp[i]>>sh|rp[i+1]<<(64-sh);
          rp[e]>>=sh;
        }
        break;
      }
      case 3:                         { u8*  xp=xv; u8*  rp = m_tyarrc(&r, 1, x, xt); vfor (ux i=0; i<n; i++) rp[i]=xp[n-i-1]; break; }
      case 4:                         { u16* xp=xv; u16* rp = m_tyarrc(&r, 2, x, xt); vfor (ux i=0; i<n; i++) rp[i]=xp[n-i-1]; break; }
      case 5:                         { u32* xp=xv; u32* rp = m_tyarrc(&r, 4, x, xt); vfor (ux i=0; i<n; i++) rp[i]=xp[n-i-1]; break; }
      case 6: if (TI(x,elType)!=el_B) { u64* xp=xv; u64* rp = m_tyarrc(&r, 8, x, xt); vfor (ux i=0; i<n; i++) rp[i]=xp[n-i-1]; break; }
      else {
        HArr_p rp = m_harrUc(x);
        B* xp = arr_bptr(x);
        if (xp!=NULL)  for (ux i=0; i<n; i++) rp.a[i] = inc(xp[n-i-1]);
        else { SGet(x) for (ux i=0; i<n; i++) rp.a[i] = Get(x, n-i-1); }
        NOGC_E;
        r = rp.b;
        B xf = getFillR(x);
        decG(x);
        return withFill(r, xf);
      }
    }
    decG(x);
    return r;
  }
  B xf = getFillR(x);
  SLOW1("⌽𝕩", x);
  usz csz = arr_csz(x);
  usz cam = SH(x)[0];
  usz rp = 0;
  usz ip = IA(x);
  MAKE_MUT_INIT_WITHFILL(r, ip, TI(x,elType), xf); MUTG_INIT(r);
  for (usz i = 0; i < cam; i++) {
    ip-= csz;
    mut_copyG(r, rp, x, ip, csz);
    rp+= csz;
  }
  return mut_fcd(r, x);
}


B reverse_c2(B t, B w, B x);

#define WRAP_ROT(V, L) ({ i64 v_ = (V); usz l_ = (L); if ((u64)v_ >= (u64)l_) { v_%= (i64)l_; if(v_<0) v_+= l_; } v_; })
NOINLINE B rotate_highrank(bool inv, B w, B x) {
  #define INV (inv? "⁼" : "")
  if (RNK(w)>1) thrF("𝕨⌽%U𝕩: 𝕨 must have rank at most 1 (%H ≡ ≢𝕨)", INV, w);
  B r;
  usz wia = IA(w);
  if (isAtm(x) || RNK(x)==0) {
    if (wia!=0) goto badlen;
    r = isAtm(x)? m_unit(x) : x;
    goto decW_ret;
  }
  
  ur xr = RNK(x);
  if (wia==1) {
    lastaxis:;
    f64 wf = o2f(IGetU(w, 0));
    r = C2(reverse, m_f64(inv? -wf : wf), x);
    goto decW_ret;
  }
  if (wia>xr) goto badlen;
  
  if (wia==0) { r=x; goto decW_ret; }
  u8 we = TI(w,elType);
  if (!elNum(we)) {
    w = squeeze_numTry(w, &we, SQ_NUM);
    if (!elNum(we)) thrF("𝕨⌽%U𝕩: 𝕨 contained non-number", INV);
  }
  bool origF64 = we==el_f64;
  w = toF64Any(w);
  f64* wp = tyany_ptr(w);
  if (origF64) for (ux i = 0; i < wia; i++) o2i64(m_f64(wp[i]));
  
  if (IA(x)==0) { r=x; goto decW_ret; }
  
  usz* xsh = SH(x);
  ur cr = wia-1;
  usz rot0, l0;
  usz csz = 1;
  while (1) {
    if (cr==0) goto lastaxis;
    usz xshc = xsh[cr];
    i64 wv = WRAP_ROT((i64)wp[cr], xshc);
    if (wv != 0) {
      rot0 = inv? xshc-wv : wv;
      l0 = xshc;
      break;
    }
    csz*= xshc;
    cr--;
  }
  NOUNROLL for (usz i = xr; i-->wia; ) csz*= xsh[i];
  
  TALLOC(usz, tmp, cr*3);
  usz* pos = tmp+cr*0; // current position
  usz* rot = tmp+cr*1; // (≠𝕩)|𝕨
  usz* xcv = tmp+cr*2; // sizes to skip by in x
  
  usz ri=0, xi=0; // current index in r & x
  usz rSkip = csz*l0;
  usz ccsz = rSkip;
  for (usz i = cr; i-->0; ) {
    usz xshc = xsh[i];
    i64 v = WRAP_ROT((i64)wp[i], xshc);
    if (inv && v!=0) v = xshc-v;
    pos[i] = rot[i] = v;
    xi+= v*ccsz;
    xcv[i] = ccsz;
    ccsz*= xshc;
  }
  
  MAKE_MUT_INIT_COPYFILL(rm, IA(x), TI(x,elType), x); MUTG_INIT(rm);
  
  usz n0 = csz*rot0;
  usz n1 = csz*(l0-rot0);
  while (true) {
    mut_copyG(rm, ri+n1, x, xi, n0);
    mut_copyG(rm, ri, x, xi+n0, n1);
    usz c = cr-1;
    while (true) {
      if (xsh[c] == ++pos[c]) { xi-=xcv[c]*pos[c]; pos[c]=0; }
      xi+= xcv[c];
      if (pos[c]!=rot[c]) break;
      if (c==0) goto endCopy;
      c--;
    }
    ri+= rSkip;
  }
  endCopy:;
  
  TFREE(tmp);
  r = mut_fcd(rm, x);
  
  decW_ret: decG(w);
  return r;
  badlen: thrF("𝕨⌽%U𝕩: Length of list 𝕨 must be at most rank of 𝕩 (%s ≡ ≠𝕨, %H ≡ ≢𝕩⟩", INV, wia, x);
  #undef INV
}
B reverse_c2(B t, B w, B x) {
  if (isArr(w)) return rotate_highrank(0, w, x);
  if (isAtm(x) || RNK(x)==0) thrM("𝕨⌽𝕩: 𝕩 must have rank at least 1 for atom 𝕨");
  usz xia = IA(x);
  if (xia==0) { o2i64(w); return x; }
  usz cam = SH(x)[0];
  usz csz = arr_csz(x);
  i64 am = WRAP_ROT(o2i64(w), cam);
  if (am==0) return x;
  am*= csz;
  MAKE_MUT_INIT_COPYFILL(r, xia, TI(x,elType), x); MUTG_INIT(r);
  mut_copyG(r, 0, x, am, xia-am);
  mut_copyG(r, xia-am, x, 0, am);
  return mut_fcd(r, x);
}


static usz pick_oneIndex(B w, ur xr, usz* xsh) { // throws if guaranteed bad; returns USZ_MAX if not a plain index
  assert(xr!=0);
  if (RARE(isAtm(w) || IA(w)!=xr || RNK(w)!=1)) return USZ_MAX;
  switch(TI(w,elType)) { default: UD;
    case el_i8:  { i8*  wp = i8any_ptr (w); PICK_IDX(c, wp[i],                 xr, goto oob) return c; }
    case el_i16: { i16* wp = i16any_ptr(w); PICK_IDX(c, wp[i],                 xr, goto oob) return c; }
    case el_i32: { i32* wp = i32any_ptr(w); PICK_IDX(c, wp[i],                 xr, goto oob) return c; }
    case el_f64: { f64* wp = f64any_ptr(w); PICK_IDX(c, pick_convFloat(wp[i]), xr, goto oob) return c; }
    case el_c8: case el_c16: case el_c32: case el_bit:
    case el_B: {
      SGetU(w)
      if (!isNum(GetU(w,0))) return USZ_MAX;
      PICK_IDX(c, ({
        B cw = GetU(w,i);
        if (!isNum(cw)) thrM("𝕨⊑𝕩: 𝕨 contained list with mixed-type elements");
        o2i64(cw);
      }), xr, goto oob);
      return c;
    }
  }
  oob: checkIndexList(w, xr); thrF("𝕨⊑𝕩: Indexing out-of-bounds (index %B in array of shape %2H)", w, xr, xsh);
}

static B pick_replaceOne(B fn, usz pos, B x, usz xia) {
  if (TI(x,elType)==el_B) {
    B* xp;
    if (TY(x)==t_harr || TY(x)==t_hslice) {
      if (TY(x)==t_harr && reusable(x)) x = REUSE(x);
      else x = taga(cpyHArr(x));
      xp = harr_ptr(x);
    } else if (TY(x)==t_fillarr && reusable(x)) {
      xp = fillarrv_ptr(a(REUSE(x)));
    } else {
      Arr* x2 = m_fillarrp(xia);
      fillarr_setFill(x2, getFillR(x));
      xp = fillarrv_ptr(x2);
      COPY_TO(xp, el_B, 0, x, 0, xia);
      NOGC_E;
      arr_shCopy(x2, x);
      decG(x);
      x = taga(x2);
    }
    B c = xp[pos];
    xp[pos] = m_f64(0);
    xp[pos] = c1(fn, c);
    return x;
  }
  B arg = IGet(x, pos);
  B rep = c1(fn, arg);
  if (reusable(x) && TI(x,canStore)(rep)) { REUSE(x);
    u8 xt = TY(x);
    void* xp = tyany_ptr(x);
    switch (xt) {
      case t_i8arr:  ((i8* )xp)[pos] = o2iG(rep); return x;
      case t_i16arr: ((i16*)xp)[pos] = o2iG(rep); return x;
      case t_i32arr: ((i32*)xp)[pos] = o2iG(rep); return x;
      case t_f64arr: ((f64*)xp)[pos] = o2fG(rep); return x;
      case t_c8arr:  ((u8* )xp)[pos] = o2cG(rep); return x;
      case t_c16arr: ((u16*)xp)[pos] = o2cG(rep); return x;
      case t_c32arr: ((u32*)xp)[pos] = o2cG(rep); return x;
    }
  }
  MAKE_MUT_INIT_COPYFILL(r, xia, el_orSelf(TI(x,elType), rep), x); MUTG_INIT(r);
  mut_setG(r, pos, rep);
  mut_copyG(r, 0, x, 0, pos);
  mut_copyG(r, pos+1, x, pos+1, xia-pos-1);
  return mut_fcd(r, x);
}

B pick_uc1(B t, B o, B x) {
  if (isAtm(x) || IA(x)==0) return def_fn_uc1(t, o, x);
  return pick_replaceOne(o, 0, x, IA(x));
}



B select_replace(u32 chr, B w, B x, B rep, usz wia, usz xl, usz xcia);
B select_ucw(B t, B o, B w, B x);
B select_c2(B,B,B);
B pick_ucw(B t, B o, B w, B x) {
  if (RARE(isAtm(x))) def: return def_fn_ucw(t, o, w, x);
  usz xia = IA(x);
  usz pos;
  if (isAtm(w)) {
    if (RARE(RNK(x)!=1)) goto def;
    pos = WRAP(o2i64(w), xia, thrF("𝔽⌾(n⊸⊑)𝕩: reading out-of-bounds (n≡%R, %s≡≠𝕩)", w, xia));
  } else {
    ur xr = RNK(x);
    if (RARE(xr==0)) goto def;
    usz* xsh = SH(x);
    pos = pick_oneIndex(w, xr, xsh);
    if (pos == USZ_MAX) {
      usz wia = IA(w);
      MAKE_MUT_INIT(r, wia, xia<I8_MAX? el_i8 : xia<I16_MAX? el_i16 : xia<I32_MAX? el_i32 : el_f64); MUTG_INIT(r);
      SGetU(w)
      for (usz i = 0; i < wia; i++) {
        usz c = pick_oneIndex(GetU(w,i), xr, xsh);
        if (RARE(c==USZ_MAX)) { mut_pfree(r, i); goto def; }
        mut_setG(r, i, m_usz(c));
      }
      w = squeeze_numNew(mut_fcd(r, w));
      B rep = isArr(o)? incG(o) : c1(o, C2(select, incG(w), C1(shape, incG(x))));
      // error messages will need to get more non-trivial for deeper mismatches
      if (isAtm(rep) || !eqShape(w, rep)) thrF("𝔽⌾(nested⊸⊑)𝕩: 𝔽 must return an array with the same shape as its input (expected %0H, got %0H)", w, rep);
      return select_replace(U'⊑', w, x, rep, wia, xia, 1);
    }
    decG(w);
  }
  return pick_replaceOne(o, pos, x, xia);
}

static B takedrop_ucw(bool take, i64 wi, B o, u64 am, B x, ux xr) {
  usz xia = IA(x);
  usz csz = arr_csz(x);
  usz tk = csz*am; // taken element count
  usz lv = xia-tk; // elements left alone
  
  Arr* arg = TI(x,slice)(incG(x), (wi>=0) ^ take? lv : 0, tk);
  usz* ash = arr_shAlloc(arg, xr);
  if (ash) { ash[0] = am; shcpy(ash+1, SH(x)+1, xr-1); }
  
  B rep = c1(o, taga(arg));
  if (isAtm(rep)) thrF("𝔽⌾(n⊸%c): 𝔽 returned an atom", take?U'↑':U'↓');
  usz* repsh = SH(rep);
  if (RNK(rep)!=xr || repsh[0]!=am || !eqShPart(repsh+1, SH(x)+1, xr-1)) thrF("𝔽⌾(n⊸%c)𝕩: 𝔽 must return an array with the same shape as its input (%l ≡ n, %H ≡ shape of result of 𝔽)", take?U'↑':U'↓', wi, rep);
  
  MAKE_MUT_INIT_COPYFILL(r, xia, el_or(TI(x,elType), TI(rep,elType)), x); MUTG_INIT(r);
  if ((wi>=0) ^ take) {
    mut_copyG(r, 0, x, 0, lv);
    mut_copyG(r, lv, rep, 0, tk);
  } else {
    mut_copyG(r, 0, rep, 0, tk);
    mut_copyG(r, tk, x, tk, lv);
  }
  
  decG(rep);
  return mut_fcd(r, x);
}

B take_ucw(B t, B o, B w, B x) {
  if (!isF64(w)) return def_fn_ucw(t, o, w, x);
  i64 wi = o2i64(w);
  u64 am = IABS(u64,wi);
  if (isAtm(x)) x = m_vec1(x);
  ur xr = RNK(x); if (xr==0) xr = 1;
  if (am>SH(x)[0]) thrF("𝔽⌾(n⊸↑)𝕩: Cannot modify fill with Under (%l ≡ n, %H ≡ ≢𝕩)", wi, x);
  return takedrop_ucw(1, wi, o, am, x, xr);
}

B drop_ucw(B t, B o, B w, B x) {
  if (!isF64(w)) return def_fn_ucw(t, o, w, x);
  i64 wi = o2i64(w);
  u64 am = IABS(u64,wi);
  if (isAtm(x)) x = m_vec1(x);
  ur xr = RNK(x); if (xr==0) xr = 1;
  usz cam = SH(x)[0];
  if (am>cam) am = cam;
  return takedrop_ucw(0, wi, o, cam-am, x, xr);
}

static B shape_uc1_t(B r, usz ia) {
  if (!isArr(r) || RNK(r)!=1 || IA(r)!=ia) thrF("𝔽⌾⥊: 𝔽 must return an array with the same shape as its input (%s ≡ ≢⥊𝕩, %H ≡ shape of result of 𝔽)", ia, r);
  return r;
}
B shape_uc1(B t, B o, B x) {
  ur xr;
  if (!isArr(x) || (xr=RNK(x))==0) {
    usz xia = isArr(x)? IA(x) : 1;
    return C2(shape, emptyIVec(), shape_uc1_t(c1(o, shape_c1(t, x)), xia));
  }
  usz xia = IA(x);
  if (xr==1) return shape_uc1_t(c1(o, x), xia);
  assert(xr>1);
  ShArr* sh = ptr_inc(shObj(x));
  return truncReshape(shape_uc1_t(c1(o, shape_c1(t, x)), xia), xia, xia, xr, sh);
}

B shape_ucw(B t, B o, B w, B x) {
  if (!isArr(x)) return def_fn_ucw(t, o, w, x);
  B arg = shape_c2(t, inc(w), incG(x));
  usz xia = IA(x);
  usz aia = IA(arg);
  if (aia > xia) {
    decG(arg);
    return def_fn_ucw(t, o, w, x);
  }
  dec(w);
  B rep = c1(o, incG(arg));
  if (!isArr(rep) || !eqShape(arg, rep)) thrF("𝔽⌾(a⊸⥊): 𝔽 must return an array with the same shape as its input (%H ≡ ≢a⥊𝕩, %H ≡ shape of result of 𝔽)", arg, rep);
  
  B r;
  if (xia == aia) {
    r = taga(arr_shCopy(customizeShape(rep), x));
    decG(x);
  } else {
    MAKE_MUT_INIT_COPYFILL(rm, xia, el_or(TI(x,elType), TI(rep,elType)), x); MUTG_INIT(rm);
    mut_copyG(rm, 0, rep, 0, aia);
    mut_copyG(rm, aia, x, aia, xia-aia);
    decG(rep);
    r = mut_fcd(rm, x);
  }
  
  decG(arg);
  return r;
}


B reverse_ix(B t, B w, B x) {
  if (isAtm(x)) thrM("𝕨⌽⁼𝕩: 𝕩 cannot be an atom");
  if (isArr(w)) return rotate_highrank(1, w, x);
  if (RNK(x)==0) thrM("𝕨⌽⁼𝕩: 𝕩 must have rank at least 1 for atom 𝕨");
  if (!isF64(w)) thrM("𝕨⌽⁼𝕩: 𝕨 must consist of integers");
  return C2(reverse, m_f64(-o2fG(w)), x);
}
static i64 modmul(i64 a, i64 b, usz n) {
  // Compute am = am*wi % n in high precision
  a = WRAP_ROT(a,n);
  b = WRAP_ROT(b,n);
  if (n == (u32)n) {
    u64 p = (u64)(u32)a * (u32)b;
    return p<n ? p : p%n;
  } else {
    assert((u64)n <= 1ull<<53); // ia!=0 so we must have n<=ia<=USZ_MAX
    double d = ((double)a * b) / n;
    i64 m = a*b - (i64)d*n; // may be off by a small multiple of n
    return WRAP_ROT(m, n);  // otherwise m_f64 may lose precision
  }
}
B mul_c2(B, B, B);
B reverse_powd(i64 am, B w, B x) {
  if (am<0 && isAtm(x)) thrM("𝕨⌽⁼𝕩: 𝕩 cannot be an atom");
  if (isAtm(w)) {
    if (isAtm(x) || RNK(x)==0) thrF("𝕨⌽%U𝕩: 𝕩 must have rank at least 1 for atom 𝕨", am<0?"⁼":"");
    i64 wi = o2i64(w);
    if (IA(x)==0) return x;
    return C2(reverse, m_f64(modmul(am, wi, *SH(x))), x);
  }
  // Leave w alone and call once for empty x, result is the same
  // Also don't do any adjustment logic for erroring cases that ignore values in w (don't want to throw a different message)
  if (isArr(x) && IA(x)!=0 && RNK(w)<=1 && IA(w)<=RNK(x)) {
    i64 bd = 1<<(53-31); // <=bd times i32 gives an integer-valued float
    if (MAY_F(elInt(TI(w,elType)) && am<=bd && am>=-bd)) {
      w = C2(mul, m_f64(am<0?-am:am), w);
    } else {
      u8 we; w = squeeze_numTry(w, &we, SQ_NUM);
      if (!elNum(we)) thrF("𝕨⌽%U𝕩: 𝕨 contained non-number", am<0?"⁼":"");
      f64* rp; B r = m_f64arrc(&rp, w);
      SGetU(w); usz wia = IA(w);
      usz* xsh = SH(x);
      for (usz i=0; i<wia; i++) rp[i] = modmul(am, o2i64(GetU(w, i)), xsh[i]);
      decG(w); w=r;
    }
  }
  return rotate_highrank(am<0, w, x);
}

B drop_powd(i64 am, B w, B x) {
  assert(am > 1);
  if (isArr(x) && IA(x)==0) { // axis lengths may not fit in f64
    if (!isArr(w)) w = m_unit(w);
    if (RNK(w)>1) thrF("𝕨↓𝕩: 𝕨 must have rank at most 1 (%H ≡ ≢𝕨)", w);
    usz wia = IA(w);
    if (wia >= UR_MAX) thrM("𝕨↓𝕩: Result rank too large");
    ur xr = RNK(x);
    ur rr = IMAX(xr, wia);
    if (rr==1) { dec(w); return x; } // shape is ⟨0⟩
    ShArr* sh = m_shArr(rr);
    usz* rsh = sh->a;
    for (ur i=0; i<rr-xr; i++) rsh[i] = 1;
    shcpy(rsh+rr-xr, SH(x), xr);
    SGetU(w);
    for (usz i=0; i<wia; i++) {
      i64 wi = o2i64(GetU(w, i));
      if (mulOn(wi, am)) { rsh[i] = 0; continue; }
      if (wi<0) wi = -wi;
      usz l = rsh[i];
      rsh[i] = l<wi? 0 : l-wi;
    }
    dec(w);
    return taga(arr_shSetUG(customizeShape(x), rr, sh));
  } else if (isAtm(w)) {
    i64 wi = o2i64(w);
    if (mulOn(wi, am)) wi = USZ_MAX;
    return C2(drop, m_f64(wi), x);
  } else {
    if (elInt(TI(w,elType)) && am<=1ll<<32) {
      w = C2(mul, m_f64(am), w); // fits in i64, doesn't matter if it's precise
    } else {
      f64* rp; B r = m_f64arrc(&rp, w);
      SGetU(w); usz wia = IA(w);
      for (usz i=0; i<wia; i++) {
        i64 wi = o2i64(GetU(w, i));
        if (mulOn(wi, am)) wi = USZ_MAX;
        rp[i] = wi;
      }
      decG(w); w=r;
    }
    return takedrop_highrank(0, w, x);
  }
}

NOINLINE B enclose_im(B t, B x) {
  if (isAtm(x) || RNK(x)!=0) thrM("<⁼𝕩: Argument wasn't a rank 0 array");
  return TO_GET(x, 0);
}

NOINLINE B pair_im(B t, B x) {
  if (isAtm(x) || RNK(x)!=1 || IA(x)!=1) thrM("⋈⁼𝕩: Argument wasn't a length-1 list");
  return TO_GET(x, 0);
}

B couple_powm(i64 am, B x) {
  if (am >= 0) {
    assert(am > 1);
    if (isAtm(x)) x = m_unit(x);
    ur xr = RNK(x);
    if (am > UR_MAX-xr) thrF("≍𝕩: Result rank too large (%i≡=𝕩)", UR_MAX);
    ur a = am;
    ur rr = xr + a;
    ShArr* sh = m_shArr(rr);
    for (ur i=0; i<am; i++) sh->a[i] = 1;
    shcpy(sh->a+am, SH(x), xr);
    Arr* r = customizeShape(x);
    return taga(arr_shSetUG(r, rr, sh));
  } else {
    ur xr;
    // Can only eventually hit a rank-0 case if all axis lengths are 1
    if (isAtm(x) || (am+(xr=RNK(x))<0 && (am=-xr, IA(x)==1))) thrM("≍⁼𝕩: Argument must have a leading axis of 1 (0 ≡ =𝕩)");
    ur rr = am+xr, dr = xr-rr;
    usz* sh = SH(x);
    for (ur i=0; i<dr; i++) if (sh[i]!=1) thrF("≍⁼𝕩: Argument must have a leading axis of 1 (%i ≡ ≠𝕩)", sh[i]);
    Arr* r = TI(x,slice)(incG(x), 0, IA(x));
    usz* rsh = arr_shAlloc(r, rr);
    if (rsh) shcpy(rsh, sh+dr, rr);
    decG(x); return taga(r);
  }
}
NOINLINE B couple_im(B t, B x) { return couple_powm(-1, x); }

B shift_powm(bool aft, i64 am, B x) {
  u32 c = aft ? U'«' : U'»';
  assert(am > 1);
  if (isAtm(x) || RNK(x)==0) thrF("%c𝕩: 𝕩 cannot be a unit", c);
  usz ia = IA(x);
  if (ia==0) return x;
  B xf = getFillE(x, aft? "«𝕩: Fill element of 𝕩 not known" : "»𝕩: Fill element of 𝕩 not known");
  usz n = *SH(x);
  if (am >= n) {
    Arr* r = arr_shCopy(reshape_one(ia, xf), x);
    decG(x); return taga(r);
  }
  usz sz = (usz)am * arr_csz(x);
  usz r0 = aft? 0 : sz;
  MAKE_MUT_INIT(r, ia, el_orFill(TI(x,elType))); MUTG_INIT(r);
  mut_copyG(r, r0, x, sz-r0, ia-sz);
  mut_fillG(r, aft?ia-sz:0, xf, sz);
  return qWithFill(mut_fcd(r, x), xf);
}
static B join_cycled(bool aft, usz wl, B w, B x, usz* csh, ur cr) {
  if (wl == 0) goto no_reshape; // join_powd sets this if reshape already done
  usz ria = wl; if (mulOn(ria, shProd(csh, 0, cr))) thrOOM();
  usz wia = IA(w);
  Arr *wa = ria==wia? customizeShape(w) : reshape_cycle(ria, wia, w);
  if (cr==0) arr_shVec(wa); else {
    usz* wsh = arr_shAlloc(wa, cr+1);
    wsh[0] = wl; shcpy(wsh+1, csh, cr);
  }
  w = taga(wa);
  no_reshape:;
  if (aft) { B t=w; w=x; x=t; }
  return C2(join, w, x);
}
B shift_powd(bool aft, i64 am, B w, B x) {
  assert(am > 0);
  u32 c = aft ? U'«' : U'»';
  if (isAtm(x) || RNK(x)==0) thrF("𝕨%c𝕩: 𝕩 cannot be a unit", c);
  if (isAtm(w)) w = m_unit(w);
  shift_check(w, x, c);
  ur xr = RNK(x);
  usz xn = *SH(x); usz wn = RNK(w)<xr? 1 : *SH(w);
  if (wn >= xn) return aft? C2(shifta, w, x) : C2(shiftb, w, x);
  i64 xd = wn; if (mulOn(xd, am)) xd = xn;
  if (xd >= xn) {
    xd = xn;
    if (aft) { usz o = xn%wn; if (o) w = C2(reverse, m_usz(wn-o), w); }
  }
  x = C2(drop, m_f64(aft? xd : -xd), x);
  return join_cycled(aft, xd, w, x, SH(x)+1, xr-1);
}
B join_powd(bool aft, i64 am, B w, B x) {
  assert(am > 0);
  ur wr = isArr(w)? RNK(w) : 0;
  ur xr = isArr(x)? RNK(x) : 0;
  ur rr = IMAX(1, IMAX(wr,xr));
  ur cr = rr - 1;
  if (cr>wr || cr>xr || (cr>0 && !eqShPart(SH(w)+wr-cr, SH(x)+xr-cr, cr))) {
    if (aft) { B t=w; w=x; x=t; }
    C2(join, w, x); fatal("expected join_c2 to error");
  }
  usz wc = wr? *SH(w) : 1; if (mulOn(wc, am)) thrOOM();
  if (isAtm(w)) { w = taga(arr_shVec(reshape_one(wc, w))); wc = 0; } // suppress join_cycled reshape
  if (isAtm(x)) x = m_unit(x);
  return join_cycled(aft, wc, w, x, SH(x)+xr-cr, cr);
}

B reverse_ucw(B t, B o, B w, B x) { return reverse_ix(bi_z, w, c1(o, reverse_c2(t, inc(w), x))); }
B reverse_uc1(B t, B o, B x) { return C1(reverse,      c1(o, reverse_c1(t, x))); }
B enclose_uc1(B t, B o, B x) { return enclose_im(bi_z, c1(o, m_unit(x))); }
B pair_uc1   (B t, B o, B x) { return pair_im   (bi_z, c1(o, m_vec1(x))); }
B couple_uc1 (B t, B o, B x) { return couple_im (bi_z, c1(o, couple_c1(t, x))); }

void sfns_init(void) {
  c(BFn,bi_pick)->uc1 = pick_uc1;
  c(BFn,bi_reverse)->im = reverse_c1;
  c(BFn,bi_reverse)->ix = reverse_ix;
  c(BFn,bi_reverse)->uc1 = reverse_uc1;
  c(BFn,bi_reverse)->ucw = reverse_ucw;
  c(BFn,bi_pick)->ucw = pick_ucw;
  c(BFn,bi_select)->ucw = select_ucw; // TODO move to new init fn
  c(BFn,bi_shape)->ucw = shape_ucw;
  c(BFn,bi_shape)->uc1 = shape_uc1;
  c(BFn,bi_take)->ucw = take_ucw;
  c(BFn,bi_drop)->ucw = drop_ucw;
  c(BFn,bi_lt)->im = enclose_im;
  c(BFn,bi_lt)->uc1 = enclose_uc1;
  c(BFn,bi_pair)->im = pair_im;
  c(BFn,bi_pair)->uc1 = pair_uc1;
  c(BFn,bi_couple)->im = couple_im;
  c(BFn,bi_couple)->uc1 = couple_uc1;
}
