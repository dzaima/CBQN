#include "h.h"


bool isPureFn(B x) { // doesn't consume
  if (isCallable(x)) {
    if (v(x)->flags) return true;
    B2B dcf = TI(x).decompose;
    B xd = dcf(inc(x));
    B* xdp = harr_ptr(xd);
    i32 t = o2iu(xdp[0]);
    if (t<2) { dec(xd); return t==0; }
    usz xdia = a(xd)->ia;
    for (i32 i = 1; i<xdia; i++) if(!isPureFn(xdp[i])) { dec(xd); return false; }
    dec(xd); return true;
  } else if (isArr(x)) {
    usz ia = a(x)->ia;
    BS2B xgetU = TI(x).getU;
    for (usz i = 0; i < ia; i++) if (!isPureFn(xgetU(x,i))) return false;
    return true;
  } else return isNum(x) || isC32(x);
}

B homFil1(B f, B r, B xf) {
  assert(EACH_FILLS);
  if (isPureFn(f)) {
    if (f.u==bi_eq.u || f.u==bi_ne.u || f.u==bi_feq.u) { dec(xf); return tag(toI32Arr(r), ARR_TAG); } // ≠ may return ≥2⋆31, but whatever, this thing is stupid anyway
    if (f.u==bi_fne.u) { dec(xf); return withFill(r, m_harrUv(0).b); }
    if (!noFill(xf)) {
      if (CATCH) { dec(catchMessage); return r; }
      B rf = asFill(c1(f, xf));
      popCatch();
      return withFill(r, rf);
    }
  }
  dec(xf);
  return r;
}
B homFil2(B f, B r, B wf, B xf) {
  assert(EACH_FILLS);
  if (isPureFn(f)) {
    if (f.u==bi_feq.u || f.u==bi_fne.u) { dec(wf); dec(xf); return tag(toI32Arr(r), ARR_TAG); }
    if (!noFill(wf) && !noFill(xf)) {
      if (CATCH) { dec(catchMessage); return r; }
      B rf = asFill(c2(f, wf, xf));
      popCatch();
      return withFill(r, rf);
    }
  }
  dec(wf); dec(xf);
  return r;
}

B tbl_c1(B d, B x) { B f = c(Md1D,d)->f;
  if (!EACH_FILLS) return eachm(f, x);
  B xf = getFillQ(x);
  return homFil1(f, eachm(f, x), xf);
}
B tbl_c2(B d, B w, B x) { B f = c(Md1D,d)->f;
  B wf, xf;
  if (EACH_FILLS) wf = getFillQ(w);
  if (EACH_FILLS) xf = getFillQ(x);
  if (isAtm(w)) w = m_atomUnit(w);
  if (isAtm(x)) x = m_atomUnit(x);
  usz wia = a(w)->ia; ur wr = rnk(w);
  usz xia = a(x)->ia; ur xr = rnk(x);
  usz ria = wia*xia;  ur rr = wr+xr;
  if (rr<xr) thrF("⌜: Result rank too large (%i≡=𝕨, %i≡=𝕩)", wr, xr);
  
  BS2B wgetU = TI(w).getU;
  BS2B xget = TI(x).get;
  BBB2B fc2 = c2fn(f);
  
  usz ri = 0;
  HArr_p r = m_harrs(ria, &ri);
  for (usz wi = 0; wi < wia; wi++) {
    B cw = wgetU(w,wi);
    for (usz xi = 0; xi < xia; xi++,ri++) {
      r.a[ri] = fc2(f, inc(cw), xget(x,xi));
    }
  }
  usz* rsh = harr_fa(r, rr);
  if (rsh) {
    memcpy(rsh   , a(w)->sh, wr*sizeof(usz));
    memcpy(rsh+wr, a(x)->sh, xr*sizeof(usz));
  }
  dec(w); dec(x);
  if (EACH_FILLS) return homFil2(f, r.b, wf, xf);
  return r.b;
}

B each_c1(B d, B x) { B f = c(Md1D,d)->f;
  if (!EACH_FILLS) return eachm(f, x);
  B xf = getFillQ(x);
  return homFil1(f, eachm(f, x), xf);
}
B each_c2(B d, B w, B x) { B f = c(Md1D,d)->f;
  if (!EACH_FILLS) return eachd(f, w, x);
  B wf = getFillQ(w);
  B xf = getFillQ(x);
  return homFil2(f, eachd(f, w, x), wf, xf);
}


