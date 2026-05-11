#include "ffiCore.c"
#include "utils/file.h"
#include "builtins.h"
#include "utils/cstr.h"
#include <stdarg.h>

NOINLINE B ffiThrImpl(u32* currOff, ParseContext* pc, char* str, va_list a) {
  B r = do_fmt(emptyCVec(), str, a);
  if (currOff == NULL) currOff = pc->currOff;
  if (pc->xia == 1) pc->curr = 0;
  
  // B l0 = emptyCVec();
  // l0 = append_fmt(l0, "at 𝕨 •FFI ", pc->curr, pc->xia);
  B l0 = utf8Decode0(pc->xia==1? "at (pointer).Cast " : "at 𝕨 •FFI ");
  i64 caretPos = -1;
  for (ux i = 0; i < pc->xia; i++) {
    if (i!=0) l0 = vec_addN(l0, m_c32(U'‿'));
    if (i==1 || i==pc->curr) {
      l0 = vec_addN(l0, m_c32(U'"'));
      B s = pc->xp[i];
      if (TI(s,elType)!=el_c32) s = taga(toC32Arr(incG(s))); // leaks, whatever
      u32* sp = c32any_ptr(s);
      ux ia = IA(s);
      for (ux i = 0; i < ia; i++) {
        u32 cc = sp[i];
        if (sp+i == currOff) caretPos = IA(l0); // also catches end-of-string via null byte; kinda weird with it pointing to the ending quote of the string but whatever
        if (cc=='"') l0 = append_fmt(l0, "\"\"");
        else if (cc==0 && i==ia-1) { /* ignore null terminator */ }
        else if (cc<32) l0 = vec_addN(l0, m_c32(U'␀' + cc));
        else l0 = vec_addN(l0, m_c32(cc));
      }
      l0 = vec_addN(l0, m_c32(U'"'));
    } else {
      l0 = vec_addN(l0, m_c32(U'·'));
    }
  }
  
  
  r = append_fmt(r, "\n%R", l0); decG(l0);
  if (caretPos>=0) {
    u8* arr;
    B l1 = m_c8arrv(&arr, caretPos+1);
    FILL_TO(arr, el_c8, 0, m_c32(' '), caretPos);
    arr[caretPos] = '^';
    r = append_fmt(r, "\n%R", l1); decG(l1);
  }
  thr(r);
}
NOINLINE NORETURN void ffiThrOF(u32* currOff, ParseContext* pc, char* str, ...) {
  va_list a; va_start(a, str);
  B r = ffiThrImpl(currOff, pc, str, a);
  va_end(a);
  thr(r);
}
NOINLINE NORETURN void ffiThrF(ParseContext* pc, char* str, ...) {
  va_list a; va_start(a, str);
  B r = ffiThrImpl(NULL, pc, str, a);
  va_end(a);
  thr(r);
}

static ux moreScratchMem(ParseContext* pc, ux alignment, ux size) {
  ux r = (pc->scratchMemSize + alignment - 1) & ~((ux)alignment - 1);
  pc->scratchMemSize = r + size;
  return r;
}

static NOINLINE u32 readUInt(ParseContext* pc, u32** p) {
  u32* c = *p;
  u32 r = 0;
  while (*c>='0' & *c<='9') {
    if (r >= U32_MAX/10 - 10) ffiThrOF(c, pc, "FFI type: number literal too large");
    r = r*10 + *c-'0';
    c++;
  }
  if (c == *p) ffiThrOF(c, pc, "FFI type: expected number");
  *p = c;
  return r;
}

