#include "../core.h"

#define SQFN_START assert(xa == a(x) && ia==IA(x) && ia>0 && TY(x)==type && a(x)==xa)
typedef B (*SqueezeFn)(B x, Arr* xa, u8 type, ux ia);

static B squeeze_smallest(B x, Arr* xa, u8 type, ux ia) {
  SQFN_START;
  assert(TI(x,elType)==el_bit || TI(x,elType)==el_c8);
  FLV_SET(xa, fl_squoze);
  return x;
}
static B squeeze_nope(B x, Arr* xa, u8 type, ux ia) {
  SQFN_START;
  return x;
}



#define SQFN_NUM_RET \
  mostI32:MAYBE_UNUSED; if(or>I16_MAX) goto r_i32; \
  mostI16:MAYBE_UNUSED; if(or>I8_MAX ) goto r_i16; \
  mostI8: MAYBE_UNUSED; if(or>0      ) goto r_i8;  \
  mostBit:MAYBE_UNUSED;                goto r_bit; \
  r_f64:MAYBE_UNUSED; xa = cpyF64Arr(x); goto tag; \
  r_i32:              xa = cpyI32Arr(x); goto tag; \
  r_i16:              xa = cpyI16Arr(x); goto tag; \
  r_i8:               xa = cpyI8Arr (x); goto tag; \
  r_bit:              xa = cpyBitArr(x); goto tag; \
  tag: x = taga(xa);                               \
  squeezed: FLV_SET(xa, fl_squoze);                \
  return x;

#define SQFN_CHR_RET \
  r_c32:MAYBE_UNUSED; xa = cpyC32Arr(x); goto tag; \
  r_c16:MAYBE_UNUSED; xa = cpyC16Arr(x); goto tag; \
  r_c8: MAYBE_UNUSED; xa = cpyC8Arr (x); goto tag; \
  tag: x = taga(xa);                               \
  squeezed: FLV_SET(xa, fl_squoze);                \
  return x;

#define SQFN_NUM(T, CORE) static B squeeze_##T(B x, Arr* xa, u8 type, ux ia) { \
  SQFN_START;     \
  u32 or=0; CORE; \
  SQFN_NUM_RET;   \
}
#define SQFN_CHR(T, CORE) static B squeeze_##T(B x, Arr* xa, u8 type, ux ia) { \
  SQFN_START;    \
  CORE;          \
  SQFN_CHR_RET;  \
}

#if SINGELI_SIMD
  #define SINGELI_FILE squeeze
  #include "../utils/includeSingeli.h"
  
  SQFN_NUM(i8,  or = si_squeeze_i8 ( i8anyv_ptr(xa), ia); if(or>       1) goto squeezed; else goto mostBit;)
  SQFN_NUM(i16, or = si_squeeze_i16(i16anyv_ptr(xa), ia); if(or>  I8_MAX) goto squeezed; else goto mostI8;)
  SQFN_NUM(i32, or = si_squeeze_i32(i32anyv_ptr(xa), ia); if(or> I16_MAX) goto squeezed; else goto mostI16;)
  SQFN_NUM(f64, or = si_squeeze_f64(f64anyv_ptr(xa), ia); if(-1==(u32)or) goto squeezed; else goto mostI32;)
  
  SQFN_CHR(c16, u32 t = si_squeeze_c16(c16anyv_ptr(xa), ia); if (t==0) goto r_c8; else goto squeezed;)
  SQFN_CHR(c32, u32 t = si_squeeze_c32(c32anyv_ptr(xa), ia); if (t==0) goto r_c8; else if (t==1) goto r_c16; else if (t==2) goto squeezed; else UD;)
