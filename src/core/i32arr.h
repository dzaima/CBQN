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
