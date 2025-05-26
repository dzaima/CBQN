#include "../core.h"
#include "../utils/file.h"
#include "../builtins.h"
#include "../ns.h"
#include "../utils/cstr.h"
#include "../utils/calls.h"
#include <stdarg.h>

B itype_c1(B t, B x) {
  B r;
  if(isVal(x)) {
    r = m_c8vec_0(type_repr(TY(x)));
  } else {
    if      (isF64(x)) r = m_c8vec_0("tagged f64");
    else if (isC32(x)) r = m_c8vec_0("tagged c32");
    else if (isTag(x)) r = m_c8vec_0("tagged tag");
    else if (isVar(x)) r = m_c8vec_0("tagged var");
    else if (isExt(x)) r = m_c8vec_0("tagged extvar");
    else               r = m_c8vec_0("tagged unknown");
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
  B r = isVal(x)? m_i32(v(x)->refc) : m_c8vec_0("(not heap-allocated)");
  dec(x);
  return r;
}
B squeeze_c1(B t, B x) {
  if (!isArr(x)) return x;
  return squeeze_any(x);
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
    AFMT("type:%i=%S ", PTY(xv), type_repr(PTY(xv)));
    AFMT("alloc:%l", mm_size(xv));
    decG(x);
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

#define F(X) STATIC_GLOBAL B v_##X;
FOR_VARIATION(F)
#undef F
STATIC_GLOBAL B listVariations_def;

B listVariations_c2(B t, B w, B x) {
  if (!isArr(x)) thrM("𝕨 •internal.ListVariations 𝕩: 𝕩 must be an array");
  
  if (!isArr(w) || RNK(w)!=1) thrM("𝕨 •internal.ListVariations 𝕩: 𝕨 must be a list");
  usz wia = IA(w);
  SGetU(w)
  bool c_incr=false, c_rmFill=false;
  for (usz i = 0; i < wia; i++) {
    u32 c = o2c(GetU(w, i));
    if (c=='i') c_incr=true;
    else if (c=='f') c_rmFill=true;
    else thrF("𝕨 internal.ListVariations 𝕩: Unknown option '%c' in 𝕨", c);
  }
  decG(w);
  
  u8 xe = TI(x,elType);
  B xf = getFillQ(x);
  bool ah = c_rmFill || noFill(xf);
  bool ai8=false, ai16=false, ai32=false, af64=false,
       ac8=false, ac16=false, ac32=false, abit=false;
  usz xia = IA(x);
  SGetU(x)
  if (isNum(xf)) {
    i32 min=I32_MAX, max=I32_MIN;
    if      (xe==el_i8 ) { i8*  xp = i8any_ptr (x); for (usz i = 0; i < xia; i++) { if (xp[i]>max) max=xp[i]; if (xp[i]<min) min=xp[i]; } }
    else if (xe==el_i16) { i16* xp = i16any_ptr(x); for (usz i = 0; i < xia; i++) { if (xp[i]>max) max=xp[i]; if (xp[i]<min) min=xp[i]; } }
    else if (xe==el_i32) { i32* xp = i32any_ptr(x); for (usz i = 0; i < xia; i++) { if (xp[i]>max) max=xp[i]; if (xp[i]<min) min=xp[i]; } }
    else if (xe==el_f64) { f64* xp = f64any_ptr(x); for (usz i = 0; i < xia; i++) { if (xp[i]>max) max=xp[i]; if (xp[i]<min) min=xp[i]; if(!q_fi32(xp[i])) goto onlyF64; } }
    else {
      bool notFloat = false;
      for (usz i = 0; i < xia; i++) {
        B c = GetU(x, i);
        if (!isF64(c)) goto noSpec;
        if (!q_fi32(o2fG(c))) {
          notFloat=true;
        } else {
          i32 v = o2iG(c);
          if (v>max) max=v;
          if (v<min) min=v;
        }
      }
      if (notFloat) goto onlyF64;
    }
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
      if (o2cG(c)>max) max = o2cG(c);
    }
    ac8  = max == (u8 )max;
    ac16 = max == (u16)max;
    ac32 = true;
  }
  noSpec:;
  B r = emptyHVec();
  if(abit) { r=vec_addN(r,incG(v_Ab  ));                            if(c_incr) { r=vec_addN(r,incG(v_AbInc  ));                               } }
  if(ai8 ) { r=vec_addN(r,incG(v_Ai8 ));r=vec_addN(r,incG(v_Si8 )); if(c_incr) { r=vec_addN(r,incG(v_Ai8Inc ));r=vec_addN(r,incG(v_Si8Inc )); } }
  if(ai16) { r=vec_addN(r,incG(v_Ai16));r=vec_addN(r,incG(v_Si16)); if(c_incr) { r=vec_addN(r,incG(v_Ai16Inc));r=vec_addN(r,incG(v_Si16Inc)); } }
  if(ai32) { r=vec_addN(r,incG(v_Ai32));r=vec_addN(r,incG(v_Si32)); if(c_incr) { r=vec_addN(r,incG(v_Ai32Inc));r=vec_addN(r,incG(v_Si32Inc)); } }
  if(ac8 ) { r=vec_addN(r,incG(v_Ac8 ));r=vec_addN(r,incG(v_Sc8 )); if(c_incr) { r=vec_addN(r,incG(v_Ac8Inc ));r=vec_addN(r,incG(v_Sc8Inc )); } }
  if(ac16) { r=vec_addN(r,incG(v_Ac16));r=vec_addN(r,incG(v_Sc16)); if(c_incr) { r=vec_addN(r,incG(v_Ac16Inc));r=vec_addN(r,incG(v_Sc16Inc)); } }
  if(ac32) { r=vec_addN(r,incG(v_Ac32));r=vec_addN(r,incG(v_Sc32)); if(c_incr) { r=vec_addN(r,incG(v_Ac32Inc));r=vec_addN(r,incG(v_Sc32Inc)); } }
  if(af64) { r=vec_addN(r,incG(v_Af64));r=vec_addN(r,incG(v_Sf64)); if(c_incr) { r=vec_addN(r,incG(v_Af64Inc));r=vec_addN(r,incG(v_Sf64Inc)); } }
  if(ah)   { r=vec_addN(r,incG(v_Ah  ));r=vec_addN(r,incG(v_Sh  )); if(c_incr) { r=vec_addN(r,incG(v_AhInc  ));r=vec_addN(r,incG(v_ShInc  )); } }
  {          r=vec_addN(r,incG(v_Af  ));r=vec_addN(r,incG(v_Sf  )); if(c_incr) { r=vec_addN(r,incG(v_AfInc  ));r=vec_addN(r,incG(v_SfInc  )); } }
  decG(x);
  dec(xf);
  return r;
}
B listVariations_c1(B t, B x) {
  return listVariations_c2(t, incG(listVariations_def), x);
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

STATIC_GLOBAL B variation_refs;
B variation_c2(B t, B w, B x) {
  if (!isArr(w)) thrM("𝕨 •internal.Variation 𝕩: Non-array 𝕨");
  if (!isArr(x)) thrM("𝕨 •internal.Variation 𝕩: Non-array 𝕩");
  usz xia = IA(x);
  C8Arr* wc = toC8Arr(w);
  u8* wp = c8arrv_ptr(wc);
  u8* wpE = wp+PIA(wc);
  if (PIA(wc)==0) thrM("𝕨 •internal.Variation 𝕩: Zero-length 𝕨");
  B res;
  if (*wp == 'A' || *wp == 'S') {
    bool slice = *wp == 'S';
    wp++;
    if      (u8_get(&wp, wpE, "b"  )) res = taga(cpyBitArr(incG(x)));
    else if (u8_get(&wp, wpE, "i8" )) res = taga(cpyI8Arr (incG(x)));
    else if (u8_get(&wp, wpE, "i16")) res = taga(cpyI16Arr(incG(x)));
    else if (u8_get(&wp, wpE, "i32")) res = taga(cpyI32Arr(incG(x)));
    else if (u8_get(&wp, wpE, "c8" )) res = taga(cpyC8Arr (incG(x)));
    else if (u8_get(&wp, wpE, "c16")) res = taga(cpyC16Arr(incG(x)));
    else if (u8_get(&wp, wpE, "c32")) res = taga(cpyC32Arr(incG(x)));
    else if (u8_get(&wp, wpE, "f64")) res = taga(cpyF64Arr(incG(x)));
    else if (u8_get(&wp, wpE, "h"  )) res = taga(cpyHArr  (incG(x)));
    else if (u8_get(&wp, wpE, "f")) {
      Arr* r = m_fillarrp(xia);
      fillarr_setFill(r, getFillR(x));
      arr_shCopy(r, x);
      COPY_TO(fillarrv_ptr(r), el_B, 0, x, 0, xia);
      NOGC_E;
      
      res = taga(r);
    } else thrF("𝕨 •internal.Variation 𝕩: Bad type \"%R\"", taga(wc));
    
    if (slice) {
      Arr* slice = TI(res,slice)(incG(res), 0, IA(res));
      arr_shCopy(slice, res);
      decG(res);
      res = taga(slice);
    }
    
    if (u8_get(&wp, wpE, "Inc")) {
      if (!variation_refs.u) {
        variation_refs = emptyHVec();
      }
      variation_refs = vec_addN(variation_refs, incG(res));
    }
    if (wp!=wpE) thrM("𝕨 •internal.Variation 𝕩: Bad 𝕨");
  } else thrM("𝕨 •internal.Variation 𝕩: Bad start of 𝕨");
  decG(x);
  ptr_dec(wc);
  return res;
}

B clearRefs_c1(B t, B x) {
  dec(x);
  if (!isArr(variation_refs)) return m_f64(0);
  usz res = IA(variation_refs);
  decG(variation_refs);
  variation_refs = m_f64(0);
  return m_f64(res);
}

static NOINLINE B unshareShape(Arr* x) {
  ur xr = PRNK(x);
  if (xr<=1) return taga(x);
  ShArr* sh = m_shArr(xr);
  shcpy(sh->a, x->sh, xr);
  arr_shReplace(x, xr, sh);
  return taga(x);
}
static B unshare(B x) { // doesn't consume
  if (!isArr(x)) return inc(x);
  usz xia = IA(x);
  switch (TY(x)) {
    case t_bitarr: return unshareShape((Arr*)cpyBitArr(incG(x)));
    case t_i8arr:  case t_i8slice:  return unshareShape((Arr*)cpyI8Arr (incG(x)));
    case t_i16arr: case t_i16slice: return unshareShape((Arr*)cpyI16Arr(incG(x)));
    case t_i32arr: case t_i32slice: return unshareShape((Arr*)cpyI32Arr(incG(x)));
    case t_c8arr:  case t_c8slice:  return unshareShape((Arr*)cpyC8Arr (incG(x)));
    case t_c16arr: case t_c16slice: return unshareShape((Arr*)cpyC16Arr(incG(x)));
    case t_c32arr: case t_c32slice: return unshareShape((Arr*)cpyC32Arr(incG(x)));
    case t_f64arr: case t_f64slice: return unshareShape((Arr*)cpyF64Arr(incG(x)));
    case t_harr: case t_hslice: {
      B* xp = TY(x)==t_harr? harr_ptr(x) : hslice_ptr(x);
      M_HARR(r, xia)
      for (usz i = 0; i < xia; i++) HARR_ADD(r, i, unshare(xp[i]));
      return unshareShape(a(HARR_FC(r, x)));
    }
    case t_fillarr: case t_fillslice: {
      Arr* r = arr_shCopy(m_fillarr0p(xia), x);
      fillarr_setFill(r, unshare(getFillR(x)));
      B* rp = fillarrv_ptr(r);
      B* xp = arr_bptr(x);
      for (usz i = 0; i < xia; i++) rp[i] = unshare(xp[i]);
      return unshareShape(r);
    }
    default: thrF("•internal.Unshare 𝕩: Cannot unshare array with type %i=%S", TY(x), type_repr(TY(x)));
  }
}



B eequal_c2(B t, B w, B x) {
  bool r = eequal(w, x);
  dec(w); dec(x);
  return m_i32(r);
}

#ifdef TEST_BITCPY
  #include "../utils/mut.h"
#endif
#if NATIVE_COMPILER
  extern B native_comp;
  void switchComp(void);
#endif
#if TEST_CELL_FILLS
  extern i32 fullCellFills;
  extern i32 cellFillErrored;
#endif
#if TEST_RANGE
  #include "../utils/calls.h"
#endif
#if TEST_GROUP_STAT
  extern void (*const si_group_statistics_i8)(void*,usz,uint8_t*,usz*,uint8_t*,usz*,int8_t*);
  extern void (*const si_group_statistics_i16)(void*,usz,uint8_t*,usz*,uint8_t*,usz*,int16_t*);
  extern void (*const si_group_statistics_i32)(void*,usz,uint8_t*,usz*,uint8_t*,usz*,int32_t*);
#endif
B internalTemp_c1(B t, B x) {
  #if TEST_GROUP_STAT
    u8 bad; usz neg; u8 sort; usz change; i32 max;
    #define CASE(T) \
      if (TI(x,elType)==el_##T) { T max_t; si_group_statistics_##T(tyany_ptr(x), IA(x), &bad, &neg, &sort, &change, &max_t); max = max_t; } \
      else
    CASE(i8) CASE(i16) CASE(i32)
    thrM("bad eltype");
    #undef CASE
    decG(x);
    f64* rp; B r = m_f64arrv(&rp, 5);
    rp[0] = bad; rp[1] = neg; rp[2] = sort; rp[3] = change; rp[4] = max;
    return r;
  #endif
  #if TEST_RANGE
    i64 buf[2];
    bool b = getRange_fns[TI(x,elType)](tyany_ptr(x), buf, IA(x));
    decG(x);
    f64* rp;
    B r = m_f64arrv(&rp, 3);
    rp[0] = buf[0];
    rp[1] = buf[1];
    rp[2] = b;
    return r;
  #endif
  #if TEST_CELL_FILLS
    if (isNum(x)) fullCellFills = o2iG(x);
    B r = m_i32(cellFillErrored);
    cellFillErrored = 0;
    return r;
  #endif
  #if NATIVE_COMPILER
    switchComp();
    B r = bqn_exec(x, bi_N);
    switchComp();
    return r;
  #endif
  #ifdef TEST_BITCPY
    SGetU(x)
    bit_cpyN(bitarr_ptr(GetU(x,0)), o2s(GetU(x,1)), bitany_ptr(GetU(x,2)), o2s(GetU(x,3)), o2s(GetU(x,4)));
  #endif
  return x;
}

B internalTemp_c2(B t, B w, B x) {
  #if NATIVE_COMPILER
    return c2(native_comp, w, x);
  #endif
  #ifdef TEST_MUT
    SGetU(x)
    FILL_TO(tyarr_ptr(w), o2s(GetU(x,0)), o2s(GetU(x,1)), GetU(x,2), o2s(GetU(x,3)));
    dec(w);
  #endif
  return x;
}

B heapDump_c1(B t, B x) {
  if (!isArr(x)) {
    cbqn_heapDump(NULL);
  } else {
    char* s = toCStr(x);
    cbqn_heapDump(s);
    freeCStr(s);
  }
  return x;
}

B internalGC_c1(B t, B x) {
  #if ENABLE_GC
    gc_forceGC(false);
    dec(x); return m_f64(1);
  #else
    dec(x); return m_f64(0);
  #endif
}


void heap_printInfoStr(char* str);
B vfyStr(B x, char* name, char* arg);
B heapStats_c1(B t, B x) {
  if (isC32(x)) {
    f64* rp; B r = m_f64arrv(&rp, 2);
    rp[0] = mm_heapAlloc;
    rp[1] = tot_heapUsed();
    return r;
  }
  vfyStr(x, "•internal.HeapStats", "𝕩");
  char* cs = toCStr(x);
  heap_printInfoStr(cs);
  freeCStr(cs);
  return m_f64(1);
}

B iObjFlags_c1(B t, B x) {
  u8 r = v(x)->flags;
  decG(x);
  return m_i32(r);
}
B iObjFlags_c2(B t, B w, B x) {
  v(x)->flags = o2iG(w);
  return x;
}

B iHasFill_c1(B t, B x) {
  B f = getFillR(x);
  dec(x);
  if (noFill(f)) return m_f64(0);
  dec(f);
  return m_f64(1);
}

B iPureKeep_c1(B t, B x) { return x; }
B iKeep_c1(B t, B x) { return x; }

B iProperties_c2(B t, B w, B x) {
  if (w.u!=m_c32(0).u || x.u != m_c32(0).u) thrM("𝕨 •internal.Properties 𝕩: bad arg");
  i32* rp;
  B r = m_i32arrv(&rp, 3);
  rp[0] = sizeof(usz)*8;
  rp[1] = PROPER_FILLS;
  rp[2] = EACH_FILLS;
  return r;
}

static NOINLINE NORETURN void validate_fail(bool crash, char* p, ...) {
  va_list a;
  va_start(a, p);
  B msg = do_fmt(emptyCVec(), p, a);
  va_end(a);
  if (!crash) thr(msg);
  printsB(msg);
  fatal("object failed validation");
}


bool validate_flags(bool crash, B x) {
  if (isArr(x) && IA(x)!=0) {
    if (FL_HAS(x, fl_squoze)) {
      B t = unshare(x);
      t = squeeze_any(t);
      if (TI(t,elType) != TI(x,elType)) validate_fail(crash, "Validate: Wrongly squeezed: array has eltype %S, expected %S", eltype_repr(TI(x,elType)), eltype_repr(TI(t,elType)));
      decG(t);
    }
    #define DSC_ASC(FLAG, CMP) \
    if (FL_HAS(x, FLAG)) {   \
      if (RNK(x)==1) {       \
        SGetU(x)             \
        usz ia = IA(x);      \
        for (ux i = 0; i < ia-1; i++) if (compare(GetU(x,i),GetU(x,i+1)) CMP) validate_fail(crash, "Validate: Incorrectly marked " #FLAG " between indices %z+0‿1", i); \
      }                      \
    }
    DSC_ASC(fl_dsc, < 0)
    DSC_ASC(fl_asc, > 0)
  }
  return true;
}

B iValidate_c1(B t, B x) {
  validate_flags(false, x);
  return x;
}

B unshare_c1(B t, B x) {
  if (!isArr(x)) thrM("•internal.Unshare 𝕩: 𝕩 must be an array");
  B r = unshare(x);
  decG(x);
  return r;
}
STATIC_GLOBAL B internalNS;
B getInternalNS(void) {
  if (internalNS.u == 0) {
    #define F(X) v_##X = m_c8vec_0(#X);
    FOR_VARIATION(F)
    #undef F
    listVariations_def = m_c8vec_0("if");
    
    gc_add(listVariations_def);
    gc_add_ref(&variation_refs);
    #define F(X) gc_add_ref(&v_##X); // 38 refs
    FOR_VARIATION(F)
    #undef F
    
    #define F(X) incG(bi_##X),
    Body* d =    m_nnsDesc("type","eltype","refc","squeeze","ispure","info", "keep", "purekeep","listvariations","variation","clearrefs", "hasfill","unshare","deepsqueeze","heapdump","eequal",        "gc",        "temp","heapstats", "objflags", "properties", "validate");
    internalNS = m_nns(d,F(itype)F(elType)F(refc)F(squeeze)F(isPure)F(info)F(iKeep)F(iPureKeep)F(listVariations)F(variation)F(clearRefs)F(iHasFill)F(unshare)F(deepSqueeze)F(heapDump)F(eequal)F(internalGC)F(internalTemp)F(heapStats)F(iObjFlags)F(iProperties)F(iValidate));
    #undef F
    gc_add(internalNS);
  }
  return incG(internalNS);
}
