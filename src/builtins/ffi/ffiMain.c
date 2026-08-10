#include "ffiCore.c"
#include "utils/talloc.h"
#include "ffiExport.c"

struct ScratchMemAlloc {
  struct Value;
  B tmpBufs;
  __attribute__((aligned(8)))
  u8 rest[];
  // scratch memory is relative to this->rest:
  //   return value at offset=0
  //   argument values at ffiEnt.argData.scratchMemOffset
  //   argument list at ffiFn->argListOffset (or their values for GPR6_SYSV64)
};



static char* sty_name(u8 sty) {
  switch (sty) {
    case sty_void: return "void";
    case sty_i8: return "i8"; case sty_i16: return "i16"; case sty_i32: return "i32"; case sty_i64: return "i64";
    case sty_u8: return "u8"; case sty_u16: return "u16"; case sty_u32: return "u32"; case sty_u64: return "u64";
    case sty_c8: return "c8"; case sty_c16: return "c16"; case sty_c32: return "c32";
    case sty_f32: return "f32"; case sty_f64: return "f64";
    case sty_rawPtr: return "*";
    case sty_a: return "a";
    case sty_bit: return "u1";
    case sty_bool: return "bool";
    default: return "(unhandled type - you shouldn't see this)";
  }
}
#if FFI_CHECKS
  static NOINLINE NORETURN void ffi_improperValue(u8 sty, B val) {
    char* name = sty_name(sty);
    bool wantsChr = sty_char(sty);
    if (wantsChr) {
      if (!isC32(val)) goto wrongType;
      u32 v = o2cG(val);
      thrF("FFI: Improper value for %S: @+%ui", name, v);
    } else {
      if (!q_f64(val)) goto wrongType;
      thrF("FFI: Improper value for %S: %R", name, val);
    }
    wrongType: thrF("FFI: Improper value for %S: %S", name, genericDesc(val));
  }
  static NOINLINE u64 ffi_numRange(B x, u8 sty, i64 min, i64 max) { // doesn't consume; assumes non-array has already been checked for; if max==0, checks for floats; for max!=0, returns an upper bound max element value
    usz ia = IA(x);
    if (ia==0) return 0;
    u8 xe = TI(x,elType);
    bool incremented = false;
    switch (xe) {
      i64 emax;
      case el_bit: return 1; // bitarrs should be in-range for all numbers; given the max==0 special-case, can't even represent cases that aren't
      case el_i8:  emax=I8_MAX;  goto echk;
      case el_i16: emax=I16_MAX; goto echk;
      case el_i32: emax=I32_MAX; goto echk;
      echk:;
        i64 emin = -emax-1;
        if (emin>=min && emax<=max) return emax;
        break;
      case el_f64: break;
      default:
        x = squeeze_numTry(incG(x), &xe, SQ_NUM);
        incremented = true;
        if (!elNum(xe)) goto hasBadElt;
    }
    
    
    i64 buf[2];
    if (max==0) { buf[1]=0; goto done; } // only checking for consisting of floats
    if (!getRange_fns[xe](tyany_ptr(x), buf, ia) || buf[0]<min || buf[1]>max) goto hasBadElt;
    
    done:
    if (incremented) decG(x);
    return buf[1];
    
    hasBadElt:; SGetU(x);
    PLAINLOOP for (ux i = 0; i < ia; i++) {
      B c = GetU(x,i); i64 v;
      if (!q_f64(c)) ffi_improperValue(sty, c);
      if (max!=0 && (!q_i64o(&v, c) || v<min || v>max)) ffi_improperValue(sty, c);
    }
    fatal("expected ffi_numRange to error");
  }
  
  static bool elChrOk(B x, u64 max, u8 xe) {
    if (xe==el_c8) return true;
    if (xe==el_c16 && max>=U16_MAX) return true;
    if (xe==el_c32 && max>=U32_MAX) return true;
    return false;
  }
  static NOINLINE void ffi_chrRange(B x, u8 sty, i64 umax) {
    if (IA(x)==0) return;
    u8 xe = TI(x,elType);
    if (elChrOk(x, umax, xe)) return;
    B sq = squeeze_chrTry(incG(x), &xe, SQ_BEST);
    bool ok = elChrOk(sq, umax, xe);
    decG(sq);
    if (ok) return;
    PLAINLOOP for (ux ia=IA(x), i=0; i < ia; i++) {
      B c = IGetU(x,i);
      if (!isC32(c) || o2cG(c) > umax) ffi_improperValue(sty, c);
    }
    fatal("expected ffi_chrRange to error");
  }
#else
  static u64 ffi_numRange(B x, u8 sty, i64 min, i64 max) { return U64_MAX; } // TODO could return an estimate based on eltype
  static void ffi_chrRange(B x, u8 sty, i64 umax) { }
  static NORETURN void ffi_improperValue(u8 sty, B val) { UD; }
#endif

static NOINLINE void trackTemp(ScratchMem sm, B v) {
  B* a = &sm.objPtr->tmpBufs;
  if (q_z(*a)) {
    HArr_p t = m_harrUv(4);
    t.c->ia = 1;
    t.a[0] = v;
    NOGC_E;
    *a = t.b;
  } else {
    *a = vec_addN(*a, v);
  }
}

static uintptr_t foreignPtrFromBQN(B x, B expElt, bool couldBeArr) {
  B h = ptrobj_checkget(x, couldBeArr);
  #if FFI_CHECKS
    if (!ty_compat(ptrh_elt(h), expElt)) thrF("FFI: Pointer object type isn't compatible with argument type");
  #endif
  return ptrh_ptr(h);
}

#define CPY_UNSIGNED(REL, DIRECT, WIDEN, WEL, NARROW) \
  /*if (TI(x,elType)<=el_##REL) return taga(DIRECT(x));*/ \
  B t = WIDEN(x); WEL* tp = WEL##any_ptr(t);      \
  usz ia = IA(x);                                 \
  REL* rp; B r = m_##REL##arrc(&rp, x);           \
  NARROW; decG(t); return a(r);

