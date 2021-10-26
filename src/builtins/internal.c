#include "../core.h"
#include "../utils/mut.h"
#include "../builtins.h"

B itype_c1(B t, B x) {
  B r;
  if(isVal(x)) {
    r = m_str8l(type_repr(v(x)->type));
  } else {
    if      (isF64(x)) r = m_str8l("tagged f64");
    else if (isC32(x)) r = m_str8l("tagged c32");
    else if (isTag(x)) r = m_str8l("tagged tag");
    else if (isVar(x)) r = m_str8l("tagged var");
    else if (isExt(x)) r = m_str8l("tagged extvar");
    else               r = m_str8l("tagged unknown");
  }
  dec(x);
  return r;
}
B elType_c1(B t, B x) {
  B r = m_i32(isArr(x)? TI(x,elType) : selfElType(x));
  dec(x);
  return r;
}
B refc_c1(B t, B x) {
  B r = isVal(x)? m_i32(v(x)->refc) : m_str8l("(not heap-allocated)");
  dec(x);
  return r;
}
B squeeze_c1(B t, B x) {
  if (!isArr(x)) return x;
  return any_squeeze(x);
}
B deepSqueeze_c1(B t, B x) {
  return squeeze_deep(x);
}
B isPure_c1(B t, B x) {
  B r = m_f64(isPureFn(x));
  dec(x);
  return r;
}

B info_c2(B t, B w, B x) {
  B s = emptyCVec();
  i32 m = o2i(w);
  if (isVal(x)) {
    if (m) AFMT("%xl: ", x.u);
    else AFMT("%xl: ", x.u>>48);
    Value* xv = v(x);
    AFMT("refc:%i ", xv->refc);
    if (m) {
      AFMT("mmInfo:%i ", xv->mmInfo);
      AFMT("flags:%i ", xv->flags);
      AFMT("extra:%i ", xv->extra);
    }
    AFMT("type:%i=%S ", xv->type, type_repr(xv->type));
    AFMT("alloc:%l", mm_size(xv));
    dec(x);
  } else {
    AFMT("%xl: ", x.u);
    A8("not heap-allocated");
  }
  return s;
}
B info_c1(B t, B x) {
  return info_c2(t, m_i32(0), x);
}

#define FOR_VARIATION(F) F(Ai8 ) F(Si8 ) F(Ai8Inc ) F(Si8Inc ) \
                         F(Ai16) F(Si16) F(Ai16Inc) F(Si16Inc) \
                         F(Ai32) F(Si32) F(Ai32Inc) F(Si32Inc) \
                         F(Ac8 ) F(Sc8 ) F(Ac8Inc ) F(Sc8Inc ) \
                         F(Ac16) F(Sc16) F(Ac16Inc) F(Sc16Inc) \
                         F(Ac32) F(Sc32) F(Ac32Inc) F(Sc32Inc) \
                         F(Af64) F(Sf64) F(Af64Inc) F(Sf64Inc) \
                         F(Ah)   F(Sh)   F(AhInc)   F(ShInc) \
                         F(Af)   F(Sf)   F(AfInc)   F(SfInc) \
                         F(Ab)           F(AbInc)

#define F(X) static B v_##X;
FOR_VARIATION(F)
#undef F
static B listVariations_def;

