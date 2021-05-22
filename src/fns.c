#include "h.h"

typedef struct BFn {
  struct Fun;
  B ident;
   BBB2B uc1;
  BBBB2B ucw;
} BFn;

void print_funBI(B x) { printf("%s", format_pf(c(Fun,x)->extra)); }
B funBI_uc1(B t, B o,      B x) { return c(BFn,t)->uc1(t, o,    x); }
B funBI_ucw(B t, B o, B w, B x) { return c(BFn,t)->ucw(t, o, w, x); }
B funBI_identity(B x) { return inc(c(BFn,x)->ident); }


B bqn_merge(B x) {
  assert(isArr(x));
  usz xia = a(x)->ia;
  ur xr = rnk(x);
  if (xia==0) {
    B xf = getFillE(x);
    if (isAtm(xf)) { dec(xf); return x; }
    i32 xfr = rnk(xf);
    B xff = getFillQ(xf);
    B r = m_fillarrp(0);
    fillarr_setFill(r, xff);
    if (xr+xfr > UR_MAX) thrM(">: Result rank too large");
    usz* rsh = arr_shAllocI(r, 0, xr+xfr);
    if (rsh) {
      memcpy       (rsh   , a(x)->sh,  xr *sizeof(usz));
      if(xfr)memcpy(rsh+xr, a(xf)->sh, xfr*sizeof(usz));
    }
    return r;
  }
  
  BS2B xgetU = TI(x).getU;
  B x0 = xgetU(x, 0);
  usz* elSh = isArr(x0)? a(x0)->sh : NULL;
  ur elR = isArr(x0)? rnk(x0) : 0;
  usz elIA = isArr(x0)? a(x0)->ia : 1;
  B fill = getFillQ(x0);
  if (xr+elR > UR_MAX) thrM(">: Result rank too large");
  
  MAKE_MUT(r, xia*elIA);
  usz rp = 0;
  for (usz i = 0; i < xia; i++) {
    B c = xgetU(x, i);
    if (isArr(c)? (elR!=rnk(c) || !eqShPrefix(elSh, a(c)->sh, elR)) : elR!=0) { mut_pfree(r, rp); thrF(">: Elements didn't have equal shapes (contained %H and %H)", x0, c); }
    if (isArr(c)) mut_copy(r, rp, c, 0, elIA);
    else mut_set(r, rp, c);
    if (!noFill(fill)) fill = fill_or(fill, getFillQ(c));
    rp+= elIA;
  }
  B rb = mut_fp(r);
  usz* rsh = arr_shAllocR(rb, xr+elR);
  if (rsh) {
    memcpy         (rsh   , a(x)->sh, xr *sizeof(usz));
    if (elSh)memcpy(rsh+xr, elSh,     elR*sizeof(usz));
  }
  dec(x);
  return withFill(rb,fill);
}


void ud_rec(B** p, usz d, usz r, usz* pos, usz* sh) {
  if (d==r) {
    i32* rp;
    *(*p)++ = m_i32arrv(&rp, r);
    memcpy(rp, pos, 4*r);
  } else {
    usz c = sh[d];
    for (usz i = 0; i < c; i++) {
      pos[d] = i;
      ud_rec(p, d+1, r, pos, sh);
    }
  }
}
B ud_c1(B t, B x) {
  if (isAtm(x)) {
    usz xu = o2s(x);
    if (RARE(xu>=I32_MAX)) {
      f64* rp; B r = m_f64arrv(&rp, xu);
      for (usz i = 0; i < xu; i++) rp[i] = i;
      return r;
    }
    if (xu==0) { B r = bi_emptyIVec; ptr_inc(v(r)); return r; }
    i32* rp; B r = m_i32arrv(&rp, xu);
    for (usz i = 0; i < xu; i++) rp[i] = i;
    return r;
  }
  BS2B xgetU = TI(x).getU;
  usz xia = a(x)->ia;
  if (rnk(x)!=1) thrF("↕: Argument must be either an integer or integer list (had rank %i)", rnk(x));
  if (xia>UR_MAX) thrF("↕: Result rank too large (%s≡≠𝕩)", xia);
  usz sh[xia];
  usz ria = 1;
  for (usz i = 0; i < xia; i++) {
    usz c = o2s(xgetU(x, i));
    if (c > I32_MAX) thrM("↕: Result too large");
    sh[i] = c;
    if (c*(u64)ria >= U32_MAX) thrM("↕: Result too large");
    ria*= c;
  }
  dec(x);
  B r = m_fillarrp(ria);
  
  fillarr_setFill(r, m_f64(0));
  B* rp = fillarr_ptr(r);
  for (usz i = 0; i < ria; i++) rp[i] = m_f64(0); // don't break if allocation errors
  usz* rsh = arr_shAllocI(r, ria, xia);
  if (rsh) memcpy(rsh, sh, sizeof(usz)*xia);
  
  usz pos[xia]; B* crp = rp;
  ud_rec(&crp, 0, xia, pos, sh);
  
  if (ria) fillarr_setFill(r, inc(rp[0]));
  else {
    i32* fp;
    fillarr_setFill(r, m_i32arrv(&fp, xia));
    for (usz i = 0; i < xia; i++) fp[i] = 0;
  }
  return r;
}

