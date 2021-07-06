typedef struct F64Arr {
  struct Arr;
  f64 a[];
} F64Arr;
typedef struct F64Slice {
  struct Slice;
  f64* a;
} F64Slice;


static B m_f64arrv(f64** p, usz ia) {
  F64Arr* r = mm_alloc(fsizeof(F64Arr,a,f64,ia), t_f64arr);
  *p = r->a;
  arr_shVec((Arr*)r, ia);
  return taga(r);
}
static B m_f64arrc(f64** p, B x) { assert(isArr(x));
  F64Arr* r = mm_alloc(fsizeof(F64Arr,a,f64,a(x)->ia), t_f64arr);
  *p = r->a;
  arr_shCopy((Arr*)r, x);
  return taga(r);
}
static Arr* m_f64arrp(f64** p, usz ia) { // doesn't write shape/rank
  F64Arr* r = mm_alloc(fsizeof(F64Arr,a,f64,ia), t_f64arr);
  *p = r->a;
  r->ia = ia;
  return (Arr*)r;
}

B m_caf64(usz sz, f64* a);

static f64* f64arr_ptr(B x) { VTY(x, t_f64arr); return c(F64Arr,x)->a; }
static f64* f64any_ptr(B x) { assert(isArr(x)); u8 t=v(x)->type; if(t==t_f64arr) return c(F64Arr,x)->a; assert(t==t_f64slice); return c(F64Slice,x)->a; }

static F64Arr* toF64Arr(B x) {
  if (v(x)->type==t_f64arr) return c(F64Arr,x);
  f64* rp; B r = m_f64arrc(&rp, x);
  usz ia = a(r)->ia;
  BS2B xgetU = TI(x,getU);
  for (usz i = 0; i < ia; i++) rp[i] = o2f(xgetU(x,i));
  dec(x);
  return c(F64Arr,r);
}