#else
  SQFN_NUM(i8,  i8*  xp = i8anyv_ptr (xa); for (ux i=0; i<ia; i++) { i32 c = xp[i]; or|= (u8)c;                        } if(or>      1) goto squeezed; goto mostBit;)
  SQFN_NUM(i16, i16* xp = i16anyv_ptr(xa); for (ux i=0; i<ia; i++) { i32 c = xp[i]; or|= ((u32)c & ~1) ^ (u32)(c>>31); } if(or> I8_MAX) goto squeezed; goto mostI8;)
  SQFN_NUM(i32, i32* xp = i32anyv_ptr(xa); for (ux i=0; i<ia; i++) { i32 c = xp[i]; or|= ((u32)c & ~1) ^ (u32)(c>>31); } if(or>I16_MAX) goto squeezed; goto mostI16;)
  SQFN_NUM(f64,
    f64* xp = f64anyv_ptr(xa);
    for (ux i = 0; i < ia; i++) {
      f64 cf = xp[i];
      i32 c = (i32)cf;
      if (c!=cf) goto squeezed;
      or|= ((u32)c & ~1) ^ (u32)(c>>31);
    }
    goto mostI32;
  )
  
  SQFN_CHR(c16,
    u16* xp = c16anyv_ptr(xa);
    for (ux i = 0; i < ia; i++) if (xp[i] != (u8)xp[i]) goto squeezed;
    goto r_c8;
  )
  SQFN_CHR(c32,
    u32* xp = c32anyv_ptr(xa);
    bool c8 = true;
    for (ux i = 0; i < ia; i++) {
      if (xp[i] != (u16)xp[i]) goto squeezed;
      if (xp[i] != (u8 )xp[i]) c8 = false;
    }
    if (c8) goto r_c8;
    else    goto r_c16;
  )
#endif



#define SQUEEZE_NUM_B_LOOP(GET) \
  u32 or = 0;                                              \
  for (ux i = 0; i < ia; i++) {                            \
    B c = GET;                                             \
    if (RARE(!q_i32(c))) {                                 \
      while (i<ia) { if (!isF64(GET)) goto not_num; i++; } \
      goto r_f64;                                          \
    }                                                      \
    i32 ci = o2iG(c);                                      \
    or|= ((u32)ci & ~1) ^ (u32)(ci>>31);                   \
  }                                                        \
  goto mostI32;

#define SQFN_NUM_B            \
  not_num:                    \
  if (notChar) goto squeezed; \
  else return x;              \
  SQFN_NUM_RET;
  
NOINLINE B squeeze_B_numSlow(B x, Arr* xa, u8 type, usz ia, bool notChar) {
  SQFN_START;
  SGetU(x)
  SQUEEZE_NUM_B_LOOP(GetU(x,i))
  SQFN_NUM_B;
}
SHOULD_INLINE B squeeze_BV_numImpl(B x, Arr* xa, u8 type, ux ia, B* xp, bool notChar) {
  SQFN_START;
  #if SINGELI_SIMD
    u32 or = si_squeeze_numB(xp, ia);
    if (-2==(i32)or) goto not_num;
    if (-1==(i32)or) goto r_f64;
    goto mostI32;
  #else
    SQUEEZE_NUM_B_LOOP(xp[i])
  #endif
  SQFN_NUM_B;
}



#define SQFN_CHR_B \
  if (or>U16_MAX) goto r_c32; \
  if (or>U8_MAX ) goto r_c16; \
  goto r_c8;                  \
  not_chr:                    \
  if (notNum) goto squeezed;  \
  else return x;              \
  SQFN_CHR_RET;

NOINLINE B squeeze_B_chrSlow(B x, Arr* xa, u8 type, usz ia, bool notNum) {
  SQFN_START;
  u32 or = 0;
  SGetU(x)
  for (ux i = 0; i < ia; i++) {
    B cr = GetU(x,i);
    if (!isC32(cr)) goto not_chr;
    or|= o2cG(cr);
  }
  SQFN_CHR_B;
}
SHOULD_INLINE B squeeze_BV_chrImpl(B x, Arr* xa, u8 type, usz ia, B* xp, bool notNum) {
  SQFN_START;
  u32 or = 0;
  #if SINGELI_SIMD
    u32 t = si_squeeze_chrB(xp, ia);
    if      (t==0) goto r_c8;
    else if (t==1) goto r_c16;
    else if (t==2) goto r_c32;
    else if (t==3) goto not_chr;
    else UD;
  #else
    for (ux i = 0; i < ia; i++) {
      if (!isC32(xp[i])) goto not_chr;
      or|= o2cG(xp[i]);
    }
  #endif
  SQFN_CHR_B;
}

