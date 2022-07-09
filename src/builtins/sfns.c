#include "../core.h"
#include "../utils/each.h"
#include "../utils/mut.h"
#include "../utils/talloc.h"
#include "../builtins.h"

static Arr* take_impl(usz ria, B x) { // consumes x; returns v↑⥊𝕩 without set shape; v is non-negative
  usz xia = a(x)->ia;
  if (ria>xia) {
    B xf = getFillE(x);
    MAKE_MUT(r, ria); mut_init(r, el_or(TI(x,elType), selfElType(xf)));
    MUTG_INIT(r);
    mut_copyG(r, 0, x, 0, xia);
    mut_fillG(r, xia, xf, ria-xia);
    decG(x);
    if (r->fns->elType!=el_B) { dec(xf); return mut_fp(r); }
    return a(withFill(mut_fv(r), xf));
  } else {
    return TI(x,slice)(x,0,ria);
  }
}

B m_vec1(B a) {
  if (isF64(a)) {
    i32 i = (i32)a.f;
    if (RARE(a.f != i))   { f64* rp; B r = m_f64arrv(&rp, 1); rp[0] = a.f; return r; }
    else if (q_ibit(i))   { u64* rp; B r = m_bitarrv(&rp, 1); rp[0] = i; return r; }
    else if (i == (i8 )i) { i8*  rp; B r = m_i8arrv (&rp, 1); rp[0] = i; return r; }
    else if (i == (i16)i) { i16* rp; B r = m_i16arrv(&rp, 1); rp[0] = i; return r; }
    else                  { i32* rp; B r = m_i32arrv(&rp, 1); rp[0] = i; return r; }
  }
  if (isC32(a)) {
    u32 c = o2cu(a);
    if      (LIKELY(c<U8_MAX )) { u8*  rp; B r = m_c8arrv (&rp, 1); rp[0] = c; return r; }
    else if (LIKELY(c<U16_MAX)) { u16* rp; B r = m_c16arrv(&rp, 1); rp[0] = c; return r; }
    else                        { u32* rp; B r = m_c32arrv(&rp, 1); rp[0] = c; return r; }
  }
  Arr* ra = arr_shVec(m_fillarrp(1));
  fillarr_ptr(ra)[0] = a;
  fillarr_setFill(ra, m_f64(0));
  fillarr_setFill(ra, asFill(inc(a)));
  return taga(ra);
}

