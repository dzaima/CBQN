#define T_ARR TP(t_,arr)
#define T_SLICE TP(t_,slice)
#define TEl JOIN(TU,Atom)
typedef TyArr JOIN(TU,Arr);
typedef TP(,) JOIN(TU,Atom);

SHOULD_INLINE B TP(m_,arrv) (TEl** p, u64 ia) {
  CHECK_IA(ia, sizeof(TEl));
  TyArr* r = m_arr(TYARR_SZ2(TU,ia), T_ARR, ia);
  arr_shVec((Arr*)r);
  *p = (TEl*)r->a;
  return taga(r);
}
SHOULD_INLINE B TP(m_,arrc) (TEl** p, B x) { assert(isArr(x));
  usz ia = IA(x);
  CHECK_IA(ia, sizeof(TEl));
  TyArr* r = m_arr(TYARR_SZ2(TU,ia), T_ARR, IA(x));
  *p = (TEl*)r->a;
  arr_shCopy((Arr*)r, x);
  return taga(r);
}
SHOULD_INLINE Arr* TP(m_,arrp) (TEl** p, u64 ia) {
  CHECK_IA(ia, sizeof(TEl));
  TyArr* r = m_arr(TYARR_SZ2(TU,ia), T_ARR, ia);
  *p = (TEl*)r->a;
  return (Arr*)r;
}

static TEl* TP(,anyv_ptr) (Arr* x) {
  u8 t = PTY(x);
  if (t == T_ARR) return (TEl*) ((TyArr*) x)->a;
  assert(t == T_SLICE);
  return (TEl*) ((TySlice*) x)->a;
}
static TEl* TP(,arrv_ptr) (TyArr* x) { return (TEl*) ((TyArr*) x)->a; }
static TEl* TP(,arr_ptr)  (B x) { VTY(x, T_ARR);    return TP(,arrv_ptr)((TyArr*) a(x)); }
static TEl* TP(,any_ptr)  (B x) { assert(isArr(x)); return TP(,anyv_ptr)(a(x)); }

#undef TEl
#undef TU
#undef TP