static char* const ffi_nullCharMsg = "FFI type: Types must not contain null characters";
static B parseFFIType0(ArgParseState* st) {
  ParseContext* pc = &st->pc;
  u32* c = pc->currOff;
  #define NESTED_TYPE() ({ pc->currOff = c; B r = parseFFIType0(st); c = pc->currOff; r; })
  #define CUSTOM_RET(VAL) pc->currOff = c; return (VAL)
  #define COMPOUND_RET(R) CUSTOM_RET(tag(R, OBJ_TAG))
  bool isOutermost = st->outermost;
  st->outermost = false;
  switch (*c) {
    case ':': case ',': case '}': case '\0': // characters that must never start a type, used for detecting primitive "*" vs "*T"
    default:
      if (*c == '\0') ffiThrF(pc, c!=st->pc.currEnd? ffi_nullCharMsg : "FFI type: Expected type, got end of string");
      ffiThrF(pc, "FFI type: Unexpected character where type is expected: '%c' (@+%ui)", *c, *c);
    case 'a': { c++;
      CUSTOM_RET(m_ffiPrim(sty_a, sty_void));
    }
    case 'b': { c++;
      if (*c++!='o' || *c++!='o' || *c++!='l') ffiThrF(pc, "FFI type: Expected \"bool\"");
      CUSTOM_RET(m_ffiPrim(sty_bool, sty_void));
    }
    case 'i': case 'u': case 'f': {
      u8 qual = *c++;
      u32 w;
      if (!(*c >= '0' && *c <= '9')) {
        if (qual!='f' && c[0]=='s' && c[1]=='i' && c[2]=='z' && c[3]=='e') {
          c+= 4;
          w = sizeof(size_t) * 8;
          goto gotWidth;
        }
        ffiThrF(pc, "FFI type: Expected '%c' to be followed by either a number, or the text \"size\"", qual);
      }
      w = readUInt(pc, &c);
      gotWidth:;
      PrimType ty;
      switch (qual) { default: UD;
        case 'f': switch(w) { default:goto bad_sty;                                                   case 32:ty=sty_f32;break; case 64:ty=sty_f64;break; } break;
        case 'u': switch(w) { default:goto bad_sty; case 8:ty=sty_u8;break; case 16:ty=sty_u16;break; case 32:ty=sty_u32;break; case 64:ty=sty_u64;break; } break;
        case 'i': switch(w) { default:goto bad_sty; case 8:ty=sty_i8;break; case 16:ty=sty_i16;break; case 32:ty=sty_i32;break; case 64:ty=sty_i64;break; } break;
      }
      if (0) { bad_sty: ffiThrF(pc, "FFI type: Unsupported width in %c%ui", qual, w); }
      if (0) { prim_ptr: ty=sty_rawPtr; }
      
      ConvType conv = sty_void;
      if (*c == ':') { c++;
        u32 convQual = *c++;
        if (convQual == 0) ffiThrF(pc, "FFI type: Unexpected trailing ':'");
        u32 w = readUInt(pc, &c);
        switch (convQual) {
          default: ffiThrF(pc, "FFI type: ':' must be followed by one of 'i', 'u', or 'c'");
          case 'u': if (w!=1) bad_conv: ffiThrF(pc, "FFI type: ':%c%ui' not supported", convQual, w); conv=sty_bit; break;
          case 'i': switch(w) { default:goto bad_conv; case 8:conv=sty_i8;break; case 16:conv=sty_i16;break; case 32:conv=sty_i32;break; } break;
          case 'c': switch(w) { default:goto bad_conv; case 8:conv=sty_c8;break; case 16:conv=sty_c16;break; case 32:conv=sty_c32;break; } break;
        }
      }
      
      i32 d = sty_lb(conv) - sty_lb(ty);
      if (RARE(d > 0)) { // comparing sty log sizes as signed allows nicely handling the T:u1 case
        ffiThrF(pc, "FFI type: %S:%S has a wider representation than its value", sty_name(ty), sty_name(conv));
      }
      
      CUSTOM_RET(m_ffiPrim(ty, conv));
    }
    #define ARR_INPLEN ({ ux inpLen = arrElts; if (isFFIPrim(elt) && ffiPrimConv(elt)!=sty_void) inpLen = inpLen << sty_lb(ffiPrimTy(elt)) >> sty_lb(ffiPrimConv(elt)); inpLen; })
    u32 arrElts;
    case '&': case '*': case U'⥊': {
      u32 ptrChar = *c++;
      PtrKind ptrKind = ptrChar==U'⥊'? ptrk_out : ptrChar=='&'? ptrk_mut : ptrk_in;
      bool read = ptrKind != ptrk_in;
      if (*c == U'·') { c++; read=false; if (ptrKind==ptrk_in) ffiThrF(pc, "FFI type: Cannot do *·T"); }
      arrElts = 0;
      if (0) { toplevelArray:; read=false; ptrKind=ptrk_in; }
      
      ArgParseState st0 = *st;
      if (ptrKind == ptrk_in) {
        if (arrElts==0) if (*c==':' || *c==',' || *c=='}' || *c==0) goto prim_ptr;
      } else {
        if (!st->allowMut) ffiThrF(pc, "FFI type: \"%cT\" %S", ptrChar, st->kind==0? "cannot be used here" : "can only be used in function argument types");
        st->argMayRead = true;
      }
      st->mayNeedTmpBufs = true;
      st->needFull = st->allowMut = false;
      B elt = NESTED_TYPE();
      
      st->needFull = st0.needFull;
      st->allowMut = st0.allowMut;
      
      FFICompoundType* r = m_ffiCompound(1, arrElts? cty_tlarr : cty_ptr, FFI_TYPE_VOID, 0);
      r->ptr.inpLen = ARR_INPLEN;
      r->ptr.kind = ptrKind;
      r->ptr.read = read;
      r->a[0].ptr.ptrObjRefOffset = moreScratchMem(pc, _Alignof(B), sizeof(B));
      r->foreign = ffi_type_pointer;
      r->a[0].o = elt;
      NOGC_E;
      if (read) {
        *st->mutList = vec_addN(*st->mutList, incG(tag(r,OBJ_TAG)));
      }
      
      COMPOUND_RET(r);
    }
    case '[': { c++;
      arrElts = readUInt(pc, &c);
      if (*c++!=']') ffiThrOF(c-1, pc, "FFI type: Expected ']'");
      if (arrElts==0) ffiThrOF(c-2, pc, "FFI type: 0-element arrays not supported");
      if (isOutermost) {
        if (st->kind == 1) ffiThrF(pc, "•FFI: Functions cannot return arrays");
        goto toplevelArray; // TODO maybe could handle here, allowing the buffer to be statically allocated into ScratchMem?
      }
      
      
      ArgParseState st0 = *st;
      st->allowMut = false;
      B elt = NESTED_TYPE();
      st->allowMut = st0.allowMut;
      ffi_type* foreignElt = foreignType(elt);
      ux expectedSize = foreignElt->size;
      if (MUL_ON(expectedSize, arrElts) || expectedSize >= USZ_MAX) thrM("FFI type: Array size too large"); // arrays are the only real place where size overflow is reasonably-possible without an equivalent memory cost. (could still overflow via a 2^16-element struct of 2^48-sized arrays, but whatever)
      
      FFICompoundType* r = m_ffiCompound(1, cty_starr, FFI_TYPE_STRUCT, st->needFull? arrElts : 1);
      r->starr.arrElts = arrElts;
      r->starr.inpLen = ARR_INPLEN;
      r->a[0].o = elt;
      NOGC_E;
      
      if (st->needFull) {
        r->foreign.alignment = r->foreign.size = 0;
        PLAINLOOP for (ux i = 0; i < arrElts; i++) r->foreign.elements[i] = foreignElt;
        libffiOk(ffi_get_struct_offsets(FFI_DEFAULT_ABI, &r->foreign, NULL));
      } else { // optimized manual implementation when only CBQN code will need to read/write elements
        debug_assert(foreignElt->size % foreignElt->alignment == 0);
        r->foreign.alignment = foreignElt->alignment;
        r->foreign.size = expectedSize;
        r->foreign.elements[0] = foreignElt;
      }
      COMPOUND_RET(r);
    }
    #undef ARR_INPLEN
    case '{': { c++;
      TSALLOC(B, fields0, 4);
      if (*c == '}') ffiThrF(pc, "FFI type: structs must have at least one field");
      while (true) {
        TSADD(fields0, NESTED_TYPE());
        if (*c == '}') { c++; break; }
        if (*c != ',') ffiThrOF(c, pc, *c=='\0'? "FFI type: Unfinished '{'" : "FFI type: Expected ',' or '}' after struct field");
        c++;
      }
      
      ux n = TSSIZE(fields0);
      FFICompoundType* r = m_ffiCompound(n, cty_struct, FFI_TYPE_STRUCT, n);
      r->foreign.alignment = r->foreign.size = 0;
      for (ux i = 0; i < n; i++) {
        B o = fields0[i];
        r->a[i].o = o;
        r->foreign.elements[i] = foreignType(o);
      }
      NOGC_E;
      TSFREE(fields0);
      
      TALLOC(size_t, offsets, n);
      libffiOk(ffi_get_struct_offsets(FFI_DEFAULT_ABI, &r->foreign, offsets));
      PLAINLOOP for (ux i = 0; i < n; i++) r->a[i].st.fieldOffset = offsets[i];
      TFREE(offsets);
      
      COMPOUND_RET(r);
    }
  }
  #undef CUSTOM_RET
  #undef COMPOUND_RET
  #undef NESTED_TYPE
}
static NOINLINE B parseFFIType(ArgParseState* st, u8 kind, U32Span span) {
  st->kind = kind;
  st->pc.currOff = span.start;
  st->pc.currEnd = span.end;
  B r = parseFFIType0(st);
  if (st->pc.currOff != st->pc.currEnd) ffiThrF(&st->pc, *st->pc.currOff? "FFI type: Expected type to end" : ffi_nullCharMsg);
  return r;
}