FORCE_INLINE B m_vec2Base(B a, B b, bool fills) {
  if (isAtm(a)&isAtm(b)) {
    if (LIKELY(isNum(a)&isNum(b))) {
      i32 ai=a.f; i32 bi=b.f;
      if (RARE(ai!=a.f | bi!=b.f))        { f64* rp; B r = m_f64arrv(&rp, 2); rp[0]=o2fu(a); rp[1]=o2fu(b); return r; }
      else if (q_ibit(ai)  &  q_ibit(bi)) { u64* rp; B r = m_bitarrv(&rp, 2); rp[0]=ai | (bi<<1);           return r; }
      else if (ai==(i8 )ai & bi==(i8 )bi) { i8*  rp; B r = m_i8arrv (&rp, 2); rp[0]=ai;      rp[1]=bi;      return r; }
      else if (ai==(i16)ai & bi==(i16)bi) { i16* rp; B r = m_i16arrv(&rp, 2); rp[0]=ai;      rp[1]=bi;      return r; }
      else                                { i32* rp; B r = m_i32arrv(&rp, 2); rp[0]=ai;      rp[1]=bi;      return r; }
    }
    if (isC32(b)&isC32(a)) {
      u32 ac=o2cu(a); u32 bc=o2cu(b);
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
    fillarr_ptr(ra)[0] = a;
    fillarr_ptr(ra)[1] = b;
    return taga(ra);
  }
  noFills:
  return m_hVec2(a,b);
}

B m_vec2(B a, B b) { return m_vec2Base(a, b, false); }

B pair_c1(B t,      B x) { return m_vec1(x); }
B pair_c2(B t, B w, B x) { return m_vec2Base(w, x, true); }

B shape_c1(B t, B x) {
  if (isAtm(x)) return m_vec1(x);
  if (rnk(x)==1) return x;
  usz ia = a(x)->ia;
  if (ia==1 && TI(x,elType)<el_B) {
    B n = IGet(x,0);
    decG(x);
    return m_vec1(n);
  }
  if (reusable(x)) { FL_KEEP(x, fl_squoze);
    decSh(v(x)); arr_shVec(a(x));
    return x;
  }
  return taga(arr_shVec(TI(x,slice)(x, 0, ia)));
}
static B truncReshape(B x, usz xia, usz nia, ur nr, ShArr* sh) { // consumes all
  B r; Arr* ra;
  if (reusable(x) && xia==nia) { r = x; decSh(v(x)); ra = (Arr*)v(r); }
  else { ra = TI(x,slice)(x, 0, nia); r = taga(ra); }
  arr_shSetU(ra, nr, sh);
  return r;
}
B shape_c2(B t, B w, B x) {
  usz xia = isArr(x)? a(x)->ia : 1;
  usz nia = 1;
  ur nr;
  ShArr* sh;
  if (isF64(w)) {
    nia = o2s(w);
    nr = 1;
    sh = NULL;
  } else {
    if (isAtm(w)) w = m_atomUnit(w);
    if (rnk(w)>1) thrM("⥊: 𝕨 must have rank at most 1");
    if (a(w)->ia>UR_MAX) thrM("⥊: Result rank too large");
    nr = a(w)->ia;
    sh = nr<=1? NULL : m_shArr(nr);
    if (TI(w,elType)==el_i32) {
      i32* wi = i32any_ptr(w);
      if (nr>1) for (i32 i = 0; i < nr; i++) sh->a[i] = wi[i];
      bool bad=false, good=false;
      for (i32 i = 0; i < nr; i++) {
        if (wi[i]<0) thrF("⥊: 𝕨 contained %i", wi[i]);
        bad|= mulOn(nia, wi[i]);
        good|= wi[i]==0;
      }
      if (bad && !good) thrM("⥊: 𝕨 too large");
    } else {
      SGetU(w)
      i32 unkPos = -1;
      i32 unkInd;
      bool bad=false, good=false;
      for (i32 i = 0; i < nr; i++) {
        B c = GetU(w, i);
        if (isF64(c)) {
          usz v = o2s(c);
          if (sh) sh->a[i] = v;
          bad|= mulOn(nia, v);
          good|= v==0;
        } else {
          if (isArr(c) || !isVal(c)) thrM("⥊: 𝕨 must consist of natural numbers or ∘ ⌊ ⌽ ↑");
          if (unkPos!=-1) thrM("⥊: 𝕨 contained multiple computed axes");
          unkPos = i;
          if (!isPrim(c)) thrM("⥊: 𝕨 must consist of natural numbers or ∘ ⌊ ⌽ ↑");
          unkInd = ((i32)v(c)->flags) - 1;
        }
      }
      if (bad && !good) thrM("⥊: 𝕨 too large");
      if (unkPos!=-1) {
        if (unkInd!=n_atop & unkInd!=n_floor & unkInd!=n_reverse & unkInd!=n_take) thrM("⥊: 𝕨 must consist of natural numbers or ∘ ⌊ ⌽ ↑");
        if (nia==0) thrM("⥊: Can't compute axis when the rest of the shape is empty");
        i64 div = xia/nia;
        i64 mod = xia%nia;
        usz item;
        bool fill = false;
        if (unkInd == n_atop) {
          if (mod!=0) thrM("⥊: Shape must be exact when reshaping with ∘");
          item = div;
        } else if (unkInd == n_floor) {
          item = div;
        } else if (unkInd == n_reverse) {
          item = mod? div+1 : div;
        } else if (unkInd == n_take) {
          item = mod? div+1 : div;
          fill = true;
        } else UD;
        if (sh) sh->a[unkPos] = item;
        nia = uszMul(nia, item);
        if (fill) {
          if (!isArr(x)) x = m_atomUnit(x);
          x = taga(arr_shVec(take_impl(nia, x)));
          xia = nia;
        }
      }
    }
    decG(w);
  }
  
  B xf;
  if (isAtm(x)) {
    xf = asFill(inc(x));
    // goes to unit
  } else {
    if (nia <= xia) {
      return truncReshape(x, xia, nia, nr, sh);
    } else {
      xf = getFillQ(x);
      if (xia<=1) {
        if (RARE(xia==0)) {
          thrM("⥊: Empty 𝕩 and non-empty result");
          // if (noFill(xf)) thrM("⥊: No fill for empty array");
          // dec(x);
          // x = inc(xf);
        } else {
          B n = IGet(x,0);
          decG(x);
          x = n;
        }
        goto unit;
      }
      
      MAKE_MUT(m, nia); mut_init(m, TI(x,elType));
      MUTG_INIT(m);
      i64 div = nia/xia;
      i64 mod = nia%xia;
      for (i64 i = 0; i < div; i++) mut_copyG(m, i*xia, x, 0, xia);
      mut_copyG(m, div*xia, x, 0, mod);
      decG(x);
      Arr* ra = mut_fp(m);
      arr_shSetU(ra, nr, sh);
      return withFill(taga(ra), xf);
    }
  }
  
  unit:
  if (isF64(x)) { decA(xf);
    i32 n = (i32)x.f;
    if (RARE(n!=x.f))  { f64* rp; Arr* r = m_f64arrp(&rp,nia); arr_shSetU(r,nr,sh); for (u64 i=0; i<nia; i++) rp[i]=x.f; return taga(r); }
    else if(n==(n&1))  { Arr* r=n?allOnes(nia):allZeroes(nia); arr_shSetU(r,nr,sh); return taga(r); }
    else if(n==(i8 )n) { i8*  rp; Arr* r = m_i8arrp (&rp,nia); arr_shSetU(r,nr,sh); for (u64 i=0; i<nia; i++) rp[i]=n  ; return taga(r); }
    else if(n==(i16)n) { i16* rp; Arr* r = m_i16arrp(&rp,nia); arr_shSetU(r,nr,sh); for (u64 i=0; i<nia; i++) rp[i]=n  ; return taga(r); }
    else               { i32* rp; Arr* r = m_i32arrp(&rp,nia); arr_shSetU(r,nr,sh); for (u64 i=0; i<nia; i++) rp[i]=n  ; return taga(r); }
  }
  if (isC32(x)) { decA(xf);
    u32* rp; Arr* r = m_c32arrp(&rp, nia); arr_shSetU(r, nr, sh);
    u32 c = o2cu(x);
    for (u64 i = 0; i < nia; i++) rp[i] = c;
    return taga(r);
  }
  Arr* r = m_fillarrp(nia); arr_shSetU(r, nr, sh);
  B* rp = fillarr_ptr(r);
  if (nia) incBy(x, nia-1);
  else dec(x);
  for (u64 i = 0; i < nia; i++) rp[i] = x;
  fillarr_setFill(r, xf);
  return taga(r);
}

extern B rt_pick;
B pick_c1(B t, B x) {
  if (isAtm(x)) return x;
  if (RARE(a(x)->ia==0)) {
    thrM("⊑: Argument cannot be empty");
    // B r = getFillE(x);
    // dec(x);
    // return r;
  }
  B r = IGet(x, 0);
  decG(x);
  return r;
}

static NOINLINE void checkNumeric(B w) {
  SGetU(w)
  usz ia = a(w)->ia;
  for (usz i = 0; i < ia; i++) if (!isNum(GetU(w,i))) thrM("⊑: 𝕨 contained list with mixed-type elements");
}
static B recPick(B w, B x) { // doesn't consume
  assert(isArr(w) && isArr(x));
  usz ia = a(w)->ia;
  ur xr = rnk(x);
  usz* xsh = a(x)->sh;
  switch(TI(w,elType)) { default: UD;
    case el_i8:  { i8*  wp = i8any_ptr (w); if(rnk(w)!=1)goto wrr; if (ia!=xr)goto wrl; usz c=0; for (usz i = 0; i < ia; i++) { c = c*xsh[i] + WRAP(wp[i], xsh[i], goto oob); }; return IGet(x,c); }
    case el_i16: { i16* wp = i16any_ptr(w); if(rnk(w)!=1)goto wrr; if (ia!=xr)goto wrl; usz c=0; for (usz i = 0; i < ia; i++) { c = c*xsh[i] + WRAP(wp[i], xsh[i], goto oob); }; return IGet(x,c); }
    case el_i32: { i32* wp = i32any_ptr(w); if(rnk(w)!=1)goto wrr; if (ia!=xr)goto wrl; usz c=0; for (usz i = 0; i < ia; i++) { c = c*xsh[i] + WRAP(wp[i], xsh[i], goto oob); }; return IGet(x,c); }
    case el_f64: { f64* wp = f64any_ptr(w); if(rnk(w)!=1)goto wrr; if (ia!=xr)goto wrl; usz c=0; for (usz i = 0; i < ia; i++) { i64 ws = (i64)wp[i]; if (wp[i]!=ws) thrM(ws==I64_MIN? "⊑: 𝕨 contained value too large" : "⊑: 𝕨 contained a non-integer");
                                                                                                                                c = c*xsh[i] + WRAP(ws,    xsh[i], goto oob); }; return IGet(x,c); }
    case el_c8: case el_c16: case el_c32: case el_bit:
    case el_B: {
      if (ia==0) {
        if (xr!=0) thrM("⊑: Empty array in 𝕨 must correspond to unit in 𝕩");
        return IGet(x,0);
      }
      SGetU(w)
      if (isNum(GetU(w,0))) {
        if(rnk(w)!=1) goto wrr;
        if (ia!=xr) goto wrl;
        usz c=0;
        for (usz i = 0; i < ia; i++) {
          B cw = GetU(w,i);
          if (!isNum(cw)) thrM("⊑: 𝕨 contained list with mixed-type elements");
          c = c*xsh[i] + WRAP(o2i64(cw), xsh[i], goto oob);
        }
        return IGet(x,c);
      } else {
        M_HARR(r, ia);
        for(usz i=0; i<ia; i++) {
          B c = GetU(w, i);
          if (isAtm(c)) thrM("⊑: 𝕨 contained list with mixed-type elements");
          HARR_ADD(r, i, recPick(c, x));
        }
        return HARR_FC(r, w);
      }
    }
  }
  #undef PICK
  
  wrr: checkNumeric(w); thrF("⊑: Leaf arrays in 𝕨 must have rank 1 (element: %B)", w); // wrong index rank
  wrl: checkNumeric(w); thrF("⊑: Picking item at wrong rank (index %B in array of shape %H)", w, x); // wrong index length
  oob: checkNumeric(w); thrF("⊑: Indexing out-of-bounds (index %B in array of shape %H)", w, x);
}

B pick_c2(B t, B w, B x) {
  if (RARE(isAtm(x))) {
    if (isAtm(w) || rnk(w)!=1 || a(w)->ia!=0) return c2(rt_pick, w, x); // ugh this is such a lame case that'd need a whole another recursive fn to implement
    dec(w);
    return x;
  }
  if (isNum(w)) {
    if (rnk(x)!=1) thrF("⊑: 𝕩 must be a list when 𝕨 is a number (%H ≡ ≢𝕩)", x);
    usz p = WRAP(o2i64(w), a(x)->ia, thrF("⊑: indexing out-of-bounds (𝕨≡%R, %s≡≠𝕩)", w, iaW));
    B r = IGet(x, p);
    decG(x);
    return r;
  }
  if (!isArr(w)) thrM("⊑: 𝕨 must be a numeric array");
  B r = recPick(w, x);
  decG(w); decG(x);
  return r;
}

#ifdef __BMI2__
  #include <immintrin.h>
  
  #if USE_VALGRIND
    #define DBG_VG_SLASH 0
    u64 loadMask(u64* p, u64 unk, u64 exp, u64 i, u64 pos) {
      // #if DBG_VG_SLASH
      //   if (pos==0) printf("index %2ld, got %016lx\n", i, p[i]);
      // #endif
      if (pos==0) return ~(p[i]^exp);
      u64 res =          loadMask(p, unk, exp, i,     pos<<1);
      if (unk&pos) res&= loadMask(p, unk, exp, i|pos, pos<<1);
      return res;
    }
    NOINLINE u64 vg_load64(u64* p, u64 i) {
      u64 unk = ~vg_getDefined_u64(i);
      u64 res = p[vg_withDefined_u64(i, ~0ULL)]; // result value will always be the proper indexing operation
      
      i32 undefCount = POPC(unk);
      if (undefCount>0) {
        if (undefCount>8) err("too many unknown bits in index of vg_load64");
        res = vg_withDefined_u64(res, loadMask(p, unk, res, i & ~unk, 1));
      }
      #if DBG_VG_SLASH
        vg_printDefined_u64("idx", i);
        vg_printDefined_u64("res", res);
      #endif
      return res;
    }
    NOINLINE u64 vg_pext_u64(u64 src, u64 mask) {
      u64 maskD = vg_getDefined_u64(mask);
      u64 r = vg_undef_u64(0);
      i32 ri = 0;
      u64 undefMask = 0;
      for (i32 i = 0; i < 64; i++) {
        u64 c = 1ull<<i;
        if (!(maskD&c) && undefMask==0) undefMask = (~0ULL)<<ri;
        if (vg_def_u64(mask&c)) r = vg_withBit_u64(r, ri++, (c&src)!=0);
      }
      while (ri<64) r = vg_withBit_u64(r, ri++, 0);
      r = vg_withDefined_u64(r, vg_getDefined_u64(r) & ~undefMask);
      #if DBG_VG_SLASH
        printf("pext:\n");
        vg_printDefined_u64("src", src);
        vg_printDefined_u64("msk", mask);
        vg_printDefined_u64("res", r);
      vg_printDefined_u64("exp", _pext_u64(src, mask));
      #endif
      return r;
    }
    NOINLINE u64 vg_pdep_u64(u64 src, u64 mask) {
      if (0 != ~vg_getDefined_u64(mask)) err("pdep impl assumes mask is defined everywhere");
      u64 c = src;
      u64 r = 0;
      for (i32 i = 0; i < 64; i++) {
        if ((mask>>i)&1) {
          r|= (c&1) << i;
          c>>= 1;
        }
      }
      #if DBG_VG_SLASH
        printf("pdep:\n");
        vg_printDefined_u64("src", src);
        vg_printDefined_u64("msk", mask);
        vg_printDefined_u64("res", r);
        vg_printDefined_u64("exp", _pdep_u64(src, mask));
      #endif
      return r;
    }
    NOINLINE u64 rand_popc64(u64 x) {
      u64 def = vg_getDefined_u64(x);
      if (def==~0ULL) return POPC(x);
      i32 min = POPC(x & def);
      i32 diff = POPC(~def);
      i32 res = min + vgRand64Range(diff);
      #if DBG_VG_SLASH
        printf("popc:\n");
        vg_printDefined_u64("x", x);
        printf("popc in %d-%d; res: %d\n", min, min+diff, res);
      #endif
      return res;
    }
    #define _pext_u32 vg_pext_u64
    #define _pext_u64 vg_pext_u64
    #define _pdep_u32 vg_pdep_u64
    #define _pdep_u64 vg_pdep_u64
  #else
    #define vg_load64(p, i) p[i]
    #define rand_popc64(X) POPC(X)
  #endif
  
  void storeu_u64(u64* p, u64 v) { memcpy(p, &v, 8); }
  u64 loadu_u64(u64* p) { u64 v; memcpy(&v, p, 8); return v; }
  #if SINGELI
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wunused-variable"
    #include "../singeli/gen/slash.c"
    #pragma GCC diagnostic pop
  #endif
#endif

extern B rt_slash;
B slash_c1(B t, B x) {
  if (RARE(isAtm(x)) || RARE(rnk(x)!=1)) thrF("/: Argument must have rank 1 (%H ≡ ≢𝕩)", x);
  u64 s = usum(x);
  if (s>=USZ_MAX) thrOOM();
  usz xia = a(x)->ia;
  if (RARE(xia>=I32_MAX)) {
    usz xia = a(x)->ia;
    SGetU(x)
    f64* rp; B r = m_f64arrv(&rp, s); usz ri = 0;
    for (usz i = 0; i < xia; i++) {
      usz c = o2s(GetU(x, i));
      for (usz j = 0; j < c; j++) rp[ri++] = i;
    }
    decG(x);
    return r;
  }
  B r;
  u8 xe = TI(x,elType);
  #if SINGELI && defined(__BMI2__)
  if (xia<=32768 && xe==el_bit) {
    u64* xp = bitarr_ptr(x);
    if (xia<=128) { i8*  rp = m_tyarrvO(&r, 1, s, t_i8arr ,  8); bmipopc_1slash8 (xp, rp, xia); }
    else          { i16* rp = m_tyarrvO(&r, 2, s, t_i16arr, 16); bmipopc_1slash16(xp, rp, xia); }
  } else
  #endif
  {
    i32* rp; r = m_i32arrv(&rp, s);
    if (xe==el_bit) {
      u64* xp = bitarr_ptr(x);
      while (xia>0 && !bitp_get(xp,xia-1)) xia--;
      for (u64 i = 0; i < xia; i++) {
        *rp = i;
        rp+= bitp_get(xp, i);
      }
    } else if (xe==el_i8) {
      i8* xp = i8any_ptr(x);
      while (xia>0 && !xp[xia-1]) xia--;
      for (u64 i = 0; i < xia; i++) {
        i32 c = xp[i];
        if (LIKELY(c==0 || c==1)) {
          *rp = i;
          rp+= c;
        } else {
          for (i32 j = 0; j < c; j++) *rp++ = i;
        }
      }
    } else if (xe==el_i32) {
      i32* xp = i32any_ptr(x);
      while (xia>0 && !xp[xia-1]) xia--;
      for (u64 i = 0; i < xia; i++) {
        i32 c = xp[i];
        if (LIKELY(c==0 || c==1)) {
          *rp = i;
          rp+= c;
        } else {
          for (i32 j = 0; j < c; j++) *rp++ = i;
        }
      }
    } else {
      SLOW1("/𝕩", x);
      SGetU(x)
      for (u64 i = 0; i < xia; i++) {
        usz c = o2s(GetU(x, i));
        for (u64 j = 0; j < c; j++) *rp++ = i;
      }
    }
  }
  decG(x);
  return r;
}

B slash_c2(B t, B w, B x) {
  if (isArr(x) && rnk(x)==1 && isArr(w) && rnk(w)==1 && depth(w)==1) {
    usz wia = a(w)->ia;
    usz xia = a(x)->ia;
    if (RARE(wia!=xia)) {
      if (wia==0) { decG(w); return x; }
      thrF("/: Lengths of components of 𝕨 must match 𝕩 (%s ≠ %s)", wia, xia);
    }
    B xf = getFillQ(x);
    
    usz ri = 0;
    if (TI(w,elType)==el_bit) {
      B r;
      u64* wp = bitarr_ptr(w);
      u8 xe = TI(x,elType);
      #ifdef __BMI2__
      usz wsum = bit_sum(wp, wia);
      switch(xe) {
        case el_bit: {
          u64* xp = bitarr_ptr(x); u64* rp; r = m_bitarrv(&rp,wsum+128); a(r)->ia = wsum;
          u64 cw = 0; // current word
          u64 ro = 0; // offset in word where next bit should be written; never 64
          for (usz i=0; i<BIT_N(wia); i++) {
            u64 wv = wp[i];
            u64 v = _pext_u64(xp[i], wv);
            u64 c = rand_popc64(wv);
            cw|= v<<ro;
            u64 ro2 = ro+c;
            if (ro2>=64) {
              *(rp++) = cw;
              cw = ro? v>>(64-ro) : 0;
            }
            ro = ro2&63;
          }
          if (ro) *rp = cw;
          goto bit_ret;
        }
        #if SINGELI
        case el_i8: case el_c8:  { i8*  xp=tyany_ptr(x); i8*  rp=m_tyarrvO(&r,1,wsum,el2t(xe),  8); bmipopc_2slash8 (wp, xp, rp, wia); goto bit_ret; }
        case el_i16:case el_c16: { i16* xp=tyany_ptr(x); i16* rp=m_tyarrvO(&r,2,wsum,el2t(xe), 16); bmipopc_2slash16(wp, xp, rp, wia); goto bit_ret; }
        #endif
      }
      #endif
      
      while (wia>0 && !bitp_get(wp,wia-1)) wia--;
      #ifndef __BMI2__
      usz wsum = bit_sum(wp, wia);
      #endif
      if (wsum==0) { decG(w); decG(x); return q_N(xf)? emptyHVec() : isF64(xf)? emptyIVec() : isC32(xf)? emptyCVec() : m_emptyFVec(xf); }
      switch(xe) { default: UD;
        
        #if !SINGELI
        case el_i8: case el_c8:  { i8*  xp=tyany_ptr(x); i8*  rp=m_tyarrv(&r,1,wsum,el2t(xe)); for (usz i=0; i<wia; i++) { *rp = xp[i]; rp+= bitp_get(wp,i); } break; }
        case el_i16:case el_c16: { i16* xp=tyany_ptr(x); i16* rp=m_tyarrv(&r,2,wsum,el2t(xe)); for (usz i=0; i<wia; i++) { *rp = xp[i]; rp+= bitp_get(wp,i); } break; }
        #ifndef __BMI2__
        case el_bit: { u64* xp = bitarr_ptr(x); u64* rp; r = m_bitarrv(&rp,wsum); for (usz i=0; i<wia; i++) { bitp_set(rp,ri,bitp_get(xp,i)); ri+= bitp_get(wp,i); } break; }
        #endif
        #endif
        
        case el_i32:case el_c32: { i32* xp=tyany_ptr(x); i32* rp=m_tyarrv(&r,4,wsum,el2t(xe)); for (usz i=0; i<wia; i++) { *rp = xp[i]; rp+= bitp_get(wp,i); } break; }
        case el_f64: { f64* xp = f64any_ptr(x); f64* rp; r = m_f64arrv(&rp,wsum); for (usz i=0; i<wia; i++) { *rp = xp[i]; rp+= bitp_get(wp,i); } break; }
        case el_B: {
          B* xp = arr_bptr(x);
          if (xp!=NULL) {
            HArr_p rp = m_harrUv(wsum);
            for (usz i=0; i<wia; i++) { rp.a[ri] = xp[i]; ri+= bitp_get(wp,i); }
            for (usz i=0; i<wsum; i++) inc(rp.a[i]);
            r = withFill(rp.b, xf);
          } else {
            SLOW2("𝕨/𝕩", w, x);
            M_HARR(rp, wsum) SGet(x)
            for (usz i = 0; i < wia; i++) if (bitp_get(wp,i)) HARR_ADDA(rp, Get(x,i));
            r = withFill(HARR_FV(rp), xf);
          }
          break;
        }
      }
      #if __BMI2__
      bit_ret:;
      #endif
      decG(w); decG(x); return r;
    }
    #define CASE(WT,XT) if (TI(x,elType)==el_##XT) { \
      XT* xp = XT##any_ptr(x);                       \
      XT* rp; B r = m_##XT##arrv(&rp, wsum);         \
      if (or<2) for (usz i = 0; i < wia; i++) {      \
        *rp = xp[i];                                 \
        rp+= wp[i];                                  \
      } else for (usz i = 0; i < wia; i++) {         \
        WT cw = wp[i]; XT cx = xp[i];                \
        for (i64 j = 0; j < cw; j++) *rp++ = cx;     \
      }                                              \
      decG(w); decG(x); return r;                    \
    }
    #define TYPED(WT,SIGN) { \
      WT* wp = WT##any_ptr(w);           \
      while (wia>0 && !wp[wia-1]) wia--; \
      i64 wsum = 0;                      \
      u32 or = 0;                        \
      for (usz i = 0; i < wia; i++) {    \
        wsum+= wp[i];                    \
        or|= (u32)wp[i];                 \
      }                                  \
      if (or>>SIGN) thrM("/: 𝕨 must consist of natural numbers"); \
      if (TI(x,elType)==el_bit) {                  \
        u64* xp = bitarr_ptr(x); u64 ri=0;         \
        u64* rp; B r = m_bitarrv(&rp, wsum);       \
        if (or<2) for (usz i = 0; i < wia; i++) {  \
          bitp_set(rp, ri, bitp_get(xp,i));        \
          ri+= wp[i];                              \
        } else for (usz i = 0; i < wia; i++) {     \
          WT cw = wp[i]; bool cx = bitp_get(xp,i); \
          for (i64 j = 0; j < cw; j++) bitp_set(rp, ri++, cx); \
        }                                          \
        decG(w); decG(x); return r;                \
      }                                            \
      CASE(WT,i8) CASE(WT,i16) CASE(WT,i32) CASE(WT,f64) \
      SLOW2("𝕨/𝕩", w, x);                    \
      M_HARR(r, wsum) SGetU(x)               \
      for (usz i = 0; i < wia; i++) {        \
        i32 cw = wp[i]; if (cw==0) continue; \
        B cx = incBy(GetU(x, i), cw);        \
        for (i64 j = 0; j < cw; j++) HARR_ADDA(r, cx);\
      }                                      \
      decG(w); decG(x);                      \
      return withFill(HARR_FV(r), xf);       \
    }
    if (TI(w,elType)==el_i8 ) TYPED(i8,7);
    if (TI(w,elType)==el_i32) TYPED(i32,31);
    #undef TYPED
    #undef CASE
    SLOW2("𝕨/𝕩", w, x);
    u64 ria = usum(w);
    if (ria>=USZ_MAX) thrOOM();
    M_HARR(r, ria) SGetU(w) SGetU(x)
    for (usz i = 0; i < wia; i++) {
      usz c = o2s(GetU(w, i));
      if (c) {
        B cx = incBy(GetU(x, i), c);
        for (usz j = 0; RARE(j < c); j++) HARR_ADDA(r, cx);
      }
    }
    decG(w); decG(x);
    return withFill(HARR_FV(r), xf);
  }
  if (isArr(x) && rnk(x)==1 && q_i32(w)) {
    usz xia = a(x)->ia;
    i32 wv = o2i(w);
    if (wv<=0) {
      if (wv<0) thrM("/: 𝕨 cannot be negative");
      return taga(arr_shVec(TI(x,slice)(x, 0, 0)));
    }
    if (TI(x,elType)==el_i32) {
      i32* xp = i32any_ptr(x);
      i32* rp; B r = m_i32arrv(&rp, xia*wv);
      for (usz i = 0; i < xia; i++) {
        for (i64 j = 0; j < wv; j++) *rp++ = xp[i];
      }
      decG(x);
      return r;
    } else {
      SLOW2("𝕨/𝕩", w, x);
      B xf = getFillQ(x);
      HArr_p r = m_harrUv(xia*wv);
      SGetU(x)
      for (usz i = 0; i < xia; i++) {
        B cx = incBy(GetU(x, i), wv);
        for (i64 j = 0; j < wv; j++) *r.a++ = cx;
      }
      decG(x);
      return withFill(r.b, xf);
    }
  }
  return c2(rt_slash, w, x);
}


