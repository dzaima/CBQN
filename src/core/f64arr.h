typedef struct F64Arr {
  struct Arr;
  f64 a[];
} F64Arr;
typedef struct F64Slice {
  struct Slice;
  f64* a;
} F64Slice;


#define F64A_SZ(IA) fsizeof(F64Arr,a,f64,IA)
static B m_f64arrv(f64** p, usz ia) {
  F64Arr* r = m_arr(F64A_SZ(ia), t_f64arr, ia);
  arr_shVec((Arr*)r);
  *p = r->a;
  return taga(r);
}
static B m_f64arrc(f64** p, B x) { assert(isArr(x));
  F64Arr* r = m_arr(F64A_SZ(a(x)->ia), t_f64arr, a(x)->ia);
  *p = r->a;
  arr_shCopy((Arr*)r, x);
  return taga(r);
}
static Arr* m_f64arrp(f64** p, usz ia) {
  F64Arr* r = m_arr(F64A_SZ(ia), t_f64arr, ia);
  *p = r->a;
  return (Arr*)r;
}

static f64* f64arr_ptr(B x) { VTY(x, t_f64arr); return c(F64Arr,x)->a; }
static f64* f64any_ptr(B x) { assert(isArr(x)); u8 t=v(x)->type; if(t==t_f64arr) return c(F64Arr,x)->a; assert(t==t_f64slice); return c(F64Slice,x)->a; }

B m_caf64(usz sz, f64* a);

static F64Arr* toF64Arr(B x) {
  if (v(x)->type==t_f64arr) return c(F64Arr,x);
  f64* rp; B r = m_f64arrc(&rp, x);
  usz ia = a(r)->ia;
  BS2B xgetU = TI(x,getU);
  for (usz i = 0; i < ia; i++) rp[i] = o2f(xgetU(x,i));
  dec(x);
  return c(F64Arr,r);
}