typedef struct PreprocessedArg {
  bool varargsSeparator; // if true, all other fields are irrelevant
  bool onW, wholeArg; // whether "𝕨" and ">" respectively are set
  U32Span rest;
} PreprocessedArg;
static PreprocessedArg preprocessArg(ParseContext* pc, B* xp) {
  U32Span src = toC32Null(xp, false);
  u32* start = src.start;
  if (start[0]=='.' && start[1]=='.' && start[2]=='.' && start+3 == src.end) return (PreprocessedArg) {.varargsSeparator = true};
  
  PreprocessedArg r = {.varargsSeparator = false};
  bool sideSpecified = false;
  more: switch(*src.start) {
    case '>':
      if (r.wholeArg) ffiThrOF(src.start, pc, "FFI: '>' specified multiple times on the same arg");
      r.wholeArg = true;
      src.start++;
      goto more;
    case U'𝕨': case U'𝕩':
      if (sideSpecified) ffiThrOF(src.start, pc, "FFI: Must have at most one of '𝕨' or '𝕩' on one arg");
      sideSpecified = true;
      r.onW = *src.start==U'𝕨';
      src.start++;
      goto more;
  }
  
  r.rest = src;
  return r;
}



static NOINLINE void computeMutOffsets(B obj, u32 offset) {
  if (isFFIPrim(obj)) return;
  FFICompoundType* ct = c(FFICompoundType, obj);
  if (ct->cty == cty_ptr) {
    if (ct->ptr.kind != ptrk_in) ct->a[0].ptr.dataPtrOffset = offset;
  } else if (ct->cty == cty_struct) {
    ux ia = ct->ia;
    for (ux i = 0; i < ia; i++) computeMutOffsets(ct->a[i].o, offset + ct->a[i].st.fieldOffset);
  }
}