NOINLINE void cpy_f64_to_f32(f32* dst, f64* src, ux ia) { vfor (ux i=0; i<ia; i++) dst[i] = src[i]; }
NOINLINE void cpy_f64_to_u32(u32* dst, f64* src, ux ia) { vfor (ux i=0; i<ia; i++) dst[i] = src[i]; }
NOINLINE void cpy_f64_to_u64(u64* dst, f64* src, ux ia) { vfor (ux i=0; i<ia; i++) dst[i] = src[i]; }
NOINLINE void cpy_f64_to_i64(i64* dst, f64* src, ux ia) { vfor (ux i=0; i<ia; i++) dst[i] = src[i]; }
static f64 f64_from_ffi(f64 x) { return x==x? x : 0.0/0.0; } // don't allow SNaNs into CBQN heap
static f64 f32_from_ffi(f32 x) { return x==x? x : 0.0/0.0; } // TODO might not be necessary?
NOINLINE void f64s_from_ffi(f64* xp, ux ia) { vfor (ux i = 0; i < ia; i++) xp[i] = f64_from_ffi(xp[i]); }

// copy elements of x to array of unsigned integers (output being a signed intered array as a "container"); consumes argument
// undefined behavior if x contains a number outside the respective unsigned range (incl. any negative numbers)
// TODO these can all do xe≤el_i32 input better via doing a wider range of COPY_TO_FROM source el_c*
NOINLINE Arr* cpyU32Bits(B x) { CPY_UNSIGNED(i32, cpyI32Arr, toF64Any, f64, cpy_f64_to_u32((u32*)rp, tp, ia)) }
NOINLINE Arr* cpyU16Bits(B x) { CPY_UNSIGNED(i16, cpyI16Arr, toI32Any, i32, if (ia) COPY_TO_FROM(rp, el_c16, tp, el_c32, ia)) }
NOINLINE Arr* cpyU8Bits(B x)  { CPY_UNSIGNED(i8,  cpyI8Arr,  toI16Any, i16, if (ia) COPY_TO_FROM(rp, el_c8,  tp, el_c16, ia)) }

#define CONV_TO_STY(PTRK) \
  ConvType conv = ffiPrimConv(el); \
  PrimType sty = ffiPrimTy(el); \
  if (conv != sty_void) { \
    i32 d = sty_lb(sty) - sty_lb(conv); \
    if (FFI_CHECKS && d > 0) { \
      ux mul = (ux)1 << d; \
      if ((IA(x) & (mul-1)) != 0) thrF("FFI: Array provided for %S%S:%S needs to have a length that's a multiple of %z, but array had shape %H", PTRK, sty_name(sty), sty_name(conv), mul, x); \
    } \
    sty = conv; \
  }

static NOINLINE void foreignMemWriteHArray(void* mem, B el, B x, ux ia, ux stride) {
  SGetU(x);
  for (ux i = 0; i < ia; i++) {
    foreignMemFromBQN(SM_NONE, mem, el, GetU(x,i));
    mem = stride + (u8*)mem;
  }
}
static NOINLINE void foreignMemWriteArray(void* mem, B el, B x) {
  assert(isArr(x));
  ux ia = IA(x);
  if (!isFFIPrim(el)) { generic: foreignMemWriteHArray(mem, el, x, ia, foreignSize(el)); return; }
  
  ux bytes;
  CONV_TO_STY("");
  switch (sty) {
    case sty_bool:ffi_numRange(x, sty, 0, 1); goto as_i8;
    case sty_bit: ffi_numRange(x, sty, 0, 1);                     x = toBitAny(incG(x)); bytes = ia>>3; goto memcpyTy;
    case sty_i8:  ffi_numRange(x, sty,  I8_MIN,  I8_MAX);  as_i8: x = toI8Any (incG(x)); bytes = ia<<0; goto memcpyTy;
    case sty_i16: ffi_numRange(x, sty, I16_MIN, I16_MAX); as_i16: x = toI16Any(incG(x)); bytes = ia<<1; goto memcpyTy;
    case sty_i32: ffi_numRange(x, sty, I32_MIN, I32_MAX); as_i32: x = toI32Any(incG(x)); bytes = ia<<2; goto memcpyTy;
    case sty_f64: ffi_numRange(x, sty, 0, 0);                     x = toF64Any(incG(x)); bytes = ia<<3; goto memcpyTy;
    case sty_c8:  ffi_chrRange(x, sty,           U8_MAX);         x = toC8Any (incG(x)); bytes = ia<<0; goto memcpyTy;
    case sty_c16: ffi_chrRange(x, sty,          U16_MAX);         x = toC16Any(incG(x)); bytes = ia<<1; goto memcpyTy;
    case sty_c32: ffi_chrRange(x, sty,          U32_MAX);         x = toC32Any(incG(x)); bytes = ia<<2; goto memcpyTy;
    case sty_u8:  if (ffi_numRange(x, sty, 0,  U8_MAX)<= I8_MAX) goto as_i8;  x = toI16Any(incG(x)); { i16* xp=i16any_ptr(x); vfor (usz i=0; i<ia; i++) ((u8* )mem)[i] = xp[i]; } goto dec_ret; // TODO for xe≤el_i32 these could be done via bitarr narrow, or character COPY_TO_FROM then memcpy; f64 input still needs either 3 passes or still the special loop though
    case sty_u16: if (ffi_numRange(x, sty, 0, U16_MAX)<=I16_MAX) goto as_i16; x = toI32Any(incG(x)); { i32* xp=i32any_ptr(x); vfor (usz i=0; i<ia; i++) ((u16*)mem)[i] = xp[i]; } goto dec_ret;
    case sty_u32: if (ffi_numRange(x, sty, 0, U32_MAX)<=I32_MAX) goto as_i32; x = toF64Any(incG(x)); cpy_f64_to_u32(mem, f64any_ptr(x), ia); goto dec_ret;
    case sty_u64:     ffi_numRange(x, sty, 0,           (1LL<<53)-1);         x = toF64Any(incG(x)); cpy_f64_to_u64(mem, f64any_ptr(x), ia); goto dec_ret;
    case sty_i64:     ffi_numRange(x, sty, 1-(1LL<<53), (1LL<<53)-1);         x = toF64Any(incG(x)); cpy_f64_to_i64(mem, f64any_ptr(x), ia); goto dec_ret;
    case sty_f32:     ffi_numRange(x, sty, 0, 0);                             x = toF64Any(incG(x)); cpy_f64_to_f32(mem, f64any_ptr(x), ia); goto dec_ret;
    default: goto generic;
  }
  if (0) { memcpyTy:;
    memcpy(mem, tyany_ptr(x), bytes);
    dec_ret: decG(x); return;
  }
}