B listVariations_c2(B t, B w, B x) {
  if (!isArr(x)) thrM("•internal.ListVariations: 𝕩 must be an array");
  
  if (!isArr(w) || rnk(w)!=1) thrM("•internal.ListVariations: 𝕨 must be a list");
  usz wia = a(w)->ia;
  SGetU(w)
  bool c_incr=false, c_rmFill=false;
  for (usz i = 0; i < wia; i++) {
    u32 c = o2c(GetU(w, i));
    if (c=='i') c_incr=true;
    else if (c=='f') c_rmFill=true;
    else thrF("internal.ListVariations: Unknown option '%c' in 𝕨", c);
  }
  dec(w);
  
  u8 xe = TI(x,elType);
  B xf = getFillQ(x);
  bool ah = c_rmFill || noFill(xf);
  bool ai8=false, ai16=false, ai32=false, af64=false,
       ac8=false, ac16=false, ac32=false, abit=false;
  usz xia = a(x)->ia;
  SGetU(x)
  if (isNum(xf)) {
    i32 min=I32_MAX, max=I32_MIN;
    if      (xe==el_i8 ) { i8*  xp = i8any_ptr (x); for (usz i = 0; i < xia; i++) { if (xp[i]>max) max=xp[i]; if (xp[i]<min) min=xp[i]; } }
    else if (xe==el_i16) { i16* xp = i16any_ptr(x); for (usz i = 0; i < xia; i++) { if (xp[i]>max) max=xp[i]; if (xp[i]<min) min=xp[i]; } }
    else if (xe==el_i32) { i32* xp = i32any_ptr(x); for (usz i = 0; i < xia; i++) { if (xp[i]>max) max=xp[i]; if (xp[i]<min) min=xp[i]; } }
    else if (xe==el_f64) { f64* xp = f64any_ptr(x); for (usz i = 0; i < xia; i++) { if (xp[i]>max) max=xp[i]; if (xp[i]<min) min=xp[i]; if(xp[i]!=(i32)xp[i]) goto onlyF64; } }
    else for (usz i = 0; i < xia; i++) { B c = GetU(x, i); if (!isF64(c)) goto noSpec; if (c.f>max) max=c.f; if (c.f<min) min=c.f; }
    ai8  = min==(i8 )min && max==(i8 )max;
    ai16 = min==(i16)min && max==(i16)max;
    ai32 = min==(i32)min && max==(i32)max;
    abit = min>=0 && max<=1;
    onlyF64:
    af64 = true;
  } else if (isC32(xf)) {
    u32 max = 0;
    if (xe!=el_c8) for (usz i = 0; i < xia; i++) {
      B c = GetU(x, i);
      if (!isC32(c)) goto noSpec;
      if (o2cu(c)>max) max = o2cu(c);
    }
    ac8  = max == (u8 )max;
    ac16 = max == (u16)max;
    ac32 = true;
  }
  noSpec:;
  B r = emptyHVec();
  if(abit) { r=vec_add(r,inc(v_Ab  ));                          if(c_incr) { r=vec_add(r,inc(v_AbInc  ));                             } }
  if(ai8 ) { r=vec_add(r,inc(v_Ai8 ));r=vec_add(r,inc(v_Si8 )); if(c_incr) { r=vec_add(r,inc(v_Ai8Inc ));r=vec_add(r,inc(v_Si8Inc )); } }
  if(ai16) { r=vec_add(r,inc(v_Ai16));r=vec_add(r,inc(v_Si16)); if(c_incr) { r=vec_add(r,inc(v_Ai16Inc));r=vec_add(r,inc(v_Si16Inc)); } }
  if(ai32) { r=vec_add(r,inc(v_Ai32));r=vec_add(r,inc(v_Si32)); if(c_incr) { r=vec_add(r,inc(v_Ai32Inc));r=vec_add(r,inc(v_Si32Inc)); } }
  if(ac8 ) { r=vec_add(r,inc(v_Ac8 ));r=vec_add(r,inc(v_Sc8 )); if(c_incr) { r=vec_add(r,inc(v_Ac8Inc ));r=vec_add(r,inc(v_Sc8Inc )); } }
  if(ac16) { r=vec_add(r,inc(v_Ac16));r=vec_add(r,inc(v_Sc16)); if(c_incr) { r=vec_add(r,inc(v_Ac16Inc));r=vec_add(r,inc(v_Sc16Inc)); } }
  if(ac32) { r=vec_add(r,inc(v_Ac32));r=vec_add(r,inc(v_Sc32)); if(c_incr) { r=vec_add(r,inc(v_Ac32Inc));r=vec_add(r,inc(v_Sc32Inc)); } }
  if(af64) { r=vec_add(r,inc(v_Af64));r=vec_add(r,inc(v_Sf64)); if(c_incr) { r=vec_add(r,inc(v_Af64Inc));r=vec_add(r,inc(v_Sf64Inc)); } }
  if(ah)   { r=vec_add(r,inc(v_Ah  ));r=vec_add(r,inc(v_Sh  )); if(c_incr) { r=vec_add(r,inc(v_AhInc  ));r=vec_add(r,inc(v_ShInc  )); } }
  {          r=vec_add(r,inc(v_Af  ));r=vec_add(r,inc(v_Sf  )); if(c_incr) { r=vec_add(r,inc(v_AfInc  ));r=vec_add(r,inc(v_SfInc  )); } }
  dec(x);
  dec(xf);
  return r;
}
B listVariations_c1(B t, B x) {
  return listVariations_c2(t, inc(listVariations_def), x);
}
static bool u8_get(u8** cv, u8* cE, const char* x) {
  u8* c = *cv;
  while (true) {
    if (!*x) {
      *cv = c;
      return true;
    }
    if (c==cE || *c!=*x) return false;
    c++; x++;
  }
  
}

static B variation_refs;
static void variation_gcRoot() {
  mm_visit(variation_refs);
  mm_visit(listVariations_def);
  #define F(X) mm_visit(v_##X);
  FOR_VARIATION(F)
  #undef F
}

