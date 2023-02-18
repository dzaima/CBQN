#include "../core.h"
#include "../utils/hash.h"
#include "../utils/mut.h"
#include "../utils/talloc.h"
#include "../builtins.h"
#include "../nfns.h"



static B* ud_rec(B* p, usz d, usz r, i32* pos, usz* sh) {
  usz cl = sh[d];
  if (d+1==r) {
    
    for (usz i = 0; i < cl; i++) {
      i32* p1; *p = m_i32arrv(&p1, r);
      NOUNROLL for (usz i = 0; i < d; i++) p1[i] = pos[i];
      p1[d] = i;
      p++;
    }
  } else {
    for (usz i = 0; i < cl; i++) {
      pos[d] = i;
      p = ud_rec(p, d+1, r, pos, sh);
    }
  }
  return p;
}
Arr* bitUD[3];
B bit2x[2]; // ⟨0‿1 ⋄ 1‿0⟩
B ud_c1(B t, B x) {
  if (isAtm(x)) {
    usz xu = o2s(x);
    if (LIKELY(xu<=I8_MAX)) {
      if (RARE(xu<=2)) return taga(ptr_inc(bitUD[xu]));
      i8* rp; B r = m_i8arrv(&rp, xu);
      NOUNROLL for (usz i = 0; i < xu; i++) rp[i] = i;
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
  if (xia==0) { decG(x); return m_unit(emptyIVec()); }
  usz sh[xia]; // stack allocation of rank items
  i32 pos[xia];
  usz ria = 1;
  for (usz i = xia; i--; ) {
    usz c = o2s(GetU(x, i));
    if (c > I32_MAX) thrM("↕: Result too large");
    sh[i] = c;
    if (mulOn(ria, c)) thrM("↕: Result too large");
  }
  decG(x);
  
  Arr* r = m_fillarrp(ria); fillarr_setFill(r, m_f64(0));
  B* rp = fillarr_ptr(r);
  FILL_TO(rp, el_B, 0, m_f64(0), ria); // don't have r in undefined state if allocation errors
  
  ud_rec(rp, 0, xia, pos, sh);
  
  usz* rsh = arr_shAlloc(r, xia);
  if (rsh) shcpy(rsh, sh, xia);
  
  if (ria) fillarr_setFill(r, inc(rp[0]));
  else {
    i32* fp;
    fillarr_setFill(r, m_i32arrv(&fp, xia));
    for (usz i = 0; i < xia; i++) fp[i] = 0;
  }
  return taga(r);
}

B slash_c2(B t, B w, B x);

B ud_c2(B t, B w, B x) {
  usz wia=1;
  if (isArr(w)) {
    if (RNK(w)>1) thrM("↕: 𝕨 must have rank at most 1");
    wia = IA(w);
    if (wia==0) { decG(w); return isArr(x)? x : m_atomUnit(x); }
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
    Arr* ra = arr_shSetU(m_fillarrp(0), rr, sh);
    fillarr_setFill(ra, getFillQ(x));
    decG(x);
    return taga(ra);
  }
  if (wr==1 && wsh[0]==2 && xr==1) {
    B t = C2(slash, w, x);
    return taga(arr_shSetU(TI(t,slice)(t, 1, IA(t)-2), 2, sh));
  }
  
  ur fr=2*wr; // Frame rank in result
  usz cia=1; // Cell length
  for (usz i=fr; i<rr; i++) if (mulOn(cia, rsh[i])) thrM("↕: result shape too large");
  usz ria=cia;
  for (usz i=0;  i<fr; i++) if (mulOn(ria, rsh[i])) thrM("↕: result shape too large");
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
      i+= k;
      if (i == ria) break;
      j+= str;
      for (usz a=fr-2, b=xr-2; RARE(++ri[a] == rsh[a]); ) {
        ri[a] = 0;
        j-= rsh[a] * str;
        str *= xsh[b]; if (!b) { str=cia; b=xr; }
        a--; b--;
        j+= str;
      }
    }
  }
  TFREE(ri);
  B xf = getFillQ(x);
  decG(x);
  return withFill(taga(arr_shSetU(mut_fp(r), rr, sh)), xf);
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


extern B rt_find;
B find_c2(B t, B w, B x) {
  return c2rt(find, w, x);
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
void clearImportCacheMap(void) {
  if (prevImports!=NULL) free_b2i(prevImports);
  prevImports = NULL;
}

static H_b2i* globalNames;
B globalNameList;
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
B allNsFields(void) {
  return globalNames==NULL? bi_N : incG(globalNameList);
}

void* profiler_makeMap(void) {
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

B tack_uc1(B t, B o, B x) {
  return c1(o, x);
}


void fun_gcFn(void) {
  if (prevImports!=NULL) mm_visitP(prevImports);
  if (globalNames!=NULL) mm_visitP(globalNames);
}
static void print_funBI(FILE* f, B x) { fprintf(f, "%s", pfn_repr(c(Fun,x)->extra)); }
static B funBI_uc1(B t, B o,      B x) { return c(BFn,t)->uc1(t, o,    x); }
static B funBI_ucw(B t, B o, B w, B x) { return c(BFn,t)->ucw(t, o, w, x); }
static B funBI_im(B t, B x) { return c(BFn,t)->im(t, x); }
static B funBI_iw(B t, B w, B x) { return c(BFn,t)->iw(t, w, x); }
static B funBI_ix(B t, B w, B x) { return c(BFn,t)->ix(t, w, x); }
static B funBI_identity(B x) { return inc(c(BFn,x)->ident); }
void fns_init(void) {
  gc_addFn(fun_gcFn);
  gc_add_ref(&globalNameList);
  
  TIi(t_funBI,print) = print_funBI;
  TIi(t_funBI,identity) = funBI_identity;
  TIi(t_funBI,fn_uc1) = funBI_uc1;
  TIi(t_funBI,fn_ucw) = funBI_ucw;
  TIi(t_funBI,fn_im) = funBI_im;
  TIi(t_funBI,fn_iw) = funBI_iw;
  TIi(t_funBI,fn_ix) = funBI_ix;
  bitUD[0] = a(bi_emptyIVec); // don't increment as it's already gc_add-ed
  { u64* p; B a=m_bitarrv(&p, 1); *p=0;                  bitUD[1] = a(a);               gc_add(a); }
  { u64* p; B a=m_bitarrv(&p, 2); *p=0; bitp_set(p,1,1); bitUD[2] = a(a); bit2x[0] = a; gc_add(a); }
  { u64* p; B a=m_bitarrv(&p, 2); *p=0; bitp_set(p,0,1);                  bit2x[1] = a; gc_add(a); }
  c(BFn,bi_ltack)->uc1 = tack_uc1;
  c(BFn,bi_rtack)->uc1 = tack_uc1;
}
