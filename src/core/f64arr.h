B m_caf64(usz sz, f64* a);

static F64Arr* toF64Arr(B x) {
  if (v(x)->type==t_f64arr) return c(F64Arr,x);
  f64* rp; B r = m_f64arrc(&rp, x);
  usz ia = a(r)->ia;
  SGetU(x)
  for (usz i = 0; i < ia; i++) rp[i] = o2f(GetU(x,i));
  dec(x);
  return c(F64Arr,r);
}
