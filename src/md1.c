#include "h.h"

B tbl_c1(B d, B x) { B f = c(Md1D,d)->f;
  if (!isArr(x)) thrM("⌜: argument cannot be an atom");
  usz ia = a(x)->ia;
  if (ia==0) return x;
  BS2B xget = TI(x).get;
  usz i = 0;
  B cr = c1(f, xget(x,0));
  HArr_p rH;
  if (TI(x).canStore(cr)) {
    bool reuse = reusable(x);
    if (v(x)->type==t_harr) {
      B* xp = harr_ptr(x);
      if (reuse) {
        dec(xp[i]); xp[i++] = cr;
        for (; i < ia; i++) xp[i] = c1(f, xp[i]);
        return x;
      } else {
        HArr_p rp = m_harrc(x);
        rp.a[i++] = cr;
        for (; i < ia; i++) rp.a[i] = c1(f, inc(xp[i]));
        dec(x);
        return rp.b;
      }
    } else if (v(x)->type==t_i32arr) {
      i32* xp = i32arr_ptr(x);
      B r = reuse? x : m_i32arrc(x);
      i32* rp = i32arr_ptr(r);
      rp[i++] = o2iu(cr);
      for (; i < ia; i++) {
        cr = c1(f, m_i32(xp[i]));
        if (!q_i32(cr)) {
          rH = m_harrc(x);
          for (usz j = 0; j < i; j++) rH.a[j] = m_i32(rp[j]);
          if (!reuse) dec(r);
          goto fallback;
        }
        rp[i] = o2iu(cr);
      }
      if (!reuse) dec(x);
      return r;
    } else if (v(x)->type==t_fillarr) {
      B* xp = fillarr_ptr(x);
      if (reuse) {
        dec(c(FillArr,x)->fill);
        c(FillArr,x)->fill = bi_noFill;
        dec(xp[i]); xp[i++] = cr;
        for (; i < ia; i++) xp[i] = c1(f, xp[i]);
        return x;
      } else {
        HArr_p rp = m_harrc(x);
        rp.a[i++] = cr;
        for (; i < ia; i++) rp.a[i] = c1(f, inc(xp[i]));
        dec(x);
        return rp.b;
      }
    } else
    rH = m_harrc(x);
  } else
  rH = m_harrc(x);
  fallback:
  rH.a[i++] = cr;
  for (; i < ia; i++) rH.a[i] = c1(f, xget(x,i));
  dec(x);
  return rH.b;
}
B tbl_c2(B d, B w, B x) { B f = c(Md1D,d)->f;
  if (isArr(w) & isArr(x)) {
    usz wia = a(w)->ia; ur wr = rnk(w);
    usz xia = a(x)->ia; ur xr = rnk(x);
    usz ria = wia*xia;  ur rr = wr+xr;
    if (rr<xr) thrM("⌜: required result rank too large");
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
        r.a[ri++] = c2(f, inc(cw), xget(x,xi));
      }
      dec(cw);
    }
    dec(w); dec(x);
    return r.b;
  } else thrM("⌜: 𝕨 and 𝕩 must be arrays");
}


B scan_c1(B d, B x) { B f = c(Md1D,d)->f;
  if (!isArr(x) || rnk(x)==0) thrM("`: argument cannot have rank 0");
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
  return r.b;
}
B scan_c2(B d, B w, B x) { B f = c(Md1D,d)->f;
  if (!isArr(x) || rnk(x)==0) thrM("`: 𝕩 cannot have rank 0");
  ur xr = rnk(x); usz* xsh = a(x)->sh; usz ia = a(x)->ia;
  bool reuse = v(x)->type==t_harr && reusable(x);
  HArr_p r = reuse? harr_parts(inc(x)) : m_harrc(x);
  BS2B xget = reuse? TI(x).getU : TI(x).get;
  if (isArr(w)) {
    ur wr = rnk(w); usz* wsh = a(w)->sh; BS2B wget = TI(w).get;
    if (wr+1 != xr) thrM("`: shape of 𝕨 must match the cell of 𝕩");
    if (memcmp(wsh, xsh+1, wr)) thrM("`: shape of 𝕨 must match the cell of 𝕩");
    if (ia==0) { ptr_dec(r.c); return x; } // only safe as r would have 0 items too
    usz csz = arr_csz(x);
    for (usz i = 0; i < csz; i++) r.a[i] = c2(f, wget(w,i), xget(x,i));
    for (usz i = csz; i < ia; i++) r.a[i] = c2(f, inc(r.a[i-csz]), xget(x,i));
    dec(w);
  } else {
    if (xr!=1) thrM("`: shape of 𝕨 must match the cell of 𝕩");
    if (ia==0) { ptr_dec(r.c); return x; }
    B pr = r.a[0] = c2(f, w, xget(x,0));
    for (usz i = 1; i < ia; i++) r.a[i] = pr = c2(f, inc(pr), xget(x,i));
  }
  dec(x);
  return r.b;
}


#define ba(NAME) bi_##NAME = mm_alloc(sizeof(Md1), t_md1_def, ftag(MD1_TAG)); c(Md1,bi_##NAME)->c2 = NAME##_c2; c(Md1,bi_##NAME)->c1 = NAME##_c1 ; c(Md1,bi_##NAME)->extra=pm1_##NAME; gc_add(bi_##NAME);
#define bd(NAME) bi_##NAME = mm_alloc(sizeof(Md1), t_md1_def, ftag(MD1_TAG)); c(Md1,bi_##NAME)->c2 = NAME##_c2; c(Md1,bi_##NAME)->c1 = c1_invalid; c(Md1,bi_##NAME)->extra=pm1_##NAME; gc_add(bi_##NAME);
#define bm(NAME) bi_##NAME = mm_alloc(sizeof(Md1), t_md1_def, ftag(MD1_TAG)); c(Md1,bi_##NAME)->c2 = c2_invalid;c(Md1,bi_##NAME)->c1 = NAME##_c1 ; c(Md1,bi_##NAME)->extra=pm1_##NAME; gc_add(bi_##NAME);

void print_md1_def(B x) { printf("%s", format_pm1(c(Md1,x)->extra)); }

B                 bi_tbl, bi_scan;
void md1_init() { ba(tbl) ba(scan)
  ti[t_md1_def].print = print_md1_def;
}

#undef ba
#undef bd
#undef bm
