#include "h.h"

B tbl_c1(B t, B f, B x) {
  if (!isArr(x)) return err("⌜: argument was atom");
  usz ia = a(x)->ia;
  if (v(x)->type==t_harr && reusable(x)) {
    B* p = harr_ptr(x);
    for (usz i = 0; i < ia; i++) p[i] = c1(f, p[i]);
    return x;
  }
  HArr_p r = m_harrc(x);
  BS2B xget = TI(x).get;
  for (usz i = 0; i < ia; i++) r.a[i] = c1(f, xget(x,i));
  dec(x);
  return r.b;
}
B tbl_c2(B t, B f, B w, B x) {
  if (isArr(w) & isArr(x)) {
    usz wia = a(w)->ia; ur wr = a(w)->rank;
    usz xia = a(x)->ia; ur xr = a(x)->rank;
    usz ria = wia*xia;  ur rr = wr+xr;
    if (rr<xr) return err("⌜: result rank too large");
    HArr_p r = m_harrp(ria);
    usz* rsh = arr_shAlloc(r.b, ria, rr);
    if (rsh) {
      memcpy(rsh   , a(w)->sh, wr*sizeof(usz));
      memcpy(rsh+wr, a(x)->sh, xr*sizeof(usz));
    }
    
    BS2B wget = TI(w).get;
    BS2B xget = TI(x).get;
    usz ri = 0;
    for (usz wi = 0; wi < wia; wi++) {
      B cw = wget(w,wi);
      for (usz xi = 0; xi < xia; xi++) {
        r.a[ri++] = c2(f, inci(cw), xget(x,xi));
      }
      dec(cw);
    }
    dec(w); dec(x);
    return r.b;
  } else return err("⌜: one argument was an atom");
}


B scan_c1(B t, B f, B x) {
  if (!isArr(x)) return err("`: argument cannot be a scalar");
  ur xr = a(x)->rank;
  if (xr==0) return err("`: argument cannot be a scalar");
  HArr_p r = (v(x)->type==t_harr && reusable(x))? harr_parts(inci(x)) : m_harrc(x);
  usz ia = r.c->ia;
  if (ia==0) { dec(x); return r.b; }
  BS2B xget = TI(x).get;
  if (xr==1) {
    r.a[0] = xget(x,0);
    for (usz i = 1; i < ia; i++) r.a[i] = c2(f, inci(r.a[i-1]), xget(x,i));
  } else {
    usz csz = arr_csz(x);
    for (usz i = 0; i < csz; i++) r.a[i] = xget(x,i);
    for (usz i = csz; i < ia; i++) r.a[i] = c2(f, inci(r.a[i-csz]), xget(x,i));
  }
  dec(x);
  return r.b;
}
B scan_c2(B t, B f, B w, B x) {
  if (!isArr(x)) return err("`: 𝕩 cannot be a scalar");
  ur xr = a(x)->rank; usz* xsh = a(x)->sh; BS2B xget = TI(x).get;
  HArr_p r = (v(x)->type==t_harr && reusable(x))? harr_parts(inci(x)) : m_harrc(x);
  usz ia = r.c->ia;
  if (isArr(w)) {
    ur wr = a(w)->rank; usz* wsh = a(w)->sh; BS2B wget = TI(w).get;
    if (xr==0) return err("`: 𝕩 cannot be a scalar");
    if (wr+1 != xr) return err("`: shape of 𝕨 must match the cell of 𝕩");
    if (memcmp(wsh, xsh+1, wr)) return err("`: shape of 𝕨 must match the cell of 𝕩");
    if (ia==0) { dec(x); return r.b; }
    usz csz = arr_csz(x);
    for (usz i = 0; i < csz; i++) r.a[i] = c2(f, wget(w,i), xget(x,i));
    for (usz i = csz; i < ia; i++) r.a[i] = c2(f, inci(r.a[i-csz]), xget(x,i));
    dec(w);
  } else {
    if (xr!=1) return err("`: if 𝕨 is scalar, 𝕩 must be a vector");
    if (ia==0) { dec(x); return r.b; }
    B pr = r.a[0] = c2(f, w, xget(x,0));
    for (usz i = 1; i < ia; i++) r.a[i] = pr = c2(f, inci(pr), xget(x,i));
  }
  dec(x);
  return r.b;
}


#define ba(NAME) bi_##NAME = mm_alloc(sizeof(Md1), t_md1_def, ftag(MD1_TAG)); c(Md1,bi_##NAME)->c2 = NAME##_c2; c(Md1,bi_##NAME)->c1 = NAME##_c1 ; c(Md1,bi_##NAME)->id=pm1_##NAME;
#define bd(NAME) bi_##NAME = mm_alloc(sizeof(Md1), t_md1_def, ftag(MD1_TAG)); c(Md1,bi_##NAME)->c2 = NAME##_c2; c(Md1,bi_##NAME)->c1 = c1_invalid; c(Md1,bi_##NAME)->id=pm1_##NAME;
#define bm(NAME) bi_##NAME = mm_alloc(sizeof(Md1), t_md1_def, ftag(MD1_TAG)); c(Md1,bi_##NAME)->c2 = c2_invalid;c(Md1,bi_##NAME)->c1 = NAME##_c1 ; c(Md1,bi_##NAME)->id=pm1_##NAME;

void print_md1_def(B x) { printf("%s", format_pm1(c(Md1,x)->id)); }

B                 bi_tbl, bi_scan;
void md1_init() { ba(tbl) ba(scan)
  ti[t_md1_def].print = print_md1_def;
}

#undef ba
#undef bd
#undef bm