B m_cai32(usz ia, i32* a);
B m_caf64(usz sz, f64* a);

static I8Arr* toI8Arr(B x) { // assumes it's possible
  if (v(x)->type==t_i8arr) return c(I8Arr,x);
  i8* rp; B r = m_i8arrc(&rp, x);
  usz ia = a(r)->ia;
  if (TI(x,elType)==el_i32) {
    i32* fp = i32any_ptr(x);
    for (usz i = 0; i < ia; i++) rp[i] = (i8)fp[i];
  } else {
    SGetU(x)
    for (usz i = 0; i < ia; i++) rp[i] = o2iu(GetU(x,i));
  }
  dec(x);
  return c(I8Arr,r);
}

static I16Arr* toI16Arr(B x) { // assumes it's possible
  if (v(x)->type==t_i16arr) return c(I16Arr,x);
  i16* rp; B r = m_i16arrc(&rp, x);
  usz ia = a(r)->ia;
  if (TI(x,elType)==el_f64) {
    f64* fp = f64any_ptr(x);
    for (usz i = 0; i < ia; i++) rp[i] = (i16)fp[i];
  } else {
    SGetU(x)
    for (usz i = 0; i < ia; i++) rp[i] = o2iu(GetU(x,i));
  }
  dec(x);
  return c(I16Arr,r);
}

static I32Arr* toI32Arr(B x) { // assumes it's possible
  if (v(x)->type==t_i32arr) return c(I32Arr,x);
  i32* rp; B r = m_i32arrc(&rp, x);
  usz ia = a(r)->ia;
  if (TI(x,elType)==el_f64) {
    f64* fp = f64any_ptr(x);
    for (usz i = 0; i < ia; i++) rp[i] = (i32)fp[i];
  } else {
    SGetU(x)
    for (usz i = 0; i < ia; i++) rp[i] = o2iu(GetU(x,i));
  }
  dec(x);
  return c(I32Arr,r);
}

static F64Arr* toF64Arr(B x) {
  if (v(x)->type==t_f64arr) return c(F64Arr,x);
  f64* rp; B r = m_f64arrc(&rp, x);
  usz ia = a(r)->ia;
  SGetU(x)
  for (usz i = 0; i < ia; i++) rp[i] = o2f(GetU(x,i));
  dec(x);
  return c(F64Arr,r);
}