B scan_c1(B d, B x) { B f = c(Md1D,d)->f;
  if (isAtm(x) || rnk(x)==0) thrM("`: Argument cannot have rank 0");
  B xf = getFillQ(x);
  ur xr = rnk(x);
  usz ia = a(x)->ia;
  if (ia==0) return x;
  
  bool reuse = v(x)->type==t_harr && reusable(x);
  usz i = 0;
  HArr_p r = reuse? harr_parts(x) : m_harrs(a(x)->ia, &i);
  BS2B xget = reuse? TI(x).getU : TI(x).get;
  BBB2B fc2 = c2fn(f);
  
  if (xr==1) {
    r.a[i] = xget(x,0); i++;
    for (i = 1; i < ia; i++) r.a[i] = fc2(f, inc(r.a[i-1]), xget(x,i));
  } else {
    usz csz = arr_csz(x);
    for (; i < csz; i++) r.a[i] = xget(x,i);
    for (; i < ia; i++) r.a[i] = fc2(f, inc(r.a[i-csz]), xget(x,i));
  }
  return withFill(reuse? x : harr_fcd(r, x), xf);
}
B scan_c2(B d, B w, B x) { B f = c(Md1D,d)->f;
  if (isAtm(x) || rnk(x)==0) thrM("`: 𝕩 cannot have rank 0");
  ur xr = rnk(x); usz* xsh = a(x)->sh; usz ia = a(x)->ia;
  B wf = getFillQ(w);
  bool reuse = (v(x)->type==t_harr && reusable(x)) | !ia;
  usz i = 0;
  HArr_p r = reuse? harr_parts(x) : m_harrs(a(x)->ia, &i);
  BS2B xget = reuse? TI(x).getU : TI(x).get;
  BBB2B fc2 = c2fn(f);
  
  if (isArr(w)) {
    ur wr = rnk(w); usz* wsh = a(w)->sh; BS2B wget = TI(w).get;
    if (wr+1!=xr || !eqShPrefix(wsh, xsh+1, wr)) thrF("`: Shape of 𝕨 must match the cell of 𝕩 (%H ≡ ≢𝕨, %H ≡ ≢𝕩)", w, x);
    if (ia==0) return x;
    usz csz = arr_csz(x);
    for (; i < csz; i++) r.a[i] = fc2(f, wget(w,i), xget(x,i));
    for (; i < ia; i++) r.a[i] = fc2(f, inc(r.a[i-csz]), xget(x,i));
    dec(w);
  } else {
    if (xr!=1) thrF("`: Shape of 𝕨 must match the cell of 𝕩 (%H ≡ ≢𝕨, %H ≡ ≢𝕩)", w, x);
    if (ia==0) return x;
    B pr = r.a[0] = fc2(f, w, xget(x,0)); i++;
    for (; i < ia; i++) r.a[i] = pr = fc2(f, inc(pr), xget(x,i));
  }
  return withFill(reuse? x : harr_fcd(r, x), wf);
}

B fold_c1(B d, B x) { B f = c(Md1D,d)->f;
  if (isAtm(x) || rnk(x)!=1) thrF("´: Argument must be a list (%H ≡ ≢𝕩)", x);
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
  BBB2B fc2 = c2fn(f);
  B c;
  if (TI(x).elType==el_i32) {
    i32* xp = i32any_ptr(x);
    c = m_i32(xp[ia-1]);
    for (usz i = ia-1; i>0; i--) c = fc2(f, m_i32(xp[i-1]), c);
  } else {
    c = xget(x, ia-1);
    for (usz i = ia-1; i>0; i--) c = fc2(f, xget(x, i-1), c);
  }
  dec(x);
  return c;
}
B fold_c2(B d, B w, B x) { B f = c(Md1D,d)->f;
  if (isAtm(x) || rnk(x)!=1) thrF("´: 𝕩 must be a list (%H ≡ ≢𝕩)", x);
  usz ia = a(x)->ia;
  B c = w;
  BS2B xget = TI(x).get;
  BBB2B fc2 = c2fn(f);
  for (usz i = ia; i>0; i--) c = fc2(f, xget(x, i-1), c);
  dec(x);
  return c;
}

B const_c1(B d     , B x) {         dec(x); return inc(c(Md1D,d)->f); }
B const_c2(B d, B w, B x) { dec(w); dec(x); return inc(c(Md1D,d)->f); }

B swap_c1(B d     , B x) { return c2(c(Md1D,d)->f, inc(x), x); }
B swap_c2(B d, B w, B x) { return c2(c(Md1D,d)->f,     x , w); }


B timed_c2(B d, B w, B x) { B f = c(Md1D,d)->f;
  i64 am = o2i64(w);
  for (i64 i = 0; i < am; i++) inc(x);
  dec(x);
  u64 sns = nsTime();
  for (i64 i = 0; i < am; i++) dec(c1(f, x));
  u64 ens = nsTime();
  return m_f64((ens-sns)/(1e9*am));
}
B timed_c1(B d, B x) { B f = c(Md1D,d)->f;
  u64 sns = nsTime();
  dec(c1(f, x));
  u64 ens = nsTime();
  return m_f64((ens-sns)/1e9);
}

