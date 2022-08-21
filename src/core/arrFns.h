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
static void* tyarr_ptr(B x) { assert(IS_ARR(TY(x))); return c(TyArr,x)->a; }
static void* tyslice_ptr(B x) { assert(IS_SLICE(TY(x))); return c(TySlice,x)->a; }
static void* tyany_ptr(B x) { assert(IS_ARR(TY(x)) || IS_SLICE(TY(x)));
  u8 t = TY(x);
  return IS_SLICE(t)? c(TySlice,x)->a : c(TyArr,x)->a;
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
static void* m_tyarrp (Arr** rp, usz w, usz ia, u8 type          ) M_TYARR(0,     , , r, )
static void* m_tyarrpO(Arr** rp, usz w, usz ia, u8 type, usz over) M_TYARR(0,+over, , r, )
static void* m_tyarrv (B*    rp, usz w, usz ia, u8 type          ) M_TYARR(0,     , arr_shVec((Arr*)r);, taga(r), )
static void* m_tyarrvO(B*    rp, usz w, usz ia, u8 type, usz over) M_TYARR(0,+over, arr_shVec((Arr*)r);, taga(r), )
static void* m_tyarrc (B*    rp, usz w, B x,    u8 type          ) M_TYARR(0,     , arr_shCopy((Arr*)r,x);, taga(r), usz ia = IA(x);)
static void* m_tyarrcO(B*    rp, usz w, B x,    u8 type, usz over) M_TYARR(0,+over, arr_shCopy((Arr*)r,x);, taga(r), usz ia = IA(x);)

// width in log2(bytes)
static void* m_tyarrlp(Arr** rp, usz w, usz ia, u8 type) M_TYARR(1, , , r, )
static void* m_tyarrlv(B*    rp, usz w, usz ia, u8 type) M_TYARR(1, , arr_shVec((Arr*)r);, taga(r), )
static void* m_tyarrlc(B*    rp, usz w, B x,    u8 type) M_TYARR(1, , arr_shCopy((Arr*)r,x);, taga(r), usz ia = IA(x);)

// width in log2(bits)
static void* m_tyarrlbp(Arr** rp, usz w, usz ia, u8 type) M_TYARR(2, , , r, )
static void* m_tyarrlbv(B*    rp, usz w, usz ia, u8 type) M_TYARR(2, , arr_shVec((Arr*)r);, taga(r), )
static void* m_tyarrlbc(B*    rp, usz w, B x,    u8 type) M_TYARR(2, , arr_shCopy((Arr*)r,x);, taga(r), usz ia = IA(x);)

extern u8 elType2type[];
#define el2t(X) elType2type[X] // TODO maybe reorganize array types such that this can just be addition?
extern u8 elTypeWidth[];
#define elWidth(X) elTypeWidth[X]
extern u8 arrTypeWidthLog[];
#define arrTypeWidthLog(X) arrTypeWidthLog[X]