static uintptr_t foreignMemCtyPtrFromBQN(ScratchMem sm, FFICompoundType* ct, B x, PtrKind ptrKind) { // for ptrKind!=ptrk_in, always returns a new TyArr (maybe t_unkArr for ⥊T or non-native sty), with proper shape
  B el = ct->a[0].o;
  if (!isArr(x)) {
    if (ptrKind == ptrk_out) {
      u64 n;
      if (!q_u64o(&n, x)) thrF("FFI: A non-negative integer must be passed to ⥊T, but got %S", isF64(x)? "invalid number" : genericDesc(x));
      TyArr* a = m_arrUnchecked(fsizeof(TyArr,a,u8,n*foreignSize(el)), t_unkArr, n); // need to use a special type to avoid debug builds complaining about wrong initialization state
      arr_shVec((Arr*) a);
      trackTemp(sm, taga(a));
      return PTR_TO_U64(a->a);
    } else if (ptrKind == ptrk_mut && ct->ptr.read) {
      *AT_SM(B, ct->a[0].ptr.ptrObjRefOffset) = inc(x); // even though ScratchMem won't keep the object alive, the argument will (and direct array read accesses already require the arg to stay alive)
    }
    return foreignPtrFromBQN(x, el, true);
  }
  if (ptrKind == ptrk_out) thrM("FFI: A non-negative integer must be passed to ⥊T, but got array");
  
  if (isFFIPrim(el)) {
    CONV_TO_STY(ptrk_names[ptrKind]);
    
    if (ptrKind == ptrk_in) { // fast path for giving a pointer directly to CBQNs heap
      u8 xe = TI(x,elType);
      if (xe == styToEltype(sty)) directRet: return PTR_TO_U64(tyany_ptr(x)); // primary element type matches input eltype: fine to directly pass
      if (sty>=sty_u8 && sty<=sty_u32 && xe == sty-sty_u8+el_i8) { // *u8 can be passed el_i8, & co, via only checking for non-negativity
        ffi_numRange(x, sty, 0, U32_MAX); // max doesn't need to be checked, so just pick a value that works for all cases
        goto directRet;
      }
    } else {
      assert(ptrKind == ptrk_mut);
      *AT_SM(B, ct->a[0].ptr.ptrObjRefOffset) = bi_z;
    }
    
    incG(x); Arr* a;
    switch (sty) { default: UD;
      case sty_void: fatal("shouldn't write void");
      case sty_a: case sty_rawPtr: incByG(x, -1); goto generic;
      case sty_bit: ffi_numRange(x, sty, 0, 1);             a = cpyBitArr(x); break;
      case sty_bool:ffi_numRange(x, sty, 0, 1);             a = cpyI8Arr (x); break;
      case sty_i8:  ffi_numRange(x, sty,  I8_MIN,  I8_MAX); a = cpyI8Arr (x); break;
      case sty_i16: ffi_numRange(x, sty, I16_MIN, I16_MAX); a = cpyI16Arr(x); break;
      case sty_i32: ffi_numRange(x, sty, I32_MIN, I32_MAX); a = cpyI32Arr(x); break;
      case sty_c8:  ffi_chrRange(x, sty,           U8_MAX); a = cpyC8Arr (x); break;
      case sty_c16: ffi_chrRange(x, sty,          U16_MAX); a = cpyC16Arr(x); break;
      case sty_c32: ffi_chrRange(x, sty,          U32_MAX); a = cpyC32Arr(x); break;
      case sty_f64: ffi_numRange(x, sty, 0, 0);             a = cpyF64Arr(x); break;
      case sty_u8:  if (ffi_numRange(x, sty, 0,  U8_MAX) <=  I8_MAX) a = cpyI8Arr (x); else a = cpyU8Bits (x); break;
      case sty_u16: if (ffi_numRange(x, sty, 0, U16_MAX) <= I16_MAX) a = cpyI16Arr(x); else a = cpyU16Bits(x); break;
      case sty_u32: if (ffi_numRange(x, sty, 0, U32_MAX) <= I32_MAX) a = cpyI32Arr(x); else a = cpyU32Bits(x); break;
      case sty_u64:     ffi_numRange(x, sty, 0,           (1LL<<53)-1); { a = cpyF64Arr(x); void* ptr=f64arrv_ptr((TyArr*)a); cpy_f64_to_u64(ptr, ptr, PIA(a)); } break;
      case sty_i64:     ffi_numRange(x, sty, 1-(1LL<<53), (1LL<<53)-1); { a = cpyF64Arr(x); void* ptr=f64arrv_ptr((TyArr*)a); cpy_f64_to_i64(ptr, ptr, PIA(a)); } break;
      case sty_f32:     ffi_numRange(x, sty, 0, 0); { B t=toF64Any(x); i32* rp; B r=m_i32arrc(&rp,x); cpy_f64_to_f32((f32*)rp, f64any_ptr(t), IA(t)); decG(t); a = a(r); } break;
    }
    trackTemp(sm, taga(a));
    return PTR_TO_U64(tyanyv_ptr(a));
  }
  
  generic:;
  ScratchMem sm2 = sm;
  if (ptrKind == ptrk_mut) {
    *AT_SM(B, ct->a[0].ptr.ptrObjRefOffset) = bi_z;
    sm2 = SM_NONE;
  } else assert(ptrKind == ptrk_in);
  
  usz ia = IA(x);
  ux elsz = foreignSize(el);
  TyArr* a = mm_alloc(fsizeof(TyArr,a,u8,ia*elsz), t_unkArr);
  a->ia = ia;
  arr_shCopyUnchecked((Arr*)a, x);
  void* start = a->a;
  u8* curr = start; SGetU(x);
  for (ux i = 0; i < ia; i++) {
    foreignMemFromBQN(sm2, curr, el, GetU(x,i));
    curr+= elsz;
  }
  trackTemp(sm, taga(a));
  return PTR_TO_U64(start);
}

