#include "h.h"

static inline B  mv(B*     p, usz n) { B r = p  [n]; p  [n] = m_f64(0); return r; }
static inline B hmv(HArr_p p, usz n) { B r = p.a[n]; p.a[n] = m_f64(0); return r; }
B eachd_fn(BBB2B f, B fo, B w, B x) { // consumes w,x; assumes at least one is array
  if (isAtm(w)) w = m_atomUnit(w);
  if (isAtm(x)) x = m_atomUnit(x);
  ur wr = rnk(w); BS2B wget = TI(w).get;
  ur xr = rnk(x); BS2B xget = TI(x).get;
  bool wg = wr>xr;
  ur rM = wg? wr : xr;
  ur rm = wg? xr : wr;
  if (rM==0) {
    B r = f(fo, wget(w,0), xget(x,0));
    dec(w); dec(x);
    return m_hunit(r);
  }
  if (rm && !eqShPrefix(a(w)->sh, a(x)->sh, rm)) thrM("Mapping: Expected equal shape prefix");
  bool rw = rM==wr && ((v(w)->type==t_harr) & reusable(w)); // v(…) is safe as rank>0
  bool rx = rM==xr && ((v(x)->type==t_harr) & reusable(x));
  if (rw|rx && (wr==xr | rm==0)) {
    HArr_p r = harr_parts(rw? w : x);
    usz ria = r.c->ia;
    if      (wr==0) { B c=wget(w, 0); for(usz i = 0; i < ria; i++) r.a[i] = f(fo, inc(c),   hmv(r,i)); dec(c); }
    else if (xr==0) { B c=xget(x, 0); for(usz i = 0; i < ria; i++) r.a[i] = f(fo, hmv(r,i), inc(c)  ); dec(c); }
    else {
      assert(wr==xr);
      if (rw) for (usz i = 0; i < ria; i++) r.a[i] = f(fo, hmv(r,i),  xget(x,i));
      else    for (usz i = 0; i < ria; i++) r.a[i] = f(fo, wget(w,i), hmv(r,i));
    }
    dec(rw? x : w);
    return r.b;
  }
  
  B bo = wg? w : x;
  usz ria = a(bo)->ia;
  usz ri = 0;
  HArr_p r = m_harrs(ria, &ri);
  if (wr==xr)                       for(; ri < ria; ri++) r.a[ri] = f(fo, wget(w,ri), xget(x,ri));
  else if (wr==0) { B c=wget(w, 0); for(; ri < ria; ri++) r.a[ri] = f(fo, inc(c)    , xget(x,ri)); dec(c); }
  else if (xr==0) { B c=xget(x, 0); for(; ri < ria; ri++) r.a[ri] = f(fo, wget(w,ri), inc(c)    ); dec(c); }
  else if (ria>0) {
    usz min = wg? a(x)->ia : a(w)->ia;
    usz ext = ria / min;
    if (wg) for (usz i = 0; i < min; i++) { B c=xget(x,i); for (usz j = 0; j < ext; j++,ri++) r.a[ri] = f(fo, wget(w,ri), inc(c)); }
    else    for (usz i = 0; i < min; i++) { B c=wget(w,i); for (usz j = 0; j < ext; j++,ri++) r.a[ri] = f(fo, inc(c), xget(x,ri)); }
  }
  B rb = harr_fc(r, bo);
  dec(w); dec(x);
  return rb;
}
B eachm_fn(BB2B f, B fo, B x) { // consumes x; x must be array
  usz ia = a(x)->ia;
  if (ia==0) return x;
  BS2B xget = TI(x).get;
  usz i = 0;
  B cr = f(fo, xget(x,0));
  HArr_p rH;
  if (TI(x).canStore(cr)) {
    bool reuse = reusable(x);
    if (v(x)->type==t_harr) {
      B* xp = harr_ptr(x);
      if (reuse) {
        dec(xp[i]); xp[i++] = cr;
        for (; i < ia; i++) xp[i] = f(fo, mv(xp,i));
        return x;
      } else {
        rH = m_harrs(ia, &i);
        rH.a[i++] = cr;
        for (; i < ia; i++) rH.a[i] = f(fo, inc(xp[i]));
        return harr_fcd(rH, x);
      }
    } else if (v(x)->type==t_i32arr) {
      i32* xp = i32arr_ptr(x);
      B r = reuse? x : m_i32arrc(x);
      i32* rp = i32arr_ptr(r);
      rp[i++] = o2iu(cr);
      for (; i < ia; i++) {
        cr = f(fo, m_i32(xp[i]));
        if (!q_i32(cr)) {
          rH = m_harrs(ia, &i);
          for (usz j = 0; j < i; j++) rH.a[j] = m_i32(rp[j]);
          if (!reuse) dec(r);
          goto fallback;
        }
        rp[i] = o2iu(cr);
      }
      if (!reuse) dec(x);
      return r;
    } else if (v(x)->type==t_f64arr) {
      f64* xp = f64arr_ptr(x);
      B r = reuse? x : m_f64arrc(x);
      f64* rp = f64arr_ptr(r);
      rp[i++] = o2fu(cr);
      for (; i < ia; i++) {
        cr = f(fo, m_f64(xp[i]));
        if (!q_f64(cr)) {
          rH = m_harrs(ia, &i);
          for (usz j = 0; j < i; j++) rH.a[j] = m_f64(rp[j]);
          if (!reuse) dec(r);
          goto fallback;
        }
        rp[i] = o2fu(cr);
      }
      if (!reuse) dec(x);
      return r;
    } else if (v(x)->type==t_fillarr) {
      B* xp = fillarr_ptr(x);
      if (reuse) {
        dec(c(FillArr,x)->fill);
        c(FillArr,x)->fill = bi_noFill;
        dec(xp[i]); xp[i++] = cr;
        for (; i < ia; i++) xp[i] = f(fo, mv(xp,i));
        return x;
      } else {
        HArr_p rp = m_harrs(ia, &i);
        rp.a[i++] = cr;
        for (; i < ia; i++) rp.a[i] = f(fo, inc(xp[i]));
        return harr_fcd(rp, x);
      }
    } else
    rH = m_harrs(ia, &i);
  } else
  rH = m_harrs(ia, &i);
  fallback:
  rH.a[i++] = cr;
  for (; i < ia; i++) rH.a[i] = f(fo, xget(x,i));
  return harr_fcd(rH, x);
}
B eachm(B f, B x) { // complete F¨ x without fills
  if (isAtm(x)) return m_hunit(c1(f, x));
  if (isFun(f)) return eachm_fn(c(Fun,f)->c1, f, x);
  if (isMd(f)) if (isAtm(x) || a(x)->ia) { decR(x); thrM("Calling a modifier"); }
  
  usz ia = a(x)->ia;
  MAKE_MUT(r, ia);
  mut_fill(r, 0, f, ia);
  return mut_fcd(r, x);
}

