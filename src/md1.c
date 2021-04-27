#include "h.h"

B tbl_c1(B d, B x) { B f = c(Md1D,d)->f;
  return eachm(f, x);
}
B tbl_c2(B d, B w, B x) { B f = c(Md1D,d)->f;
  if (isAtm(w)) w = m_hunit(w);
  if (isAtm(x)) x = m_hunit(x);
  usz wia = a(w)->ia; ur wr = rnk(w);
  usz xia = a(x)->ia; ur xr = rnk(x);
  usz ria = wia*xia;  ur rr = wr+xr;
  if (rr<xr) thrM("⌜: Required result rank too large");
  HArr_p r = m_harrp(ria);
  usz* rsh = arr_shAllocR(r.b, rr);
  if (rsh) {
    memcpy(rsh   , a(w)->sh, wr*sizeof(usz));
    memcpy(rsh+wr, a(x)->sh, xr*sizeof(usz));
  }
  
  BS2B wgetU = TI(w).getU;
  BS2B xget = TI(x).get;
  usz ri = 0;
  for (usz wi = 0; wi < wia; wi++) {
    B cw = wgetU(w,wi);
    for (usz xi = 0; xi < xia; xi++) {
      r.a[ri++] = c2(f, inc(cw), xget(x,xi));
    }
  }
  dec(w); dec(x);
  return r.b;
}

B each_c1(B d, B x) { B f = c(Md1D,d)->f;
  return eachm(f, x);
}
B each_c2(B d, B w, B x) { B f = c(Md1D,d)->f;
  return eachd(f, w, x);
}


B scan_c1(B d, B x) { B f = c(Md1D,d)->f;
  if (!isArr(x) || rnk(x)==0) thrM("`: Argument cannot have rank 0");
  B xf = getFill(inc(x));
  ur xr = rnk(x);
  usz ia = a(x)->ia;
  if (ia==0) return x;
  bool reuse = v(x)->type==t_harr && reusable(x);
  HArr_p r = reuse? harr_parts(inc(x)) : m_harrc(x);
  BS2B xget = reuse? TI(x).getU : TI(x).get;
  if (xr==1) {
    r.a[0] = xget(x,0);
    for (usz i = 1; i < ia; i++) r.a[i] = c2(f, inc(r.a[i-1]), xget(x,i));
  } else {
    usz csz = arr_csz(x);
    for (usz i = 0; i < csz; i++) r.a[i] = xget(x,i);
    for (usz i = csz; i < ia; i++) r.a[i] = c2(f, inc(r.a[i-csz]), xget(x,i));
  }
  dec(x);
  return withFill(r.b, xf);
}
B scan_c2(B d, B w, B x) { B f = c(Md1D,d)->f;
  if (!isArr(x) || rnk(x)==0) thrM("`: 𝕩 cannot have rank 0");
  ur xr = rnk(x); usz* xsh = a(x)->sh; usz ia = a(x)->ia;
  bool reuse = v(x)->type==t_harr && reusable(x);
  HArr_p r = reuse? harr_parts(inc(x)) : m_harrc(x);
  BS2B xget = reuse? TI(x).getU : TI(x).get;
  if (isArr(w)) {
    ur wr = rnk(w); usz* wsh = a(w)->sh; BS2B wget = TI(w).get;
    if (wr+1 != xr) thrM("`: Shape of 𝕨 must match the cell of 𝕩");
    if (memcmp(wsh, xsh+1, wr)) thrM("`: Shape of 𝕨 must match the cell of 𝕩");
    if (ia==0) { ptr_dec(r.c); return x; } // only safe as r would have 0 items too
    usz csz = arr_csz(x);
    for (usz i = 0; i < csz; i++) r.a[i] = c2(f, wget(w,i), xget(x,i));
    for (usz i = csz; i < ia; i++) r.a[i] = c2(f, inc(r.a[i-csz]), xget(x,i));
    dec(w);
  } else {
    if (xr!=1) thrM("`: Shape of 𝕨 must match the cell of 𝕩");
    if (ia==0) { ptr_dec(r.c); return x; }
    B pr = r.a[0] = c2(f, w, xget(x,0));
    for (usz i = 1; i < ia; i++) r.a[i] = pr = c2(f, inc(pr), xget(x,i));
  }
  dec(x);
  return r.b;
}

B fold_c1(B d, B x) { B f = c(Md1D,d)->f;
  if (!isArr(x) || rnk(x)!=1) thrM("´: argument must be a list");
  usz ia = a(x)->ia;
  if (ia==0) {
    dec(x);
    if (isFun(f)) {
      B r = TI(f).identity(f);
      if (!isNothing(r)) return inc(r);
    }
    thrM("´: No identity found");
  }
  BS2B xget = TI(x).get;
  B c = xget(x, ia-1);
  for (usz i = ia-1; i>0; i--) c = c2(f, xget(x, i-1), c);
  dec(x);
  return c;
}
B fold_c2(B d, B w, B x) { B f = c(Md1D,d)->f;
  if (!isArr(x) || rnk(x)!=1) thrM("´: 𝕩 must be a list");
  usz ia = a(x)->ia;
  B c = w;
  BS2B xget = TI(x).get;
  for (usz i = ia; i>0; i--) c = c2(f, xget(x, i-1), c);
  dec(x);
  return c;
}


#define ba(NAME) bi_##NAME = mm_alloc(sizeof(Md1), t_md1BI, ftag(MD1_TAG)); c(Md1,bi_##NAME)->c2 = NAME##_c2; c(Md1,bi_##NAME)->c1 = NAME##_c1 ; c(Md1,bi_##NAME)->extra=pm1_##NAME; gc_add(bi_##NAME);
#define bd(NAME) bi_##NAME = mm_alloc(sizeof(Md1), t_md1BI, ftag(MD1_TAG)); c(Md1,bi_##NAME)->c2 = NAME##_c2; c(Md1,bi_##NAME)->c1 = c1_invalid; c(Md1,bi_##NAME)->extra=pm1_##NAME; gc_add(bi_##NAME);
#define bm(NAME) bi_##NAME = mm_alloc(sizeof(Md1), t_md1BI, ftag(MD1_TAG)); c(Md1,bi_##NAME)->c2 = c2_invalid;c(Md1,bi_##NAME)->c1 = NAME##_c1 ; c(Md1,bi_##NAME)->extra=pm1_##NAME; gc_add(bi_##NAME);

void print_md1_def(B x) { printf("%s", format_pm1(c(Md1,x)->extra)); }

B                               bi_tbl, bi_each, bi_fold, bi_scan;
static inline void md1_init() { ba(tbl) ba(each) ba(fold) ba(scan)
  ti[t_md1BI].print = print_md1_def;
}

#undef ba
#undef bd
#undef bm