B rt_ud;
B ud_c2(B t, B w, B x) {
  return c2(rt_ud, w, x);
}

B pair_c1(B t,      B x) { return m_v1(   x); }
B pair_c2(B t, B w, B x) { return m_v2(w, x); }
B ltack_c1(B t,      B x) {         return x; }
B ltack_c2(B t, B w, B x) { dec(x); return w; }
B rtack_c1(B t,      B x) {         return x; }
B rtack_c2(B t, B w, B x) { dec(w); return x; }

B fmtF_c1(B t, B x) {
  if (!isVal(x)) return m_str32(U"(fmtF: not given a function)");
  u8 fl = v(x)->flags;
  if (fl==0 || fl>rtLen) {
    u8 ty = v(x)->type;
    if (ty==t_funBI) { B r = fromUTF8l(format_pf (c(Fun,x)->extra)); dec(x); return r; }
    if (ty==t_md1BI) { B r = fromUTF8l(format_pm1(c(Md1,x)->extra)); dec(x); return r; }
    if (ty==t_md2BI) { B r = fromUTF8l(format_pm2(c(Md2,x)->extra)); dec(x); return r; }
    return m_str32(U"(fmtF: not given a runtime primitive)");
  }
  dec(x);
  return m_c32(U"+-×÷⋆√⌊⌈|¬∧∨<>≠=≤≥≡≢⊣⊢⥊∾≍↑↓↕«»⌽⍉/⍋⍒⊏⊑⊐⊒∊⍷⊔!˙˜˘¨⌜⁼´˝`∘○⊸⟜⌾⊘◶⎉⚇⍟⎊"[fl-1]);
}

i64 isum(B x) { // doesn't consume; may error; TODO error on overflow
  assert(isArr(x));
  i64 r = 0;
  usz xia = a(x)->ia;
  u8 xe = TI(x).elType;
  if (xe==el_i32) {
    i32* p = i32any_ptr(x);
    for (usz i = 0; i < xia; i++) r+= p[i];
  } else if (xe==el_f64) {
    f64* p = f64any_ptr(x);
    for (usz i = 0; i < xia; i++) { if(p[i]!=(f64)p[i]) thrM("Expected integer"); r+= p[i]; }
  } else {
    BS2B xgetU = TI(x).getU;
    for (usz i = 0; i < xia; i++) r+= o2i64(xgetU(x,i));
  }
  return r;
}


B fne_c1(B t, B x) {
  if (isArr(x)) {
    ur xr = rnk(x);
    usz* sh = a(x)->sh;
    for (i32 i = 0; i < xr; i++) if (sh[i]>I32_MAX) {
      f64* rp; B r = m_f64arrv(&rp, xr);
      for (i32 j = 0; j < xr; j++) rp[j] = sh[j];
      dec(x);
      return r;
    }
    i32* rp;
    B r = m_i32arrv(&rp, xr);
    for (i32 i = 0; i < xr; i++) rp[i] = sh[i];
    dec(x);
    return r;
  } else {
    dec(x);
    return inc(bi_emptyIVec);
  }
}
u64 depth(B x) { // doesn't consume
  if (isAtm(x)) return 0;
  if (TI(x).arrD1) return 1;
  u64 r = 0;
  usz ia = a(x)->ia;
  BS2B xgetU = TI(x).getU;
  for (usz i = 0; i < ia; i++) {
    u64 n = depth(xgetU(x,i));
    if (n>r) r = n;
  }
  return r+1;
}
B feq_c1(B t, B x) {
  u64 r = depth(x);
  dec(x);
  return m_f64(r);
}