B slash_im(B t, B x) {
  if (!isArr(x) || rnk(x)!=1) thrM("/⁼: Argument must be an array");
  u8 xe = TI(x,elType);
  usz xia = a(x)->ia;
  if (xia==0) { decG(x); return emptyIVec(); }
  switch(xe) { default: UD;
    case el_i8: {
      i8* xp = i8any_ptr(x);
      usz i,j; B r; i8 max=-1;
      for (i = 0; i < xia; i++) { i8 c=xp[i]; if (c<=max) break; max=c; }
      for (j = i; j < xia; j++) { i8 c=xp[j]; max=c>max?c:max; if (c<0) thrM("/⁼: Argument cannot contain negative numbers"); }
      usz ria = max+1;
      if (i==xia) {
        u64* rp; r = m_bitarrv(&rp, ria); for (usz i=0; i<BIT_N(ria); i++) rp[i]=0;
        for (usz i = 0; i < xia; i++) bitp_set(rp, xp[i], 1);
      } else {
        i32* rp; r = m_i32arrv(&rp, ria); for (usz i=0; i<ria; i++) rp[i]=0;
        for (usz i = 0; i < xia; i++) rp[xp[i]]++;
      }
      decG(x); return r;
    }
    case el_i16: {
      i16* xp = i16any_ptr(x);
      usz i,j; B r; i16 max=-1;
      for (i = 0; i < xia; i++) { i16 c=xp[i]; if (c<=max) break; max=c; }
      for (j = i; j < xia; j++) { i16 c=xp[j]; max=c>max?c:max; if (c<0) thrM("/⁼: Argument cannot contain negative numbers"); }
      usz ria = max+1;
      if (i==xia) {
        u64* rp; r = m_bitarrv(&rp, ria); for (usz i=0; i<BIT_N(ria); i++) rp[i]=0;
        for (usz i = 0; i < xia; i++) bitp_set(rp, xp[i], 1);
      } else {
        i32* rp; r = m_i32arrv(&rp, ria); for (usz i=0; i<ria; i++) rp[i]=0;
        for (usz i = 0; i < xia; i++) rp[xp[i]]++;
      }
      decG(x); return r;
    }
    case el_i32: {
      i32* xp = i32any_ptr(x);
      usz i,j; B r; i32 max=-1;
      for (i = 0; i < xia; i++) { i32 c=xp[i]; if (c<=max) break; max=c; }
      for (j = i; j < xia; j++) { i32 c=xp[j]; max=c>max?c:max; if (c<0) thrM("/⁼: Argument cannot contain negative numbers"); }
      usz ria = max+1;
      if (i==xia) {
        u64* rp; r = m_bitarrv(&rp, ria); for (usz i=0; i<BIT_N(ria); i++) rp[i]=0;
        for (usz i = 0; i < xia; i++) bitp_set(rp, xp[i], 1);
      } else {
        i32* rp; r = m_i32arrv(&rp, ria); for (usz i=0; i<ria; i++) rp[i]=0;
        for (usz i = 0; i < xia; i++) rp[xp[i]]++;
      }
      decG(x); return r;
    }
    case el_f64: {
      f64* xp = f64any_ptr(x);
      usz i,j; B r; f64 max=-1;
      for (i = 0; i < xia; i++) { f64 c=xp[i]; if (c!=(usz)c) thrM("/⁼: Argument must consist of natural numbers"); if (c<=max) break; max=c; }
      for (j = i; j < xia; j++) { f64 c=xp[j]; if (c!=(usz)c) thrM("/⁼: Argument must consist of natural numbers"); max=c>max?c:max; if (c<0) thrM("/⁼: Argument cannot contain negative numbers"); }
      usz ria = max+1; if (ria==0) thrOOM();
      if (i==xia) {
        u64* rp; r = m_bitarrv(&rp, ria); for (usz i=0; i<BIT_N(ria); i++) rp[i]=0;
        for (usz i = 0; i < xia; i++) bitp_set(rp, xp[i], 1);
      } else {
        i32* rp; r = m_i32arrv(&rp, ria); for (usz i=0; i<ria; i++) rp[i]=0;
        for (usz i = 0; i < xia; i++) rp[(usz)xp[i]]++;
      }
      decG(x); return r;
    }
    case el_bit: case el_c8: case el_c16: case el_c32: case el_B: {
      SLOW1("/⁼", x);
      B* xp = arr_bptr(x);
      if (xp==NULL) { HArr* xa=cpyHArr(x); x=taga(xa); xp=xa->a; }
      usz i,j; B r; i64 max=-1;
      for (i = 0; i < xia; i++) { i64 c=o2i64(xp[i]); if (c<=max) break; max=c; }
      for (j = i; j < xia; j++) { i64 c=o2i64(xp[j]); max=c>max?c:max; if (c<0) thrM("/⁼: Argument cannot contain negative numbers"); }
      if (max > USZ_MAX-1) thrOOM();
      usz ria = max+1;
      if (i==xia) {
        u64* rp; r = m_bitarrv(&rp, ria); for (usz i=0; i<BIT_N(ria); i++) rp[i]=0;
        for (usz i = 0; i < xia; i++) bitp_set(rp, o2i64u(xp[i]), 1);
      } else {
        i32* rp; r = m_i32arrv(&rp, ria); for (usz i=0; i<ria; i++) rp[i]=0;
        for (usz i = 0; i < xia; i++) rp[o2i64u(xp[i])]++;
      }
      decG(x); return r;
    }
  }
}

