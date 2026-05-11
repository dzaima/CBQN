#include "ffiCore.c"
#include "utils/nfns.h"
#include "core/ns.h"
#include "vm/vm.h"

STATIC_GLOBAL Body* ptrobj_ns;
DEFINE_NFN ptrReadDesc, ptrWriteDesc, ptrCastDesc, ptrAddDesc, ptrSubDesc, ptrFieldDesc, ptrReadListDesc, ptrWriteListDesc;

static B m_ptrobj(uintptr_t ptr, B elt, ux stride) { // consumes
  FFICompoundType* ptrh = m_ffiCompound(1, cty_ptrh, FFI_TYPE_VOID, 0);
  ptrh->a[0].o = elt;
  NOGC_E;
  ptrh->ptrh.stride = stride;
  ptrh->ptrh.ptr = ptr;
  
  B obj = tag(ptr_incBy(ptrh, 7), OBJ_TAG);
  return m_nns(ptrobj_ns, // when adding more fields, also increase the ptr_incBy!
    m_nfn(ptrReadDesc,  obj),
    m_nfn(ptrWriteDesc, obj),
    m_nfn(ptrCastDesc,  obj),
    m_nfn(ptrAddDesc,   obj),
    m_nfn(ptrSubDesc,   obj),
    m_nfn(ptrFieldDesc, obj),
    m_nfn(ptrReadListDesc, obj),
    m_nfn(ptrWriteListDesc, obj),
  );
}
static B m_ptrobj_s(uintptr_t ptr, B elt) { // consumes
  return m_ptrobj(ptr, elt, foreignSize(elt));
}

static uintptr_t ptrh_ptrOff(B h, B off, bool negate) { // doesn't consume h, off's refcounting doesn't matter
  if (ptrh_elt(h).u == FFIPRIM_VOID.u) thrM("Cannot offset an untyped pointer");
  uintptr_t ptr = ptrh_ptr(h);
  i64 el = o2i64(off);
  if (negate) el = -el;
  return ptr + el * (i64)ptrh_stride(h);
}
static char* const couldAlsoBes[] = { "", " or array", " or integer" };
static NOINLINE B ptrobj_checkget(B x, u8 couldAlsoBe) { // doesn't consume
  if (!isNsp(x)) thrF("Expected pointer object%S, got %S", couldAlsoBes[couldAlsoBe], genericDesc(x));
  if (c(NS,x)->desc != ptrobj_ns->nsDesc) thrF("Expected pointer object%S, got some other kind of namespace", couldAlsoBes[couldAlsoBe]);
  return nfn_objU(c(NS,x)->sc->vars[0]);
}
static bool ty_compat(B a, B b) {
  assert(isFFIPrim(a) || v(a)->type==t_ffiType);
  assert(isFFIPrim(b) || v(b)->type==t_ffiType);
  if (isFFIPrim(a)) {
    primA:;
    u8 at = ffiPrimTy(a);
    if (at == sty_void) return true;
    if (!isFFIPrim(b)) return at == sty_rawPtr && c(FFICompoundType, b)->cty == cty_ptr;
    u8 bt = ffiPrimTy(b);
    return bt==sty_void || at==bt;
  }
  if (isFFIPrim(b)) {
    B t = a; a = b; b = t;
    goto primA;
  }
  
  FFICompoundType* at = c(FFICompoundType, a);
  FFICompoundType* bt = c(FFICompoundType, b);
  if (at->cty != bt->cty) return false;
  switch (at->cty) { default: UD;
    case cty_tlarr: fatal("top-level arrays shouldn't ever be checked for compatibility");
    case cty_starr:
      if (arrEltsFromLen(at->starr.inpLen, at) != arrEltsFromLen(bt->starr.inpLen, bt)) return false;
      goto cmp1;
    case cty_ptr: cmp1: return ty_compat(at->a[0].o, bt->a[0].o);
    case cty_struct: {
      if (at->ia != bt->ia) return false;
      ux n = at->ia;
      for (ux i = 0; i < n; i++) {
        if (!ty_compat(at->a[i].o, bt->a[i].o)) return false;
      }
      return true;
    }
  }
}

