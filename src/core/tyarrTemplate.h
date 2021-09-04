#define T_ARR TP(t_,arr)
#define T_SLICE TP(t_,slice)
#define TArr JOIN(TU,Arr)
#define TSlice JOIN(TU,Slice)
#define TEl JOIN(TU,Atom)

typedef struct TArr {
  struct Arr;
  TEl a[];
} TArr;
typedef struct TSlice {
  struct Slice;
  TEl* a;
} TSlice;


static B TP(m_,arrv) (TEl** p, usz ia) {
  TArr* r = m_arr(TYARR_SZ2(TU,ia), T_ARR, ia);
  arr_shVec((Arr*)r);
  *p = r->a;
  return taga(r);
}
static B TP(m_,arrc) (TEl** p, B x) { assert(isArr(x));
  TArr* r = m_arr(TYARR_SZ2(TU,a(x)->ia), T_ARR, a(x)->ia);
  *p = r->a;
  arr_shCopy((Arr*)r, x);
  return taga(r);
}
static Arr* TP(m_,arrp) (TEl** p, usz ia) {
  TArr* r = m_arr(TYARR_SZ2(TU,ia), T_ARR, ia);
  *p = r->a;
  return (Arr*)r;
}

static TEl* TP(,arr_ptr) (B x) { VTY(x, T_ARR); return c(TArr,x)->a; }
static TEl* TP(,any_ptr) (B x) { assert(isArr(x)); u8 t=v(x)->type; if(t==T_ARR) return c(TArr,x)->a; assert(t==T_SLICE); return c(TSlice,x)->a; }

#undef TEl
#undef TSlice
#undef TArr
#undef TU
#undef TP
