#define T_ARR TP(t_,arr)
#define T_SLICE TP(t_,slice)
#define TEl JOIN(TU,Atom)

static Arr* TP(m_,slice) (Arr* p, TEl* ptr, usz ia) {
  TySlice* r = m_arr(sizeof(TySlice), T_SLICE, ia);
  r->p = p;
  r->a = ptr;
  return (Arr*)r;
}
static Arr* TP(,arr_slice)   (B x, usz s, usz ia) { return TP(m_,slice) (a(x), ((TEl*)c(TyArr,x)->a)+s, ia); }
static Arr* TP(,slice_slice) (B x, usz s, usz ia) { Arr* p = ptr_inc(c(Slice,x)->p); Arr* r = TP(m_,slice) (p, ((TEl*)c(TySlice,x)->a)+s, ia); decG(x); return r; }

static B TP(,arr_get)   (Arr* x, usz n) { assert(PTY(x)==T_ARR  ); return TP(m_,) (((TEl*)((TyArr*  )x)->a)[n]); }
static B TP(,slice_get) (Arr* x, usz n) { assert(PTY(x)==T_SLICE); return TP(m_,) (((TEl*)((TySlice*)x)->a)[n]); }
static bool TP(,arr_canStore) (B x) { return TP(q_,) (x); }

static void TP(,arr_init)() {
  TIi(T_ARR,get)   = TP(,arr_get);   TIi(T_SLICE,get)   = TP(,slice_get);
  TIi(T_ARR,getU)  = TP(,arr_get);   TIi(T_SLICE,getU)  = TP(,slice_get);
  TIi(T_ARR,slice) = TP(,arr_slice); TIi(T_SLICE,slice) = TP(,slice_slice);
  TIi(T_ARR,freeO) = tyarr_freeO;    TIi(T_SLICE,freeO) =     slice_freeO; // typed array slice frees calling the generic function is relied on by mmap & •bit._cast
  TIi(T_ARR,freeF) = tyarr_freeF;    TIi(T_SLICE,freeF) =     slice_freeF;
  TIi(T_ARR,visit) =  arr_visit;     TIi(T_SLICE,visit) =     slice_visit;
  TIi(T_ARR,print) = farr_print;     TIi(T_SLICE,print) = farr_print;
  TIi(T_ARR,isArr) = true;           TIi(T_SLICE,isArr) = true;
  TIi(T_ARR,arrD1) = true;           TIi(T_SLICE,arrD1) = true;
  TIi(T_ARR,elType) = TP(el_,);      TIi(T_SLICE,elType) = TP(el_,);
  TIi(T_ARR,canStore) = TP(,arr_canStore);
}

#undef TEl
#undef TU
#undef TP
