#pragma once

#define ARRV_BPTR_BODY switch (PTY(xa)) {      \
  case t_harr:      return harrv_ptr(xa);      \
  case t_fillarr:   return fillarrv_ptr(xa);   \
  case t_hslice:    return hslicev_ptr(xa);    \
  case t_fillslice: return fillslicev_ptr(xa); \
}

static B* arrv_bptrG(Arr* xa) {          ARRV_BPTR_BODY; UD; }
static B* arr_bptrG(B x) { Arr* xa=a(x); ARRV_BPTR_BODY; UD; }

#if ARR_BPTR_NEVER
  static B* arr_bptr(B x) { return NULL; }
  static B* arrv_bptr(Arr* x) { return NULL; }
#else
  static B* arr_bptr(B x) { Arr* xa=a(x); ARRV_BPTR_BODY; return NULL; }
  static B* arrv_bptr(Arr* xa) {          ARRV_BPTR_BODY; return NULL; }
#endif

static void* tyarrv_ptr(TyArr* x) {
  assert(IS_ANY_ARR(PTY(x)) && !IS_SLICE(PTY(x)));
  return x->a;
}
static void* tyanyv_ptr(Arr* x) {
  assert(IS_ANY_ARR(PTY(x)));
  return IS_SLICE(PTY(x))? ((TySlice*)x)->a : ((TyArr*)x)->a;
}
static void* tyslicev_ptr(Arr* x) {
  assert(IS_SLICE(PTY(x)));
  return ((TySlice*)x)->a;
}

static void* tyarr_ptr(B x) { return tyarrv_ptr(c(TyArr,x)); }
static void* tyany_ptr(B x) { return tyanyv_ptr(a(x)); }

static void* arr_ptr(Arr* t, u8 el) {
  return el==el_B? (void*)harrv_ptr(t) : tyarrv_ptr((TyArr*)t);
}



#define M_TYARR(WM, OVER, MID, RV, PRE) { PRE \
  Arr* r = m_arr((offsetof(TyArr, a) + (      \
      WM==0? ((u64)ia)*w     \
    : WM==1? ((u64)ia)<<w    \
    : ((((u64)ia)<<w)+7)>>3) \
  ) OVER, type, ia);         \
  MID                        \
  *rp = RV;                  \
  return ((TyArr*)r)->a;     \
}
// width in bytes; overalloc is a byte count
SHOULD_INLINE void* m_tyarrp (Arr** rp, usz w, usz ia, u8 type          ) M_TYARR(0,     , , r, )
SHOULD_INLINE void* m_tyarrpO(Arr** rp, usz w, usz ia, u8 type, u64 over) M_TYARR(0,+over, , r, )
SHOULD_INLINE void* m_tyarrv (B*    rp, usz w, usz ia, u8 type          ) M_TYARR(0,     , arr_shVec((Arr*)r);, taga(r), )
SHOULD_INLINE void* m_tyarrvO(B*    rp, usz w, usz ia, u8 type, u64 over) M_TYARR(0,+over, arr_shVec((Arr*)r);, taga(r), )
SHOULD_INLINE void* m_tyarrc (B*    rp, usz w, B x,    u8 type          ) M_TYARR(0,     , arr_shCopy((Arr*)r,x);, taga(r), usz ia = IA(x);)
SHOULD_INLINE void* m_tyarrcO(B*    rp, usz w, B x,    u8 type, u64 over) M_TYARR(0,+over, arr_shCopy((Arr*)r,x);, taga(r), usz ia = IA(x);)

// width in log2(bytes)
SHOULD_INLINE void* m_tyarrlp(Arr** rp, usz w, usz ia, u8 type) M_TYARR(1, , , r, )
SHOULD_INLINE void* m_tyarrlv(B*    rp, usz w, usz ia, u8 type) M_TYARR(1, , arr_shVec((Arr*)r);, taga(r), )
SHOULD_INLINE void* m_tyarrlc(B*    rp, usz w, B x,    u8 type) M_TYARR(1, , arr_shCopy((Arr*)r,x);, taga(r), usz ia = IA(x);)

// width in log2(bits)
SHOULD_INLINE void* m_tyarrlbp(Arr** rp, usz w, usz ia, u8 type) M_TYARR(2, , , r, )
SHOULD_INLINE void* m_tyarrlbv(B*    rp, usz w, usz ia, u8 type) M_TYARR(2, , arr_shVec((Arr*)r);, taga(r), )
SHOULD_INLINE void* m_tyarrlbc(B*    rp, usz w, B x,    u8 type) M_TYARR(2, , arr_shCopy((Arr*)r,x);, taga(r), usz ia = IA(x);)

extern u8 const elType2type[];
#define el2t(X) elType2type[X] // TODO maybe reorganize array types such that this can just be addition?
extern u8 const elTypeWidth[];
#define elWidth(X) elTypeWidth[X]
extern u8 const elwBitLogT[];
#define elwBitLog(X) elwBitLogT[X]
extern u8 const elwByteLogT[];
#define elwByteLog(X) elwByteLogT[X]
extern u8 const arrTypeWidthLog[];
#define arrTypeWidthLog(X) arrTypeWidthLog[X]
extern u8 const arrTypeBitsLog[];
#define arrTypeBitsLog(X) arrTypeBitsLog[X]
#define arrNewType(X) el2t(TIi(X,elType))

SHOULD_INLINE void arr_check_size(u64 sz, u8 type, u64 ia) {
  #if DEBUG
    assert(IS_ANY_ARR(type) || type==t_harrPartial);
    if (!IS_SLICE(type)) {
      if (type==t_harr || type==t_harrPartial) assert(sz >= fsizeof(HArr,a,B,ia));
      else assert(sz >= offsetof(TyArr,a) + (((ia<<arrTypeBitsLog(type))+7)>>3));
    }
  #endif
}
// Log of width in bits: max of 7, and also return 7 if not power of 2
SHOULD_INLINE u8 multWidthLog(usz n, u8 lw) { // Of n elements, 1<<lw bit
  if (n & (n-1)) return 7;      // Not power of 2
  return lw + CTZ(n | 128>>lw); // Max of 7; also handle n==0
}
SHOULD_INLINE u8 kCellWidthLog(B x, ur k) {
  assert(isArr(x) && RNK(x)>=1);
  u8 lw = arrTypeBitsLog(TY(x));
  ur xr = RNK(x);
  if (LIKELY(xr <= k)) return lw;
  return multWidthLog(shProd(SH(x), k, xr), lw);
}
SHOULD_INLINE u8 cellWidthLog(B x) { return kCellWidthLog(x, 1); }

static Arr* m_tyslice(void* data, Arr* parent, u8 type, ux ia) {
  assert(IS_ANY_ARR(type) && IS_SLICE(type));
  Arr* a = m_arr(sizeof(TySlice), type, ia);
  ((TySlice*) a)->p = parent;
  ((TySlice*) a)->a = data;
  return a;
}
