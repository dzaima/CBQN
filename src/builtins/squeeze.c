#include "../core.h"

#if SINGELI_SIMD || SINGELI_X86_64
  #define SINGELI_FILE squeeze
  #include "../utils/includeSingeli.h"
#endif

NOINLINE B num_squeezeF(B x, usz ia) {
  u32 or = 0;
  SGetU(x)
  Arr* a;
  for (usz i = 0; i < ia; i++) {
    B cr = GetU(x,i);
    if (RARE(!q_i32(cr))) {
      while (i<ia) if (!isF64(GetU(x,i++))) return FL_SET(x, fl_squoze);
      a = (Arr*) cpyF64Arr(x);
      goto retn;
    }
    i32 c = o2iG(cr);
    or|= ((u32)c & ~1) ^ (u32)(c>>31);
  }
  a = or==0?          (Arr*)cpyBitArr(x)
  : or<=(u32)I8_MAX?  (Arr*)cpyI8Arr (x)
  : or<=(u32)I16_MAX? (Arr*)cpyI16Arr(x)
  :                   (Arr*)cpyI32Arr(x);
  
  retn:
  FLV_SET(a, fl_squoze);
  return taga(a);
}
B num_squeeze(B x) {
  usz ia = IA(x);
  u8 xe = TI(x,elType);
  if (ia==0) {
    if (xe==el_bit) return x;
    goto r_bit;
  }
  
  #if !SINGELI_SIMD && !SINGELI_X86_64
  usz i = 0;
  #endif
  
  u32 or = 0; // using bitwise or as an approximate ⌈´
  switch (xe) { default: UD;
    case el_bit: goto r_x;
    #if SINGELI_SIMD || SINGELI_X86_64
      case el_i8:  { or = avx2_squeeze_i8 (i8any_ptr (x), ia); if(or>       1) goto r_x; else goto mostBit; }
      case el_i16: { or = avx2_squeeze_i16(i16any_ptr(x), ia); if(or>  I8_MAX) goto r_x; else goto mostI8; }
      case el_i32: { or = avx2_squeeze_i32(i32any_ptr(x), ia); if(or> I16_MAX) goto r_x; else goto mostI16; }
      case el_f64: { or = avx2_squeeze_f64(f64any_ptr(x), ia); if(-1==(u32)or) goto r_x; else goto mostI32; }
    #else
      case el_i8:  { i8*  xp = i8any_ptr (x); for (; i < ia; i++) { i32 c = xp[i]; or|= (u8)c;                        } if(or>      1) goto r_x; goto mostBit; }
      case el_i16: { i16* xp = i16any_ptr(x); for (; i < ia; i++) { i32 c = xp[i]; or|= ((u32)c & ~1) ^ (u32)(c>>31); } if(or> I8_MAX) goto r_x; goto mostI8; }
      case el_i32: { i32* xp = i32any_ptr(x); for (; i < ia; i++) { i32 c = xp[i]; or|= ((u32)c & ~1) ^ (u32)(c>>31); } if(or>I16_MAX) goto r_x; goto mostI16; }
      case el_f64: {
        f64* xp = f64any_ptr(x);
        for (; i < ia; i++) {
          f64 cf = xp[i];
          i32 c = (i32)cf;
          if (c!=cf) goto r_x; // already f64
          or|= ((u32)c & ~1) ^ (u32)(c>>31);
        }
        goto mostI32;
      }
    #endif
    case el_B: case el_c8: case el_c16: case el_c32:; /*fallthrough*/
  }
  
  B* xp = arr_bptr(x);
  if (xp==NULL) goto r_f;
  
  #if SINGELI_SIMD || SINGELI_X86_64
    or = avx2_squeeze_numB(xp, ia);
    if (-2==(i32)or) goto r_x;
    if (-1==(i32)or) goto r_f64;
    goto mostI32;
  #else
    for (; i < ia; i++) {
      if (RARE(!q_i32(xp[i]))) {
        while (i<ia) if (!isF64(xp[i++])) goto r_x;
        goto r_f64;
      }
      i32 c = o2iG(xp[i]);
      or|= ((u32)c & ~1) ^ (u32)(c>>31);
    }
    goto mostI32;
  #endif
  
  mostI32: if(or>I16_MAX  ) goto r_i32;
  mostI16: if(or>I8_MAX   ) goto r_i16;
  mostI8:  if(or>0        ) goto r_i8;
  mostBit: goto r_bit;
  
  B rb; Arr* ra;
  r_f:   return num_squeezeF(x, ia); // rb = num_squeezeF(x,ia); goto retn;
  r_x:   ra = a(x);   rb = x;     goto retn;
  r_f64: ra = (Arr*)cpyF64Arr(x); goto tag;
  r_i32: ra = (Arr*)cpyI32Arr(x); goto tag;
  r_i16: ra = (Arr*)cpyI16Arr(x); goto tag;
  r_i8:  ra = (Arr*)cpyI8Arr (x); goto tag;
  r_bit: ra = (Arr*)cpyBitArr(x); goto tag;
  
  tag:
  rb = taga(ra);
  retn:
  FLV_SET(ra, fl_squoze);
  return rb;
}
B chr_squeeze(B x) {
  usz ia = IA(x);
  u8 xe = TI(x,elType);
  if (ia==0) {
    if (xe==el_c8) return x;
    goto r_c8;
  }
  usz i = 0;
  i32 or = 0;
  switch(xe) { default: UD;
    case el_c8: goto r_x;
    #if SINGELI_SIMD || SINGELI_X86_64
    case el_c16: { u32 t = avx2_squeeze_c16(c16any_ptr(x), ia); if (t==0) goto r_c8; else goto r_x; }
    case el_c32: { u32 t = avx2_squeeze_c32(c32any_ptr(x), ia); if (t==0) goto r_c8; else if (t==1) goto r_c16; else if (t==2) goto r_x; else UD; }
    #else
    case el_c16: {
      u16* xp = c16any_ptr(x);
      for (; i < ia; i++) if (xp[i] != (u8)xp[i]) goto r_x;
      goto r_c8;
    }
    case el_c32: {
      u32* xp = c32any_ptr(x);
      bool c8 = true;
      for (; i < ia; i++) {
        if (xp[i] != (u16)xp[i]) goto r_x;
        if (xp[i] != (u8 )xp[i]) c8 = false;
      }
      if (c8) goto r_c8;
      else    goto r_c16;
    }
    #endif
    case el_bit: case el_i8: case el_i16: case el_i32: case el_f64: case el_B:; /*fallthrough*/
  }
  
  B* xp = arr_bptr(x);
  if (xp!=NULL) {
    #if SINGELI_SIMD || SINGELI_X86_64
    u32 t = avx2_squeeze_chrB(xp, ia);
    if      (t==0) goto r_c8;
    else if (t==1) goto r_c16;
    else if (t==2) goto r_c32;
    else if (t==3) goto r_x;
    else UD;
    #else
    for (; i < ia; i++) {
      if (!isC32(xp[i])) goto r_x;
      or|= o2cG(xp[i]);
    }
    #endif
  } else {
    SGetU(x)
    for (; i < ia; i++) {
      B cr = GetU(x,i);
      if (!isC32(cr)) goto r_x;
      or|= o2cG(cr);
    }
  }
  if      (or<=U8_MAX ) r_c8:  return FL_SET(toC8Any(x), fl_squoze);
  else if (or<=U16_MAX) r_c16: return FL_SET(toC16Any(x), fl_squoze);
  else { goto r_c32; }  r_c32: return FL_SET(toC32Any(x), fl_squoze);
  /*when known typed:*/ r_x:   return FL_SET(x, fl_squoze);
}

B any_squeeze(B x) {
  assert(isArr(x));
  if (FL_HAS(x,fl_squoze)) return x;
  if (IA(x)==0) return FL_SET(x, fl_squoze); // TODO return a version of the smallest type
  B x0 = IGetU(x, 0);
  if (isNum(x0)) return num_squeeze(x);
  else if (isC32(x0)) return chr_squeeze(x);
  return FL_SET(x, fl_squoze);
}

B squeeze_deep(B x) {
  if (!isArr(x)) return x;
  x = any_squeeze(x);
  if (TI(x,elType)!=el_B) return x;
  usz ia = IA(x);
  M_HARR(r, ia)
  B* xp = arr_bptr(x);
  B xf = getFillQ(x);
  if (xp!=NULL) {
    for (usz i=0; i<ia; i++) { HARR_ADD(r, i, squeeze_deep(inc(xp[i]))); }
  } else {
    SGet(x);
    for (usz i=0; i<ia; i++) { HARR_ADD(r, i, squeeze_deep(Get(x,i))); }
  }
  return any_squeeze(qWithFill(HARR_FCD(r, x), xf));
}