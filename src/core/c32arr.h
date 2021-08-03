typedef struct C32Arr {
  struct Arr;
  u32 a[];
} C32Arr;
typedef struct C32Slice {
  struct Slice;
  u32* a;
} C32Slice;


#define C32A_SZ(IA) fsizeof(F64Arr,a,f64,IA)
static B m_c32arrv(u32** p, usz ia) {
  C32Arr* r = m_arr(C32A_SZ(ia), t_c32arr, ia);
  arr_shVec((Arr*)r);
  *p = r->a;
  return taga(r);
}
static B m_c32arrc(u32** p, B x) { assert(isArr(x));
  C32Arr* r = m_arr(C32A_SZ(a(x)->ia), t_c32arr, a(x)->ia);
  *p = r->a;
  arr_shCopy((Arr*)r, x);
  return taga(r);
}
static Arr* m_c32arrp(u32** p, usz ia) {
  C32Arr* r = m_arr(C32A_SZ(ia), t_c32arr, ia);
  *p = r->a;
  return (Arr*)r;
}

B m_str8(usz sz, char* s);
B m_str8l(char* s);
B m_str32(u32* s);

static u32* c32arr_ptr(B x) { VTY(x, t_c32arr); return c(C32Arr,x)->a; }
static u32* c32any_ptr(B x) { assert(isArr(x)); u8 t=v(x)->type; if(t==t_c32arr) return c(C32Arr,x)->a; assert(t==t_c32slice); return c(C32Slice,x)->a; }


static C32Arr* toC32Arr(B x) {
  if (v(x)->type==t_c32arr) return c(C32Arr,x);
  u32* rp; B r = m_c32arrc(&rp, x);
  usz ia = a(r)->ia;
  BS2B xgetU = TI(x,getU);
  for (usz i = 0; i < ia; i++) rp[i] = o2c(xgetU(x,i));
  dec(x);
  return c(C32Arr,r);
}
static bool eqStr(B w, u32* x) {
  if (isAtm(w) || rnk(w)!=1) return false;
  BS2B wgetU = TI(w,getU);
  u64 i = 0;
  while (x[i]) {
    B c = wgetU(w, i);
    if (!isC32(c) || x[i]!=(u32)c.u) return false;
    i++;
  }
  return i==a(w)->ia;
}
