#include "../core.h"
#include "../utils/each.h"
#include "../utils/file.h"
#include "../builtins.h"



static B homFil1(B f, B r, B xf) {
  assert(EACH_FILLS);
  if (isPureFn(f)) {
    if (f.u==bi_eq.u || f.u==bi_ne.u || f.u==bi_feq.u) { dec(xf); return toI32Any(r); } // ≠ may return ≥2⋆31, but whatever, this thing is stupid anyway
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
static B homFil2(B f, B r, B wf, B xf) {
  assert(EACH_FILLS);
  if (isPureFn(f)) {
    if (f.u==bi_feq.u || f.u==bi_fne.u) { dec(wf); dec(xf); return toI32Any(r); }
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

B tbl_c1(Md1D* d, B x) { B f = d->f;
  if (!EACH_FILLS) return eachm(f, x);
  B xf = getFillQ(x);
  return homFil1(f, eachm(f, x), xf);
}
B tbl_c2(Md1D* d, B w, B x) { B f = d->f;
  B wf, xf;
  if (EACH_FILLS) wf = getFillQ(w);
  if (EACH_FILLS) xf = getFillQ(x);
  if (isAtm(w)) w = m_atomUnit(w);
  if (isAtm(x)) x = m_atomUnit(x);
  ur wr = rnk(w); usz wia = a(w)->ia;
  ur xr = rnk(x); usz xia = a(x)->ia;
  ur rr = wr+xr;  usz ria = uszMul(wia, xia);
  if (rr<xr) thrF("⌜: Result rank too large (%i≡=𝕨, %i≡=𝕩)", wr, xr);
  
  SGetU(w)
  SGet(x)
  BBB2B fc2 = c2fn(f);
  
  M_HARR(r, ria)
  for (usz wi = 0; wi < wia; wi++) {
    B cw = GetU(w,wi);
    for (usz xi = 0; xi < xia; xi++) {
      HARR_ADDA(r, fc2(f, inc(cw), Get(x,xi)));
    }
  }
  usz* rsh = HARR_FA(r, rr);
  if (rsh) {
    memcpy(rsh   , a(w)->sh, wr*sizeof(usz));
    memcpy(rsh+wr, a(x)->sh, xr*sizeof(usz));
  }
  dec(w); dec(x);
  if (EACH_FILLS) return homFil2(f, HARR_O(r).b, wf, xf);
  return HARR_O(r).b;
}

B each_c1(Md1D* d, B x) { B f = d->f;
  if (!EACH_FILLS) return eachm(f, x);
  B xf = getFillQ(x);
  return homFil1(f, eachm(f, x), xf);
}
B each_c2(Md1D* d, B w, B x) { B f = d->f;
  if (!EACH_FILLS) return eachd(f, w, x);
  B wf = getFillQ(w);
  B xf = getFillQ(x);
  return homFil2(f, eachd(f, w, x), wf, xf);
}

B scan_ne(u64 p, B x, u64 ia) {
  u64* xp=bitarr_ptr(x); u64* rp; B r=m_bitarrv(&rp,ia);
  for (usz i = 0; i < BIT_N(ia); i++) {
    u64 c = xp[i];
    u64 r = c ^ (c<<1);
    r^= r<< 2; r^= r<< 4; r^= r<<8;
    r^= r<<16; r^= r<<32; r^=    p;
    rp[i] = r;
    p = -(r>>63); // repeat sign bit
  }
  dec(x); return r;
}

B scan_c1(Md1D* d, B x) { B f = d->f;
  if (isAtm(x) || rnk(x)==0) thrM("`: Argument cannot have rank 0");
  ur xr = rnk(x);
  usz ia = a(x)->ia;
  if (ia==0) return x;
  B xf = getFillQ(x);
  u8 xe = TI(x,elType);
  if (xr==1 && xe<=el_f64 && isFun(f) && v(f)->flags) {
    u8 rtid = v(f)->flags-1;
    if (rtid==n_add) { // +
      if (ia<I32_MAX) \
      if (xe==el_bit) { u64* xp=bitarr_ptr(x); i32* rp; B r=m_i32arrv(&rp, ia); i32 c=0; for (usz i=0; i<ia; i++) { c+= bitp_get(xp,i);          rp[i]=c; } dec(x); return r; }
      if (xe==el_i8 ) { i8*  xp=i8any_ptr (x); i32* rp; B r=m_i32arrv(&rp, ia); i32 c=0; for (usz i=0; i<ia; i++) { if(addOn(c,xp[i]))goto base; rp[i]=c; } dec(x); return r; }
      if (xe==el_i16) { i16* xp=i16any_ptr(x); i32* rp; B r=m_i32arrv(&rp, ia); i32 c=0; for (usz i=0; i<ia; i++) { if(addOn(c,xp[i]))goto base; rp[i]=c; } dec(x); return r; }
      if (xe==el_i32) { i32* xp=i32any_ptr(x); i32* rp; B r=m_i32arrv(&rp, ia); i32 c=0; for (usz i=0; i<ia; i++) { if(addOn(c,xp[i]))goto base; rp[i]=c; } dec(x); return r; }
    }
    if (rtid==n_ceil) { // ⌈
      if (xe==el_i8 ) { i8*  xp=i8any_ptr (x); i8*  rp; B r=m_i8arrv (&rp, ia); i8  c=I8_MIN ; for (usz i=0; i<ia; i++) { if (xp[i]>c)c=xp[i]; rp[i]=c; } dec(x); return r; }
      if (xe==el_i16) { i16* xp=i16any_ptr(x); i16* rp; B r=m_i16arrv(&rp, ia); i16 c=I16_MIN; for (usz i=0; i<ia; i++) { if (xp[i]>c)c=xp[i]; rp[i]=c; } dec(x); return r; }
      if (xe==el_i32) { i32* xp=i32any_ptr(x); i32* rp; B r=m_i32arrv(&rp, ia); i32 c=I32_MIN; for (usz i=0; i<ia; i++) { if (xp[i]>c)c=xp[i]; rp[i]=c; } dec(x); return r; }
    }
    if (rtid==n_ne) { // ≠
      if (xe==el_bit) return scan_ne(0, x, ia);
      f64 x0 = IGetU(x,0).f; if (x0!=0 && x0!=1) goto base;
      if (xe==el_i8 ) { i8*  xp=i8any_ptr (x); u64* rp; B r=m_bitarrv(&rp,ia); bool c=x0; rp[0]=c; for (usz i=1; i<ia; i++) { c = c!=xp[i]; bitp_set(rp,i,c); } dec(x); return r; }
      if (xe==el_i16) { i16* xp=i16any_ptr(x); u64* rp; B r=m_bitarrv(&rp,ia); bool c=x0; rp[0]=c; for (usz i=1; i<ia; i++) { c = c!=xp[i]; bitp_set(rp,i,c); } dec(x); return r; }
      if (xe==el_i32) { i32* xp=i32any_ptr(x); u64* rp; B r=m_bitarrv(&rp,ia); bool c=x0; rp[0]=c; for (usz i=1; i<ia; i++) { c = c!=xp[i]; bitp_set(rp,i,c); } dec(x); return r; }
    }
    if (rtid==n_or) { // ∨
      if (xe==el_bit) { u64* xp=bitarr_ptr(x); u64* rp; B r=m_bitarrv(&rp,ia); bool c=0; for (usz i=0; i<ia; i++) { c|= bitp_get(xp,i);                       bitp_set(rp,i,c); } dec(x); return r; }
      if (xe==el_i8 ) { i8*  xp=i8any_ptr (x); u64* rp; B r=m_bitarrv(&rp,ia); bool c=0; for (usz i=0; i<ia; i++) { if ((xp[i]&1)!=xp[i])goto base; c|=xp[i]; bitp_set(rp,i,c); } dec(x); return r; }
      if (xe==el_i16) { i16* xp=i16any_ptr(x); u64* rp; B r=m_bitarrv(&rp,ia); bool c=0; for (usz i=0; i<ia; i++) { if ((xp[i]&1)!=xp[i])goto base; c|=xp[i]; bitp_set(rp,i,c); } dec(x); return r; }
      if (xe==el_i32) { i32* xp=i32any_ptr(x); u64* rp; B r=m_bitarrv(&rp,ia); bool c=0; for (usz i=0; i<ia; i++) { if ((xp[i]&1)!=xp[i])goto base; c|=xp[i]; bitp_set(rp,i,c); } dec(x); return r; }
    }
  }
  base:;
  SLOW2("𝕎` 𝕩", f, x);
  
  bool reuse = v(x)->type==t_harr && reusable(x);
  HArr_p r = reuse? harr_parts(REUSE(x)) : m_harr0c(x);
  AS2B xget = reuse? TI(x,getU) : TI(x,get); Arr* xa = a(x);
  BBB2B fc2 = c2fn(f);
  
  if (xr==1) {
    r.a[0] = xget(xa,0);
    for (usz i=1; i<ia; i++) r.a[i] = fc2(f, inc(r.a[i-1]), xget(xa,i));
  } else {
    usz csz = arr_csz(x);
    usz i = 0;
    for (; i<csz; i++) r.a[i] = xget(xa,i);
    for (; i<ia; i++) r.a[i] = fc2(f, inc(r.a[i-csz]), xget(xa,i));
  }
  if (!reuse) dec(x);
  return withFill(r.b, xf);
}
B scan_c2(Md1D* d, B w, B x) { B f = d->f;
  if (isAtm(x) || rnk(x)==0) thrM("`: 𝕩 cannot have rank 0");
  ur xr = rnk(x); usz* xsh = a(x)->sh; usz ia = a(x)->ia;
  B wf = getFillQ(w);
  u8 xe = TI(x,elType);
  if (xr==1 && q_i32(w) && xe<el_f64 && isFun(f) && v(f)->flags) {
    u8 rtid = v(f)->flags-1;
    i32 wv = o2iu(w);
    if (rtid==n_add) { // +
      if (xe==el_bit) { u64* xp=bitarr_ptr(x); i32* rp; B r=m_i32arrv(&rp, ia); i64 c=wv; for (usz i=0; i<ia; i++) { c+= bitp_get(xp,i);          rp[i]=c; } dec(x); return r; }
      if (xe==el_i8 ) { i8*  xp=i8any_ptr (x); i32* rp; B r=m_i32arrv(&rp, ia); i32 c=wv; for (usz i=0; i<ia; i++) { if(addOn(c,xp[i]))goto base; rp[i]=c; } dec(x); return r; }
      if (xe==el_i16) { i16* xp=i16any_ptr(x); i32* rp; B r=m_i32arrv(&rp, ia); i32 c=wv; for (usz i=0; i<ia; i++) { if(addOn(c,xp[i]))goto base; rp[i]=c; } dec(x); return r; }
      if (xe==el_i32) { i32* xp=i32any_ptr(x); i32* rp; B r=m_i32arrv(&rp, ia); i32 c=wv; for (usz i=0; i<ia; i++) { if(addOn(c,xp[i]))goto base; rp[i]=c; } dec(x); return r; }
    }
    if (rtid==n_ceil) { // ⌈
      if (xe==el_i8  && wv==(i8 )wv) { i8*  xp=i8any_ptr (x); i8*  rp; B r=m_i8arrv (&rp, ia); i8  c=wv; for (usz i=0; i<ia; i++) { if (xp[i]>c)c=xp[i]; rp[i]=c; } dec(x); return r; }
      if (xe==el_i16 && wv==(i16)wv) { i16* xp=i16any_ptr(x); i16* rp; B r=m_i16arrv(&rp, ia); i16 c=wv; for (usz i=0; i<ia; i++) { if (xp[i]>c)c=xp[i]; rp[i]=c; } dec(x); return r; }
      if (xe==el_i32 && wv==(i32)wv) { i32* xp=i32any_ptr(x); i32* rp; B r=m_i32arrv(&rp, ia); i32 c=wv; for (usz i=0; i<ia; i++) { if (xp[i]>c)c=xp[i]; rp[i]=c; } dec(x); return r; }
    }
    if (rtid==n_ne) { // ≠
      if (!q_ibit(wv)) goto base; bool c=wv;
      if (xe==el_bit) return scan_ne(-(u64)wv, x, ia);
      if (xe==el_i8 ) { i8*  xp=i8any_ptr (x); u64* rp; B r=m_bitarrv(&rp, ia); for (usz i=0; i<ia; i++) { c^= xp[i]; bitp_set(rp,i,c); } dec(x); return r; }
      if (xe==el_i16) { i16* xp=i16any_ptr(x); u64* rp; B r=m_bitarrv(&rp, ia); for (usz i=0; i<ia; i++) { c^= xp[i]; bitp_set(rp,i,c); } dec(x); return r; }
      if (xe==el_i32) { i32* xp=i32any_ptr(x); u64* rp; B r=m_bitarrv(&rp, ia); for (usz i=0; i<ia; i++) { c^= xp[i]; bitp_set(rp,i,c); } dec(x); return r; }
    }
  }
  base:;
  SLOW3("𝕨 F` 𝕩", w, x, f);
  
  bool reuse = (v(x)->type==t_harr && reusable(x)) | !ia;
  usz i = 0;
  HArr_p r = reuse? harr_parts(REUSE(x)) : m_harr0c(x);
  AS2B xget = reuse? TI(x,getU) : TI(x,get); Arr* xa = a(x);
  BBB2B fc2 = c2fn(f);
  
  if (isArr(w)) {
    ur wr = rnk(w); usz* wsh = a(w)->sh; SGet(w)
    if (wr+1!=xr || !eqShPrefix(wsh, xsh+1, wr)) thrF("`: Shape of 𝕨 must match the cell of 𝕩 (%H ≡ ≢𝕨, %H ≡ ≢𝕩)", w, x);
    if (ia==0) return x;
    usz csz = arr_csz(x);
    for (; i < csz; i++) r.a[i] = fc2(f, Get(w,i), xget(xa,i));
    for (; i < ia; i++) r.a[i] = fc2(f, inc(r.a[i-csz]), xget(xa,i));
    dec(w);
  } else {
    if (xr!=1) thrF("`: Shape of 𝕨 must match the cell of 𝕩 (%H ≡ ≢𝕨, %H ≡ ≢𝕩)", w, x);
    if (ia==0) return x;
    B pr = r.a[0] = fc2(f, w, xget(xa,0)); i++;
    for (; i < ia; i++) r.a[i] = pr = fc2(f, inc(pr), xget(xa,i));
  }
  if (!reuse) dec(x);
  return withFill(r.b, wf);
}

B fold_c1(Md1D* d, B x) { B f = d->f;
  if (isAtm(x) || rnk(x)!=1) thrF("´: Argument must be a list (%H ≡ ≢𝕩)", x);
  usz ia = a(x)->ia;
  if (ia==0) {
    dec(x);
    if (isFun(f)) {
      B r = TI(f,identity)(f);
      if (!q_N(r)) return inc(r);
    }
    thrM("´: No identity found");
  }
  u8 xe = TI(x,elType);
  if (isFun(f) && v(f)->flags && xe<=el_f64) {
    u8 rtid = v(f)->flags-1;
    if (rtid==n_add) { // +
      if (xe==el_bit) { B r = m_f64(bit_sum(bitarr_ptr(x), ia)); dec(x); return r; }
      if (xe==el_i8 ) { i8*  xp = i8any_ptr (x); i64 c=0; for (usz i=0; i<ia; i++) c+= xp[i];                    dec(x); return m_f64(c); } // won't worry about 64TB array sum float inaccuracy for now
      if (xe==el_i16) { i16* xp = i16any_ptr(x); i32 c=0; for (usz i=0; i<ia; i++) if (addOn(c,xp[i]))goto base; dec(x); return m_i32(c); }
      if (xe==el_i32) { i32* xp = i32any_ptr(x); i32 c=0; for (usz i=0; i<ia; i++) if (addOn(c,xp[i]))goto base; dec(x); return m_i32(c); }
      if (xe==el_f64) { f64* xp = f64any_ptr(x); f64 c=0; for (usz i=0; i<ia; i++) c+= xp[i];                    dec(x); return m_f64(c); }
    }
    if (rtid==n_mul | rtid==n_and) { // ×/∧
      if (xe==el_bit) { u64* xp = bitarr_ptr(x); bool r=1; for (usz i=0; i<(ia>>6); i++) if (~xp[i]){r=0;break;} if(~bitp_l1(xp,ia))r=0; dec(x);return m_i32(r); }
      if (xe==el_i8 ) { i8*  xp = i8any_ptr (x); i32 c=1; for (usz i=0; i<ia; i++) if (mulOn(c,xp[i]))goto base; dec(x); return m_f64(c); }
      if (xe==el_i16) { i16* xp = i16any_ptr(x); i32 c=1; for (usz i=0; i<ia; i++) if (mulOn(c,xp[i]))goto base; dec(x); return m_i32(c); }
      if (xe==el_i32) { i32* xp = i32any_ptr(x); i32 c=1; for (usz i=0; i<ia; i++) if (mulOn(c,xp[i]))goto base; dec(x); return m_i32(c); }
      if (xe==el_f64) { f64* xp = f64any_ptr(x); f64 c=1; for (usz i=0; i<ia; i++) c*= xp[i];                    dec(x); return m_f64(c); }
    }
    if (rtid==n_floor) { // ⌊
      if (xe==el_i8 ) { i8*  xp = i8any_ptr (x); i8  c=I8_MAX ; for (usz i=0; i<ia; i++) if (xp[i]<c) c=xp[i]; dec(x); return m_i32(c); }
      if (xe==el_i16) { i16* xp = i16any_ptr(x); i16 c=I16_MAX; for (usz i=0; i<ia; i++) if (xp[i]<c) c=xp[i]; dec(x); return m_i32(c); }
      if (xe==el_i32) { i32* xp = i32any_ptr(x); i32 c=I32_MAX; for (usz i=0; i<ia; i++) if (xp[i]<c) c=xp[i]; dec(x); return m_i32(c); }
    }
    if (rtid==n_ceil) { // ⌈
      if (xe==el_i8 ) { i8*  xp = i8any_ptr (x); i8  c=I8_MIN ; for (usz i=0; i<ia; i++) if (xp[i]>c) c=xp[i]; dec(x); return m_i32(c); }
      if (xe==el_i16) { i16* xp = i16any_ptr(x); i16 c=I16_MIN; for (usz i=0; i<ia; i++) if (xp[i]>c) c=xp[i]; dec(x); return m_i32(c); }
      if (xe==el_i32) { i32* xp = i32any_ptr(x); i32 c=I32_MIN; for (usz i=0; i<ia; i++) if (xp[i]>c) c=xp[i]; dec(x); return m_i32(c); }
    }
    if (rtid==n_or) { // ∨
      if (xe==el_bit) { u64* xp = bitarr_ptr(x); bool r=0; for (usz i=0; i<(ia>>6); i++) if(xp[i]){r=1;break;} if(bitp_l0(xp,ia))r=1;dec(x); return m_i32(r); }
      if (xe==el_i8 ) { i8*  xp = i8any_ptr (x); bool r=0; for (usz i=0; i<ia; i++) { i8  c=xp[i]; if (c!=0&&c!=1)goto base; r|=c; } dec(x); return m_i32(r); }
      if (xe==el_i16) { i16* xp = i16any_ptr(x); bool r=0; for (usz i=0; i<ia; i++) { i16 c=xp[i]; if (c!=0&&c!=1)goto base; r|=c; } dec(x); return m_i32(r); }
      if (xe==el_i32) { i32* xp = i32any_ptr(x); bool r=0; for (usz i=0; i<ia; i++) { i32 c=xp[i]; if (c!=0&&c!=1)goto base; r|=c; } dec(x); return m_i32(r); }
    }
  }
  base:;
  SLOW2("𝕎´ 𝕩", f, x);
  
  SGet(x)
  BBB2B fc2 = c2fn(f);
  B c;
  if (TI(x,elType)==el_i32) {
    i32* xp = i32any_ptr(x);
    c = m_i32(xp[ia-1]);
    for (usz i = ia-1; i>0; i--) c = fc2(f, m_i32(xp[i-1]), c);
  } else {
    c = Get(x, ia-1);
    for (usz i = ia-1; i>0; i--) c = fc2(f, Get(x, i-1), c);
  }
  dec(x);
  return c;
}
B fold_c2(Md1D* d, B w, B x) { B f = d->f;
  if (isAtm(x) || rnk(x)!=1) thrF("´: 𝕩 must be a list (%H ≡ ≢𝕩)", x);
  usz ia = a(x)->ia;
  u8 xe = TI(x,elType);
  if (q_i32(w) && isFun(f) && v(f)->flags && xe<el_f64) {
    i32 wi = o2iu(w);
    u8 rtid = v(f)->flags-1;
    if (rtid==n_add) { // + 
      if (xe==el_bit) { B r = m_f64(wi + bit_sum(bitarr_ptr(x), ia)); dec(x); return r; }
      if (xe==el_i8 ) { i8*  xp = i8any_ptr (x); i64 c=wi; for (usz i=0; i<ia; i++) c+=xp[i];                     dec(x); return m_f64(c); }
      if (xe==el_i16) { i16* xp = i16any_ptr(x); i32 c=wi; for (usz i=0; i<ia; i++) if (addOn(c,xp[i]))goto base; dec(x); return m_i32(c); }
      if (xe==el_i32) { i32* xp = i32any_ptr(x); i32 c=wi; for (usz i=0; i<ia; i++) if (addOn(c,xp[i]))goto base; dec(x); return m_i32(c); }
    }
    if (rtid==n_mul | rtid==n_and) { // ×/∧
      if (xe==el_bit) { u64* xp = bitarr_ptr(x); bool r=wi; if (r) { for (usz i=0; i<(ia>>6); i++) if (~xp[i]){r=0;break;} if(~bitp_l1(xp,ia))r=0; } dec(x); return m_i32(r); }
      if (xe==el_i8 ) { i8*  xp = i8any_ptr (x); i32 c=wi; for (usz i=0; i<ia; i++) if (mulOn(c,xp[i]))goto base; dec(x); return m_i32(c); }
      if (xe==el_i16) { i16* xp = i16any_ptr(x); i32 c=wi; for (usz i=0; i<ia; i++) if (mulOn(c,xp[i]))goto base; dec(x); return m_i32(c); }
      if (xe==el_i32) { i32* xp = i32any_ptr(x); i32 c=wi; for (usz i=0; i<ia; i++) if (mulOn(c,xp[i]))goto base; dec(x); return m_i32(c); }
    }
    if (rtid==n_floor) { // ⌊
      if (xe==el_i8 ) { i8*  xp = i8any_ptr (x); i32 c=wi; for (usz i=0; i<ia; i++) if (xp[i]<c) c=xp[i]; dec(x); return m_i32(c); }
      if (xe==el_i16) { i16* xp = i16any_ptr(x); i32 c=wi; for (usz i=0; i<ia; i++) if (xp[i]<c) c=xp[i]; dec(x); return m_i32(c); }
      if (xe==el_i32) { i32* xp = i32any_ptr(x); i32 c=wi; for (usz i=0; i<ia; i++) if (xp[i]<c) c=xp[i]; dec(x); return m_i32(c); }
    }
    if (rtid==n_ceil) { // ⌈
      if (xe==el_i8 ) { i8*  xp = i8any_ptr (x); i32 c=wi; for (usz i=0; i<ia; i++) if (xp[i]>c) c=xp[i]; dec(x); return m_i32(c); }
      if (xe==el_i16) { i16* xp = i16any_ptr(x); i32 c=wi; for (usz i=0; i<ia; i++) if (xp[i]>c) c=xp[i]; dec(x); return m_i32(c); }
      if (xe==el_i32) { i32* xp = i32any_ptr(x); i32 c=wi; for (usz i=0; i<ia; i++) if (xp[i]>c) c=xp[i]; dec(x); return m_i32(c); }
    }
    if (rtid==n_or && (wi&1)==wi) { // ∨
      if (xe==el_bit) { u64* xp = bitarr_ptr(x); bool r=wi; if (!r) { for (usz i=0; i<(ia>>6); i++) if(xp[i]){r=1;break;} if(bitp_l0(xp,ia))r=1; } dec(x); return m_i32(r); }
      if (xe==el_i8 ) { i8*  xp = i8any_ptr (x); bool q=wi; for (usz i=0; i<ia; i++) { i8  c=xp[i]; if (c!=0&&c!=1)goto base; q|=c; } dec(x); return m_i32(q); }
      if (xe==el_i16) { i16* xp = i16any_ptr(x); bool q=wi; for (usz i=0; i<ia; i++) { i16 c=xp[i]; if (c!=0&&c!=1)goto base; q|=c; } dec(x); return m_i32(q); }
      if (xe==el_i32) { i32* xp = i32any_ptr(x); bool q=wi; for (usz i=0; i<ia; i++) { i32 c=xp[i]; if (c!=0&&c!=1)goto base; q|=c; } dec(x); return m_i32(q); }
    }
  }
  base:;
  SLOW3("𝕨 F´ 𝕩", w, x, f);
  
  B c = w;
  SGet(x)
  BBB2B fc2 = c2fn(f);
  for (usz i = ia; i>0; i--) c = fc2(f, Get(x, i-1), c);
  dec(x);
  return c;
}

B const_c1(Md1D* d,      B x) {         dec(x); return inc(d->f); }
B const_c2(Md1D* d, B w, B x) { dec(w); dec(x); return inc(d->f); }

B swap_c1(Md1D* d,      B x) { return c2(d->f, inc(x), x); }
B swap_c2(Md1D* d, B w, B x) { return c2(d->f,     x , w); }


B timed_c2(Md1D* d, B w, B x) { B f = d->f;
  i64 am = o2i64(w);
  for (i64 i = 0; i < am; i++) inc(x);
  dec(x);
  u64 sns = nsTime();
  for (i64 i = 0; i < am; i++) dec(c1(f, x));
  u64 ens = nsTime();
  return m_f64((ens-sns)/(1e9*am));
}
B timed_c1(Md1D* d, B x) { B f = d->f;
  u64 sns = nsTime();
  dec(c1(f, x));
  u64 ens = nsTime();
  return m_f64((ens-sns)*1e-9);
}


extern B rt_cell;
B cell_c1(Md1D* d, B x) { B f = d->f;
  if (isAtm(x) || rnk(x)==0) {
    B r = c1(f, x);
    return isAtm(r)? m_atomUnit(r) : r;
  }
  if (f.u==bi_lt.u && a(x)->ia!=0 && rnk(x)>1) return toCells(x);
  usz cr = rnk(x)-1;
  usz cam = a(x)->sh[0];
  usz csz = arr_csz(x);
  ShArr* csh;
  if (cr>1) {
    csh = m_shArr(cr);
    memcpy(csh->a, a(x)->sh+1, cr*sizeof(usz));
  }
  BSS2A slice = TI(x,slice);
  M_HARR(r, cam);
  usz p = 0;
  for (usz i = 0; i < cam; i++) {
    Arr* s = slice(inc(x), p, csz); arr_shSetI(s, cr, csh);
    HARR_ADD(r, i, c1(f, taga(s)));
    p+= csz;
  }
  if (cr>1) ptr_dec(csh);
  dec(x);
  return bqn_merge(HARR_FV(r));
}
B cell_c2(Md1D* d, B w, B x) { B f = d->f;
  if ((isAtm(x) || rnk(x)==0) && (isAtm(w) || rnk(w)==0)) {
    B r = c2(f, w, x);
    return isAtm(r)? m_atomUnit(r) : r;
  }
  B fn = m1_d(inc(rt_cell), inc(f)); // TODO
  B r = c2(fn, w, x);
  dec(fn);
  return r;
}



static void print_md1BI(B x) { printf("%s", pm1_repr(c(Md1,x)->extra)); }
void md1_init() {
  TIi(t_md1BI,print) = print_md1BI;
}
