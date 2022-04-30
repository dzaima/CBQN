#pragma once

static B* arr_bptr(B x) { assert(isArr(x));
  if (v(x)->type==t_harr) return harr_ptr(x);
  if (v(x)->type==t_fillarr) return fillarr_ptr(a(x));
  if (v(x)->type==t_hslice) return c(HSlice,x)->a;
  if (v(x)->type==t_fillslice) return c(FillSlice,x)->a;
  return NULL;
}
static B* arrV_bptr(Arr* x) {
  if (x->type==t_harr) return ((HArr*)x)->a;
  if (x->type==t_fillarr) return fillarr_ptr(x);
  if (x->type==t_hslice) return ((HSlice*)x)->a;
  if (x->type==t_fillslice) return ((FillSlice*)x)->a;
  return NULL;
}
static void* tyarr_ptr(B x) { return c(TyArr,x)->a; }
static void* tyany_ptr(B x) {
  u8 t = v(x)->type;
  return IS_SLICE(t)? c(TySlice,x)->a : c(TyArr,x)->a;
}

#define M_TYARR(OVER, MID, RV) { \
  Arr* r = m_arr(TYARR_SZW(w, ia) OVER, type, ia); \
  MID                    \
  *rp = RV;              \
  return ((TyArr*)r)->a; \
}
// width in bytes for m_tyarr*; overalloc is a byte count
static void* m_tyarrp (Arr** rp, usz w, usz ia, u8 type          ) M_TYARR(     , , r)
static void* m_tyarrpO(Arr** rp, usz w, usz ia, u8 type, usz over) M_TYARR(+over, , r)
static void* m_tyarrv (B*    rp, usz w, usz ia, u8 type          ) M_TYARR(     , arr_shVec((Arr*)r);, taga(r))
static void* m_tyarrvO(B*    rp, usz w, usz ia, u8 type, usz over) M_TYARR(+over, arr_shVec((Arr*)r);, taga(r))

extern u8 elType2type[];
#define el2t(X) elType2type[X] // TODO maybe reorganize array types such that this can just be addition?