static B slicev(B x, usz s, usz ia) {
  usz xia = a(x)->ia; assert(s+ia <= xia);
  return taga(arr_shVec(TI(x,slice)(x, s, ia)));
}

FORCE_INLINE B affixes(B x, i32 post) {
  if (!isArr(x) || rnk(x)==0) thrM(post? "↓: Argument must have rank at least 1" : "↑: Argument must have rank at least 1");
  ur xr = rnk(x);
  usz* xsh = a(x)->sh;
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
    dec(x);
  }
  B rf = incG(HARR_O(r).a[post? cam : 0]);
  return withFill(HARR_FV(r), rf);
}

B take_c1(B t, B x) { return affixes(x, 0); }
B drop_c1(B t, B x) { return affixes(x, 1); }

extern B rt_take, rt_drop;
B take_c2(B t, B w, B x) {
  if (isNum(w)) {
    if (!isArr(x)) x = m_atomUnit(x);
    i64 wv = o2i64(w);
    ur xr = rnk(x);
    usz csz = 1;
    usz* xsh;
    if (xr>1) {
      csz = arr_csz(x);
      xsh = a(x)->sh;
      ptr_inc(shObjS(xsh)); // we'll look at it at the end and dec there
    }
    i64 t = wv*csz; // TODO error on overflow somehow
    Arr* a;
    if (t>=0) {
      a = take_impl(t, x);
    } else {
      t = -t;
      usz xia = a(x)->ia;
      if (t>xia) {
        B xf = getFillE(x);
        MAKE_MUT(r, t); mut_init(r, el_or(TI(x,elType), selfElType(xf)));
        MUTG_INIT(r);
        mut_fillG(r, 0, xf, t-xia);
        mut_copyG(r, t-xia, x, 0, xia);
        decG(x); dec(xf);
        a = mut_fp(r);
      } else {
        a = TI(x,slice)(x,xia-t,t);
      }
    }
    if (xr<=1) {
      arr_shVec(a);
    } else {
      usz* rsh = arr_shAlloc(a, xr); // xr>1, don't have to worry about 0
      rsh[0] = wv<0?-wv:wv;
      shcpy(rsh+1, xsh+1, xr-1);
      ptr_dec(shObjS(xsh));
    }
    return taga(a);
  }
  return c2(rt_take, w, x);
}
B drop_c2(B t, B w, B x) {
  if (isNum(w) && isArr(x) && rnk(x)==1) {
    i64 v = o2i64(w);
    usz ia = a(x)->ia;
    if (v<0) return -v>ia? slicev(x, 0, 0) : slicev(x, 0, v+ia);
    else     return  v>ia? slicev(x, 0, 0) : slicev(x, v, ia-v);
  }
  return c2(rt_drop, w, x);
}