#if GPR6_SYSV64
static NOINLINE void gprSysvType(bool* gprSysvOk, B arg) {
  if (isFFIPrim(arg)) {
    switch (ffiPrimTy(arg)) {
      case sty_void: case sty_a:
      case sty_i8: case sty_i16: case sty_i32: case sty_i64:
      case sty_u8: case sty_u16: case sty_u32: case sty_u64:
      case sty_bool: case sty_rawPtr: break;
      default: *gprSysvOk = false;
    }
  } else {
    FFICompoundType* ct = c(FFICompoundType, arg);
    if (!(ct->cty==cty_ptr || ct->cty==cty_tlarr)) *gprSysvOk = false;
  }
}
#endif

static void checkList(B x, ux expLen, bool isW) {
  if (!FFI_CHECKS) return;
  char* what = isW? "𝕨" : "𝕩";
  if (!isArr(x)) thrF("FFI: Expected list %U, but %U was an atom", what, what);
  if (RNK(x)!=1 || IA(x)!=expLen) {
    thrF("FFI: Wrong value for %U: Expected list with %z elements, got array with shape %H", what, expLen, x);
  }
}
#define TAKE_LIST(X, EXP_LEN, IS_W) ({ checkList(X, EXP_LEN, IS_W); TO_BPTR(X); }) // don't use inline in a call that also uses X in another arg!

