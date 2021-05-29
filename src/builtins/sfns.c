#include "../core.h"
#include "../utils/each.h"
#include "../utils/mut.h"


B shape_c1(B t, B x) {
  if (isAtm(x)) thrM("⥊: deshaping non-array");
  usz ia = a(x)->ia;
  if (reusable(x)) {
    decSh(v(x));
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
  if (reusable(x)) { r = x; decSh(v(x)); }
  else r = TI(x).slice(x, 0);
  usz* sh = arr_shAllocI(r, nia, nr);
  if (sh) for (u32 i = 0; i < nr; i++) sh[i] = o2s(wget(w,i));
  dec(w);
  return r;
}

extern B rt_pick;
B pick_c1(B t, B x) {
  if (isAtm(x)) return x;
  if (a(x)->ia==0) {
    B r = getFillE(x);
    dec(x);
    return r;
  }
  B r = TI(x).get(x, 0);
  dec(x);
  return r;
}
B pick_c2(B t, B w, B x) {
  if (isNum(w) && isArr(x) && rnk(x)==1) {
    usz p = WRAP(o2i64(w), a(x)->ia, thrF("⊑: indexing out-of-bounds (𝕨≡%R, %s≡≠𝕩)", w, iaW));
    B r = TI(x).get(x, p);
    dec(x);
    return r;
  }
  return c2(rt_pick, w, x);
}

extern B rt_select;
B select_c1(B t, B x) {
  if (isAtm(x)) thrM("⊏: Argument cannot be an atom");
  ur xr = rnk(x);
  if (xr==0) thrM("⊏: Argument cannot be rank 0");
  if (a(x)->sh[0]==0) thrF("⊏: Argument shape cannot start with 0 (%H ≡ ≢𝕩)", x);
  B r = TI(x).slice(inc(x),0);
  usz* sh = arr_shAllocR(r, xr-1);
  usz ia = 1;
  for (i32 i = 1; i < xr; i++) {
    if (sh) sh[i-1] = a(x)->sh[i];
    ia*= a(x)->sh[i];
  }
  a(r)->ia = ia;
  dec(x);
  return r;
}
B select_c2(B t, B w, B x) {
  if (isAtm(x)) thrM("⊏: 𝕩 cannot be an atom");
  ur xr = rnk(x);
  if (isAtm(w)) {
    if (xr==0) thrM("⊏: 𝕩 cannot be a unit");
    usz csz = arr_csz(x);
    usz cam = a(x)->sh[0];
    usz wi = WRAP(o2i64(w), cam, thrF("⊏: Indexing out-of-bounds (𝕨≡%R, %s≡≠𝕩)", w, cam));
    B r = TI(x).slice(inc(x), wi*csz);
    usz* sh = arr_shAllocI(r, csz, xr-1);
    if (sh) memcpy(sh, a(x)->sh+1, (xr-1)*sizeof(usz));
    dec(x);
    return r;
  }
  B xf = getFillQ(x);
  BS2B xget = TI(x).get;
  usz wia = a(w)->ia;
  
  if (xr==1) {
    usz xia = a(x)->ia;
    if (TI(w).elType==el_i32) {
      i32* wp = i32any_ptr(w);
      if (TI(x).elType==el_i32) {
        i32* rp; B r = m_i32arrc(&rp, w);
        i32* xp = i32any_ptr(x);
        for (usz i = 0; i < wia; i++) {
          usz c = WRAP(wp[i], xia, thrF("⊏: Indexing out-of-bounds (%i∊𝕨, %s≡≠𝕩)", wp[i], xia));
          rp[i] = xp[c];
        }
        dec(w); dec(x);
        return r;
      } else if (TI(x).elType==el_f64) {
        f64* rp; B r = m_f64arrc(&rp, w);
        f64* xp = f64any_ptr(x);
        for (usz i = 0; i < wia; i++) {
          usz c = WRAP(wp[i], xia, thrF("⊏: Indexing out-of-bounds (%i∊𝕨, %s≡≠𝕩)", wp[i], xia));
          rp[i] = xp[c];
        }
        dec(w); dec(x);
        return r;
      } else if (TI(x).elType==el_c32) {
        u32* rp; B r = m_c32arrc(&rp, w);
        u32* xp = c32any_ptr(x);
        for (usz i = 0; i < wia; i++) {
          usz c = WRAP(wp[i], xia, thrF("⊏: Indexing out-of-bounds (%i∊𝕨, %s≡≠𝕩)", wp[i], xia));
          rp[i] = xp[c];
        }
        dec(w); dec(x);
        return r;
      } else if (v(x)->type==t_harr) {
        usz i = 0;
        B* xp = harr_ptr(x);
        HArr_p r = m_harrs(wia, &i);
        for (; i < wia; i++) {
          usz c = WRAP(wp[i], xia, thrF("⊏: Indexing out-of-bounds (%i∊𝕨, %s≡≠𝕩)", wp[i], xia));
          r.a[i] = inc(xp[c]);
        }
        dec(x);
        return harr_fcd(r, w);
      } else {
        usz i = 0;
        HArr_p r = m_harrs(wia, &i);
        for (; i < wia; i++) {
          usz c = WRAP(wp[i], xia, thrF("⊏: Indexing out-of-bounds (%i∊𝕨, %s≡≠𝕩)", wp[i], xia));
          r.a[i] = xget(x, c);
        }
        dec(x);
        return withFill(harr_fcd(r,w),xf);
      }
    } else {
      usz i = 0;
      HArr_p r = m_harrs(wia, &i);
      BS2B wgetU = TI(w).getU;
      for (; i < wia; i++) {
        B cw = wgetU(w, i);
        if (!isNum(cw)) { harr_pfree(r.b, i); goto base; }
        usz c = WRAP(o2i64(cw), xia, thrF("⊏: Indexing out-of-bounds (%R∊𝕨, %s≡≠𝕩)", cw, xia));
        r.a[i] = xget(x, c);
      }
      dec(x);
      return withFill(harr_fcd(r,w),xf);
    }
  } else {
    BS2B wgetU = TI(w).getU;
    ur wr = rnk(w);
    i32 rr = wr+xr-1;
    if (xr==0) thrM("⊏: 𝕩 cannot be a unit");
    if (rr>UR_MAX) thrF("⊏: Result rank too large (%i≡=𝕨, %i≡=𝕩)", wr, xr);
    usz csz = arr_csz(x);
    usz cam = a(x)->sh[0];
    MAKE_MUT(r, wia*csz);
    mut_to(r, fillElType(xf));
    for (usz i = 0; i < wia; i++) {
      B cw = wgetU(w, i);
      if (!isNum(cw)) { mut_pfree(r, i*csz); goto base; }
      f64 c = o2f(cw);
      if (c<0) c+= cam;
      if ((usz)c >= cam) { mut_pfree(r, i*csz); thrF("⊏: Indexing out-of-bounds (%R∊𝕨, %s≡≠𝕩)", cw, cam); }
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

static NOINLINE B slash_c1R(B x, u64 s) {
  usz xia = a(x)->ia;
  BS2B xgetU = TI(x).getU;
  f64* rp; B r = m_f64arrv(&rp, s); usz ri = 0;
  for (usz i = 0; i < xia; i++) {
    usz c = o2s(xgetU(x, i));
    for (usz j = 0; j < c; j++) rp[ri++] = i;
  }
  dec(x);
  return r;
}
extern B rt_slash;
B slash_c1(B t, B x) {
  if (RARE(isAtm(x)) || RARE(rnk(x)!=1)) thrF("/: Argument must have rank 1 (%H ≡ ≢𝕩)", x);
  i64 s = isum(x);
  if(s<0) thrM("/: Argument must consist of natural numbers");
  usz xia = a(x)->ia;
  if (RARE(xia>=I32_MAX)) return slash_c1R(x, s);
  i32* rp; B r = m_i32arrv(&rp, s); usz ri = 0;
  if (TI(x).elType==el_i32) {
    i32* xp = i32any_ptr(x);
    for (i32 i = 0; i < xia; i++) {
      if (RARE(xp[i])<0) thrF("/: Argument must consist of natural numbers (contained %i)", xp[i]);
      for (usz j = 0; j < xp[i]; j++) rp[ri++] = i;
    }
  } else {
    BS2B xgetU = TI(x).getU;
    for (i32 i = 0; i < xia; i++) {
      usz c = o2s(xgetU(x, i));
      for (usz j = 0; j < c; j++) rp[ri++] = i;
    }
  }
  dec(x);
  return r;
}
B slash_c2(B t, B w, B x) {
  if (isArr(x) && rnk(x)==1 && isArr(w) && rnk(w)==1 && depth(w)==1) {
    usz wia = a(w)->ia;
    usz xia = a(x)->ia;
    B xf = getFillQ(x);
    if (wia!=xia) thrF("/: Lengths of components of 𝕨 must match 𝕩 (%s ≠ %s)", wia, xia);
    i64 wsum = isum(w); if (wsum>USZ_MAX) thrOOM();
    usz ria = wsum;
    usz ri = 0;
    if (TI(w).elType==el_i32) {
      i32* wp = i32any_ptr(w);
      if (TI(x).elType==el_i32) {
        i32* xp = i32any_ptr(x);
        i32* rp; B r = m_i32arrv(&rp, ria);
        for (usz i = 0; i < wia; i++) {
          i32 c = wp[i];
          if (c<0) thrF("/: 𝕨 must consist of natural numbers (%i∊𝕨)", c);
          i32 cx = xp[i];
          for (usz j = 0; j < c; j++) *rp++ = cx;
        }
        dec(w); dec(x);
        return r;
      } else {
        HArr_p r = m_harrs(ria, &ri);
        BS2B xgetU = TI(x).getU;
        for (usz i = 0; i < wia; i++) {
          i32 c = wp[i];
          if (c==0) continue;
          if (c<0) thrF("/: 𝕨 must consist of natural numbers (%i∊𝕨)", c);
          B cx = xgetU(x, i);
          for (usz j = 0; j < c; j++) r.a[ri++] = inc(cx);
        }
        dec(w); dec(x);
        return withFill(harr_fv(r), xf);
      }
    } else {
      HArr_p r = m_harrs(ria, &ri);
      BS2B wgetU = TI(w).getU;
      BS2B xgetU = TI(x).getU;
      for (usz i = 0; i < wia; i++) {
        usz c = o2s(wgetU(w, i));
        if (c) {
          B cx = xgetU(x, i);
          for (usz j = 0; j < c; j++) r.a[ri++] = inc(cx);
        }
      }
      dec(w); dec(x);
      return withFill(harr_fv(r), xf);
    }
  }
  if (isArr(x) && rnk(x)==1 && q_i32(w)) {
    usz xia = a(x)->ia;
    i32 wv = o2i(w);
    if (wv<=0) {
      if (wv<0) thrM("/: 𝕨 cannot be negative");
      B r = TI(x).slice(x, 0);
      arr_shVec(r,0);
      return r;
    }
    usz ri = 0;
    if (TI(x).elType==el_i32) {
      i32* xp = i32any_ptr(x);
      i32* rp; B r = m_i32arrv(&rp, xia*wv);
      for (usz i = 0; i < xia; i++) {
        for (usz j = 0; j < wv; j++) rp[ri++] = xp[i];
      }
      dec(x);
      return r;
    } else {
      B xf = getFillQ(x);
      HArr_p r = m_harrUv(xia*wv);
      BS2B xgetU = TI(x).getU;
      for (usz i = 0; i < xia; i++) {
        B cx = xgetU(x, i);
        for (usz j = 0; j < wv; j++) r.a[ri++] = inc(cx);
      }
      dec(x);
      return withFill(r.b, xf);
    }
  }
  return c2(rt_slash, w, x);
}

B slicev(B x, usz s, usz ia) {
  usz xia = a(x)->ia; assert(s+ia <= xia);
  B r = TI(x).slice(x, s);
  arr_shVec(r, ia);
  return r;
}
extern B rt_take, rt_drop;
B take_c1(B t, B x) { return c1(rt_take, x); }
B drop_c1(B t, B x) { return c1(rt_drop, x); }
B take_c2(B t, B w, B x) {
  if (isNum(w) && isArr(x) && rnk(x)==1) {
    i64 v = o2i64(w);
    usz ia = a(x)->ia;
    u64 va = v<0? -v : v;
    if (va>ia) {
      MAKE_MUT(r, va);
      B xf = getFillE(x);
      mut_copy(r, v<0? va-ia : 0, x, 0, ia);
      mut_fill(r, v<0? 0 : ia, xf, va-ia);
      dec(x);
      dec(xf);
      return mut_fv(r);
    }
    if (v<0) return slicev(x, ia+v, -v);
    else     return slicev(x, 0,     v);
  }
  return c2(rt_take, w, x);
}
B drop_c2(B t, B w, B x) {
  if (isNum(w) && isArr(x) && rnk(x)==1) {
    i64 v = o2i64(w);
    usz ia = a(x)->ia;
    if (v<0) return -v>ia? slicev(x, 0, 0) : slicev(x, 0, v+ia);
    else     return  v>ia? slicev(x, 0, 0) : slicev(x, v, ia-v);
  }
  return c2(rt_drop, w, x);
}

extern B rt_join;
B join_c1(B t, B x) {
  if (isAtm(x)) thrM("∾: Argument must be an array");
  if (rnk(x)==1) {
    usz xia = a(x)->ia;
    if (xia==0) {
      B xf = getFillE(x);
      if (isAtm(xf)) {
        decR(xf);
        if (!PROPER_FILLS) {
          B xfq = getFillR(x);
          bool no = noFill(xfq);
          decR(xfq);
          if (no) return x;
        }
        dec(x);
        thrM("∾: Empty vector 𝕩 cannot have an atom fill element");
      }
      dec(x);
      ur ir = rnk(xf);
      if (ir==0) thrM("∾: Empty vector 𝕩 cannot have a unit fill element");
      B xff = getFillQ(xf);
      HArr_p r = m_harrUp(0);
      usz* sh = arr_shAllocR(r.b, ir);
      if (sh) {
        sh[0] = 0;
        memcpy(sh+1, a(xf)->sh+1, sizeof(usz)*(ir-1));
      }
      dec(xf);
      return withFill(r.b, xff);
    }
    BS2B xgetU = TI(x).getU;
    
    B x0 = xgetU(x,0);
    B rf; if(SFNS_FILLS) rf = getFillQ(x0);
    if (isAtm(x0)) thrM("∾: Rank of items must be equal or greater than rank of argument");
    usz ir = rnk(x0);
    usz* x0sh = a(x0)->sh;
    if (ir==0) thrM("∾: Rank of items must be equal or greater than rank of argument");
    
    usz csz = arr_csz(x0);
    usz cam = x0sh[0];
    for (usz i = 1; i < xia; i++) {
      B c = xgetU(x, i);
      if (!isArr(c) || rnk(c)!=ir) thrF("∾: All items in argument should have same rank (contained items with ranks %i and %i)", ir, isArr(c)? rnk(c) : 0);
      usz* csh = a(c)->sh;
      if (ir>1) for (usz j = 1; j < ir; j++) if (csh[j]!=x0sh[j]) thrF("∾: Item trailing shapes must be equal (contained arrays with shapes %H and %H)", x0, c);
      cam+= a(c)->sh[0];
      if (SFNS_FILLS && !noFill(rf)) rf = fill_or(rf, getFillQ(c));
    }
    
    MAKE_MUT(r, cam*csz);
    usz ri = 0;
    for (usz i = 0; i < xia; i++) {
      B c = xgetU(x, i);
      usz cia = a(c)->ia;
      mut_copy(r, ri, c, 0, cia);
      ri+= cia;
    }
    assert(ri==cam*csz);
    B rb = mut_fp(r);
    usz* sh = arr_shAllocR(rb, ir);
    if (sh) {
      sh[0] = cam;
      memcpy(sh+1, x0sh+1, sizeof(usz)*(ir-1));
    }
    dec(x);
    return SFNS_FILLS? qWithFill(rb, rf) : rb;
  }
  return c1(rt_join, x);
}
B join_c2(B t, B w, B x) {
  if (isAtm(w)) w = m_atomUnit(w);
  ur wr = rnk(w);
  if (isAtm(x)) {
    if (wr==1 && inplace_add(w, x)) return w;
    x = m_atomUnit(x);
  }
  usz wia = a(w)->ia; usz* wsh = a(w)->sh;
  usz xia = a(x)->ia; usz* xsh = a(x)->sh; ur xr = rnk(x);
  B f = fill_both(w, x);
  ur c = wr>xr?wr:xr;
  if (c==0) {
    HArr_p r = m_harrUv(2);
    r.a[0] = TI(w).get(w,0); dec(w);
    r.a[1] = TI(x).get(x,0); dec(x);
    return qWithFill(r.b, f);
  }
  if (c-wr > 1 || c-xr > 1) thrF("∾: Argument ranks must differ by 1 or less (%i≡=𝕨, %i≡=𝕩)", wr, xr);
  if (c==1) {
    B r = vec_join(w, x);
    if (rnk(r)==0) srnk(r,1);
    return qWithFill(r, f);
  }
  MAKE_MUT(r, wia+xia);
  mut_to(r, el_or(TI(w).elType, TI(x).elType));
  mut_copy(r, 0,   w, 0, wia);
  mut_copy(r, wia, x, 0, xia);
  B rb = mut_fp(r);
  usz* sh = arr_shAllocR(rb, c);
  if (sh) {
    for (i32 i = 1; i < c; i++) {
      usz s = xsh[i+xr-c];
      if (wsh[i+wr-c] != s) { mut_pfree(r, wia+xia); thrF("∾: Lengths not matchable (%H ≡ ≢𝕨, %H ≡ ≢𝕩)", w, x); }
      sh[i] = s;
    }
    sh[0] = (wr==c? wsh[0] : 1) + (xr==c? xsh[0] : 1);
  }
  dec(w); dec(x);
  return qWithFill(rb, f);
}


B couple_c1(B t, B x) {
  if (isArr(x)) {
    usz rr = rnk(x);
    usz ia = a(x)->ia;
    B r = TI(x).slice(inc(x),0);
    usz* sh = arr_shAllocI(r, ia, rr+1);
    if (sh) { sh[0] = 1; memcpy(sh+1, a(x)->sh, rr*sizeof(usz)); }
    dec(x);
    return r;
  }
  if (q_i32(x)) { i32* rp; B r = m_i32arrv(&rp, 1); rp[0] = o2iu(x); return r; }
  if (isF64(x)) { f64* rp; B r = m_f64arrv(&rp, 1); rp[0] = o2fu(x); return r; }
  if (isC32(x)) { u32* rp; B r = m_c32arrv(&rp, 1); rp[0] = o2cu(x); return r; }
  HArr_p r = m_harrUv(1);
  r.a[0] = x;
  return r.b;
}
B couple_c2(B t, B w, B x) {
  if (isAtm(w)&isAtm(x)) {
    if (q_i32(x)&q_i32(w)) { i32* rp; B r = m_i32arrv(&rp, 2); rp[0]=o2iu(w); rp[1]=o2iu(x); return r; }
    if (isF64(x)&isF64(w)) { f64* rp; B r = m_f64arrv(&rp, 2); rp[0]=o2fu(w); rp[1]=o2fu(x); return r; }
    if (isC32(x)&isC32(w)) { u32* rp; B r = m_c32arrv(&rp, 2); rp[0]=o2cu(w); rp[1]=o2cu(x); return r; }
  }
  if (isAtm(w)) w = m_atomUnit(w);
  if (isAtm(x)) x = m_atomUnit(x);
  if (!eqShape(w, x)) thrF("≍: 𝕨 and 𝕩 must have equal shapes (%H ≡ ≢𝕨, %H ≡ ≢𝕩)", w, x);
  usz ia = a(w)->ia;
  ur wr = rnk(w);
  MAKE_MUT(r, ia*2);
  mut_copy(r, 0,  w, 0, ia);
  mut_copy(r, ia, x, 0, ia);
  B rb = mut_fp(r);
  usz* sh = arr_shAllocR(rb, wr+1);
  if (sh) { sh[0]=2; memcpy(sh+1, a(w)->sh, wr*sizeof(usz)); }
  if (!SFNS_FILLS) { dec(w); dec(x); return rb; }
  B rf = fill_both(w, x);
  dec(w); dec(x);
  return qWithFill(rb, rf);
}


static inline void shift_check(B w, B x) {
  ur wr = rnk(w); usz* wsh = a(w)->sh;
  ur xr = rnk(x); usz* xsh = a(x)->sh;
  if (wr+1!=xr & wr!=xr) thrF("shift: =𝕨 must be =𝕩 or ¯1+=𝕩 (%i≡=𝕨, %i≡=𝕩)", wr, xr);
  for (i32 i = 1; i < xr; i++) if (wsh[i+wr-xr] != xsh[i]) thrF("shift: Lengths not matchable (%H ≡ ≢𝕨, %H ≡ ≢𝕩)", w, x);
}

B shiftb_c1(B t, B x) {
  if (isAtm(x) || rnk(x)==0) thrM("»: Argument cannot be a scalar");
  usz ia = a(x)->ia;
  if (ia==0) return x;
  B xf = getFillE(x);
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
  B xf = getFillE(x);
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

extern B rt_group;
B group_c1(B t, B x) {
  return c1(rt_group, x);
}
B group_c2(B t, B w, B x) {
  if (isArr(w)&isArr(x) && rnk(w)==1 && rnk(x)==1) {
    usz wia = a(w)->ia;
    usz xia = a(x)->ia;
    if (wia-xia > 1) thrF("⊔: ≠𝕨 must be either ≠𝕩 or one bigger (%s≡≠𝕨, %s≡≠𝕩)", wia, xia);
    
    if (TI(w).elType==el_i32) {
      i32* wp = i32any_ptr(w);
      i64 ria = wia==xia? -1 : wp[xia]-1;
      for (usz i = 0; i < xia; i++) if (wp[i]>ria) ria = wp[i];
      if (ria>USZ_MAX-1) thrOOM();
      ria++;
      TALLOC(i32, len, ria);
      TALLOC(i32, pos, ria);
      for (usz i = 0; i < ria; i++) len[i] = pos[i] = 0;
      for (usz i = 0; i < xia; i++) { i32 n = wp[i]; if (n>=0) len[n]++; }
      
      B r = m_fillarrp(ria); fillarr_setFill(r, m_f64(0));
      arr_shVec(r, ria);
      B* rp = fillarr_ptr(r);
      for (usz i = 0; i < ria; i++) rp[i] = m_f64(0); // don't break if allocation errors
      B xf = getFillQ(x);
      
      B rf = m_fillarrp(0); fillarr_setFill(rf, m_f64(0));
      arr_shVec(rf, 0);
      fillarr_setFill(r, rf);
      if (TI(x).elType==el_i32) {
        for (usz i = 0; i < ria; i++) { i32* t; rp[i] = m_i32arrv(&t, len[i]); }
        i32* xp = i32any_ptr(x);
        for (usz i = 0; i < xia; i++) {
          i32 n = wp[i];
          if (n>=0) i32arr_ptr(rp[n])[pos[n]++] = xp[i];
        }
      } else if (TI(x).elType==el_c32) {
        for (usz i = 0; i < ria; i++) { u32* t; rp[i] = m_c32arrv(&t, len[i]); }
        u32* xp = c32any_ptr(x);
        for (usz i = 0; i < xia; i++) {
          i32 n = wp[i];
          if (n>=0) c32arr_ptr(rp[n])[pos[n]++] = xp[i];
        }
      } else {
        for (usz i = 0; i < ria; i++) {
          B c = m_fillarrp(len[i]);
          fillarr_setFill(c, inc(xf));
          a(c)->ia = 0;
          rp[i] = c;
        }
        BS2B xget = TI(x).get;
        for (usz i = 0; i < xia; i++) {
          i32 n = wp[i];
          if (n>=0) fillarr_ptr(rp[n])[pos[n]++] = xget(x, i);
        }
        for (usz i = 0; i < ria; i++) { arr_shVec(rp[i], len[i]); }
      }
      fillarr_setFill(rf, xf);
      dec(w); dec(x); TFREE(len); TFREE(pos);
      return r;
    } else {
      BS2B wgetU = TI(w).getU;
      i64 ria = wia==xia? -1 : o2i64(wgetU(w, xia))-1;
      for (usz i = 0; i < xia; i++) {
        if (!isNum(w)) goto base;
        i64 c = o2i64(wgetU(w, i));
        if (c>ria) ria = c;
      }
      if (ria>USZ_MAX-1) thrOOM();
      ria++;
      TALLOC(i32, len, ria);
      TALLOC(i32, pos, ria);
      for (usz i = 0; i < ria; i++) len[i] = pos[i] = 0;
      for (usz i = 0; i < xia; i++) {
        i64 n = o2i64u(wgetU(w, i));
        if (n>=0) len[n]++;
      }
      
      B r = m_fillarrp(ria); fillarr_setFill(r, m_f64(0));
      arr_shVec(r, ria);
      B* rp = fillarr_ptr(r);
      for (usz i = 0; i < ria; i++) rp[i] = m_f64(0); // don't break if allocation errors
      B xf = getFillQ(x);
      
      for (usz i = 0; i < ria; i++) {
        B c = m_fillarrp(len[i]);
        fillarr_setFill(c, inc(xf));
        a(c)->ia = 0;
        rp[i] = c;
      }
      B rf = m_fillarrp(0);
      arr_shVec(rf, 0);
      fillarr_setFill(rf, xf);
      fillarr_setFill(r, rf);
      BS2B xget = TI(x).get;
      for (usz i = 0; i < xia; i++) {
        i64 n = o2i64u(wgetU(w, i));
        if (n>=0) fillarr_ptr(rp[n])[pos[n]++] = xget(x, i);
      }
      for (usz i = 0; i < ria; i++) { arr_shVec(rp[i], len[i]); }
      dec(w); dec(x); TFREE(len); TFREE(pos);
      return r;
    }
  }
  base:
  return c2(rt_group, w, x);
}

extern B rt_reverse;
B reverse_c1(B t, B x) {
  if (isAtm(x) || rnk(x)==0) thrM("⌽: Argument cannot be a unit");
  B xf = getFillQ(x);
  u8 xe = TI(x).elType;
  usz xia = a(x)->ia;
  if (rnk(x)==1) {
    if (xe==el_i32) {
      i32* xp = i32any_ptr(x);
      i32* rp; B r = m_i32arrv(&rp, xia);
      for (usz i = 0; i < xia; i++) rp[i] = xp[xia-i-1];
      dec(x);
      return r;
    }
  }
  usz csz = arr_csz(x);
  usz cam = a(x)->sh[0];
  usz rp = 0;
  usz ip = xia;
  MAKE_MUT(r, xia); mut_to(r, xe);
  for (usz i = 0; i < cam; i++) {
    ip-= csz;
    mut_copy(r, rp, x, ip, csz);
    rp+= csz;
  }
  return withFill(mut_fcd(r, x), xf);
}
B reverse_c2(B t, B w, B x) {
  if (isArr(w)) return c2(rt_reverse, w, x);
  if (isAtm(x) || rnk(x)==0) thrM("⌽: 𝕩 must have rank at least 1 for atom 𝕨");
  usz xia = a(x)->ia;
  if (xia==0) return x;
  B xf = getFillQ(x);
  usz cam = a(x)->sh[0];
  usz csz = arr_csz(x);
  i64 am = o2i64(w);
  if ((usz)am >= cam) { am%= cam; if(am<0) am+= cam; }
  am*= csz;
  MAKE_MUT(r, xia); mut_to(r, TI(x).elType);
  mut_copy(r, 0, x, am, xia-am);
  mut_copy(r, xia-am, x, 0, am);
  return withFill(mut_fcd(r, x), xf);
}

B reverse_uc1(B t, B o, B x) {
  return reverse_c1(t, c1(o, reverse_c1(t, x)));
}

B pick_uc1(B t, B o, B x) {
  if (isAtm(x) || a(x)->ia==0) return def_fn_uc1(t, o, x);
  B xf = getFillQ(x);
  usz ia = a(x)->ia;
  B arg = TI(x).get(x, 0);
  B rep = c1(o, arg);
  MAKE_MUT(r, ia); mut_to(r, el_or(TI(x).elType, selfElType(rep)));
  mut_set(r, 0, rep);
  mut_copy(r, 1, x, 1, ia-1);
  return qWithFill(mut_fcd(r, x), xf);
}

B pick_ucw(B t, B o, B w, B x) {
  if (isArr(w) || isAtm(x) || rnk(x)!=1) return def_fn_ucw(t, o, w, x);
  usz xia = a(x)->ia;
  usz wi = WRAP(o2i64(w), xia, thrF("𝔽⌾(n⊸⊑)𝕩: reading out-of-bounds (n≡%R, %s≡≠𝕩)", w, xia));
  B xf = getFillQ(x);
  B arg = TI(x).get(x, wi);
  B rep = c1(o, arg);
  if (reusable(x) && TI(x).canStore(rep)) {
    if (TI(x).elType==el_i32) {
      i32* xp = i32any_ptr(x);
      xp[wi] = o2i(rep);
      return x;
    } else if (v(x)->type==t_harr) {
      B* xp = harr_ptr(x);
      dec(xp[wi]);
      xp[wi] = rep;
      return x;
    } else if (TI(x).elType==el_f64) {
      f64* xp = f64any_ptr(x);
      xp[wi] = o2f(rep);
      return x;
    } else if (v(x)->type==t_fillarr) {
      B* xp = fillarr_ptr(x);
      dec(xp[wi]);
      xp[wi] = rep;
      return x;
    }
  }
  MAKE_MUT(r, xia); mut_to(r, el_or(TI(x).elType, selfElType(rep)));
  mut_set(r, wi, rep);
  mut_copy(r, 0, x, 0, wi);
  mut_copy(r, wi+1, x, wi+1, xia-wi-1);
  return qWithFill(mut_fcd(r, x), xf);
}

B slash_ucw(B t, B o, B w, B x) {
  if (isAtm(w) || isAtm(x) || rnk(w)!=1 || rnk(x)!=1 || a(w)->ia!=a(x)->ia) return def_fn_ucw(t, o, w, x);
  usz ia = a(x)->ia;
  BS2B wgetU = TI(w).getU;
  if (TI(w).elType!=el_i32) for (usz i = 0; i < ia; i++) if (!q_i32(wgetU(w,i))) return def_fn_ucw(t, o, w, x);
  B arg = slash_c2(t, inc(w), inc(x));
  usz argIA = a(arg)->ia;
  B rep = c1(o, arg);
  if (isAtm(rep) || rnk(rep)!=1 || a(rep)->ia != argIA) thrF("𝔽⌾(a⊸/)𝕩: Result of 𝔽 must have the same shape as a/𝕩 (expected ⟨%s⟩, got %H)", argIA, rep);
  MAKE_MUT(r, ia); mut_to(r, el_or(TI(x).elType, TI(rep).elType));
  BS2B xget = TI(x).get;
  BS2B rgetU = TI(rep).getU;
  BS2B rget = TI(rep).get;
  usz repI = 0;
  for (usz i = 0; i < ia; i++) {
    i32 cw = o2iu(wgetU(w, i));
    if (cw) {
      B cr = rget(rep,repI);
      if (CHECK_VALID) for (i32 j = 1; j < cw; j++) if (!equal(rgetU(rep,repI+j), cr)) { mut_pfree(r,i); thrM("𝔽⌾(a⊸/): Incompatible result elements"); }
      mut_set(r, i, cr);
      repI+= cw;
    } else mut_set(r, i, xget(x,i));
  }
  dec(w); dec(rep);
  return mut_fcd(r, x);
}

B select_ucw(B t, B o, B w, B x) {
  if (isAtm(x) || rnk(x)!=1 || isAtm(w) || rnk(w)!=1) return def_fn_ucw(t, o, w, x);
  usz xia = a(x)->ia;
  usz wia = a(w)->ia;
  BS2B wgetU = TI(w).getU;
  if (TI(w).elType!=el_i32) for (usz i = 0; i < wia; i++) if (!q_i64(wgetU(w,i))) return def_fn_ucw(t, o, w, x);
  B arg = select_c2(t, inc(w), inc(x));
  B rep = c1(o, arg);
  if (isAtm(rep) || rnk(rep)!=1 || a(rep)->ia != wia) thrF("𝔽⌾(a⊸⊏)𝕩: Result of 𝔽 must have the same shape as a⊏𝕩 (expected ⟨%s⟩, got %H)", wia, rep);
  #if CHECK_VALID
    TALLOC(bool, set, xia);
    for (i64 i = 0; i < xia; i++) set[i] = false;
    #define EQ(F) if (set[cw] && (F)) thrM("𝔽⌾(a⊸⊏): Incompatible result elements"); set[cw] = true;
    #define FREE_CHECK TFREE(set)
  #else
    #define EQ(F)
    #define FREE_CHECK
  #endif
  if (TI(w).elType==el_i32) {
    i32* wp = i32any_ptr(w);
    if (reusable(x) && TI(x).elType==TI(rep).elType) {
      if (v(x)->type==t_i32arr) {
        i32* xp = i32arr_ptr(x);
        i32* rp = i32any_ptr(rep);
        for (usz i = 0; i < wia; i++) {
          i64 cw = wp[i]; if (cw<0) cw+= (i64)xia; // already checked
          i32 cr = rp[i];
          EQ(cr!=xp[cw]);
          xp[cw] = cr;
        }
        dec(w); dec(rep); FREE_CHECK;
        return x;
      } else if (v(x)->type==t_harr) {
        B* xp = harr_ptr(x);
        BS2B rget = TI(rep).get;
        for (usz i = 0; i < wia; i++) {
          i64 cw = wp[i]; if (cw<0) cw+= (i64)xia;
          B cr = rget(rep, i);
          EQ(!equal(cr,xp[cw]));
          dec(xp[cw]);
          xp[cw] = cr;
        }
        dec(w); dec(rep); FREE_CHECK;
        return x;
      }
    }
    MAKE_MUT(r, xia); mut_to(r, el_or(TI(x).elType, TI(rep).elType));
    mut_copy(r, 0, x, 0, xia);
    BS2B rget = TI(rep).get;
    for (usz i = 0; i < wia; i++) {
      i64 cw = wp[i]; if (cw<0) cw+= (i64)xia;
      B cr = rget(rep, i);
      EQ(!equal(mut_getU(r, cw), cr));
      mut_rm(r, cw);
      mut_setS(r, cw, cr);
    }
    dec(w); dec(rep); FREE_CHECK;
    return mut_fcd(r, x);
  }
  MAKE_MUT(r, xia); mut_to(r, el_or(TI(x).elType, TI(rep).elType));
  mut_copy(r, 0, x, 0, xia);
  BS2B rget = TI(rep).get;
  for (usz i = 0; i < wia; i++) {
    i64 cw = o2i64u(wgetU(w, i)); if (cw<0) cw+= (i64)xia;
    B cr = rget(rep, i);
    EQ(!equal(mut_getU(r, cw), cr));
    mut_rm(r, cw);
    mut_set(r, cw, cr);
  }
  dec(w); dec(rep); FREE_CHECK;
  return mut_fcd(r, x);
  #undef EQ
  #undef FREE_CHECK
}


void sfns_init() {
  c(BFn,bi_pick)->uc1 = pick_uc1;
  c(BFn,bi_reverse)->uc1 = reverse_uc1;
  c(BFn,bi_pick)->ucw = pick_ucw;
  c(BFn,bi_slash)->ucw = slash_ucw;
  c(BFn,bi_select)->ucw = select_ucw;
}