B eachd(B f, B w, B x) { // complete w F¨ x without fills
  if (isAtm(w) & isAtm(x)) return m_hunit(c2(f, w, x));
  return eachd_fn(c2fn(f), f, w, x);
}
B shape_c1(B t, B x) {
  if (isAtm(x)) thrM("⥊: deshaping non-array");
  usz ia = a(x)->ia;
  if (reusable(x)) {
    decSh(x);
    arr_shVec(x, ia);
    return x;
  }
  B r = TI(x).slice(x, 0);
  arr_shVec(r, ia);
  return r;
}
B shape_c2(B t, B w, B x) {
  if (isAtm(x)) { dec(x); dec(w); thrM("⥊: Reshaping non-array"); }
  if (isAtm(w)) return shape_c1(t, x);
  BS2B wget = TI(w).get;
  usz wia = a(w)->ia;
  if (wia>UR_MAX) thrM("⥊: Result rank too large");
  ur nr = (ur)wia;
  usz nia = a(x)->ia;
  B r;
  if (reusable(x)) { r = x; decSh(x); }
  else r = TI(x).slice(x, 0);
  usz* sh = arr_shAllocI(r, nia, nr);
  if (sh) for (u32 i = 0; i < nr; i++) sh[i] = o2s(wget(w,i));
  dec(w);
  return r;
}

B pick_c1(B t, B x) {
  if (isAtm(x)) return x;
  if (a(x)->ia==0) {
    B r = getFill(x);
    if (noFill(r)) thrM("⊑: called on empty array without fill");
    return r;
  }
  B r = TI(x).get(x, 0);
  dec(x);
  return r;
}
B pick_c2(B t, B w, B x) {
  // usz wu = o2s(w);
  // if (isAtm(x)) { dec(x); dec(w); thrM("⊑: 𝕩 wasn't an array"); }
  // if (wu >= a(x)->ia) thrM("⊑: 𝕨 is greater than length of 𝕩"); // no bounds check for now
  B r = TI(x).get(x, o2su(w));
  dec(x);
  return r;
}