B feq_c2(B t, B w, B x) {
  bool r = equal(w, x);
  dec(w); dec(x);
  return m_i32(r);
}
B fne_c2(B t, B w, B x) {
  bool r = !equal(w, x);
  dec(w); dec(x);
  return m_i32(r);
}


B rt_indexOf;
B indexOf_c1(B t, B x) {
  if (isAtm(x)) thrM("⊐: 𝕩 cannot have rank 0");
  usz xia = a(x)->ia;
  if (xia==0) { dec(x); return inc(bi_emptyIVec); }
  if (rnk(x)==1 && TI(x).elType==el_i32) {
    i32* xp = i32any_ptr(x);
    i32 min=I32_MAX, max=I32_MIN;
    for (usz i = 0; i < xia; i++) {
      i32 c = xp[i];
      if (c<min) min = c;
      if (c>max) max = c;
    }
    i32 dst = 1 + max-(i64)min;
    if ((dst<xia*5 || dst<50) && min!=I32_MIN) {
      i32* rp; B r = m_i32arrv(&rp, xia);
      TALLOC(i32, tmp, dst);
      for (usz i = 0; i < dst; i++) tmp[i] = I32_MIN;
      i32* tc = tmp-min;
      i32 ctr = 0;
      for (usz i = 0; i < xia; i++) {
        i32 c = xp[i];
        if (tc[c]==I32_MIN) tc[c] = ctr++;
        rp[i] = tc[c];
      }
      dec(x); TFREE(tmp);
      return r;
    }
  }
  // if (rnk(x)==1) { // relies on equal hashes implying equal objects, which has like a 2⋆¯64 chance of being false per item
  //   // u64 s = nsTime();
  //   i32* rp; B r = m_i32arrv(&rp, xia);
  //   u64 size = xia*2;
  //   wyhashmap_t idx[size];
  //   i32 val[size];
  //   for (i64 i = 0; i < size; i++) { idx[i] = 0; val[i] = -1; }
  //   BS2B xget = TI(x).get;
  //   i32 ctr = 0;
  //   for (usz i = 0; i < xia; i++) {
  //     u64 hash = bqn_hash(xget(x,i), wy_secret);
  //     u64 p = wyhashmap(idx, size, &hash, 8, true, wy_secret);
  //     if (val[p]==-1) val[p] = ctr++;
  //     rp[i] = val[p];
  //   }
  //   dec(x);
  //   // u64 e = nsTime(); q1+= e-s;
  //   return r;
  // }
  if (rnk(x)==1) {
    // u64 s = nsTime();
    i32* rp; B r = m_i32arrv(&rp, xia);
    H_b2i* map = m_b2i(64);
    BS2B xgetU = TI(x).getU;
    i32 ctr = 0;
    for (usz i = 0; i < xia; i++) {
      bool had; u64 p = mk_b2i(&map, xgetU(x,i), &had);
      if (had) rp[i] = map->a[p].val;
      else     rp[i] = map->a[p].val = ctr++;
    }
    free_b2i(map); dec(x);
    // u64 e = nsTime(); q1+= e-s;
    return r;
  }
  return c1(rt_indexOf, x);
}
B indexOf_c2(B t, B w, B x) {
  if (!isArr(w) || rnk(w)!=1 || !isArr(x) || rnk(x)!=1) return c2(rt_indexOf, w, x);
  usz wia = a(w)->ia;
  usz xia = a(x)->ia;
  // TODO O(wia×xia) for small wia
  i32* rp; B r = m_i32arrv(&rp, xia);
  H_b2i* map = m_b2i(64);
  BS2B xgetU = TI(x).getU;
  BS2B wgetU = TI(w).getU;
  for (usz i = 0; i < wia; i++) {
    bool had; u64 p = mk_b2i(&map, wgetU(w,i), &had);
    if (!had) map->a[p].val = i;
  }
  for (usz i = 0; i < xia; i++) rp[i] = getD_b2i(map, xgetU(x,i), wia);
  free_b2i(map); dec(w); dec(x);
  return r;
}

