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
  usz xia = IA(x);
  if (RNK(x)!=1) thrF("↕: Argument must be either an integer or integer list (had rank %i)", RNK(x));
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
  decG(x);
  
  Arr* r = m_fillarrp(ria); fillarr_setFill(r, m_f64(0));
  B* rp = fillarr_ptr(r);
  for (usz i = 0; i < ria; i++) rp[i] = m_f64(0); // don't break if allocation errors
  
  usz* rsh = arr_shAlloc(r, xia);
  if (rsh) shcpy(rsh, sh, xia);
  
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

B ud_c2(B t, B w, B x) {
  usz wia=1;
  if (isArr(w)) {
    if (RNK(w)>1) thrM("↕: 𝕨 must have rank at most 1");
    wia = IA(w);
    if (wia==0) { decG(w); return x; }
  }
  ur xr;
  if (isAtm(x) || (xr=RNK(x))<wia) thrM("↕: Length of 𝕨 must be at most rank of 𝕩");
  if (xr+wia > UR_MAX) thrM("↕: Result rank too large");
  ur wr = wia;
  ur rr = xr + wr;
  ShArr* sh = m_shArr(rr);
  usz* rsh = sh->a;
  usz* wsh = rsh + wr;
  if (isAtm(w)) {
    wsh[0] = o2s(w);
  } else {
    SGetU(w)
    for (usz i=0; i<wr; i++) wsh[i] = o2s(GetU(w, i));
    decG(w);
  }

  usz* xsh = SH(x);
  bool empty = IA(x)==0;
  for (usz i=0; i<wr; i++) {
    usz l = xsh[i] + 1;
    usz m = wsh[i];
    if (l<m) thrM("↕: Window length 𝕨 must be at most axis length plus one");
    empty|= m==0 | m==l;
    rsh[i] = l - m;
  }
  for (usz i=wr; i<xr; i++) wsh[i] = xsh[i];

  if (empty) {
    Arr* ra = m_fillarrp(0);
    arr_shSetU(ra, rr, sh);
    fillarr_setFill(ra, getFillQ(x));
    decG(x);
    return taga(ra);
  }
  ur fr=2*wr; // Frame rank in result
  usz cia=1; // Cell length
  for (usz i=fr; i<rr; i++) if (mulOn(cia, rsh[i])) thrM("↕: result shape too large");
  usz ria=cia;
  for (usz i=0;  i<fr; i++) if (mulOn(ria, rsh[i])) thrM("↕: result shape too large");
  B xf = getFillQ(x);
  TALLOC(usz, ri, fr-1);
  MAKE_MUT(r, ria); mut_init(r, TI(x,elType));
  MUTG_INIT(r);
  usz k = cia*rsh[fr-1];
  if (wr==1) {
    for (usz i=0, j=0; i<ria; i+=k, j+=cia) mut_copyG(r, i, x, j, k);
  } else {
    for (usz i=0; i<fr-1; i++) ri[i]=0;
    for (usz i=0, j=0;;) {
      mut_copyG(r, i, x, j, k);
      usz str = cia*xsh[xr-1];
      i += k;
      if (i == ria) break;
      j += str;
      for (usz a=fr-2, b=xr-2; RARE(++ri[a] == rsh[a]); ) {
        ri[a] = 0;
        j -= rsh[a] * str;
        str *= xsh[b]; if (!b) { str=cia; b=xr; }
        a--; b--;
        j += str;
      }
    }
  }
  decG(x); TFREE(ri);
  Arr* ra = mut_fp(r);
  arr_shSetU(ra, rr, sh);
  return withFill(taga(ra), xf);
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
  ur xr = RNK(x);
  usz* sh = SH(x);
  usz or = 0;
  for (i32 i = 0; i < xr; i++) or|= sh[i];
  B r;
  if      (or<=I8_MAX ) { i8*  rp; r = m_i8arrv (&rp, xr); for (i32 i = 0; i < xr; i++) rp[i] = sh[i]; }
  else if (or<=I16_MAX) { i16* rp; r = m_i16arrv(&rp, xr); for (i32 i = 0; i < xr; i++) rp[i] = sh[i]; }
  else if (or<=I32_MAX) { i32* rp; r = m_i32arrv(&rp, xr); for (i32 i = 0; i < xr; i++) rp[i] = sh[i]; }
  else                  { f64* rp; r = m_f64arrv(&rp, xr); for (i32 i = 0; i < xr; i++) rp[i] = sh[i]; }
  decG(x); return r;
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
B indexOf_c2(B t, B w, B x) {
  if (!isArr(w) || RNK(w)==0) thrM("⊐: 𝕨 must have rank at least 1");
  if (RNK(w)==1) {
    if (!isArr(x) || RNK(x)==0) {
      usz wia = IA(w);
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
      decG(w); dec(x);
      i32* rp; Arr* r = m_i32arrp(&rp, 1);
      arr_shAlloc(r, 0);
      rp[0] = res;
      return taga(r);
    } else {
      usz wia = IA(w);
      usz xia = IA(x);
      // TODO O(wia×xia) for small wia or xia
      i32* rp; B r = m_i32arrc(&rp, x);
      H_b2i* map = m_b2i(64);
      SGetU(x)
      SGetU(w)
      for (usz i = 0; i < wia; i++) {
        bool had; u64 p = mk_b2i(&map, GetU(w,i), &had);
        if (!had) map->a[p].val = i;
      }
      for (usz i = 0; i < xia; i++) rp[i] = getD_b2i(map, GetU(x,i), wia);
      free_b2i(map); decG(w); decG(x);
      return wia<=I8_MAX? taga(cpyI8Arr(r)) : wia<=I16_MAX? taga(cpyI16Arr(r)) : r;
    }
  }
  return c2(rt_indexOf, w, x);
}

B enclosed_0;
B enclosed_1;
extern B rt_memberOf;
B memberOf_c2(B t, B w, B x) {
  if (isAtm(x) || RNK(x)!=1) goto bad;
  if (isAtm(w)) goto single;
  ur wr = RNK(w);
  if (wr==0) {
    B w0 = IGet(w, 0);
    dec(w);
    w = w0;
    goto single;
  }
  if (wr==1) goto many;
  goto bad;
  
  bad: return c2(rt_memberOf, w, x);
  
  B r;
  single: {
    usz xia = IA(x);
    SGetU(x)
    for (usz i = 0; i < xia; i++) if (equal(GetU(x, i), w)) { r = inc(enclosed_1); goto dec_wx; }
    r = inc(enclosed_0);
    dec_wx:; dec(w);
    goto dec_x;
  }
  
  
  many: {
    usz xia = IA(x);
    usz wia = IA(w);
    // TODO O(wia×xia) for small wia or xia
    H_Sb* set = m_Sb(64);
    SGetU(x) SGetU(w)
    bool had;
    for (usz i = 0; i < xia; i++) mk_Sb(&set, GetU(x,i), &had);
    u64* rp; r = m_bitarrv(&rp, wia);
    for (usz i = 0; i < wia; i++) bitp_set(rp, i, has_Sb(set, GetU(w,i)));
    free_Sb(set); decG(w);
    goto dec_x;
  }
  
  dec_x:;
  decG(x);
  return r;
}

extern B rt_find;
B find_c2(B t, B w, B x) {
  return c2(rt_find, w, x);
}

extern B rt_count;
B count_c2(B t, B w, B x) {
  return c2(rt_count, w, x);
}

static H_b2i* prevImports;
i32 getPrevImport(B path) { // -1 for unset, -2 for unfinished
  if (prevImports==NULL) prevImports = m_b2i(16);
  
  bool had; i32 prev = mk_b2i(&prevImports, path, &had);
  if (had && prevImports->a[prev].val!=-1) return prevImports->a[prev].val;
  prevImports->a[prev].val = -2;
  return -1;
}
void setPrevImport(B path, i32 pos) {
  bool had; i32 prev = mk_b2i(&prevImports, path, &had);
  prevImports->a[prev].val = pos;
}
void clearImportCacheMap() {
  if (prevImports!=NULL) free_b2i(prevImports);
  prevImports = NULL;
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
  // if(had) print_fmt("str2gid %R → %i\n", s, globalNames->a[p].val); else print_fmt("str2gid %R → %i!!\n", s, IA(globalNameList));
  if(had) return globalNames->a[p].val;
  
  i32 r = IA(globalNameList);
  globalNameList = vec_addN(globalNameList, inc(s));
  globalNames->a[p].val = r;
  return r;
}

i32 str2gidQ(B s) { // if the name doesn't exist yet, return -1
  if (globalNames==NULL) return -1; // if there are no names, there certainly won't be the queried one
  return getD_b2i(globalNames, s, -1);
}

B gid2str(i32 n) {
  B r = IGetU(globalNameList, n);
  // print_fmt("gid2str %i → %R\n", n, r);
  return r;
}

void* profiler_makeMap() {
  return m_b2i(64);
}
i32 profiler_index(void** mapRaw, B comp) {
  H_b2i* map = *(H_b2i**)mapRaw;
  i32 r;
  bool had; u64 p = mk_b2i(&map, comp, &had);
  if (had) r = map->a[p].val;
  else     r = map->a[p].val = map->pop-1;
  *(H_b2i**)mapRaw = map;
  return r;
}
void profiler_freeMap(void* mapRaw) {
  free_b2i((H_b2i*)mapRaw);
}



void fun_gcFn() {
  if (prevImports!=NULL) mm_visitP(prevImports);
  if (globalNames!=NULL) mm_visitP(globalNames);
  mm_visit(enclosed_0);
  mm_visit(enclosed_1);
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
  { u64* p; Arr* a=m_bitarrp(&p, 1); arr_shAlloc(a,0); *p= 0;    enclosed_0=taga(a); }
  { u64* p; Arr* a=m_bitarrp(&p, 1); arr_shAlloc(a,0); *p=~0ULL; enclosed_1=taga(a); }
}