static B ptrobjCast_c1(B t, B x) {
  B h = nfn_objU(t);
  vfyStr(x, "ptrObj.Cast", "𝕩");
  U32Span src = toC32Null(&x, true); B o;
  if (src.start != src.end) {
    ArgParseState st;
    st.pc.xp = &x;
    st.pc.xia = 1;
    st.allowMut = st.needFull = false;
    st.outermost = false;
    o = parseFFIType(&st, 2, src);
  } else {
    o = FFIPRIM_VOID;
  }
  decG(x);
  return m_ptrobj_s(ptrh_ptr(h), o);
}
static B ptrobjRead_c1(B t, B x) {
  B h = nfn_objU(t);
  return foreignToBQN((void*)ptrh_ptrOff(h, x, false), SM_NONE, ptrh_elt(h));
}
static B ptrobjWrite_c2(B t, B w, B x) {
  B h = nfn_objU(t);
  foreignMemFromBQN(SM_NONE, (void*)ptrh_ptrOff(h, w, false), ptrh_elt(h), x);
  dec(x);
  return m_i32(1);
}
static B ptrobjWrite_c1(B t, B x) { return ptrobjWrite_c2(t, m_f64(0), x); }
static B ptrobjAdd_c1(B t, B x) {
  B h = nfn_objU(t);
  return m_ptrobj(ptrh_ptrOff(h, x, false), inc(ptrh_elt(h)), ptrh_stride(h));
}
static B ptrobjSub_c1(B t, B x) {
  B h = nfn_objU(t);
  if (q_f64(x)) return m_ptrobj(ptrh_ptrOff(h, x, true), inc(ptrh_elt(h)), ptrh_stride(h));
  B h2 = ptrobj_checkget(x, 2);
  B t1 = ptrh_elt(h);
  B t2 = ptrh_elt(h2);
  if (t1.u==FFIPRIM_VOID.u || t2.u==FFIPRIM_VOID.u) thrM("(pointer).Sub ptr: Both pointers must be typed");
  ux stride = ptrh_stride(h);
  if (stride!=ptrh_stride(h2)) thrM("(pointer).Sub ptr: Arguments must have the same stride");
  if (!ty_compat(t1, t2)) thrM("(pointer).Sub ptr: Arguments must have compatible types");
  ptrdiff_t diff = ptrh_ptr(h) - ptrh_ptr(h2);
  ptrdiff_t eldiff = diff / (ptrdiff_t)stride;
  if (eldiff*stride != diff) thrM("(pointer).Sub ptr: Distance between pointers isn't an exact multiple of stride");
  decG(x);
  return m_f64(eldiff);
}
static B ptrobjField_c1(B t, B x) {
  B h = nfn_objU(t);
  
  u64 idx = o2u64(x);
  B ty = ptrh_elt(h);
  if (isFFIPrim(ty)) badElType: thrM("(pointer).Field 𝕩: Pointer type must be either array or struct");
  FFICompoundType* ct = c(FFICompoundType, ty);
  
  uintptr_t ptr = ptrh_ptr(h);
  B elNew;
  if (ct->cty == cty_struct) {
    u64 n = ct->ia;
    if (idx >= n) thrF("Cannot get field %ul of a struct with %ul fields", idx, n);
    FFIEnt* fldptr = &ct->a[idx];
    elNew = fldptr->o;
    ptr+= fldptr->st.fieldOffset;
  } else if (ct->cty == cty_starr) {
    u64 n = ct->starr.inpLen;
    if (idx >= n) thrF("Cannot get pointer to element %ul of an array with %ul elements", idx, n);
    elNew = ct->a[0].o;
    ptr+= foreignSize(elNew) * idx;
  } else goto badElType;
  
  return m_ptrobj(ptr, inc(elNew), ptrh_stride(h));
}

static B ptrobjReadList_c1(B t, B x) {
  B h = nfn_objU(t);
  u64 n = o2u64(x), nx=n;
  if (n >= USZ_MAX) thrOOM();
  B elt = ptrh_elt(h);
  
  if (isFFIPrim(elt) && ffiPrimConv(elt)!=sty_void) {
    nx = n << sty_lb(ffiPrimTy(elt)) >> sty_lb(ffiPrimConv(elt));
  }
  ux meta[3] = {nx, n, ptrh_stride(h)};
  return foreignCtyToBQN(PTR_FROM_INT(void, ptrh_ptr(h)), SM_PTROBJ_META(meta), ptrh_self(h));
}
B shape_c2(B, B, B);
static B ptrobjWriteList_c1(B t, B x) {
  if (!isArr(x) || RNK(x)!=1) thrM("(pointer).WriteList 𝕩: 𝕩 must be a list");
  B h = nfn_objU(t);
  B elt = ptrh_elt(h);
  if (elt.u == FFIPRIM_VOID.u) thrM("(pointer).WriteList 𝕩: pointer must not be untyped");
  ux elsz = foreignSize(elt);
  ux stride = ptrh_stride(h);
  void* mem = PTR_FROM_INT(void, ptrh_ptr(h));
  
  if (RARE(elsz != stride)) {
    u8 conv = sty_void;
    if (isFFIPrim(elt)) conv = ffiPrimConv(elt);
    ux ia = IA(x), elts;
    if (conv != sty_void) {
      ux shift = sty_lb(ffiPrimTy(elt)) - sty_lb(conv);
      elts = ia>>shift;
      if (elts<<shift != ia) goto badLength;
      DirectArr a = toEltypeArr(x, styToEltype(conv)); // help out the individual converts that will follow
      // TODO do a direct loop; elements are already in the correct form
      x = a.obj;
      f64* shp; B sh = m_f64arrv(&shp, 2);
      shp[0] = elts;
      shp[1] = 1<<shift;
      x = toCells(C2(shape, sh, x));
    } else {
      elts = ia;
    }
    foreignMemWriteHArray(mem, elt, x, elts, stride);
  } else {
    badLength:;
    foreignMemWriteArray(mem, elt, x);
  }
  
  decG(x);
  return m_i32(1);
}



static void ptrobj_init() {
  ptrobj_ns = m_nnsDesc("read","write","cast","add","sub","field","readlist","writelist"); // first field must be an nfn whose object is the ptrh (needed for ptrobj_checkget)
  ptrReadDesc  = registerNFn(m_c8vec_0("(pointer).Read"), ptrobjRead_c1, c2_bad);
  ptrWriteDesc = registerNFn(m_c8vec_0("(pointer).Write"), ptrobjWrite_c1, ptrobjWrite_c2);
  ptrCastDesc  = registerNFn(m_c8vec_0("(pointer).Cast"), ptrobjCast_c1, c2_bad);
  ptrAddDesc   = registerNFn(m_c8vec_0("(pointer).Add"), ptrobjAdd_c1, c2_bad);
  ptrSubDesc   = registerNFn(m_c8vec_0("(pointer).Sub"), ptrobjSub_c1, c2_bad);
  ptrFieldDesc = registerNFn(m_c8vec_0("(pointer).Field"), ptrobjField_c1, c2_bad);
  ptrReadListDesc  = registerNFn(m_c8vec_0("(pointer).ReadList"), ptrobjReadList_c1, c2_bad);
  ptrWriteListDesc = registerNFn(m_c8vec_0("(pointer).WriteList"), ptrobjWriteList_c1, c2_bad);
}
