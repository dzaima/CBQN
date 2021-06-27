#include "../core.h"
#include "../utils/hash.h"
#include "../utils/mut.h"
#include "../utils/talloc.h"
#include "../nfns.h"


void print_funBI(B x) { printf("%s", format_pf(c(Fun,x)->extra)); }
B funBI_uc1(B t, B o,      B x) { return c(BFn,t)->uc1(t, o,    x); }
B funBI_ucw(B t, B o, B w, B x) { return c(BFn,t)->ucw(t, o, w, x); }
B funBI_identity(B x) { return inc(c(BFn,x)->ident); }




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
  
  B r = m_fillarrp(ria); fillarr_setFill(r, m_f64(0));
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

extern B rt_ud;
B ud_c2(B t, B w, B x) {
  return c2(rt_ud, w, x);
}

B pair_c1(B t,      B x) { return m_v1(   x); }
B pair_c2(B t, B w, B x) { return m_v2(w, x); }
B ltack_c1(B t,      B x) {         return x; }
B ltack_c2(B t, B w, B x) { dec(x); return w; }
B rtack_c1(B t,      B x) {         return x; }
B rtack_c2(B t, B w, B x) { dec(w); return x; }

#ifdef RT_WRAP
B rtWrap_unwrap(B x);
#endif
B fmtF_c1(B t, B x) {
  if (!isVal(x)) return m_str32(U"(fmtF: not given a function)");
  #ifdef RT_WRAP
    x = rtWrap_unwrap(x);
  #endif
  u8 fl = v(x)->flags;
  if (fl==0 || fl>rtLen) {
    u8 ty = v(x)->type;
    if (ty==t_funBI) { B r = fromUTF8l(format_pf (c(Fun,x)->extra)); dec(x); return r; }
    if (ty==t_md1BI) { B r = fromUTF8l(format_pm1(c(Md1,x)->extra)); dec(x); return r; }
    if (ty==t_md2BI) { B r = fromUTF8l(format_pm2(c(Md2,x)->extra)); dec(x); return r; }
    if (ty==t_nfn) { B r = nfn_name(x); dec(x); return r; }
    return m_str32(U"(fmtF: not given a runtime primitive)");
  }
  dec(x);
  return m_c32(U"+-×÷⋆√⌊⌈|¬∧∨<>≠=≤≥≡≢⊣⊢⥊∾≍↑↓↕«»⌽⍉/⍋⍒⊏⊑⊐⊒∊⍷⊔!˙˜˘¨⌜⁼´˝`∘○⊸⟜⌾⊘◶⎉⚇⍟⎊"[fl-1]);
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


extern B rt_indexOf;
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
  if (!isArr(w) || rnk(w)==0) thrM("⊐: 𝕨 must have rank at least 1");
  if (rnk(w)==1) {
    if (!isArr(x) || rnk(x)==0) {
      usz wia = a(w)->ia;
      B el = isArr(x)? TI(x).getU(x,0) : x;
      i32 res = wia;
      if (TI(w).elType==el_i32) {
        if (q_i32(el)) {
          i32* wp = i32any_ptr(w);
          i32 v = o2iu(el);
          for (usz i = 0; i < wia; i++) {
            if (wp[i] == v) { res = i; break; }
          }
        }
      } else {
        BS2B wgetU = TI(w).getU;
        for (usz i = 0; i < wia; i++) {
          if (equal(wgetU(w,i), el)) { res = i; break; }
        }
      }
      dec(w); dec(x);
      i32* rp; B r = m_i32arrp(&rp, 1);
      arr_shAllocR(r,0);
      rp[0] = res;
      return r;
    } else if (rnk(x)==1) {
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
  }
  return c2(rt_indexOf, w, x);
}

extern B rt_memberOf;
B memberOf_c1(B t, B x) {
  if (isAtm(x) || rnk(x)==0) thrM("∊: Argument cannot have rank 0");
  if (rnk(x)!=1) x = toCells(x);
  usz xia = a(x)->ia;
  
  i32* rp; B r = m_i32arrv(&rp, xia);
  H_Sb* set = m_Sb(64);
  BS2B xgetU = TI(x).getU;
  for (usz i = 0; i < xia; i++) rp[i] = !ins_Sb(&set, xgetU(x,i));
  free_Sb(set); dec(x);
  return r;
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

extern B rt_find;
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

extern B rt_count;
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



void fns_init() {
  ti[t_funBI].print = print_funBI;
  ti[t_funBI].identity = funBI_identity;
  ti[t_funBI].fn_uc1 = funBI_uc1;
  ti[t_funBI].fn_ucw = funBI_ucw;
}