static NOINLINE B fmtFFISty(B ty) {
  assert(isFFIPrim(ty));
  u8 conv=ffiPrimConv(ty), prim=ffiPrimTy(ty);
  if (conv==sty_void) return m_c8vec_0(sty_name(prim));
  return make_fmt("%S:%S", sty_name(prim), sty_name(conv));
}
static NOINLINE B fmtFFITy0(B ty) {
  if (isFFIPrim(ty)) return fmtFFISty(ty);
  FFICompoundType* ct = c(FFICompoundType, ty);
  char* name = "T";
  switch (ct->cty) {
    case cty_tlarr: case cty_starr: case cty_tlarrChecked: name = "[n]T"; break;
    case cty_struct: name = "{...}"; break;
    case cty_ptr: case cty_ptrChecked: name = "*T"; break; // doesn't handle &T / ⥊T currently
  }
  return m_c8vec_0(name);
}
static NOINLINE B fmtFFICty1(FFICompoundType* ct) {
  switch (ct->cty) {
    default: return m_c8vec_0("(unknown)");
    B arrElt;
    ux eltCount;
    case cty_tlarr: case cty_tlarrChecked: {
      arrElt = ct->a[0].o;
      eltCount = arrEltsFromLen(ct->ptr.inpLen, ct);
      goto fmtArr;
    }
    case cty_starr: {
      arrElt = ct->a[0].o;
      eltCount = ct->starr.arrElts;
      fmtArr: return make_fmt("[%z]%R", eltCount, fmtFFITy0(arrElt));
    }
    case cty_ptr: case cty_ptrChecked: return make_fmt("%S%R", ptrk_names[ct->ptr.kind], fmtFFITy0(ct->a[0].o));
  }
}

static void checkArrSize(B x, FFICompoundType* ct, ux size) {
  if (!FFI_CHECKS) return;
  if (!isArr(x)) thrF("FFI: Expected array to be passed to %R, but got %S", fmtFFICty1(ct), genericDesc(x));
  if (IA(x) != size) thrF("FFI: Expected %z-element array to be passed to %R, but got array with shape %H", size, fmtFFICty1(ct), x);
}