extern B rt_join;
B join_c1(B t, B x) {
  if (isAtm(x)) thrM("∾: Argument must be an array");
  if (rnk(x)==1) {
    usz xia = a(x)->ia;
    if (xia==0) {
      B xf = getFillE(x);
      if (isAtm(xf)) {
        decA(xf);
        decG(x);
        if (!PROPER_FILLS) return emptyHVec();
        thrM("∾: Empty vector 𝕩 cannot have an atom fill element");
      }
      decG(x);
      ur ir = rnk(xf);
      if (ir==0) thrM("∾: Empty vector 𝕩 cannot have a unit fill element");
      B xff = getFillQ(xf);
      HArr_p r = m_harrUp(0);
      usz* sh = arr_shAlloc((Arr*)r.c, ir);
      if (sh) {
        sh[0] = 0;
        shcpy(sh+1, a(xf)->sh+1, ir-1);
      }
      dec(xf);
      return withFill(r.b, xff);
    }
    SGetU(x)
    
    B x0 = GetU(x,0);
    B rf; if(SFNS_FILLS) rf = getFillQ(x0);
    if (isAtm(x0)) goto base; // thrM("∾: Rank of items must be equal or greater than rank of argument");
    usz ir = rnk(x0);
    usz* x0sh = a(x0)->sh;
    if (ir==0) goto base; // thrM("∾: Rank of items must be equal or greater than rank of argument");
    
    usz csz = arr_csz(x0);
    usz cam = x0sh[0];
    for (usz i = 1; i < xia; i++) {
      B c = GetU(x, i);
      if (!isArr(c) || rnk(c)!=ir) goto base; // thrF("∾: All items in argument should have same rank (contained items with ranks %i and %i)", ir, isArr(c)? rnk(c) : 0);
      usz* csh = a(c)->sh;
      if (ir>1) for (usz j = 1; j < ir; j++) if (csh[j]!=x0sh[j]) thrF("∾: Item trailing shapes must be equal (contained arrays with shapes %H and %H)", x0, c);
      cam+= a(c)->sh[0];
      if (SFNS_FILLS && !noFill(rf)) rf = fill_or(rf, getFillQ(c));
    }
    
    MAKE_MUT(r, cam*csz);
    usz ri = 0;
    for (usz i = 0; i < xia; i++) {
      B c = GetU(x, i);
      usz cia = a(c)->ia;
      mut_copy(r, ri, c, 0, cia);
      ri+= cia;
    }
    assert(ri==cam*csz);
    Arr* ra = mut_fp(r);
    usz* sh = arr_shAlloc(ra, ir);
    if (sh) {
      sh[0] = cam;
      shcpy(sh+1, x0sh+1, ir-1);
    }
    decG(x);
    return SFNS_FILLS? qWithFill(taga(ra), rf) : taga(ra);
  }
  base:
  return c1(rt_join, x);
}
B join_c2(B t, B w, B x) {
  if (isAtm(w)) w = m_atomUnit(w);
  ur wr = rnk(w);
  if (isAtm(x)) {
    if (wr==1 && inplace_add(w, x)) return w;
    x = m_atomUnit(x);
  }
  ur xr = rnk(x);
  B f = fill_both(w, x);
  
  ur c = wr>xr?wr:xr;
  if (c==0) {
    HArr_p r = m_harrUv(2);
    r.a[0] = IGet(w,0); decG(w);
    r.a[1] = IGet(x,0); decG(x);
    return qWithFill(r.b, f);
  }
  if (c-wr > 1 || c-xr > 1) thrF("∾: Argument ranks must differ by 1 or less (%i≡=𝕨, %i≡=𝕩)", wr, xr);
  
  bool reusedW;
  B r = arr_join_inline(w, x, false, &reusedW);
  if (c==1) {
    if (rnk(r)==0) srnk(r,1);
  } else {
    assert(c>1);
    ur rnk0 = rnk(r);
    ShArr* sh0 = shObj(r);
    usz wia;
    usz* wsh;
    if (wr==1 && reusedW) {
      wia = a(w)->ia-a(x)->ia;
      wsh = &wia;
    } else {
      wsh = a(w)->sh; // when wr>1, shape object won't be disturbed by arr_join_inline
    }
    usz* xsh = a(x)->sh;
    srnk(r, 0); // otherwise shape allocation failing may break things
    usz* rsh = arr_shAlloc(a(r), c);
    #if PRINT_JOIN_REUSE
    printf(reusedW? "reuse:1;" : "reuse:0;");
    #endif
    for (i32 i = 1; i < c; i++) {
      usz s = xsh[i+xr-c];
      if (RARE(wsh[i+wr-c] != s)) {
        B msg = make_fmt("∾: Lengths not matchable (%2H ≡ ≢𝕨, %H ≡ ≢𝕩)", wr, wsh, x);
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
  if (!reusedW) decG(w);
  return qWithFill(r, f);
}


B couple_c1(B t, B x) {
  if (isAtm(x)) return m_vec1(x);
  usz rr = rnk(x);
  usz ia = a(x)->ia;
  Arr* r = TI(x,slice)(incG(x),0, ia);
  usz* sh = arr_shAlloc(r, rr+1);
  if (sh) { sh[0] = 1; shcpy(sh+1, a(x)->sh, rr); }
  decG(x);
  return taga(r);
}
B couple_c2(B t, B w, B x) {
  if (isAtm(w)&isAtm(x)) return m_vec2(w, x);
  if (isAtm(w)) w = m_atomUnit(w);
  if (isAtm(x)) x = m_atomUnit(x);
  if (!eqShape(w, x)) thrF("≍: 𝕨 and 𝕩 must have equal shapes (%H ≡ ≢𝕨, %H ≡ ≢𝕩)", w, x);
  usz ia = a(w)->ia;
  ur wr = rnk(w);
  MAKE_MUT(r, ia*2); mut_init(r, el_or(TI(w,elType), TI(x,elType)));
  MUTG_INIT(r);
  mut_copyG(r, 0,  w, 0, ia);
  mut_copyG(r, ia, x, 0, ia);
  Arr* ra = mut_fp(r);
  usz* sh = arr_shAlloc(ra, wr+1);
  if (sh) { sh[0]=2; shcpy(sh+1, a(w)->sh, wr); }
  if (!SFNS_FILLS) { decG(w); decG(x); return taga(ra); }
  B rf = fill_both(w, x);
  decG(w); decG(x);
  return qWithFill(taga(ra), rf);
}


static inline void shift_check(B w, B x) {
  ur wr = rnk(w); usz* wsh = a(w)->sh;
  ur xr = rnk(x); usz* xsh = a(x)->sh;
  if (wr+1!=xr & wr!=xr) thrF("shift: =𝕨 must be =𝕩 or ¯1+=𝕩 (%i≡=𝕨, %i≡=𝕩)", wr, xr);
  for (i32 i = 1; i < xr; i++) if (wsh[i+wr-xr] != xsh[i]) thrF("shift: Lengths not matchable (%H ≡ ≢𝕨, %H ≡ ≢𝕩)", w, x);
}

B shiftb_c1(B t, B x) {
  if (isAtm(x) || rnk(x)==0) thrM("»: Argument cannot be a scalar");
  usz ia = a(x)->ia;
  if (ia==0) return x;
  B xf = getFillE(x);
  usz csz = arr_csz(x);
  
  MAKE_MUT(r, ia); mut_init(r, el_or(TI(x,elType), selfElType(xf)));
  MUTG_INIT(r);
  mut_copyG(r, csz, x, 0, ia-csz);
  mut_fillG(r, 0, xf, csz);
  return qWithFill(mut_fcd(r, x), xf);
}
B shiftb_c2(B t, B w, B x) {
  if (isAtm(x) || rnk(x)==0) thrM("»: 𝕩 cannot be a scalar");
  if (isAtm(w)) w = m_atomUnit(w);
  shift_check(w, x);
  B f = fill_both(w, x);
  usz wia = a(w)->ia;
  usz xia = a(x)->ia;
  MAKE_MUT(r, xia); mut_init(r, el_or(TI(w,elType), TI(x,elType)));
  MUTG_INIT(r);
  int mid = wia<xia? wia : xia;
  mut_copyG(r, 0  , w, 0, mid);
  mut_copyG(r, mid, x, 0, xia-mid);
  decG(w);
  return qWithFill(mut_fcd(r, x), f);
}

B shifta_c1(B t, B x) {
  if (isAtm(x) || rnk(x)==0) thrM("«: Argument cannot be a scalar");
  usz ia = a(x)->ia;
  if (ia==0) return x;
  B xf = getFillE(x);
  usz csz = arr_csz(x);
  MAKE_MUT(r, ia); mut_init(r, el_or(TI(x,elType), selfElType(xf)));
  MUTG_INIT(r);
  mut_copyG(r, 0, x, csz, ia-csz);
  mut_fillG(r, ia-csz, xf, csz);
  return qWithFill(mut_fcd(r, x), xf);
}
B shifta_c2(B t, B w, B x) {
  if (isAtm(x) || rnk(x)==0) thrM("«: 𝕩 cannot be a scalar");
  if (isAtm(w)) w = m_atomUnit(w);
  shift_check(w, x);
  B f = fill_both(w, x);
  usz wia = a(w)->ia;
  usz xia = a(x)->ia;
  MAKE_MUT(r, xia); mut_init(r, el_or(TI(w,elType), TI(x,elType)));
  MUTG_INIT(r);
  if (wia < xia) {
    usz m = xia-wia;
    mut_copyG(r, 0, x, wia, m);
    mut_copyG(r, m, w, 0, wia);
  } else {
    mut_copyG(r, 0, w, wia-xia, xia);
  }
  decG(w);
  return qWithFill(mut_fcd(r, x), f);
}

extern B rt_group;
B group_c1(B t, B x) {
  return c1(rt_group, x);
}
B group_c2(B t, B w, B x) {
  if (isArr(w)&isArr(x) && rnk(w)==1 && rnk(x)==1 && depth(w)==1) {
    usz wia = a(w)->ia;
    usz xia = a(x)->ia;
    if (wia-xia > 1) thrF("⊔: ≠𝕨 must be either ≠𝕩 or one bigger (%s≡≠𝕨, %s≡≠𝕩)", wia, xia);
    u8 we = TI(w,elType);
    if (we<=el_i32) {
      if (we!=el_i32) w = taga(cpyI32Arr(w));
      i32* wp = i32any_ptr(w);
      i64 ria = wia==xia? 0 : wp[xia];
      if (ria<-1) thrM("⊔: 𝕨 can't contain elements less than ¯1");
      ria--;
      for (usz i = 0; i < xia; i++) if (wp[i]>ria) ria = wp[i];
      if (ria > (i64)(USZ_MAX-1)) thrOOM();
      ria++;
      TALLOC(i32, lenO, ria+1); i32* len = lenO+1;
      TALLOC(i32, pos, ria);
      for (usz i = 0; i < ria; i++) len[i] = pos[i] = 0;
      for (usz i = 0; i < xia; i++) {
        i32 n = wp[i];
        if (n<-1) thrM("⊔: 𝕨 can't contain elements less than ¯1");
        len[n]++; // overallocation makes this safe after n<-1 check
      }
      
      Arr* r = arr_shVec(m_fillarrp(ria)); fillarr_setFill(r, m_f64(0));
      B* rp = fillarr_ptr(r);
      for (usz i = 0; i < ria; i++) rp[i] = m_f64(0); // don't break if allocation errors
      B xf = getFillQ(x);
      
      Arr* rf = arr_shVec(m_fillarrp(0)); fillarr_setFill(rf, m_f64(0));
      fillarr_setFill(r, taga(rf));
      u8 xe = TI(x,elType);
      switch (xe) { default: UD;
        case el_i8: case el_c8:
        case el_i16: case el_c16:
        case el_i32: case el_c32: case el_f64: {
          u8 width = elWidth(xe);
          for (usz i = 0; i < ria; i++) m_tyarrv(rp+i, width, len[i], el2t(xe));
          void* xp = tyany_ptr(x);
          
          switch(width) { default: UD;
            case 1: for (usz i = 0; i < xia; i++) { i32 n = wp[i]; if (n>=0) ((u8* )tyarr_ptr(rp[n]))[pos[n]++] = ((u8* )xp)[i]; } break;
            case 2: for (usz i = 0; i < xia; i++) { i32 n = wp[i]; if (n>=0) ((u16*)tyarr_ptr(rp[n]))[pos[n]++] = ((u16*)xp)[i]; } break;
            case 4: for (usz i = 0; i < xia; i++) { i32 n = wp[i]; if (n>=0) ((u32*)tyarr_ptr(rp[n]))[pos[n]++] = ((u32*)xp)[i]; } break;
            case 8: for (usz i = 0; i < xia; i++) { i32 n = wp[i]; if (n>=0) ((f64*)tyarr_ptr(rp[n]))[pos[n]++] = ((f64*)xp)[i]; } break;
          }
          break;
        }
        case el_bit: case el_B: {
          for (usz i = 0; i < ria; i++) {
            Arr* c = m_fillarrp(len[i]);
            c->ia = 0;
            fillarr_setFill(c, inc(xf));
            arr_shVec(c);
            rp[i] = taga(c);
          }
          SLOW2("𝕨⊔𝕩", w, x);
          SGet(x)
          for (usz i = 0; i < xia; i++) {
            i32 n = wp[i];
            if (n>=0) fillarr_ptr(a(rp[n]))[pos[n]++] = Get(x, i);
          }
          for (usz i = 0; i < ria; i++) a(rp[i])->ia = len[i];
          break;
        }
      }
      fillarr_setFill(rf, xf);
      decG(w); decG(x); TFREE(lenO); TFREE(pos);
      return taga(r);
    } else {
      SLOW2("𝕨⊔𝕩", w, x);
      SGetU(w)
      i64 ria = wia==xia? 0 : o2i64(GetU(w, xia));
      if (ria<-1) thrM("⊔: 𝕨 can't contain elements less than ¯1");
      ria--;
      for (usz i = 0; i < xia; i++) {
        B cw = GetU(w, i);
        if (!q_i64(cw)) goto base;
        i64 c = o2i64u(cw);
        if (c>ria) ria = c;
        if (c<-1) thrM("⊔: 𝕨 can't contain elements less than ¯1");
      }
      if (ria > (i64)(USZ_MAX-1)) thrOOM();
      ria++;
      TALLOC(i32, lenO, ria+1); i32* len = lenO+1;
      TALLOC(i32, pos, ria);
      for (usz i = 0; i < ria; i++) len[i] = pos[i] = 0;
      for (usz i = 0; i < xia; i++) len[o2i64u(GetU(w, i))]++;
      
      Arr* r = arr_shVec(m_fillarrp(ria)); fillarr_setFill(r, m_f64(0));
      B* rp = fillarr_ptr(r);
      for (usz i = 0; i < ria; i++) rp[i] = m_f64(0); // don't break if allocation errors
      B xf = getFillQ(x);
      
      for (usz i = 0; i < ria; i++) {
        Arr* c = m_fillarrp(len[i]);
        c->ia = 0;
        fillarr_setFill(c, inc(xf));
        arr_shVec(c);
        rp[i] = taga(c);
      }
      Arr* rf = m_fillarrp(0); arr_shVec(rf);
      fillarr_setFill(rf, xf);
      fillarr_setFill(r, taga(rf));
      SGet(x)
      for (usz i = 0; i < xia; i++) {
        i64 n = o2i64u(GetU(w, i));
        if (n>=0) fillarr_ptr(a(rp[n]))[pos[n]++] = Get(x, i);
      }
      for (usz i = 0; i < ria; i++) a(rp[i])->ia = len[i];
      decG(w); decG(x); TFREE(lenO); TFREE(pos);
      return taga(r);
    }
  }
  base:
  return c2(rt_group, w, x);
}

extern B rt_reverse;
B reverse_c1(B t, B x) {
  if (isAtm(x) || rnk(x)==0) thrM("⌽: Argument cannot be a unit");
  usz xia = a(x)->ia;
  if (xia==0) return x;
  u8 xe = TI(x,elType);
  if (rnk(x)==1) {
    B r;
    switch(xe) { default: UD;
      case el_bit: { u64* xp = bitarr_ptr(x); u64* rp; r = m_bitarrv(&rp, xia); for (usz i = 0; i < xia; i++) bitp_set(rp, i, bitp_get(xp, xia-i-1)); break; }
      case el_i8: case el_c8:  { u8*  xp = tyany_ptr(x); u8*  rp = m_tyarrv(&r, 1, xia, el2t(xe)); for (usz i = 0; i < xia; i++) rp[i] = xp[xia-i-1]; break; }
      case el_i16:case el_c16: { u16* xp = tyany_ptr(x); u16* rp = m_tyarrv(&r, 2, xia, el2t(xe)); for (usz i = 0; i < xia; i++) rp[i] = xp[xia-i-1]; break; }
      case el_i32:case el_c32: { u32* xp = tyany_ptr(x); u32* rp = m_tyarrv(&r, 4, xia, el2t(xe)); for (usz i = 0; i < xia; i++) rp[i] = xp[xia-i-1]; break; }
      case el_f64:             { f64* xp = f64any_ptr(x); f64* rp; r = m_f64arrv(&rp, xia); for (usz i = 0; i < xia; i++) rp[i] = xp[xia-i-1]; break; }
      case el_B: {
        HArr_p rp = m_harrUc(x);
        B* xp = arr_bptr(x);
        if (xp!=NULL)  for (usz i = 0; i < xia; i++) rp.a[i] = inc(xp[xia-i-1]);
        else { SGet(x) for (usz i = 0; i < xia; i++) rp.a[i] = Get(x, xia-i-1); }
        r = rp.b;
        B xf = getFillQ(x);
        decG(x);
        return withFill(r, xf);
      }
    }
    decG(x);
    return r;
  }
  B xf = getFillQ(x);
  SLOW1("⌽𝕩", x);
  usz csz = arr_csz(x);
  usz cam = a(x)->sh[0];
  usz rp = 0;
  usz ip = xia;
  MAKE_MUT(r, xia); mut_init(r, xe);
  MUTG_INIT(r);
  for (usz i = 0; i < cam; i++) {
    ip-= csz;
    mut_copyG(r, rp, x, ip, csz);
    rp+= csz;
  }
  return withFill(mut_fcd(r, x), xf);
}
B reverse_c2(B t, B w, B x) {
  if (isArr(w)) return c2(rt_reverse, w, x);
  if (isAtm(x) || rnk(x)==0) thrM("⌽: 𝕩 must have rank at least 1 for atom 𝕨");
  usz xia = a(x)->ia;
  if (xia==0) return x;
  B xf = getFillQ(x);
  usz cam = a(x)->sh[0];
  usz csz = arr_csz(x);
  i64 am = o2i64(w);
  if ((u64)am >= (u64)cam) { am%= (i64)cam; if(am<0) am+= cam; }
  am*= csz;
  MAKE_MUT(r, xia); mut_init(r, TI(x,elType));
  MUTG_INIT(r);
  mut_copyG(r, 0, x, am, xia-am);
  mut_copyG(r, xia-am, x, 0, am);
  return withFill(mut_fcd(r, x), xf);
}


extern B rt_transp;
B transp_c1(B t, B x) {
  if (RARE(isAtm(x))) return m_atomUnit(x);
  ur xr = rnk(x);
  if (xr<=1) return x;
  
  usz ia = a(x)->ia;
  usz* xsh = a(x)->sh;
  usz h = xsh[0];
  usz w = xsh[1] * shProd(xsh, 2, xr);
  
  Arr* r;
  usz xi = 0;
  u8 xe = TI(x,elType);
  switch(xe) { default: UD;
    case el_i8: case el_c8:  { u8*  xp=tyany_ptr(x); u8*  rp = m_tyarrp(&r,1,ia,el2t(xe)); for(usz y=0;y<h;y++) for(usz x=0;x<w;x++) rp[x*h+y] = xp[xi++]; break; }
    case el_i16:case el_c16: { u16* xp=tyany_ptr(x); u16* rp = m_tyarrp(&r,2,ia,el2t(xe)); for(usz y=0;y<h;y++) for(usz x=0;x<w;x++) rp[x*h+y] = xp[xi++]; break; }
    case el_i32:case el_c32: { u32* xp=tyany_ptr(x); u32* rp = m_tyarrp(&r,4,ia,el2t(xe)); for(usz y=0;y<h;y++) for(usz x=0;x<w;x++) rp[x*h+y] = xp[xi++]; break; }
    case el_f64:             { f64* xp=f64any_ptr(x); f64* rp; r=m_f64arrp(&rp,ia); for(usz y=0;y<h;y++) for(usz x=0;x<w;x++) rp[x*h+y] = xp[xi++]; break; }
    case el_B: case el_bit: { // can't be bothered to implement a bitarr transpose
      B* xp = arr_bptr(x);
      B xf = getFillR(x);
      if (xp==NULL) { HArr* xa=cpyHArr(x); x=taga(xa); xp=xa->a; } // TODO extract this to an inline function
      
      HArr_p p = m_harrUp(ia);
      for(usz y=0;y<h;y++) for(usz x=0;x<w;x++) p.a[x*h+y] = inc(xp[xi++]);
      
      usz* rsh = arr_shAlloc((Arr*)p.c, xr);
      if (xr==2) {
        rsh[0] = w;
        rsh[1] = h;
      } else {
        shcpy(rsh, xsh+1, xr-1);
        rsh[xr-1] = h;
      }
      decG(x); return qWithFill(p.b, xf);
    }
  }
  usz* rsh = arr_shAlloc(r, xr);
  if (xr==2) {
    rsh[0] = w;
    rsh[1] = h;
  } else {
    shcpy(rsh, xsh+1, xr-1);
    rsh[xr-1] = h;
  }
  decG(x); return taga(r);
}
B transp_c2(B t, B w, B x) { return c2(rt_transp, w, x); }


B pick_uc1(B t, B o, B x) { // TODO do in-place like pick_ucw; maybe just call it?
  if (isAtm(x) || a(x)->ia==0) return def_fn_uc1(t, o, x);
  B xf = getFillQ(x);
  usz ia = a(x)->ia;
  B arg = IGet(x, 0);
  B rep = c1(o, arg);
  MAKE_MUT(r, ia); mut_init(r, el_or(TI(x,elType), selfElType(rep)));
  MUTG_INIT(r);
  mut_setG(r, 0, rep);
  mut_copyG(r, 1, x, 1, ia-1);
  return qWithFill(mut_fcd(r, x), xf);
}

B pick_ucw(B t, B o, B w, B x) {
  if (isArr(w) || isAtm(x) || rnk(x)!=1) return def_fn_ucw(t, o, w, x);
  usz xia = a(x)->ia;
  usz wi = WRAP(o2i64(w), xia, thrF("𝔽⌾(n⊸⊑)𝕩: reading out-of-bounds (n≡%R, %s≡≠𝕩)", w, xia));
  B arg = IGet(x, wi);
  B rep = c1(o, arg);
  if (reusable(x) && TI(x,canStore)(rep)) { REUSE(x);
    if      (TI(x,elType)==el_i8 ) { i8*  xp = i8any_ptr (x); xp[wi] = o2iu(rep); return x; }
    else if (TI(x,elType)==el_i16) { i16* xp = i16any_ptr(x); xp[wi] = o2iu(rep); return x; }
    else if (TI(x,elType)==el_i32) { i32* xp = i32any_ptr(x); xp[wi] = o2iu(rep); return x; }
    else if (TI(x,elType)==el_f64) { f64* xp = f64any_ptr(x); xp[wi] = o2fu(rep); return x; }
    else if (v(x)->type==t_harr) {
      B* xp = harr_ptr(x);
      dec(xp[wi]);
      xp[wi] = rep;
      return x;
    } else if (v(x)->type==t_fillarr) {
      B* xp = fillarr_ptr(a(x));
      dec(xp[wi]);
      xp[wi] = rep;
      return x;
    }
  }
  MAKE_MUT(r, xia); mut_init(r, el_or(TI(x,elType), selfElType(rep)));
  MUTG_INIT(r);
  mut_setG(r, wi, rep);
  mut_copyG(r, 0, x, 0, wi);
  mut_copyG(r, wi+1, x, wi+1, xia-wi-1);
  B xf = getFillQ(x);
  return qWithFill(mut_fcd(r, x), xf);
}

B slash_ucw(B t, B o, B w, B x) {
  if (isAtm(w) || isAtm(x) || rnk(w)!=1 || rnk(x)!=1 || a(w)->ia!=a(x)->ia) return def_fn_ucw(t, o, w, x);
  usz ia = a(x)->ia;
  SGetU(w)
  if (TI(w,elType)>el_i32) for (usz i = 0; i < ia; i++) if (!q_i32(GetU(w,i))) return def_fn_ucw(t, o, w, x);
  B arg = slash_c2(t, inc(w), inc(x));
  usz argIA = a(arg)->ia;
  B rep = c1(o, arg);
  if (isAtm(rep) || rnk(rep)!=1 || a(rep)->ia != argIA) thrF("𝔽⌾(a⊸/)𝕩: Result of 𝔽 must have the same shape as a/𝕩 (expected ⟨%s⟩, got %H)", argIA, rep);
  MAKE_MUT(r, ia); mut_init(r, el_or(TI(x,elType), TI(rep,elType)));
  SGet(x)
  SGet(rep)
  usz repI = 0;
  if (a(w)->type == t_bitarr) {
    u64* d = bitarr_ptr(w);
    if (TI(x,elType)<=el_i32 && TI(rep,elType)<=el_i32) {
      if (r->fns->elType!=el_i32) mut_to(r, el_i32);
      i32* rp = r->ai32;
      x   = toI32Any(x);   i32* xp = i32any_ptr(x);
      rep = toI32Any(rep); i32* np = i32any_ptr(rep);
      for (usz i = 0; i < ia; i++) {
        bool v = bitp_get(d, i);
        i32 nc = np[repI];
        i32 xc = xp[i];
        rp[i] = v? nc : xc;
        repI+= v;
      }
    } else {
      MUTG_INIT(r);
      for (usz i = 0; i < ia; i++) mut_setG(r, i, bitp_get(d, i)? Get(rep,repI++) : Get(x,i));
    }
  } else {
    SGetU(rep)
    MUTG_INIT(r);
    for (usz i = 0; i < ia; i++) {
      i32 cw = o2iu(GetU(w, i));
      if (cw) {
        B cr = Get(rep,repI);
        if (CHECK_VALID) for (i32 j = 1; j < cw; j++) if (!equal(GetU(rep,repI+j), cr)) { mut_pfree(r,i); thrM("𝔽⌾(a⊸/): Incompatible result elements"); }
        mut_setG(r, i, cr);
        repI+= cw;
      } else mut_setG(r, i, Get(x,i));
    }
  }
  decG(w); decG(rep);
  return mut_fcd(r, x);
}

static B takedrop_ucw(i64 wi, B o, u64 am, B x, size_t xr) {
  usz xia = a(x)->ia;
  usz csz = arr_csz(x);
  usz tk = csz*am; // taken element count
  usz lv = xia-tk; // elements left alone
  
  Arr* arg = TI(x,slice)(incG(x), wi<0? lv : 0, tk);
  usz* ash = arr_shAlloc(arg, xr);
  if (ash) { ash[0] = am; shcpy(ash+1, a(x)->sh+1, xr-1); }
  
  B rep = c1(o, taga(arg));
  if (isAtm(rep)) thrM("𝔽⌾(n⊸↑): 𝔽 returned an atom");
  usz* repsh = a(rep)->sh;
  if (rnk(rep)==0 || !eqShPart(repsh+1, a(x)->sh+1, xr-1) || repsh[0]!=am) thrM("𝔽⌾(n⊸↑)𝕩: 𝔽 returned an array with a different shape than n↑𝕩");
  
  MAKE_MUT(r, xia);
  mut_init(r, el_or(TI(x,elType), TI(rep,elType))); MUTG_INIT(r);
  if (wi<0) {
    mut_copyG(r, 0, x, 0, lv);
    mut_copyG(r, lv, rep, 0, tk);
  } else {
    mut_copyG(r, 0, rep, 0, tk);
    mut_copyG(r, tk, x, tk, lv);
  }
  
  dec(rep);
  return mut_fcd(r, x);
}

B take_ucw(B t, B o, B w, B x) {
  if (!isF64(w)) return def_fn_ucw(t, o, w, x);
  i64 wi = o2i64(w);
  u64 am = wi<0? -wi : wi;
  if (isAtm(x)) x = m_vec1(x);
  ur xr = rnk(x); if (xr==0) xr = 1;
  if (am>a(x)->sh[0]) thrF("𝔽⌾(n⊸↑)𝕩: Cannot modify fill with Under (%l ≡ 𝕨, %H ≡ ≢𝕩)", wi, x);
  return takedrop_ucw(wi, o, am, x, xr);
}

B drop_ucw(B t, B o, B w, B x) {
  if (!isF64(w)) return def_fn_ucw(t, o, w, x);
  i64 wi = o2i64(w);
  u64 am = wi<0? -wi : wi;
  if (isAtm(x)) x = m_vec1(x);
  ur xr = rnk(x); if (xr==0) xr = 1;
  usz cam = a(x)->sh[0];
  if (am>cam) am = cam;
  return takedrop_ucw(-wi, o, cam-am, x, xr);
}

static B shape_uc1_t(B r, usz ia) {
  if (!isArr(r) || rnk(r)!=1 || a(r)->ia!=ia) thrM("𝔽⌾⥊: 𝔽 changed the shape of the argument");
  return r;
}
B shape_uc1(B t, B o, B x) {
  if (!isArr(x) || rnk(x)==0) {
    usz xia = isArr(x)? a(x)->ia : 1;
    return shape_c2(t, emptyIVec(), shape_uc1_t(c1(o, shape_c1(t, x)), xia));
  }
  usz xia = a(x)->ia;
  if (rnk(x)==1) return shape_uc1_t(c1(o, x), xia);
  ur xr = rnk(x);
  ShArr* sh = ptr_inc(shObj(x));
  return truncReshape(shape_uc1_t(c1(o, shape_c1(t, x)), xia), xia, xia, xr, sh);
}

B select_ucw(B t, B o, B w, B x);

B  transp_uc1(B t, B o, B x) { return  transp_c1(t, c1(o,  transp_c1(t, x))); }
B reverse_uc1(B t, B o, B x) { return reverse_c1(t, c1(o, reverse_c1(t, x))); }

void sfns_init() {
  c(BFn,bi_pick)->uc1 = pick_uc1;
  c(BFn,bi_reverse)->uc1 = reverse_uc1;
  c(BFn,bi_pick)->ucw = pick_ucw;
  c(BFn,bi_slash)->ucw = slash_ucw;
  c(BFn,bi_select)->ucw = select_ucw; // TODO move to new init fn
  c(BFn,bi_shape)->uc1 = shape_uc1;
  c(BFn,bi_transp)->uc1 = transp_uc1;
  c(BFn,bi_take)->ucw = take_ucw;
  c(BFn,bi_drop)->ucw = drop_ucw;
  c(BFn,bi_slash)->im = slash_im;
}