B fchars_c1(B d, B x) { B f = c(Md1D,d)->f;
  return file_chars(path_resolve(f, x));
}
B fbytes_c1(B d, B x) { B f = c(Md1D,d)->f;
  TmpFile* tf = file_bytes(path_resolve(f, x));
  usz ia = tf->ia; u8* p = (u8*)tf->a;
  u32* rp; B r = m_c32arrv(&rp, ia);
  for (i64 i = 0; i < ia; i++) rp[i] = p[i];
  ptr_dec(tf);
  return r;
}
B flines_c1(B d, B x) { B f = c(Md1D,d)->f;
  TmpFile* tf = file_bytes(path_resolve(f, x));
  usz ia = tf->ia; u8* p = (u8*)tf->a;
  usz lineCount = 0;
  for (usz i = 0; i < ia; i++) {
    if (p[i]=='\n') lineCount++;
    else if (p[i]=='\r') {
      lineCount++;
      if(i+1<ia && p[i+1]=='\n') i++;
    }
  }
  if (ia && (p[ia-1]!='\n' && p[ia-1]!='\r')) lineCount++;
  usz i = 0;
  HArr_p r = m_harrs(lineCount, &i);
  usz pos = 0;
  while (i < lineCount) {
    usz spos = pos;
    while(p[pos]!='\n' && p[pos]!='\r') pos++;
    r.a[i++] = fromUTF8((char*)p+spos, pos-spos);
    if (p[pos]=='\r' && pos+1<ia && p[pos+1]=='\n') pos+= 2;
    else pos++;
  }
  return harr_fv(r);
}
B import_c1(B d, B x) { B f = c(Md1D,d)->f;
  return bqn_execFile(path_resolve(f, x), inc(bi_emptyHVec));
}
B import_c2(B d, B w, B x) { B f = c(Md1D,d)->f;
  return bqn_execFile(path_resolve(f, x), w);
}


B fchars_c2(B d, B w, B x) { B f = c(Md1D,d)->f;
  file_write(path_resolve(f, w), x);
  return x;
}

B rt_cell;
B cell_c1(B d, B x) { B f = c(Md1D,d)->f;
  if (isAtm(x) || rnk(x)==0) {
    B r = c1(f, x);
    return isAtm(r)? m_atomUnit(r) : r;
  }
  if (f.u == bi_lt.u) return toCells(x);
  usz cr = rnk(x)-1;
  usz cam = a(x)->sh[0];
  usz csz = arr_csz(x);
  ShArr* csh; if (cr>1) csh = m_shArr(cr);
  usz i = 0;
  BS2B slice = TI(x).slice;
  HArr_p r = m_harrs(cam, &i);
  usz p = 0;
  for (; i < cam; i++) {
    B s = slice(inc(x), p);
    arr_shSetI(s, csz, cr, csh);
    r.a[i] = c1(f, s);
    p+= csz;
  }
  if (cr>1) ptr_dec(csh);
  dec(x);
  return c1(bi_gt, harr_fv(r));
}
B cell_c2(B d, B w, B x) { B f = c(Md1D,d)->f;
  if ((isAtm(x) || rnk(x)==0) && (isAtm(w) || rnk(w)==0)) {
    B r = c2(f, w, x);
    return isAtm(r)? m_atomUnit(r) : r;
  }
  B fn = m1_d(inc(rt_cell), inc(f)); // TODO
  B r = c2(fn, w, x);
  dec(fn);
  return r;
}

#define ba(NAME) bi_##NAME = mm_alloc(sizeof(Md1), t_md1BI, ftag(MD1_TAG)); c(Md1,bi_##NAME)->c2 = NAME##_c2; c(Md1,bi_##NAME)->c1 = NAME##_c1 ; c(Md1,bi_##NAME)->extra=pm1_##NAME; gc_add(bi_##NAME);
#define bd(NAME) bi_##NAME = mm_alloc(sizeof(Md1), t_md1BI, ftag(MD1_TAG)); c(Md1,bi_##NAME)->c2 = NAME##_c2; c(Md1,bi_##NAME)->c1 = c1_invalid; c(Md1,bi_##NAME)->extra=pm1_##NAME; gc_add(bi_##NAME);
#define bm(NAME) bi_##NAME = mm_alloc(sizeof(Md1), t_md1BI, ftag(MD1_TAG)); c(Md1,bi_##NAME)->c2 = c2_invalid;c(Md1,bi_##NAME)->c1 = NAME##_c1 ; c(Md1,bi_##NAME)->extra=pm1_##NAME; gc_add(bi_##NAME);

void print_md1BI(B x) { printf("%s", format_pm1(c(Md1,x)->extra)); }

B                               bi_tbl, bi_each, bi_fold, bi_scan, bi_const, bi_swap, bi_cell, bi_timed, bi_fchars, bi_fbytes, bi_flines, bi_import;
static inline void md1_init() { ba(tbl) ba(each) ba(fold) ba(scan) ba(const) ba(swap) ba(cell) ba(timed) ba(fchars) bm(fbytes) bm(flines) ba(import)
  ti[t_md1BI].print = print_md1BI;
}

#undef ba
#undef bd
#undef bm