#if FFI_CHECKS
static NOINLINE void foreignMemCtyFromBQN(ScratchMem sm, void* mem, B type, B x);
static NOINLINE void foreignMemCtyCheckedPtrFromBQN(ScratchMem sm, void* mem, FFICompoundType* ct, B x) {
  u8 cty0;
  switch (ct->cty) { default: fatal("bad checked cty");
    case cty_ptrChecked: cty0 = cty_ptr; break;
    case cty_tlarrChecked: cty0 = cty_tlarr; break;
  }
  FFICompoundType* tmp = m_ffiCompound(1, cty0, FFI_TYPE_VOID, 0);
  tmp->ptr.inpLen = ct->ptr.inpLen;
  tmp->ptr.kind = ct->ptr.kind;
  tmp->ptr.read = ct->ptr.read;
  tmp->a[0].ptr.ptrObjRefOffset = ct->a[0].ptr.ptrObjRefOffset;
  tmp->foreign = ct->foreign;
  tmp->a[0].o = inc(ct->a[0].o);
  NOGC_E;
  void* nmem;
  foreignMemCtyFromBQN(sm, &nmem, tag(tmp, OBJ_TAG), x);
  ptr_dec(tmp);
  if (!SM_IS_NONE) { // pointer objects or something
    B* place = AT_SM(B, ct->a[0].ptr.ptrObjRefOffset);
    if (!isNsp(x)) {
      B el = ct->a[0].o;
      ux size, elsz = foreignSize(el);
      if (!isArr(x)) {
        size = elsz * o2u64(x);
      } else {
        size = IA(x);
        if (isFFIPrim(el) && ffiPrimConv(el)!=sty_void) size = size << sty_lb(ffiPrimConv(el)) >> 3;
        else size = size * elsz;
      }
      B ptrObjRef = ct->ptr.kind==ptrk_mut? *place : bi_z;
      *place = checked_allocBlock(&nmem, size, ct->ptr.kind != ptrk_in, ptrObjRef);
    } else {
      *place = checked_wrap(ct->ptr.kind == ptrk_mut? *place : bi_z);
    }
    trackTemp(sm, *place);
  }
  *(void**)mem = nmem;
  return;
}
#endif
static NOINLINE void foreignMemCtyFromBQN(ScratchMem sm, void* mem, B type, B x) {
  FFICompoundType* ct = c(FFICompoundType, type);
  switch (ct->cty) { default: UD;
    case cty_struct: {
      if (FFI_CHECKS && !isArr(x)) thrF("FFI: Expected array to be passed to struct, but got %S", genericDesc(x));
      ux n = ct->ia;
      if (FFI_CHECKS && n != IA(x)) thrF("FFI: Expected %z-element array to be passed to struct, but got array with shape %H", n, x);
      SGetU(x);
      for (ux i = 0; i < n; i++) {
        B c = GetU(x,i);
        foreignMemFromBQN(sm, (u8*)mem + ct->a[i].st.fieldOffset, ct->a[i].o, c);
      }
      return;
    }
    case cty_starr: { // this needs to be a proper load into variable mem
      checkArrSize(x, ct, ct->starr.inpLen);
      foreignMemWriteArray(mem, ct->a[0].o, x);
      return;
    }
    case cty_tlarr: {
      checkArrSize(x, ct, ct->ptr.inpLen);
      goto fromPtr;
    }
    case cty_ptr: {
      if (FFI_CHECKS && SM_IS_NONE && isArr(x)) thrF("FFI: Cannot pass an array to %R in this context, must use pointer objects", fmtFFICty1(ct));
      fromPtr:;
      uintptr_t val = foreignMemCtyPtrFromBQN(sm, ct, x, ct->ptr.kind);
      *(void**)mem = PTR_FROM_INT(void, val);
      return;
    }
#if FFI_CHECKS
    case cty_tlarrChecked: case cty_ptrChecked: {
      return foreignMemCtyCheckedPtrFromBQN(sm, mem, ct, x);
    }
#endif
  }
}
FORCE_INLINE u64 foreignPrimFromBQNImpl(ScratchMem sm, void* mem, PrimType sty, B x, bool retGPR) {
  #define VAL(T, V) \
  if (retGPR) { return (u64)(T)(V); } \
  else { *(T*)mem = (T)(V); return 0; }
  f64 f = x.f;
  switch(sty) {
    default: case sty_bit: case sty_c8: case sty_c16: case sty_c32: UD;
    case sty_void: fatal("shouldn't write void");
    case sty_a: VAL(BQNV, bv_mk(inc(x)));
    case sty_rawPtr: {
      uintptr_t ptr = foreignPtrFromBQN(x, FFIPRIM_VOID, 0);
      if (retGPR) return ptr;
      *(void**)mem = PTR_FROM_INT(void, ptr);
      return 0;
    }
    case sty_bool:if(!q_fbit(f)) goto improper_sty; VAL(bool,f);
    case sty_u8:  if(!q_fu8 (f)) goto improper_sty; VAL( u8, f);
    case sty_i8:  if(!q_fi8 (f)) goto improper_sty; VAL( i8, f);
    case sty_u16: if(!q_fu16(f)) goto improper_sty; VAL(u16, f);
    case sty_i16: if(!q_fi16(f)) goto improper_sty; VAL(i16, f);
    case sty_u32: if(!q_fu32(f)) goto improper_sty; VAL(u32, f);
    case sty_i32: if(!q_fi32(f)) goto improper_sty; VAL(i32, f);
    case sty_u64: { u64 i; if(!q_fu64o(&i,f) || i          >=(1ULL<<53)) goto improper_sty; VAL(u64,i); }
    case sty_i64: { i64 i; if(!q_fi64o(&i,f) || IABS(u64,i)>=(1ULL<<53)) goto improper_sty; VAL(i64,i); }
    case sty_f32: assert(!retGPR); if(!q_f64(x)) goto improper_sty; *(f32*)mem = f; return 0;
    case sty_f64: assert(!retGPR); if(!q_f64(x)) goto improper_sty; *(f64*)mem = f; return 0;
    improper_sty: ffi_improperValue(sty, x);
  }
}