B rt_select;
B select_c1(B t, B x) {
  if (isAtm(x)) thrM("⊏: Argument cannot be an atom");
  ur xr = rnk(x);
  if (xr==0) thrM("⊏: Argument cannot be rank 0");
  if (a(x)->sh[0]==0) thrM("⊏: Argument shape cannot start with 0");
  B r = TI(x).slice(x,0);
  usz* sh = arr_shAllocR(r, xr-1);
  usz ia = 1;
  for (i32 i = 1; i < xr; i++) {
    if (sh) sh[i-1] = a(x)->sh[i];
    ia*= a(x)->sh[i];
  }
  a(r)->ia = ia;
  return r;
}
B select_c2(B t, B w, B x) {
  if (isAtm(x)) thrM("⊏: 𝕩 cannot be an atom");
  ur xr = rnk(x);
  if (isAtm(w)) {
    if (xr==0) thrM("⊏: 𝕩 cannot be a unit");
    usz csz = arr_csz(x);
    usz cam = a(x)->sh[0];
    i64 wi = o2i64(w);
    if (wi<0) wi+= cam;
    if ((usz)wi >= cam) thrM("⊏: Indexing out-of-bounds");
    B r = TI(x).slice(x, wi*csz);
    usz* sh = arr_shAllocI(r, csz, xr-1);
    if (sh) memcpy(sh, a(x)->sh+1, (xr-1)*sizeof(usz));
    return r;
  }
  B xf = getFill(inc(x));
  BS2B xget = TI(x).get;
  usz wia = a(w)->ia;
  
  if (xr==1) {
    usz xia = a(x)->ia;
    if (v(w)->type==t_i32arr | v(w)->type==t_i32slice) {
      i32* wp = v(w)->type==t_i32slice? c(I32Slice,w)->a : i32arr_ptr(w);
      if (v(x)->type==t_i32arr) {
        B r = m_i32arrc(w); i32* rp = i32arr_ptr(r);
        i32* xp = i32arr_ptr(x);
        for (usz i = 0; i < wia; i++) {
          i64 c = wp[i];
          if (c<0) c+= xia;
          if (c<0 | c>=xia) thrM("⊏: Indexing out-of-bounds");
          rp[i] = xp[c];
        }
        dec(w); dec(x);
        return r;
      } else {
        HArr_p r = m_harrUc(w);
        for (usz i = 0; i < wia; i++) {
          i64 c = wp[i];
          if (c<0) c+= xia;
          if (c<0 | c>=xia) thrM("⊏: Indexing out-of-bounds");
          r.a[i] = xget(x, c);
        }
        dec(w); dec(x);
        return withFill(r.b,xf);
      }
    } else {
      HArr_p r = m_harrUc(w);
      BS2B wgetU = TI(w).getU;
      for (usz i = 0; i < wia; i++) {
        B cw = wgetU(w, i);
        if (!isNum(cw)) { harr_pfree(r.b, i); goto base; }
        f64 c = o2f(cw);
        if (c<0) c+= xia;
        if ((usz)c >= xia) thrM("⊏: Indexing out-of-bounds");
        r.a[i] = xget(x, c);
      }
      dec(w); dec(x);
      return withFill(r.b,xf);
    }
  } else {
    BS2B wgetU = TI(w).getU;
    ur wr = rnk(w);
    ur rr = wr+xr-1;
    if (xr==0) thrM("⊏: 𝕩 cannot be a unit");
    if (rr>UR_MAX) thrM("⊏: Result rank too large");
    usz csz = arr_csz(x);
    usz cam = a(x)->sh[0];
    MAKE_MUT(r, wia*csz);
    mut_to(r, fillElType(xf));
    for (usz i = 0; i < wia; i++) {
      B cw = wgetU(w, i);
      if (!isNum(cw)) { mut_pfree(r, i*csz); goto base; }
      f64 c = o2f(cw);
      if (c<0) c+= cam;
      if ((usz)c >= cam) thrM("⊏: Indexing out-of-bounds");
      mut_copy(r, i*csz, x, csz*(usz)c, csz);
    }
    B rb = mut_fp(r);
    usz* rsh = arr_shAllocR(rb, rr);
    if (rsh) {
      memcpy(rsh   , a(w)->sh  ,  wr   *sizeof(usz));
      memcpy(rsh+wr, a(x)->sh+1, (xr-1)*sizeof(usz));
    }
    dec(w); dec(x);
    return withFill(rb,xf);
  }
  base:
  dec(xf);
  return c2(rt_select, w, x);
}

