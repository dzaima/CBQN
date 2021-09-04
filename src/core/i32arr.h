typedef struct I32Arr {
  struct Arr;
  i32 a[];
} I32Arr;
typedef struct I32Slice {
  struct Slice;
  i32* a;
} I32Slice;


#define I32A_SZ(IA) fsizeof(F64Arr,a,i32,IA)
static B m_i32arrv(i32** p, usz ia) {
  I32Arr* r = m_arr(I32A_SZ(ia), t_i32arr, ia);
  arr_shVec((Arr*)r);
  *p = r->a;
  return taga(r);
}
static B m_i32arrc(i32** p, B x) { assert(isArr(x));
  I32Arr* r = m_arr(I32A_SZ(a(x)->ia), t_i32arr, a(x)->ia);
  *p = r->a;
  arr_shCopy((Arr*)r, x);
  return taga(r);
}
static Arr* m_i32arrp(i32** p, usz ia) {
  I32Arr* r = m_arr(I32A_SZ(ia), t_i32arr, ia);
  *p = r->a;
  return (Arr*)r;
}

static i32* i32arr_ptr(B x) { VTY(x, t_i32arr); return c(I32Arr,x)->a; }
static i32* i32any_ptr(B x) { assert(isArr(x)); u8 t=v(x)->type; if(t==t_i32arr) return c(I32Arr,x)->a; assert(t==t_i32slice); return c(I32Slice,x)->a; }

B m_cai32(usz ia, i32* a);

static I32Arr* toI32Arr(B x) { // assumes it's possible
  if (v(x)->type==t_i32arr) return c(I32Arr,x);
  i32* rp; B r = m_i32arrc(&rp, x);
  usz ia = a(r)->ia;
  if (TI(x,elType)==el_f64) {
    f64* fp = f64any_ptr(x);
    for (usz i = 0; i < ia; i++) rp[i] = (i32)fp[i];
  } else {
    BS2B xgetU = TI(x,getU);
    for (usz i = 0; i < ia; i++) rp[i] = o2iu(xgetU(x,i));
  }
  dec(x);
  return c(I32Arr,r);
}