SHOULD_INLINE B squeeze_B_chrImpl(B x, Arr* xa, u8 type, ux ia, bool notNum) {
  B* xp = arrv_bptr(xa);
  if (xp!=NULL) return squeeze_BV_chrImpl(x, xa, type, ia, xp, notNum);
  else return squeeze_B_chrSlow(x, xa, type, ia, notNum);
}

SHOULD_INLINE B squeeze_B_numImpl(B x, Arr* xa, u8 type, ux ia, bool notNum) {
  B* xp = arrv_bptr(xa);
  if (xp!=NULL) return squeeze_BV_numImpl(x, xa, type, ia, xp, notNum);
  else return squeeze_B_numSlow(x, xa, type, ia, notNum);
}

static B squeeze_B_numMaybe(B x, Arr* xa, u8 type, ux ia) { return squeeze_B_numImpl(x, xa, type, ia, false); }
static B squeeze_B_chrMaybe(B x, Arr* xa, u8 type, ux ia) { return squeeze_B_chrImpl(x, xa, type, ia, false); }

NOINLINE B squeeze_BGeneric(B x, Arr* xa, u8 type, ux ia) {
  SQFN_START;
  B x0 = IGetU(x,0);
  if (isNum(x0)) return squeeze_B_numSlow(x, xa, type, ia, true);
  if (isC32(x0)) return squeeze_B_chrSlow(x, xa, type, ia, true);
  FLV_SET(xa, fl_squoze);
  return x;
}
static B squeeze_B(B x, Arr* xa, u8 type, ux ia) {
  SQFN_START;
  B* xp = arrv_bptr(xa);
  if (xp == NULL) return squeeze_BGeneric(x, xa, type, ia);
  B x0 = xp[0];
  if (isNum(x0)) return squeeze_BV_numImpl(x, xa, type, ia, xp, true);
  if (isC32(x0)) return squeeze_BV_chrImpl(x, xa, type, ia, xp, true);
  FLV_SET(xa, fl_squoze);
  return x;
}

//                                  el_bit            el_i8         el_i16        el_i32        el_f64        el_c8             el_c16        el_c32        el_B
SqueezeFn squeeze_numFns[el_MAX] = {squeeze_smallest, squeeze_i8,   squeeze_i16,  squeeze_i32,  squeeze_f64,  squeeze_smallest, squeeze_nope, squeeze_nope, squeeze_B_numMaybe};
SqueezeFn squeeze_chrFns[el_MAX] = {squeeze_smallest, squeeze_nope, squeeze_nope, squeeze_nope, squeeze_nope, squeeze_smallest, squeeze_c16,  squeeze_c32,  squeeze_B_chrMaybe};
SqueezeFn squeeze_anyFns[el_MAX] = {squeeze_smallest, squeeze_i8,   squeeze_i16,  squeeze_i32,  squeeze_f64,  squeeze_smallest, squeeze_c16,  squeeze_c32,  squeeze_B};