B rt_slash;
B slash_c1(B t, B x) {
  if (isAtm(x)) thrM("/: Argument must be a list");
  if (rnk(x)!=1) thrM("/: Argument must have rank 1");
  i64 s = isum(x);
  if(s<0) thrM("/: Argument must consist of natural numbers");
  usz xia = a(x)->ia;
  BS2B xgetU = TI(x).getU;
  usz ri = 0;
  if (xia<I32_MAX) {
    B r = m_i32arrv(s); i32* rp = i32arr_ptr(r);
    for (i32 i = 0; i < xia; i++) {
      usz c = o2s(xgetU(x, i));
      for (usz j = 0; j < c; j++) rp[ri++] = i;
    }
    dec(x);
    return r;
  }
  B r = m_f64arrv(s); f64* rp = f64arr_ptr(r);
  for (usz i = 0; i < xia; i++) {
    usz c = o2s(xgetU(x, i));
    for (usz j = 0; j < c; j++) rp[ri++] = i;
  }
  dec(x);
  return r;
}
B slash_c2(B t, B w, B x) {
  if (isArr(w) && isArr(x) && rnk(w)==1 && rnk(x)==1 && depth(w)==1) {
    usz wia = a(w)->ia;
    usz xia = a(x)->ia;
    B xf = getFill(inc(x));
    if (wia!=xia) thrM("/: Lengths of components of 𝕨 must match 𝕩");
    i64 wsum = isum(w); if (wsum>USZ_MAX) thrM("/: Result too large");
    usz ria = wsum;
    usz ri = 0;
    HArr_p r = m_harrs(ria, &ri);
    BS2B wgetU = TI(w).getU;
    BS2B xgetU = TI(x).getU;
    for (usz i = 0; i < wia; i++) {
      B cw = wgetU(w, i);
      if (isNum(cw)) {
        f64 cf = o2f(cw);
        usz c = (usz)cf;
        if (cf!=c) goto base; // TODO clean up half-written r
        if (c) {
          B cx = xgetU(x, i);
          for (usz j = 0; j < c; j++) r.a[ri++] = inc(cx);
        }
      } else { dec(cw); goto base; }
    }
    dec(w); dec(x);
    return withFill(harr_fv(r), xf);
  }
  base:
  return c2(rt_slash, w, x);
}

B slicev(B x, usz s, usz ia) {
  usz xia = a(x)->ia; if (s+ia>xia) thrM("↑/↓: NYI fills");
  B r = TI(x).slice(x, s);
  arr_shVec(r, ia);
  return r;
}
B take_c2(B t, B w, B x) {
  if (isAtm(x) || rnk(x)!=1) thrM("↑: NYI 1≠=𝕩");
  i64 v = o2i64(w); usz ia = a(x)->ia;
  return v<0? slicev(x, ia+v, -v) : slicev(x, 0, v);
}
B drop_c2(B t, B w, B x) {
  if (isAtm(x) || rnk(x)!=1) thrM("↓: NYI 1≠=𝕩");
  i64 v = o2i64(w); usz ia = a(x)->ia;
  return v<0? slicev(x, 0, v+ia) : slicev(x, v, ia-v);
}

B rt_join;
B join_c1(B t, B x) {
  return c1(rt_join, x);
}
B join_c2(B t, B w, B x) {
  B f = fill_both(w, x);
  if (isAtm(w)) w = m_atomUnit(w); ur wr = rnk(w); usz wia = a(w)->ia; usz* wsh = a(w)->sh;
  if (isAtm(x)) x = m_atomUnit(x); ur xr = rnk(x); usz xia = a(x)->ia; usz* xsh = a(x)->sh;
  ur c = wr>xr?wr:xr;
  if (c==0) {
    HArr_p r = m_harrUv(2);
    r.a[0] = TI(w).get(w,0); dec(w);
    r.a[1] = TI(x).get(x,0); dec(x);
    return r.b;
  }
  if (c-wr > 1 || c-xr > 1) thrM("∾: Argument ranks must differ by 1 or less");
  MAKE_MUT(r, wia+xia);
  mut_to(r, el_or(TI(w).elType, TI(x).elType));
  mut_copy(r, 0,   w, 0, wia);
  mut_copy(r, wia, x, 0, xia);
  B rb = mut_fp(r);
  usz* sh = arr_shAllocR(rb, c);
  if (sh) {
    for (i32 i = 1; i < c; i++) {
      usz s = xsh[i+xr-c];
      if (wsh[i+wr-c] != s) { mut_pfree(r, wia+xia); thrM("∾: Lengths not matchable"); }
      sh[i] = s;
    }
    sh[0] = (wr==c? wsh[0] : 1) + (xr==c? xsh[0] : 1);
  }
  dec(w); dec(x);
  return qWithFill(rb, f);
}