B rt_memberOf;
B memberOf_c1(B t, B x) {
  return c1(rt_memberOf, x);
}
B memberOf_c2(B t, B w, B x) {
  if (!isArr(w) || rnk(w)!=1 || !isArr(x) || rnk(x)!=1) return c2(rt_memberOf, w, x);
  usz wia = a(w)->ia;
  usz xia = a(x)->ia;
  // TODO O(wia×xia) for small wia
  H_Sb* set = m_Sb(64);
  bool had;
  BS2B xgetU = TI(x).getU;
  BS2B wgetU = TI(w).getU;
  for (usz i = 0; i < xia; i++) mk_Sb(&set, xgetU(x,i), &had);
  i32* rp; B r = m_i32arrv(&rp, wia);
  for (usz i = 0; i < wia; i++) rp[i] = has_Sb(set, wgetU(w,i));
  free_Sb(set); dec(w);dec(x);
  return r;
}

B rt_find;
B find_c1(B t, B x) {
  if (isAtm(x) || rnk(x)==0) thrM("⍷: Argument cannot have rank 0");
  usz xia = a(x)->ia;
  B xf = getFillQ(x);
  if (rnk(x)!=1) return c1(rt_find, x);
  
  B r = inc(bi_emptyHVec);
  H_Sb* set = m_Sb(64);
  BS2B xgetU = TI(x).getU;
  for (usz i = 0; i < xia; i++) {
    B c = xgetU(x,i);
    if (!ins_Sb(&set, c)) r = vec_add(r, inc(c));
  }
  free_Sb(set); dec(x);
  return withFill(r, xf);
}
B find_c2(B t, B w, B x) {
  return c2(rt_find, w, x);
}

B rt_count;
B count_c1(B t, B x) {
  if (isAtm(x) || rnk(x)==0) thrM("⊒: Argument cannot have rank 0");
  if (rnk(x)>1) x = toCells(x);
  usz xia = a(x)->ia;
  i32* rp; B r = m_i32arrv(&rp, xia);
  H_b2i* map = m_b2i(64);
  BS2B xgetU = TI(x).getU;
  for (usz i = 0; i < xia; i++) {
    bool had; u64 p = mk_b2i(&map, xgetU(x,i), &had);
    rp[i] = had? ++map->a[p].val : (map->a[p].val = 0);
  }
  dec(x); free_b2i(map);
  return r;
}
B count_c2(B t, B w, B x) {
  return c2(rt_count, w, x);
}



#define BI_A(N) { B t=bi_##N = mm_alloc(sizeof(BFn), t_funBI, ftag(FUN_TAG)); BFn*f=c(BFn,t); f->c2=N##_c2    ; f->c1=N##_c1    ; f->extra=pf_##N; f->ident=bi_N; f->uc1=def_fn_uc1; f->ucw=def_fn_ucw; gc_add(t); }
#define BI_D(N) { B t=bi_##N = mm_alloc(sizeof(BFn), t_funBI, ftag(FUN_TAG)); BFn*f=c(BFn,t); f->c2=N##_c2    ; f->c1=c1_invalid; f->extra=pf_##N; f->ident=bi_N; f->uc1=def_fn_uc1; f->ucw=def_fn_ucw; gc_add(t); }
#define BI_M(N) { B t=bi_##N = mm_alloc(sizeof(BFn), t_funBI, ftag(FUN_TAG)); BFn*f=c(BFn,t); f->c2=c2_invalid; f->c1=N##_c1    ; f->extra=pf_##N; f->ident=bi_N; f->uc1=def_fn_uc1; f->ucw=def_fn_ucw; gc_add(t); }
#define BI_VAR(N) B bi_##N;
#define BI_FNS0(F) F(BI_VAR,BI_VAR,BI_VAR)
#define BI_FNS1(F) F(BI_A,BI_M,BI_D)


#define F(A,M,D) A(ud) A(fne) A(feq) A(ltack) A(rtack) M(fmtF) A(indexOf) A(memberOf) A(find) A(count)
BI_FNS0(F);
static inline void fns_init() { BI_FNS1(F)
  ti[t_funBI].print = print_funBI;
  ti[t_funBI].identity = funBI_identity;
  ti[t_funBI].fn_uc1 = funBI_uc1;
  ti[t_funBI].fn_ucw = funBI_ucw;
}
#undef F
