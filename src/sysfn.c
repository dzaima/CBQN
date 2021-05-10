#include "h.h"

B type_c1(B t, B x) {
  i32 r = -1;
       if (isArr(x)) r = 0;
  else if (isI32(x)) r = 1;
  else if (isF64(x)) r = 1;
  else if (isC32(x)) r = 2;
  else if (isFun(x)) r = 3;
  else if (isMd1(x)) r = 4;
  else if (isMd2(x)) r = 5;
  if (r==-1) { print(x); err(": getting type"); }
  decR(x);
  return m_i32(r);
}

B decp_c1(B t, B x) {
  if (!isVal(x)) return m_v2(m_i32(-1), x);
  if (v(x)->flags) return m_v2(m_i32(0), x);
  return TI(x).decompose(x);
}

usz runtimeLen;
B primInd_c1(B t, B x) {
  if (!isVal(x)) return m_i32(runtimeLen);
  if (v(x)->flags) { B r = m_i32(v(x)->flags-1); dec(x); return r; }
  dec(x);
  return m_i32(runtimeLen);
}

B glyph_c1(B t, B x) {
  return x;
}

B fill_c1(B t, B x) {
  B r = getFillE(x);
  dec(x);
  return r;
}
B fill_c2(B t, B w, B x) { // TODO not set fill for typed arrays
  if (isArr(x)) {
    B fill = asFill(w);
    if (noFill(fill)) return x;
    return withFill(x, fill);
  }
  dec(w);
  return x;
}

B grLen_c1(B t, B x) { // assumes valid arguments
  i64 ria = -1;
  usz ia = a(x)->ia;
  BS2B xgetU = TI(x).getU;
  for (usz i = 0; i < ia; i++) {
    i64 c = o2i64u(xgetU(x, i));
    if (c>ria) ria = c;
  }
  ria++;
  B r = m_i32arrv(ria); i32* rp = i32arr_ptr(r);
  for (usz i = 0; i < ria; i++) rp[i] = 0;
  for (usz i = 0; i < ia; i++) {
    i64 n = o2i64u(xgetU(x, i));
    if (n>USZ_MAX) thrM("grLen: Bad item in 𝕩");
    else if (n>=0) rp[n]++;
  }
  dec(x);
  return r;
}
B grLen_c2(B t, B w, B x) { // assumes valid arguments
  i64 ria = o2i64u(w)-1;
  usz ia = a(x)->ia;
  BS2B xgetU = TI(x).getU;
  for (usz i = 0; i < ia; i++) {
    i64 c = o2i64u(xgetU(x, i));
    if (c>ria) ria = c;
  }
  ria++;
  B r = m_i32arrv(ria); i32* rp = i32arr_ptr(r);
  for (usz i = 0; i < ria; i++) rp[i] = 0;
  for (usz i = 0; i < ia; i++) {
    i64 n = o2i64u(xgetU(x, i));
    if (n==(usz)n) rp[n]++;
    else if (n!=-1) thrM("grLen: Too large");
  }
  dec(x);
  return r;
}

B grOrd_c2(B t, B w, B x) { // assumes valid arguments
  usz wia = a(w)->ia;
  usz xia = a(x)->ia;
  if (wia==0) { dec(w); dec(x); return c1(bi_ud, m_i32(0)); }
  if (xia==0) { dec(w); return x; }
  BS2B wgetU = TI(w).getU;
  BS2B xgetU = TI(x).getU;
  usz tmp[wia];
  tmp[0] = 0;
  for (usz i = 1; i < wia; i++) tmp[i] = tmp[i-1]+o2su(wgetU(w,i-1));
  usz ria = tmp[wia-1]+o2su(wgetU(w,wia-1));
  B r = m_i32arrv(ria); i32* rp = i32arr_ptr(r);
  if (xia>=I32_MAX) thrM("grOrd: Too large");
  for (usz i = 0; i < xia; i++) {
    i64 c = o2i64(xgetU(x,i));
    if (c>=0) rp[tmp[c]++] = i;
  }
  dec(w);
  dec(x);
  return r;
}