static char* const i64_bad_msg = "FFI: i64 result absolute value ≥ 2⋆53";
static char* const u64_bad_msg = "FFI: u64 result ≥ 2⋆53";
static B foreignPrimToBQN(void* mem, PrimType primType) {
  switch(primType) {
    default: case sty_bit: case sty_c8: case sty_c16: case sty_c32: UD;
    case sty_void: return m_c32(0);
    case sty_rawPtr: return m_ptrobj_s(PTR_TO_U64(*(void**)mem), FFIPRIM_VOID);
    case sty_bool: return m_i32(*(bool*)mem);
    case sty_a:   return bv_to(*(BQNV*)mem);
    case sty_i8:  return m_i32(*( i8*)mem);  case sty_u8:  return m_i32(*( u8*)mem);
    case sty_i16: return m_i32(*(i16*)mem);  case sty_u16: return m_i32(*(u16*)mem);
    case sty_i32: return m_i32(*(i32*)mem);  case sty_u32: return m_f64(*(u32*)mem);
    case sty_i64: { i64 v = *(i64*)mem; if (IABS(u64,v)>=(1ULL<<53)) thrM(i64_bad_msg); return m_f64(v); }
    case sty_u64: { u64 v = *(u64*)mem; if (         v >=(1ULL<<53)) thrM(u64_bad_msg); return m_f64(v); }
    case sty_f32: return m_f64(f32_from_ffi(*(f32*) mem));
    case sty_f64: return m_f64(f64_from_ffi(*(f64*) mem));
  }
}
static B expectInt64Range(B x, usz ia, bool isSigned) {
  assert(ia>0);
  x = squeeze_numNewTy(el_f64,x); // TODO could instead do a manual squeeze after getting bounds
  if (elInt(TI(x,elType))) return x;
  assert(TI(x,elType)==el_f64);
  i64 bounds[2];
  if (!getRange_fns[el_f64](f64arr_ptr(x), bounds, ia)) bad: thrM(isSigned? i64_bad_msg : u64_bad_msg);
  if (isSigned) { if (bounds[0]<=-(1LL<<53) || bounds[1]>=(1LL<<53)) goto bad; }
  else          { if (bounds[0]<0           || bounds[1]>=(1LL<<53)) goto bad; }
  return x;
}
static B foreignHListToBQN(ux ia, void* mem, B el, ux stride) {
  if (!stride) stride = foreignSize(el);
  u8* curr = mem;
  HArr_p r = m_harr0v(ia);
  for (ux i = 0; i < ia; i++) {
    r.a[i] = foreignToBQN(curr, SM_NONE, el);
    curr+= stride;
  }
  return r.b;
}
static B foreignPrimListToBQN(ux ia, void* mem, u8 sty, bool fromCBQNMem) { // always returns new rank 1 array
  if (HEURISTIC(false) && ia>0 && !(sty_char(sty) || sty==sty_bit)) return squeeze_any(foreignHListToBQN(ia, mem, m_ffiPrim(sty, sty_void), 0));
  switch (sty) {
    default: UD;
    case sty_bit:
    case sty_c8: case sty_c16: case sty_c32:
    case sty_i8: case sty_i16: case sty_i32: case sty_f64: {
      u8 l = sty_lb(sty);
      B r; void* rp = m_tyarrlbv(&r, l, ia, el2t(styToEltype(sty)));
      memcpy(rp, mem, ia<<l>>3);
      if (sty==sty_f64) f64s_from_ffi(rp, ia);
      return r;
    }
    case sty_void: fatal("shouldn't read void");
    case sty_rawPtr: case sty_a: return foreignHListToBQN(ia, mem, m_ffiPrim(sty, sty_void), 0);
    #define IA0MK if (ia==0) goto empty_ivec; // squeeze may return a reused array for ia==0, so handle that case manually
    case sty_u8: {IA0MK;i16* rp; B r=m_i16arrv(&rp, ia); vfor (usz i=0; i<ia; i++) rp[i]=((u8 *)mem)[i]; return squeeze_numNewTy(el_i16,r); }
    case sty_u16:{IA0MK;i32* rp; B r=m_i32arrv(&rp, ia); vfor (usz i=0; i<ia; i++) rp[i]=((u16*)mem)[i]; return squeeze_numNewTy(el_i32,r); }
    case sty_u32:{IA0MK;f64* rp; B r=m_f64arrv(&rp, ia); vfor (usz i=0; i<ia; i++) rp[i]=((u32*)mem)[i]; return squeeze_numNewTy(el_f64,r); }
    case sty_i64:{IA0MK;f64* rp; B r=m_f64arrv(&rp, ia); vfor (usz i=0; i<ia; i++) rp[i]=((i64*)mem)[i]; return expectInt64Range(r, ia, true); } // TODO could calculate min/max in the loop, and squeeze from that
    case sty_u64:{IA0MK;f64* rp; B r=m_f64arrv(&rp, ia); vfor (usz i=0; i<ia; i++) rp[i]=((u64*)mem)[i]; return expectInt64Range(r, ia, false); }
    case sty_f32: {     f64* rp; B r=m_f64arrv(&rp, ia); vfor (usz i=0; i<ia; i++) rp[i]=f32_from_ffi(((f32*)mem)[i]); return r; }
    #undef IA0MK
    case sty_bool: {
      u64* rp; B r = m_bitarrv(&rp, ia);
      if (ia==0) {
        // don't need to do anything
      } else if (fromCBQNMem) {
        COPY_TO_FROM(rp, el_bit, mem, el_i8, ia);
      } else {
        TALLOC(u8, mem2, ia);
        memcpy(mem2, mem, ia);
        COPY_TO_FROM(rp, el_bit, mem2, el_i8, ia);
        TFREE(mem2);
      }
      return r;
    }
  }
  empty_ivec: return taga(arr_shVec(allZeroes(0)));
}
static NOINLINE B foreignConvToBQN(void* mem, PrimType prim, ConvType conv) {
  u8 tylb = sty_lb(prim);
  ux sz = (ux)1 << (tylb-3);
  TyArr* a = m_arr(offsetof(TyArr,a) + sz, el2t(styToEltype(conv)), 1 << (tylb - sty_lb(conv)));
  memcpy(a->a, mem, sz);
  return taga(arr_shVec((Arr*) a));
}
B join_c1(B, B);
static NOINLINE B foreignCtyToBQN(void* mem, ScratchMem sm, FFICompoundType* ct) {
  B extraRetRet;
  switch (ct->cty) {
    default: UD;
    case cty_retList: {
      usz n = ct->ia;
      HArr_p r = m_harr0v(n);
      extraRetRet = r.b;
      for (ux i = 0; i < n; i++) {
        r.a[i] = foreignToBQN(mem, sm, ct->a[i].o); // mem will only be used by the main return value, mutated pointers find their memory via offsets in ScratchMem
      }
      goto freeTmpBufs;
    }
    case cty_ret1: {
      extraRetRet = foreignToBQN(mem, sm, ct->a[0].o);
      freeTmpBufs:;
      B tmpBufs = sm.objPtr->tmpBufs;
      if (!q_z(tmpBufs)) { decG(tmpBufs); } NOGC_S;
      return extraRetRet;
    }
    
    void* data; PtrKind ptrk;
    #if FFI_CHECKS
    case cty_ptrChecked: {
      ptrk = ct->ptr.kind;
      if (ptrk == ptrk_in) goto ptr_result; // result value
      B* place = AT_SM(B, ct->a[0].ptr.ptrObjRefOffset);
      data = checked_readBlock(place, ct);
      if (data == NULL) goto ptr_plain;
      else goto ptr_data;
    }
    #endif
    
    case cty_ptr: {
      ptrk = ct->ptr.kind;
      if (ptrk == ptrk_in) { ptr_result: MAYBE_UNUSED; // result value
        data = *(void**)mem;
        return m_ptrobj_s(PTR_TO_U64(data), inc(ct->a[0].o));
      } else { // "&T" or "⥊T"
        ptr_plain: MAYBE_UNUSED;
        data = *AT_SM(void*, ct->a[0].ptr.dataPtrOffset);
        ptr_data: MAYBE_UNUSED;
        if (ptrk == ptrk_mut) {
          B ptrObj = *AT_SM(B, ct->a[0].ptr.ptrObjRefOffset);
          if (!q_z(ptrObj)) return ptrObj;
        }
        TyArr* arr = RFLD(data,TyArr,a);
        debug_assert(IS_DIRECT_TYARR(PTY(arr)) || PTY(arr)==t_unkArr);
        B el = ct->a[0].o;
        if (!isFFIPrim(el)) return foreignHListToBQN(PIA(arr), data, el, 0);
        
        ConvType conv = ffiPrimConv(el);
        PrimType prim = ffiPrimTy(el);
        if (conv != sty_void) { fromConv:; // can just use the original buffer allocation directly as the output array
          if (PTY(arr) == t_unkArr) {
            arr->type = el2t(styToEltype(conv)); // came from "⥊T"
            arr->ia = arr->ia << sty_lb(prim) >> sty_lb(conv);
          }
          ptr_inc(arr);
          if (conv == sty_f64) f64s_from_ffi(tyarrv_ptr(arr), PIA(arr));
          return taga(arr);
        }
        switch (prim) {
          case sty_i8: case sty_i16: case sty_i32: case sty_f64:
            conv = prim; goto fromConv;
          default:;
            B r = foreignPrimListToBQN(PIA(arr), data, prim, true);
            debug_assert(reusable(r));
            arr_shCopy(a(r), taga(arr));
            return r;
        }
      }
    }
    
    ux resLen;
    case cty_starr: {
      resLen = ct->starr.inpLen;
      readArr:;
      B el = ct->a[0].o;
      if (!isFFIPrim(el)) return foreignHListToBQN(resLen, mem, el, 0);
      u8 sty = ffiPrimTy(el);
      u8 conv = ffiPrimConv(el);
      if (conv != sty_void) sty = conv;
      return foreignPrimListToBQN(resLen, mem, sty, false);
    }
    case cty_ptrh: {
      resLen = sm.ptrObjMeta[0];
      ux inpStride = sm.ptrObjMeta[2];
      B el = ct->a[0].o;
      if (inpStride != foreignSize(el)) {
        ux eltCount = sm.ptrObjMeta[1];
        u8 conv = sty_void;
        if (isFFIPrim(el)) conv = ffiPrimConv(el);
        if (eltCount==0) return sty_char(conv)? emptyCVec() : sty_noFill(ffiPrimTy(el)) && conv==sty_void? emptyHVec() : emptyIVec();
        B r = foreignHListToBQN(eltCount, mem, el, inpStride);
        if (conv == sty_void) r = squeeze_any(r); // ensure correct fill
        else r = C1(join, r); // individual elements already have correct fill that ∾𝕩 propagates, and no-elements is handled before
        return r;
      }
      goto readArr;
    }
    
    case cty_struct: {
      ux n = ct->ia;
      HArr_p r = m_harr0v(n);
      for (ux i = 0; i < n; i++) {
        r.a[i] = foreignToBQN((u8*)mem + ct->a[i].st.fieldOffset, sm, ct->a[i].o);
      }
      return r.b;
    }
  }
}
static NOINLINE B foreignToBQN(void* mem, ScratchMem sm, B type) {
  if (!isFFIPrim(type)) return foreignCtyToBQN(mem, sm, c(FFICompoundType, type));
  ConvType conv = ffiPrimConv(type);
  PrimType prim = ffiPrimTy(type);
  return conv==sty_void? foreignPrimToBQN(mem, prim) : foreignConvToBQN(mem, prim, conv);
}



