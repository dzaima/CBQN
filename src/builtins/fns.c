#include "../core.h"
#include "../utils/hash.h"
#include "../utils/mut.h"
#include "../utils/talloc.h"
#include "../nfns.h"



void ud_rec(B** p, usz d, usz r, i32* pos, usz* sh) {
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
    if (LIKELY(xu<=I8_MAX)) {
      if (RARE(xu==0)) return emptyIVec();
      i8* rp; B r = m_i8arrv(&rp, xu);
      for (usz i = 0; i < xu; i++) rp[i] = i;
      return r;
    }
    if (xu<=I16_MAX) {
      i16* rp; B r = m_i16arrv(&rp, xu);
      for (usz i = 0; i < xu; i++) rp[i] = i;
      return r;
    }
    if (xu<=I32_MAX) {
      i32* rp; B r = m_i32arrv(&rp, xu);
      for (usz i = 0; i < xu; i++) rp[i] = i;
      return r;
    }
    f64* rp; B r = m_f64arrv(&rp, xu);
    for (usz i = 0; i < xu; i++) rp[i] = i;
    return r;
  }
  SGetU(x)
  usz xia = a(x)->ia;
  if (rnk(x)!=1) thrF("↕: Argument must be either an integer or integer list (had rank %i)", rnk(x));
  if (xia>UR_MAX) thrF("↕: Result rank too large (%s≡≠𝕩)", xia);
  usz sh[xia];
  usz ria = 1;
  for (usz i = 0; i < xia; i++) {
    usz c = o2s(GetU(x, i));
    if (c > I32_MAX) thrM("↕: Result too large");
    sh[i] = c;
    if (c*(u64)ria >= U32_MAX) thrM("↕: Result too large");
    ria*= c;
  }
  dec(x);
  
  Arr* r = m_fillarrp(ria); fillarr_setFill(r, m_f64(0));
  B* rp = fillarr_ptr(r);
  for (usz i = 0; i < ria; i++) rp[i] = m_f64(0); // don't break if allocation errors
  
  usz* rsh = arr_shAlloc(r, xia);
  if (rsh) memcpy(rsh, sh, sizeof(usz)*xia);
  
  i32 pos[xia]; B* crp = rp;
  ud_rec(&crp, 0, xia, pos, sh);
  
  if (ria) fillarr_setFill(r, inc(rp[0]));
  else {
    i32* fp;
    fillarr_setFill(r, m_i32arrv(&fp, xia));
    for (usz i = 0; i < xia; i++) fp[i] = 0;
  }
  return taga(r);
}

extern B rt_ud;
B ud_c2(B t, B w, B x) {
  return c2(rt_ud, w, x);
}

B ltack_c1(B t,      B x) {         return x; }
B ltack_c2(B t, B w, B x) { dec(x); return w; }
B rtack_c1(B t,      B x) {         return x; }
B rtack_c2(B t, B w, B x) { dec(w); return x; }