B asrt_c1(B t, B x) {
  if (isI32(x) && 1==(i32)x.u) return x;
  if (isF64(x) && 1==x.f) return x;
  dec(x);
  thrM("assertion error");
}
B asrt_c2(B t, B w, B x) {
  if (isI32(x) && 1==(u32)x.u) { dec(w); return x; }
  if (isF64(x) && 1==x.f) { dec(w); return x; }
  dec(x);
  thr(w);
}

bool isPureFn(B x);
B internal_c2(B t, B w, B x) {
  B r;
  u64 id = o2s(w);
  if(id==0) {
    if(isVal(x)) { char* c = format_type(v(x)->type); r = m_str8(strlen(c), c); }
    else {
      if      (isF64(x)) r = m_str32(U"tagged f64");
      else if (isI32(x)) r = m_str32(U"tagged i32");
      else if (isC32(x)) r = m_str32(U"tagged c32");
      else if (isTag(x)) r = m_str32(U"tagged tag");
      else if (isVar(x)) r = m_str32(U"tagged var");
      else r = m_str32(U"tagged unknown");
    }
  } else if(id==1) { r = isVal(x)? m_i32(v(x)->mmInfo & 0x7f) : m_str32(U"(not heap-allocated)"); }
  else if(id==2) { r = isVal(x)? m_i32(v(x)->refc) : m_str32(U"(not heap-allocated)"); }
  else if(id==3) { printf("%p\n", (void*)x.u); r = inc(x); }
  else if(id==4) { r = m_f64(isPureFn(x)); }
  else { dec(x); thrM("Bad 𝕨 argument for •Internal"); }
  dec(x);
  return r;
}
B sys_c1(B t, B x);
B  out_c1(B t, B x) { printRaw(x); putchar('\n'); return x; }
B show_c1(B t, B x) { print   (x); putchar('\n'); return x; }

B bqn_c1(B t, B x) {
  if (isAtm(x) || rnk(x)!=1) thrM("•BQN: Argument must be a character vector");
  if (a(x)->type!=t_c32arr && a(x)->type!=t_c32slice) {
    usz ia = a(x)->ia;
    BS2B xgetU = TI(x).getU;
    for (usz i = 0; i < ia; i++) if (!isC32(xgetU(x,i))) thrM("•BQN: Argument must be a character vector");
  }
  return bqn_exec(x);
}

B cmp_c2(B t, B w, B x) {
  B r = m_i32(compare(w, x));
  dec(w); dec(x);
  return r;
}

#define F(A,M,D) M(type) M(decp) M(primInd) M(glyph) A(fill) A(grLen) D(grOrd) A(asrt) M(out) M(show) M(sys) M(bqn) D(cmp) D(internal)
BI_FNS0(F);
static inline void sysfn_init() { BI_FNS1(F) }
#undef F

B bi_timed;
B sys_c1(B t, B x) {
  assert(isArr(x));
  usz i = 0;
  HArr_p r = m_harrs(a(x)->ia, &i);
  BS2B xgetU = TI(x).getU;
  for (; i < a(x)->ia; i++) {
    B c = xgetU(x,i);
    if (eqStr(c, U"out")) r.a[i] = inc(bi_out);
    else if (eqStr(c, U"show")) r.a[i] = inc(bi_show);
    else if (eqStr(c, U"internal")) r.a[i] = inc(bi_internal);
    else if (eqStr(c, U"type")) r.a[i] = inc(bi_type);
    else if (eqStr(c, U"decompose")) r.a[i] = inc(bi_decp);
    else if (eqStr(c, U"primind")) r.a[i] = inc(bi_primInd);
    else if (eqStr(c, U"bqn")) r.a[i] = inc(bi_bqn);
    else if (eqStr(c, U"cmp")) r.a[i] = inc(bi_cmp);
    else if (eqStr(c, U"timed")) r.a[i] = inc(bi_timed);
    else { dec(x); thrM("Unknown system function"); }
  }
  return harr_fcd(r, x);
}