NOINLINE B int_squeeze_sorted(B x, Arr* xa, u8 type, ux ia) {
  SQFN_START;
  assert(elInt(TI(x,elType)));
  if (ia==0) {
    squeezed:
    return FL_SET(x, fl_squoze);
  }
  u8 xe = TI(x,elType);
  i32 x0v, x1v;
  void* xp = tyanyv_ptr(xa);
  ux last = ia-1;
  switch (xe) { default: UD;
    case el_bit: goto squeezed;
    case el_i8:  x0v = ((i8* )xp)[0]; x1v = ((i8* )xp)[last]; break;
    case el_i16: x0v = ((i16*)xp)[0]; x1v = ((i16*)xp)[last]; break;
    case el_i32: x0v = ((i32*)xp)[0]; x1v = ((i32*)xp)[last]; break;
  }
  u8 x0e = selfElType_i32(x0v);
  u8 x1e = selfElType_i32(x1v);
  u8 re = x0e>x1e? x0e : x1e;
  if (xe == re) goto squeezed;
  Arr* ra;
  switch (re) { default: UD;
    case el_i16: ra = cpyI16Arr(x); break;
    case el_i8:  ra = cpyI8Arr (x); break;
    case el_bit: ra = cpyBitArr(x); break;
  }
  return taga(FLV_SET(ra, fl_squoze));
}



#define SQ_FAST(RET) if (FL_HAS(x, fl_squoze)) { RET; }
#define SQ_FAST_EXT(NEW, RET) \
  if (FL_HAS(x, fl_squoze|fl_asc|fl_dsc)) { \
    SQ_FAST(RET)                            \
    if (elInt(TI(x,elType))) { x = int_squeeze_sorted(x, a(x), TY(x), ia); NEW; RET; } \
  } // could check for sorted character arrays (even from a TI(x,el_B) input) but sorted character arrays aren't worth it

#define SQ_READ \
  assert(isArr(x)); \
  u8 xe = TI(x,elType); \
  usz ia = IA(x);

NOINLINE B any_squeeze(B x) {
  SQ_READ;
  SQ_FAST_EXT(, return x);
  if (ia==0) return FL_SET(x, fl_squoze); // TODO return a version of the smallest type?
  return squeeze_anyFns[xe](x, a(x), TY(x), ia);
}

NOINLINE B squeeze_numNew(B x) {
  SQ_READ;
  if (ia==0) return xe==el_bit? x : emptyNumsWithShape(x);
  return squeeze_numFns[xe](x,a(x),TY(x),ia);
}
NOINLINE B squeeze_chrNew(B x) {
  SQ_READ;
  if (ia==0) return xe==el_c8? x : emptyChrsWithShape(x);
  return squeeze_chrFns[xe](x,a(x),TY(x),ia);
}

SqRes squeeze_numTryImpl(B x) {
  SQ_READ;
  if (ia==0) return (SqRes){xe==el_bit? x : emptyNumsWithShape(x), el_bit};
  SQ_FAST_EXT(xe=TI(x,elType), return ((SqRes){x, xe}));
  x = squeeze_numFns[xe](x,a(x),TY(x),ia);
  return (SqRes){x, TI(x,elType)};
}
SqRes squeeze_chrTryImpl(B x) {
  SQ_READ;
  SQ_FAST(return ((SqRes){x, xe}));
  if (ia==0) return (SqRes){xe==el_c8? x : emptyChrsWithShape(x), el_c8};
  x = squeeze_chrFns[xe](x,a(x),TY(x),ia);
  return (SqRes){x, TI(x,elType)};
}

B squeeze_numOut(B x) { return squeeze_numTryImpl(x).r; }
B squeeze_chrOut(B x) { return squeeze_chrTryImpl(x).r; }



B squeeze_deep(B x) {
  if (!isArr(x)) return x;
  x = any_squeeze(x);
  if (TI(x,elType)!=el_B) return x;
  usz ia = IA(x);
  M_HARR(r, ia)
  B* xp = arr_bptr(x);
  B xf = getFillR(x);
  if (xp!=NULL) {
    for (ux i=0; i<ia; i++) { HARR_ADD(r, i, squeeze_deep(inc(xp[i]))); }
  } else {
    SGet(x);
    for (ux i=0; i<ia; i++) { HARR_ADD(r, i, squeeze_deep(Get(x,i))); }
  }
  return any_squeeze(qWithFill(HARR_FCD(r, x), xf));
}
