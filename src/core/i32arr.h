typedef struct I32Arr {
  struct Arr;
  i32 a[];
} I32Arr;
typedef struct I32Slice {
  struct Slice;
  i32* a;
} I32Slice;


static B m_i32arrv(i32** p, usz ia) {
  I32Arr* r = mm_alloc(fsizeof(I32Arr,a,i32,ia), t_i32arr);
  *p = r->a;
  arrP_shVec((Arr*)r, ia);
  return tag(r, ARR_TAG);
}
static B m_i32arrc(i32** p, B x) { assert(isArr(x));
  I32Arr* r = mm_alloc(fsizeof(I32Arr,a,i32,a(x)->ia), t_i32arr);
  *p = r->a;
  arrP_shCopy((Arr*)r, x);
  return tag(r, ARR_TAG);
}
static B m_i32arrp(i32** p, usz ia) { // doesn't write shape/rank
  I32Arr* r = mm_alloc(fsizeof(I32Arr,a,i32,ia), t_i32arr);
  *p = r->a;
  r->ia = ia;
  return tag(r, ARR_TAG);
}

B m_cai32(usz ia, i32* a);

static i32* i32arr_ptr(B x) { VTY(x, t_i32arr); return c(I32Arr,x)->a; }
static i32* i32any_ptr(B x) { assert(isArr(x)); u8 t=v(x)->type; if(t==t_i32arr) return c(I32Arr,x)->a; assert(t==t_i32slice); return c(I32Slice,x)->a; }

static I32Arr* toI32Arr(B x) { // assumes it's possible
  if (v(x)->type==t_i32arr) return c(I32Arr,x);
  i32* rp; B r = m_i32arrc(&rp, x);
  usz ia = a(r)->ia;
  if (TI(x).elType==el_f64) {
    f64* fp = f64any_ptr(x);
    for (usz i = 0; i < ia; i++) rp[i] = (i32)fp[i];
  } else {
    BS2B xgetU = TI(x).getU;
    for (usz i = 0; i < ia; i++) rp[i] = o2iu(xgetU(x,i));
  }
  dec(x);
  return c(I32Arr,r);
}