B fne_c1(B t, B x) {
  if (isAtm(x)) {
    dec(x);
    return emptyIVec();
  }
  ur xr = rnk(x);
  usz* sh = a(x)->sh;
  usz or = 0;
  for (i32 i = 0; i < xr; i++) or|= sh[i];
  B r;
  if      (or<=I8_MAX ) { i8*  rp; r = m_i8arrv (&rp, xr); for (i32 i = 0; i < xr; i++) rp[i] = sh[i]; }
  else if (or<=I16_MAX) { i16* rp; r = m_i16arrv(&rp, xr); for (i32 i = 0; i < xr; i++) rp[i] = sh[i]; }
  else if (or<=I32_MAX) { i32* rp; r = m_i32arrv(&rp, xr); for (i32 i = 0; i < xr; i++) rp[i] = sh[i]; }
  else                  { f64* rp; r = m_f64arrv(&rp, xr); for (i32 i = 0; i < xr; i++) rp[i] = sh[i]; }
  dec(x); return r;
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
  if (xia==0) { dec(x); return emptyIVec(); }
  if (rnk(x)==1 && TI(x,elType)==el_i32) {
    i32* xp = i32any_ptr(x);
    i32 min=I32_MAX, max=I32_MIN;
    for (usz i = 0; i < xia; i++) {
      i32 c = xp[i];
      if (c<min) min = c;
      if (c>max) max = c;
    }
    i64 dst = 1 + (max-(i64)min);
    if ((dst<xia*5 || dst<50) && min!=I32_MIN) {
      i32* rp; B r = m_i32arrv(&rp, xia);
      TALLOC(i32, tmp, dst);
      for (i64 i = 0; i < dst; i++) tmp[i] = I32_MIN;
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
  //   SGet(x)
  //   i32 ctr = 0;
  //   for (usz i = 0; i < xia; i++) {
  //     u64 hash = bqn_hash(Get(x,i), wy_secret);
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
    SGetU(x)
    i32 ctr = 0;
    for (usz i = 0; i < xia; i++) {
      bool had; u64 p = mk_b2i(&map, GetU(x,i), &had);
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
      B el = isArr(x)? IGetU(x,0) : x;
      i32 res = wia;
      if (TI(w,elType)==el_i32) {
        if (q_i32(el)) {
          i32* wp = i32any_ptr(w);
          i32 v = o2iu(el);
          for (usz i = 0; i < wia; i++) {
            if (wp[i] == v) { res = i; break; }
          }
        }
      } else {
        SGetU(w)
        for (usz i = 0; i < wia; i++) {
          if (equal(GetU(w,i), el)) { res = i; break; }
        }
      }
      dec(w); dec(x);
      i32* rp; Arr* r = m_i32arrp(&rp, 1);
      arr_shAlloc(r, 0);
      rp[0] = res;
      return taga(r);
    } else if (rnk(x)==1) {
      usz wia = a(w)->ia;
      usz xia = a(x)->ia;
      // TODO O(wia×xia) for small wia or xia
      i32* rp; B r = m_i32arrv(&rp, xia);
      H_b2i* map = m_b2i(64);
      SGetU(x)
      SGetU(w)
      for (usz i = 0; i < wia; i++) {
        bool had; u64 p = mk_b2i(&map, GetU(w,i), &had);
        if (!had) map->a[p].val = i;
      }
      for (usz i = 0; i < xia; i++) rp[i] = getD_b2i(map, GetU(x,i), wia);
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
  
  u64* rp; B r = m_bitarrv(&rp, xia);
  H_Sb* set = m_Sb(64);
  SGetU(x)
  for (usz i = 0; i < xia; i++) bitp_set(rp, i, !ins_Sb(&set, GetU(x,i)));
  free_Sb(set); dec(x);
  return r;
}
B memberOf_c2(B t, B w, B x) {
  if (!isArr(w) || rnk(w)!=1 || !isArr(x) || rnk(x)!=1) return c2(rt_memberOf, w, x);
  usz wia = a(w)->ia;
  usz xia = a(x)->ia;
  // TODO O(wia×xia) for small wia or xia
  H_Sb* set = m_Sb(64);
  bool had;
  SGetU(x)
  SGetU(w)
  for (usz i = 0; i < xia; i++) mk_Sb(&set, GetU(x,i), &had);
  u64* rp; B r = m_bitarrv(&rp, wia);
  for (usz i = 0; i < wia; i++) bitp_set(rp, i, has_Sb(set, GetU(w,i)));
  free_Sb(set); dec(w);dec(x);
  return r;
}

extern B rt_find;
B find_c1(B t, B x) {
  if (isAtm(x) || rnk(x)==0) thrM("⍷: Argument cannot have rank 0");
  usz xia = a(x)->ia;
  B xf = getFillQ(x);
  if (rnk(x)!=1) return c1(rt_find, x);
  
  B r = emptyHVec();
  H_Sb* set = m_Sb(64);
  SGetU(x)
  for (usz i = 0; i < xia; i++) {
    B c = GetU(x,i);
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
  SGetU(x)
  for (usz i = 0; i < xia; i++) {
    bool had; u64 p = mk_b2i(&map, GetU(x,i), &had);
    rp[i] = had? ++map->a[p].val : (map->a[p].val = 0);
  }
  dec(x); free_b2i(map);
  return r;
}
B count_c2(B t, B w, B x) {
  return c2(rt_count, w, x);
}

static H_b2i* prevImports;
i32 getPrevImport(B path) { // -1 for unset, -2 for unfinished
  if (prevImports==NULL) prevImports = m_b2i(16);
  
  bool had; i32 prev = mk_b2i(&prevImports, path, &had);
  if (had) return prevImports->a[prev].val;
  prevImports->a[prev].val = -2;
  return -1;
}
void setPrevImport(B path, i32 pos) {
  bool had; i32 prev = mk_b2i(&prevImports, path, &had);
  prevImports->a[prev].val = pos;
}

static H_b2i* globalNames;
static B globalNameList;
i32 str2gid(B s) {
  if (globalNames==NULL) {
    globalNames = m_b2i(32);
    globalNameList = emptyHVec();
  }
  bool had;
  u64 p = mk_b2i(&globalNames, s, &had);
  // if(had) print_fmt("str2gid %R → %i\n", s, globalNames->a[p].val); else print_fmt("str2gid %R → %i!!\n", s, a(globalNameList)->ia);
  if(had) return globalNames->a[p].val;
  
  i32 r = a(globalNameList)->ia;
  globalNameList = vec_add(globalNameList, inc(s));
  globalNames->a[p].val = r;
  return r;
}

B gid2str(i32 n) {
  B r = IGetU(globalNameList, n);
  // print_fmt("gid2str %i → %R\n", n, r);
  return r;
}



void fun_gcFn() {
  if (prevImports!=NULL) mm_visitP(prevImports);
  if (globalNames!=NULL) mm_visitP(globalNames);
  mm_visit(globalNameList);
}



static void print_funBI(FILE* f, B x) { fprintf(f, "%s", pfn_repr(c(Fun,x)->extra)); }
static B funBI_uc1(B t, B o,      B x) { return c(BFn,t)->uc1(t, o,    x); }
static B funBI_ucw(B t, B o, B w, B x) { return c(BFn,t)->ucw(t, o, w, x); }
static B funBI_im(B t, B x) { return c(BFn,t)->im(t, x); }
static B funBI_identity(B x) { return inc(c(BFn,x)->ident); }
void fns_init() {
  gc_addFn(fun_gcFn);
  TIi(t_funBI,print) = print_funBI;
  TIi(t_funBI,identity) = funBI_identity;
  TIi(t_funBI,fn_uc1) = funBI_uc1;
  TIi(t_funBI,fn_ucw) = funBI_ucw;
  TIi(t_funBI,fn_im) = funBI_im;
}