B variation_c2(B t, B w, B x) {
  if (!isArr(x)) thrM("•internal.Variation: Non-array 𝕩");
  usz xia = a(x)->ia;
  u8 xe = TI(x,elType);
  SGet(x)
  C8Arr* wc = toC8Arr(w);
  u8* wp = c8arrv_ptr(wc);
  u8* wpE = wp+wc->ia;
  if (wc->ia==0) thrM("•internal.Variation: Zero-length 𝕨");
  B res;
  if (*wp == 'A' || *wp == 'S') {
    bool slice = *wp == 'S';
    wp++;
    if      (u8_get(&wp, wpE, "b"  )) res = taga(cpyBitArr(inc(x)));
    else if (u8_get(&wp, wpE, "i8" )) res = taga(cpyI8Arr(inc(x)));
    else if (u8_get(&wp, wpE, "i16")) res = taga(cpyI16Arr(inc(x)));
    else if (u8_get(&wp, wpE, "i32")) res = taga(cpyI32Arr(inc(x)));
    else if (u8_get(&wp, wpE, "c8" )) res = taga(cpyC8Arr(inc(x)));
    else if (u8_get(&wp, wpE, "c16")) res = taga(cpyC16Arr(inc(x)));
    else if (u8_get(&wp, wpE, "c32")) res = taga(cpyC32Arr(inc(x)));
    else if (u8_get(&wp, wpE, "f64")) res = taga(cpyF64Arr(inc(x)));
    else if (u8_get(&wp, wpE, "h"  )) res = taga(cpyHArr(inc(x)));
    else if (u8_get(&wp, wpE, "f")) {
      Arr* t = m_fillarrp(xia);
      res = taga(t);
      fillarr_setFill(t, getFillQ(x));
      arr_shCopy(t, x);
      B* rp = fillarr_ptr(t);
      if      (xe==el_i32) { i32* xp=i32any_ptr(x); for (usz i = 0; i < xia; i++) rp[i] = m_f64(xp[i]); }
      else if (xe==el_f64) { f64* xp=f64any_ptr(x); for (usz i = 0; i < xia; i++) rp[i] = m_f64(xp[i]); }
      else for (usz i = 0; i < xia; i++) rp[i] = Get(x,i);
    } else thrF("•internal.Variation: Bad type \"%R\"", taga(wc));
    if (slice) {
      Arr* slice = TI(res,slice)(res, 0, a(res)->ia);
      arr_shCopy(slice, res);
      res = taga(slice);
    }
    if (u8_get(&wp, wpE, "Inc")) {
      if (!variation_refs.u) {
        variation_refs = emptyHVec();
      }
      variation_refs = vec_add(variation_refs, inc(res));
    }
    if (wp!=wpE) thrM("•internal.Variation: Bad 𝕨");
  } else thrM("•internal.Variation: Bad start of 𝕨");
  dec(x);
  ptr_dec(wc);
  return res;
}

B clearRefs_c1(B t, B x) {
  dec(x);
  if (!isArr(variation_refs)) return m_f64(0);
  usz res = a(variation_refs)->ia;
  dec(variation_refs);
  variation_refs = m_f64(0);
  return m_f64(res);
}

static B unshare(B x) {
  if (!isArr(x)) return x;
  usz xia = a(x)->ia;
  switch (v(x)->type) {
    case t_i8arr:  return taga(cpyI8Arr (inc(x)));
    case t_i16arr: return taga(cpyI16Arr(inc(x)));
    case t_i32arr: return taga(cpyI32Arr(inc(x)));
    case t_c8arr:  return taga(cpyC8Arr (inc(x)));
    case t_c16arr: return taga(cpyC16Arr(inc(x)));
    case t_c32arr: return taga(cpyC32Arr(inc(x)));
    case t_f64arr: return taga(cpyF64Arr(inc(x)));
    case t_harr: {
      HArr_p r = m_harrUc(x);
      B* xp = harr_ptr(x);
      for (usz i = 0; i < xia; i++) r.a[i] = unshare(xp[i]);
      return r.b;
    }
    case t_fillarr: {
      Arr* r = m_fillarrp(xia); arr_shCopy(r, x);
      fillarr_setFill(r, unshare(c(FillArr,x)->fill));
      B* rp = fillarr_ptr(r); B* xp = fillarr_ptr(a(x));
      for (usz i = 0; i < xia; i++) rp[i] = unshare(xp[i]);
      return taga(r);
    }
    default: thrF("•internal.Unshare: Cannot unshare array with type %i=%S", v(x)->type, type_repr((v(x)->type)));
  }
}

B unshare_c1(B t, B x) {
  if (!isArr(x)) thrM("•internal.Unshare: Argument must be an array");
  B r = unshare(x);
  dec(x);
  return r;
}
static B internalNS;
B getInternalNS() {
  if (internalNS.u == 0) {
    #define F(X) v_##X = m_str8l(#X);
    FOR_VARIATION(F)
    #undef F
    listVariations_def = m_str8l("if");
    gc_addFn(variation_gcRoot);
    #define F(X) inc(bi_##X),
    B fn = bqn_exec(m_str32(U"{⟨ Type,  ElType,  Refc,  Squeeze,  IsPure,  Info,  ListVariations,  Variation,  ClearRefs,  Unshare,  DeepSqueeze⟩⇐𝕩}"), emptyCVec(), emptySVec());
    B arg =    m_caB(11,(B[]){F(itype)F(elType)F(refc)F(squeeze)F(isPure)F(info)F(listVariations)F(variation)F(clearRefs)F(unshare)F(deepSqueeze)});
    #undef F
    internalNS = c1(fn,arg);
    gc_add(internalNS);
    dec(fn);
  }
  return inc(internalNS);
}