static NOINLINE B ffiFn_lx_c1(B t, B x) {
  FFIFn* f = c(FFIFn,t);
  B* xp = TAKE_LIST(x, f->xLen, 0);
  return ffiFn_core(f, NULL, xp, x);
}
static NOINLINE B ffiFn_vx_c1(B t, B x) {
  FFIFn* f = c(FFIFn,t);
  return ffiFn_core(f, NULL, &x, x);
}
static NOINLINE B ffiFn_vwvx_c2(B t, B w, B x) {
  FFIFn* f = c(FFIFn,t);
  B xp[] = {x,w};
  B r = ffiFn_core(f, NULL, xp, x);
  dec(w);
  return r;
}
static NOINLINE B ffiFn_vwlx_c2(B t, B w, B x) {
  FFIFn* f = c(FFIFn,t);
  B* xp = TAKE_LIST(x, f->xLen, 0);
  B r = ffiFn_core(f, &w, xp, x);
  dec(w);
  return r;
}
static NOINLINE B ffiFn_lwax_c2(B t, B w, B x) {
  FFIFn* f = c(FFIFn,t);
  B* wp = TAKE_LIST(w, f->wLen, 1);
  B* xp = f->xLen==U32_MAX? &x : TAKE_LIST(x, f->xLen, 0);
  B r = ffiFn_core(f, wp, xp, x);
  decG(w);
  return r;
}

