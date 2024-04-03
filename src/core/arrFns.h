#pragma once

static B* arr_bptr(B x) { assert(isArr(x));
  if (TY(x)==t_harr) return harr_ptr(x);
  if (TY(x)==t_fillarr) return fillarr_ptr(a(x));
  if (TY(x)==t_hslice) return c(HSlice,x)->a;
  if (TY(x)==t_fillslice) return c(FillSlice,x)->a;
  return NULL;
}
static B* arrV_bptr(Arr* x) {
  if (PTY(x)==t_harr) return ((HArr*)x)->a;
  if (PTY(x)==t_fillarr) return fillarr_ptr(x);
  if (PTY(x)==t_hslice) return ((HSlice*)x)->a;
  if (PTY(x)==t_fillslice) return ((FillSlice*)x)->a;
  return NULL;
}
static void* tyarr_ptr(B x)   { assert(IS_ANY_ARR(TY(x)) && !IS_SLICE(TY(x))); return c(TyArr,x)->a; }
static void* tyslice_ptr(B x) { assert(IS_ANY_ARR(TY(x)) &&  IS_SLICE(TY(x))); return c(TySlice,x)->a; }
static void* tyany_ptr(B x) {
  assert(IS_ANY_ARR(TY(x)));
  return IS_SLICE(TY(x))? c(TySlice,x)->a : c(TyArr,x)->a;
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

// Log of width in bits: max of 7, and also return 7 if not power of 2
SHOULD_INLINE u8 cellWidthLog(B x) {
  assert(isArr(x) && RNK(x)>=1);
  u8 lw = arrTypeBitsLog(TY(x));
  if (LIKELY(RNK(x)==1)) return lw;
  usz csz = arr_csz(x);
  if (csz & (csz-1)) return 7;    // Not power of 2
  return lw + CTZ(csz | 128>>lw); // Max of 7; also handle csz==0
}