static NOINLINE void foreignConvMemFromBQN(void* mem, PrimType prim, ConvType conv, B x) {
  if (!isArr(x)) {
    if (prim == sty_rawPtr) {
      foreignMemFromBQN(SM_NONE, mem, m_ffiPrim(sty_rawPtr,sty_void), x);
      return;
    }
    if (!FFI_CHECKS) UD;
    thrF("FFI: Expected array to be passed to %S:%S", sty_name(prim), sty_name(conv));
  }
  u8 tylb = sty_lb(prim);
  u8 elt = styToEltype(conv);
  usz ia = IA(x);
  ux iaExp = 1 << (tylb-sty_lb(conv));
  if (FFI_CHECKS && ia != iaExp) thrF("FFI: Expected %i-element array to be passed to %S:%S, but got array with shape %H", iaExp, sty_name(prim), sty_name(conv), x);
  switch (elt) { default: UD;
    case el_bit: ffi_numRange(x, conv, 0, 1);             break;
    case el_i8:  ffi_numRange(x, conv,  I8_MIN,  I8_MAX); break;
    case el_i16: ffi_numRange(x, conv, I16_MIN, I16_MAX); break;
    case el_i32: ffi_numRange(x, conv, I32_MIN, I32_MAX); break;
    case el_c8:  ffi_chrRange(x, conv,           U8_MAX); break;
    case el_c16: ffi_chrRange(x, conv,          U16_MAX); break;
    case el_c32: ffi_chrRange(x, conv,          U32_MAX); break;
  }
  
  ux bytes = (ux)1 << (tylb-3);
  if (TI(x,elType)==elt) {
    memcpy(mem, tyany_ptr(x), bytes);
  } else {
    DirectArr a = toEltypeArr(incG(x), elt); // TODO try to avoid temporary allocation?
    memcpy(mem, a.data, bytes);
    decG(a.obj);
  }
}
static u64 foreignSysvGPRFromBQN(ScratchMem sm, B type, B x) {
  if (isFFIPrim(type)) {
    ConvType conv = ffiPrimConv(type);
    PrimType prim = ffiPrimTy(type);
    if (conv==sty_void) return foreignPrimFromBQNImpl(sm, NULL, prim, x, true);
    u64 r = 0;
    foreignConvMemFromBQN(&r, prim, conv, x);
    return r;
  }
  debug_assert(c(FFICompoundType,type)->cty==cty_tlarr || c(FFICompoundType,type)->cty==cty_ptr);
  void* mem;
  foreignMemCtyFromBQN(sm, &mem, type, x);
  return PTR_TO_U64(mem);
}
static NOINLINE void foreignMemFromBQN(ScratchMem sm, void* mem, B type, B x) {
  if (!isFFIPrim(type)) { foreignMemCtyFromBQN(sm, mem, type, x); return; }
  ConvType conv = ffiPrimConv(type);
  PrimType prim = ffiPrimTy(type);
  if (conv==sty_void) foreignPrimFromBQNImpl(sm, mem, prim, x, false);
  else foreignConvMemFromBQN(mem, prim, conv, x);
}