usz indexOfOne(B l, B e); // from search.c
B ffiload_c2(B t, B w, B x0) {
  if (!isArr(x0) || RNK(x0)!=1) thrM("•FFI: 𝕩 must be a list");
  usz xia = IA(x0);
  if (xia<2) thrM("•FFI: Function specification must have at least two items");
  usz argn0 = xia-2;
  if (argn0 >= U16_MAX) thrM("•FFI: Too many arguments"); // TODO what max
  Arr* x = cpyHArr(x0); // to allow things to convert to c32arrs, storing the reference of what to free later in this array
  B* xp = harrv_ptr(x);
  
  B symName = xp[1];
  vfyStr(symName, "•FFI", "Symbol name");
  if (IA(symName) != indexOfOne(symName, m_c32(0))) thrM("•FFI: Symbol name must not contain a null character");
  
  char* ws = NULL;
  if (w.u != m_c32(0).u) {
    if (isArr(w) && RNK(w)==1 && IA(w)==2) { // ↑‿path
      SGetU(w);
      B e0 = GetU(w,0);
      if (isFun(e0) && RTID(e0) == n_take) {
        B e1 = GetU(w,1);
        if (!isStr(e1)) thrM("↑‿path •FFI 𝕩: Path must be a list of characters");
        if (!path_isSingleComponent(e1)) thrM("↑‿path •FFI 𝕩: Path cannot contain slashes");
        ws = toCStr(e1);
        goto wsSet;
      }
    }
    w = path_rel(nfn_objU(t), w, "•FFI");
    ws = toCStr(w);
    wsSet:;
  }
  void* dl = dlopen(ws, RTLD_NOW);
  
  if (ws) freeCStr(ws);
  dec(w);
  if (dl==NULL) thrF("•FFI: Failed to load library: %S", dlerror());
  
  char* nameStr = toCStr(symName);
  void* sym = dlsym(dl, nameStr);
  freeCStr(nameStr);
  if (sym==NULL) thrF("•FFI: Failed to find symbol: %S", dlerror());
  
  ArgParseState st;
  ParseContext* pc = &st.pc;
  pc->xp = xp;
  pc->xia = xia;
  
  B retObj;
  u8 resType = 0; // 0: some value; 1: void - ""; 2: "&"
  {
    vfyStr(xp[0], "•FFI", "Result type specifier");
    U32Span retSrc = toC32Null(xp+0, true);
    if (retSrc.start == retSrc.end) {
      resType = 1;
      resVoid: retObj = FFIPRIM_VOID;
    } else if (retSrc.start[0] == '&' && retSrc.start+1 == retSrc.end) {
      resType = 2;
      goto resVoid;
    } else {
      pc->curr = 0;
      st.allowMut = st.needFull = false;
      st.outermost = true;
      retObj = parseFFIType(&st, 1, retSrc);
    }
  }
  ffi_type* retForeign = foreignType(retObj);
  
  if (retForeign->alignment > 8) thrF("•FFI: Return values with %z-byte alignment not implemented", (ux)retForeign->alignment);
  pc->scratchMemSize = IMAX(retForeign->size, IMAX(sizeof(u64), sizeof(ffi_arg)));
  
  // preprocess args, to find varargs separator and proper arg count (also validating the args being strings, and converting them to null-terminated-c32 form)
  i32 fixedArgCount = -1;
  i32 argLengths[2] = {0,0};
  TALLOC(PreprocessedArg, preprocessedArgs, argn0);
  PreprocessedArg* currArg = preprocessedArgs;
  for (ux i = 0; i < argn0; i++) {
    B* val = xp+i+2;
    vfyStr(*val, "•FFI", "Argument type specifier");
    pc->curr = i+2;
    *currArg = preprocessArg(pc, val);
    if (currArg->varargsSeparator) {
      if (fixedArgCount != -1) thrM("•FFI: Cannot have multiple instances of \"...\"");
      // leave currArg unchanged, compacting away the "..."
      fixedArgCount = i;
    } else {
      i32* len = argLengths + currArg->onW;
      if (currArg->wholeArg) {
        if (*len != 0) { mixedMix:;
          char* side = currArg->onW? "𝕨" : "𝕩";
          thrF("•FFI: Cannot use \">%U\" with multiple arguments on %U", side, side);
        }
        *len = -1;
      } else {
        if (*len == -1) goto mixedMix;
        else (*len)++;
      }
      currArg++;
    }
  }
  // length is -1 if whole arg, else number of elements on the side (0 if none)
  i32 wLen = argLengths[1]; bool vw = wLen==-1;
  i32 xLen = argLengths[0]; bool vx = xLen==-1;
  
  ux argCount = argn0 - (fixedArgCount>=0);
  assert(argCount == (vw? 1 : wLen) + (vx? 1 : xLen));
  
  bool bothScalar = vw && vx;
  
  // now that we know the argument layout, can finally allocate the function object
  FFICompoundType* argData = m_ffiCompound(1+argCount, cty_argData, FFI_TYPE_VOID, 0);
  PLAINLOOP for (ux i = 0; i < 1+argCount; i++) argData->a[i].o = bi_z;
  NOGC_E;
  
  FFIFn* r = (FFIFn*) m_ffiFn(
    fsizeof(FFIFn, cif_args, ffi_type*, argCount),
    foreignFnDesc,
    tag(argData,OBJ_TAG),
    wLen!=0? c1_bad : vx? ffiFn_vx_c1 : ffiFn_lx_c1,
    bothScalar? ffiFn_vwvx_c2 : wLen==0? ffiFn_lwax_c2 : vw? ffiFn_vwlx_c2 : ffiFn_lwax_c2
  );
  r->sym = sym;
  r->xLen = xLen;
  r->wLen = wLen;
  r->argCount = argCount;
  r->argData = argData->a + 1;
  
  #if GPR6_SYSV64
  bool gprSysvOk = true;
  gprSysvType(&gprSysvOk, retObj);
  #endif
  // process actual arguments
  i32 positions[2] = {0,0};
  st.mayNeedTmpBufs = false;
  B mutList = emptyHVec();
  st.mutList = &mutList;
  for (ux i = 0; i < argCount; i++) {
    pc->curr = i+2;
    PreprocessedArg* arg0 = &preprocessedArgs[i];
    st.argMayRead = false;
    bool srcArgWP = !bothScalar && arg0->onW;
    i32 srcArgIdx = bothScalar && arg0->onW? 1 : positions[arg0->onW]++;
    st.allowMut = st.needFull = true;
    st.outermost = true;
    B arg = parseFFIType(&st, 0, arg0->rest);
    #if GPR6_SYSV64
    gprSysvType(&gprSysvOk, arg);
    #endif
    
    ffi_type* foreign = foreignType(arg);
    r->cif_args[i] = foreign;
    ux selfOffset = moreScratchMem(pc, foreign->alignment, foreign->size);
    r->argData[i] = (FFIEnt) {.o=arg, .argData = {.scratchMemOffset = selfOffset, .srcPos = srcArgWP? ~srcArgIdx : srcArgIdx}};
    if (st.argMayRead) computeMutOffsets(arg, selfOffset);
  }
  TFREE(preprocessedArgs);
  
  usz mutCount = IA(mutList);
  if (resType==2 && mutCount!=1) thrF("•FFI: Can only have result type of \"&\" when exactly one object is mutated, but %ui are", mutCount);
  if (mutCount != 0) debug_assert(st.mayNeedTmpBufs);
  
  #if GPR6_SYSV64
  r->gprSysv = MAY_F(argCount<=6 && fixedArgCount==-1 && gprSysvOk);
  #endif
  
  r->argListOffset = moreScratchMem(pc, _Alignof(void*), sizeof(void*) * argCount);
  
  if (st.mayNeedTmpBufs) {
    usz resElts = IMAX(1, (resType==0) + mutCount);
    FFICompoundType* extraRet = m_ffiCompound(resElts, mutCount>0 && resType!=2? cty_retList : cty_ret1, FFI_TYPE_VOID, 0);
    ux ri = 0;
    if (resElts == mutCount) debug_assert(isFFIPrim(retObj));
    else extraRet->a[ri++].o = retObj;
    B* mutElts = harr_ptr(mutList);
    for (ux i = 0; i < mutCount; i++) {
      B o = mutElts[i];
      FFICompoundType* ct = c(FFICompoundType, o);
      assert(ct->cty == cty_ptr);
      extraRet->a[ri++].o = incG(o);
    }
    #if GPR6_SYSV64
    if (r->gprSysv) for (ux i = 0; i < argCount; i++) {
      B o = r->argData[i].o;
      if (isFFIPrim(o)) continue;
      FFICompoundType* ct = c(FFICompoundType, o);
      if (ct->cty == cty_ptr) ct->a[0].ptr.dataPtrOffset = r->argListOffset + 8*i;
    }
    #endif
    NOGC_E;
    debug_assert(ri == resElts);
    retObj = tag(extraRet, OBJ_TAG);
  }
  decG(mutList);
  argData->a[0].o = retObj;
  r->retObj = retObj;
  
  if (fixedArgCount==-1) libffiOk(ffi_prep_cif(&r->cif, FFI_DEFAULT_ABI,                argCount, retForeign, r->cif_args));
  else               libffiOk(ffi_prep_cif_var(&r->cif, FFI_DEFAULT_ABI, fixedArgCount, argCount, retForeign, r->cif_args));
  
  ptr_dec(x);
  if (pc->scratchMemSize >= U32_MAX) thrM("•FFI: Too much static data");
  r->scratchMemSize = pc->scratchMemSize;
  return tag(r, FUN_TAG);
}