static void shift_check(B w, B x) {
  ur wr = rnk(w); usz* wsh = a(w)->sh;
  ur xr = rnk(x); usz* xsh = a(x)->sh;
  if (wr+1!=xr & wr!=xr) thrM("shift: =𝕨 must be =𝕩 or ¯1+=𝕩");
  for (i32 i = 1; i < xr; i++) if (wsh[i+wr-xr] != xsh[i]) thrM("shift: Lengths not matchable");
}

B shiftb_c1(B t, B x) {
  if (isAtm(x) || rnk(x)==0) thrM("»: Argument cannot be a scalar");
  usz ia = a(x)->ia;
  if (ia==0) return x;
  B xf = getFillE(inc(x));
  usz csz = arr_csz(x);
  
  MAKE_MUT(r, ia); mut_to(r, TI(x).elType);
  mut_copy(r, csz, x, 0, ia-csz);
  mut_fill(r, 0, xf, csz);
  return qWithFill(mut_fcd(r, x), xf);
}
B shiftb_c2(B t, B w, B x) {
  if (isAtm(x) || rnk(x)==0) thrM("»: 𝕩 cannot be a scalar");
  if (isAtm(w)) w = m_atomUnit(w);
  shift_check(w, x);
  B f = fill_both(w, x);
  usz wia = a(w)->ia;
  usz xia = a(x)->ia;
  MAKE_MUT(r, xia); mut_to(r, el_or(TI(w).elType, TI(x).elType));
  int mid = wia<xia? wia : xia;
  mut_copy(r, 0  , w, 0, mid);
  mut_copy(r, mid, x, 0, xia-mid);
  dec(w);
  return qWithFill(mut_fcd(r, x), f);
}

B shifta_c1(B t, B x) {
  if (isAtm(x) || rnk(x)==0) thrM("«: Argument cannot be a scalar");
  usz ia = a(x)->ia;
  if (ia==0) return x;
  B xf = getFillE(inc(x));
  usz csz = arr_csz(x);
  MAKE_MUT(r, ia); mut_to(r, TI(x).elType);
  mut_copy(r, 0, x, csz, ia-csz);
  mut_fill(r, ia-csz, xf, csz);
  return qWithFill(mut_fcd(r, x), xf);
}
B shifta_c2(B t, B w, B x) {
  if (isAtm(x) || rnk(x)==0) thrM("«: 𝕩 cannot be a scalar");
  if (isAtm(w)) w = m_atomUnit(w);
  shift_check(w, x);
  B f = fill_both(w, x);
  usz wia = a(w)->ia;
  usz xia = a(x)->ia;
  MAKE_MUT(r, xia); mut_to(r, el_or(TI(w).elType, TI(x).elType));
  if (wia < xia) {
    usz m = xia-wia;
    mut_copy(r, 0, x, wia, m);
    mut_copy(r, m, w, 0, wia);
  } else {
    mut_copy(r, 0, w, wia-xia, xia);
  }
  dec(w);
  return qWithFill(mut_fcd(r, x), f);
}

#define ba(N) bi_##N = mm_alloc(sizeof(BFn), t_funBI, ftag(FUN_TAG)); c(Fun,bi_##N)->c2 = N##_c2    ;c(Fun,bi_##N)->c1 = N##_c1    ; c(Fun,bi_##N)->extra=pf_##N; c(BFn,bi_##N)->ident=bi_N; gc_add(bi_##N);
#define bd(N) bi_##N = mm_alloc(sizeof(BFn), t_funBI, ftag(FUN_TAG)); c(Fun,bi_##N)->c2 = N##_c2    ;c(Fun,bi_##N)->c1 = c1_invalid; c(Fun,bi_##N)->extra=pf_##N; c(BFn,bi_##N)->ident=bi_N; gc_add(bi_##N);
#define bm(N) bi_##N = mm_alloc(sizeof(BFn), t_funBI, ftag(FUN_TAG)); c(Fun,bi_##N)->c2 = c2_invalid;c(Fun,bi_##N)->c1 = N##_c1    ; c(Fun,bi_##N)->extra=pf_##N; c(BFn,bi_##N)->ident=bi_N; gc_add(bi_##N);

B                                bi_shape, bi_pick, bi_pair, bi_select, bi_slash, bi_join, bi_shiftb, bi_shifta, bi_take, bi_drop;
static inline void sfns_init() { ba(shape) ba(pick) ba(pair) ba(select) ba(slash) ba(join) ba(shiftb) ba(shifta) bd(take) bd(drop)
}

#undef ba
#undef bd
#undef bm