#if GPR6_SYSV64
#if __clang__
__attribute__((no_sanitize("function"))) // this is very much technically UB, but it's Fine™; only other option would be writing per-arch non-inline non-inlineable assembly of the whole function, which is just worse
#endif
static u64 gpr6_sysv64_call(void* sym, ux argCount, u64* gprs) {
  switch (argCount) { default: UD;
    case 0: return ((u64(*)(                       ))sym)(); break;
    case 1: return ((u64(*)(u64                    ))sym)(gprs[0]); break;
    case 2: return ((u64(*)(u64,u64                ))sym)(gprs[0],gprs[1]); break;
    case 3: return ((u64(*)(u64,u64,u64            ))sym)(gprs[0],gprs[1],gprs[2]); break;
    case 4: return ((u64(*)(u64,u64,u64,u64        ))sym)(gprs[0],gprs[1],gprs[2],gprs[3]); break;
    case 5: return ((u64(*)(u64,u64,u64,u64,u64    ))sym)(gprs[0],gprs[1],gprs[2],gprs[3],gprs[4]); break;
    case 6: return ((u64(*)(u64,u64,u64,u64,u64,u64))sym)(gprs[0],gprs[1],gprs[2],gprs[3],gprs[4],gprs[5]); break;
  }
}
#endif

static NOINLINE B ffiFn_core(FFIFn* f, B* wp, B* xp, B x) {
  ScratchMem sm = (ScratchMem){mm_alloc(fsizeof(ScratchMemAlloc,rest,u8,f->scratchMemSize), t_ffiScratchMem)};
  sm.objPtr->tmpBufs = bi_z;
  void* retPos = AT_SM(void, 0);
  void** argList = AT_SM(void*, f->argListOffset);
  ux argCount = f->argCount;
  FFIEnt* argData = f->argData;
  
  #if GPR6_SYSV64
  if (f->gprSysv) {
    u64* gprs = (u64*) argList; // same-length buffer, with same-width elements
    for (ux i = 0; i < argCount; i++) {
      FFIEnt e = argData[i];
      i32 srcPos = e.argData.srcPos;
      B xv = srcPos<0? wp[~srcPos] : xp[srcPos];
      gprs[i] = foreignSysvGPRFromBQN(sm, argData[i].o, xv);
    }
    *(u64*)retPos = gpr6_sysv64_call(f->sym, argCount, gprs);
    goto no_ffi_call;
  }
  #endif
  
  NOUNROLL for (ux i = 0; i < argCount; i++) {
    FFIEnt e = argData[i];
    void* argBuffer = AT_SM(void, e.argData.scratchMemOffset);
    i32 srcPos = e.argData.srcPos;
    B xv = srcPos<0? wp[~srcPos] : xp[srcPos];
    // printf("%zu@%u: from %d\n", i, e.argData.scratchMemOffset, srcPos);
    argList[i] = argBuffer;
    foreignMemFromBQN(sm, argBuffer, e.o, xv);
  }
  ffi_call(&f->cif, f->sym, retPos, argList);
  no_ffi_call: MAYBE_UNUSED;
  
  B r = foreignToBQN(retPos, sm, f->retObj);
  
  NOGC_E; // tmpBufs shall be freed by foreignToBQN, at which point it starts NOGC_S up to here
  mm_free((Value*) sm.objPtr);
  dec(x);
  return r;
}

static ffi_type* foreignType(B t) {
  if (!isFFIPrim(t)) return &c(FFICompoundType, t)->foreign;
  switch (ffiPrimTy(t)) {
    case sty_bit: case sty_c8: case sty_c16: case sty_c32: default: UD;
    case sty_bool: return &ffi_type_uint8;
    case sty_rawPtr: return &ffi_type_pointer;
    case sty_void: return &ffi_type_void;
    case sty_a: return &ffi_type_uint64;
    case sty_u8:  return &ffi_type_uint8;   case sty_i8:  return &ffi_type_sint8;
    case sty_u16: return &ffi_type_uint16;  case sty_i16: return &ffi_type_sint16;
    case sty_u32: return &ffi_type_uint32;  case sty_i32: return &ffi_type_sint32;
    case sty_u64: return &ffi_type_uint64;  case sty_i64: return &ffi_type_sint64;
    case sty_f32: return &ffi_type_float;
    case sty_f64: return &ffi_type_double;
  }
}
static ux foreignSize(B t) {
  return foreignType(t)->size;
}



#define FFI_TYPE_FLDS(OBJ) \
  FFICompoundType* t = (FFICompoundType*) x;   \
  FFIEnt* arr=t->a; usz ia=t->ia; \
  for (usz i = 0; i < ia; i++) OBJ(arr[i].o);

DEF_FREE(ffiType) {
  FFI_TYPE_FLDS(dec);
}
void ffiType_visit(Value* x) {
  FFI_TYPE_FLDS(mm_visit);
}
void ffiType_print(FILE* f, B x) {
  FFICompoundType* t = c(FFICompoundType,x);
  fprintf(f, "cty_%d⟨", t->cty);
  usz ia = t->ia;
  for (usz i=0; i<ia; i++) {
    if (i) fprintf(f, ", ");
    B e = t->a[i].o;
    if (isFFIPrim(e)) {
      fprintf(f, "%s:%s", sty_name(ffiPrimTy(e)), sty_name(ffiPrimConv(e)));
    } else fprintI(f, e);
  }
  fprintf(f, "⟩");
}

DEF_FREE(ffiScratchMem) {
  dec(((ScratchMemAlloc*)x)->tmpBufs);
}
void ffiScratchMem_visit(Value* x) {
  mm_visit(((ScratchMemAlloc*)x)->tmpBufs);
}
