B m_str8(usz sz, char* s);
B m_str8l(char* s);
B m_str32(u32* s);

static C8Arr* toC8Arr(B x) {
  if (v(x)->type==t_c8arr) return c(C8Arr,x);
  u8* rp; B r = m_c8arrc(&rp, x);
  usz ia = a(r)->ia;
  BS2B xgetU = TI(x,getU);
  for (usz i = 0; i < ia; i++) rp[i] = o2cu(xgetU(x,i));
  dec(x);
  return c(C8Arr,r);
}
static C16Arr* toC16Arr(B x) {
  if (v(x)->type==t_c16arr) return c(C16Arr,x);
  u16* rp; B r = m_c16arrc(&rp, x);
  usz ia = a(r)->ia;
  BS2B xgetU = TI(x,getU);
  for (usz i = 0; i < ia; i++) rp[i] = o2cu(xgetU(x,i));
  dec(x);
  return c(C16Arr,r);
}
static C32Arr* toC32Arr(B x) {
  if (v(x)->type==t_c32arr) return c(C32Arr,x);
  u32* rp; B r = m_c32arrc(&rp, x);
  usz ia = a(r)->ia;
  BS2B xgetU = TI(x,getU);
  for (usz i = 0; i < ia; i++) rp[i] = o2cu(xgetU(x,i));
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
