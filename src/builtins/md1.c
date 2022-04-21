#include "../core.h"
#include "../utils/each.h"
#include "../utils/file.h"
#include "../utils/time.h"
#include "../utils/mut.h"
#include "../builtins.h"



static B homFil1(B f, B r, B xf) {
  assert(EACH_FILLS);
  if (isPureFn(f)) {
    if (f.u==bi_eq.u || f.u==bi_ne.u || f.u==bi_feq.u) { dec(xf); return toI32Any(r); } // ≠ may return ≥2⋆31, but whatever, this thing is stupid anyway
    if (f.u==bi_fne.u) { dec(xf); return withFill(r, m_harrUv(0).b); }
    if (!noFill(xf)) {
      if (CATCH) { freeThrown(); return r; }
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
      if (CATCH) { freeThrown(); return r; }
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

B slash_c2(B f, B w, B x);
B shape_c2(B f, B w, B x);
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
  
  B r;
  usz* rsh;
  
  if (!EACH_FILLS && isFun(f) && isPervasiveDy(f) && TI(w,arrD1)) {
    BBB2B fc2 = c(Fun,f)->c2;
    if (TI(x,arrD1) && xia<80 && wia>130) {
      Arr* wd = TI(w,slice)(inc(w), 0, wia);
      arr_shVec(wd);
      r = fc2(f, slash_c2(f, m_i32(xia), taga(wd)), shape_c2(f, m_f64(ria), inc(x)));
    } else {
      SGet(w)
      M_HARR(r, wia)
      for (usz wi = 0; wi < wia; wi++) HARR_ADD(r, wi, fc2(f, Get(w,wi), inc(x)));
      r = bqn_merge(HARR_FV(r));
    }
    if (rnk(r)>1) ptr_dec(shObj(r));
    rsh = arr_shAlloc(a(r), rr);
  } else {
    BBB2B fc2 = c2fn(f);
    SGetU(w) SGet(x)
    
    M_HARR(r, ria)
    for (usz wi = 0; wi < wia; wi++) {
      B cw = GetU(w,wi);
      for (usz xi = 0; xi < xia; xi++) HARR_ADDA(r, fc2(f, inc(cw), Get(x,xi)));
    }
    rsh = HARR_FA(r, rr);
    r = HARR_O(r).b;
  }
  if (rsh) {
    memcpy(rsh   , a(w)->sh, wr*sizeof(usz));
    memcpy(rsh+wr, a(x)->sh, xr*sizeof(usz));
  }
  decG(w); decG(x);
  if (EACH_FILLS) return homFil2(f, r, wf, xf);
  return r;
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
  decG(x); return r;
}


#if SINGELI
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wunused-variable"
  #include "../singeli/gen/scan.c"
  #pragma GCC diagnostic pop
#endif

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
      if (xe==el_bit && ia<I32_MAX) { u64* xp=bitarr_ptr(x); i32* rp; B r=m_i32arrv(&rp, ia); 
      #if SINGELI
        avx2_bcs32(xp, rp, ia);
      #else
        i32 c=0; for (usz i=0; i<ia; i++) { c+= bitp_get(xp,i); rp[i]=c; }
      #endif
      decG(x); return r; }
      if (xe==el_i8 ) { i8*  xp=i8any_ptr (x); i32* rp; B r=m_i32arrv(&rp, ia); i32 c=0; for (usz i=0; i<ia; i++) { if(addOn(c,xp[i]))goto base; rp[i]=c; } decG(x); return r; }
      if (xe==el_i16) { i16* xp=i16any_ptr(x); i32* rp; B r=m_i32arrv(&rp, ia); i32 c=0; for (usz i=0; i<ia; i++) { if(addOn(c,xp[i]))goto base; rp[i]=c; } decG(x); return r; }
      if (xe==el_i32) { i32* xp=i32any_ptr(x); i32* rp; B r=m_i32arrv(&rp, ia); i32 c=0; for (usz i=0; i<ia; i++) { if(addOn(c,xp[i]))goto base; rp[i]=c; } decG(x); return r; }
    }
    if (rtid==n_ceil) { // ⌈
      if (xe==el_i8 ) { i8*  xp=i8any_ptr (x); i8*  rp; B r=m_i8arrv (&rp, ia); i8  c=I8_MIN ; for (usz i=0; i<ia; i++) { if (xp[i]>c)c=xp[i]; rp[i]=c; } decG(x); return r; }
      if (xe==el_i16) { i16* xp=i16any_ptr(x); i16* rp; B r=m_i16arrv(&rp, ia); i16 c=I16_MIN; for (usz i=0; i<ia; i++) { if (xp[i]>c)c=xp[i]; rp[i]=c; } decG(x); return r; }
      if (xe==el_i32) { i32* xp=i32any_ptr(x); i32* rp; B r=m_i32arrv(&rp, ia); i32 c=I32_MIN; for (usz i=0; i<ia; i++) { if (xp[i]>c)c=xp[i]; rp[i]=c; } decG(x); return r; }
    }
    if (rtid==n_ne) { // ≠
      if (xe==el_bit) return scan_ne(0, x, ia);
      f64 x0 = IGetU(x,0).f; if (x0!=0 && x0!=1) goto base;
      if (xe==el_i8 ) { i8*  xp=i8any_ptr (x); u64* rp; B r=m_bitarrv(&rp,ia); bool c=x0; rp[0]=c; for (usz i=1; i<ia; i++) { c = c!=xp[i]; bitp_set(rp,i,c); } decG(x); return r; }
      if (xe==el_i16) { i16* xp=i16any_ptr(x); u64* rp; B r=m_bitarrv(&rp,ia); bool c=x0; rp[0]=c; for (usz i=1; i<ia; i++) { c = c!=xp[i]; bitp_set(rp,i,c); } decG(x); return r; }
      if (xe==el_i32) { i32* xp=i32any_ptr(x); u64* rp; B r=m_bitarrv(&rp,ia); bool c=x0; rp[0]=c; for (usz i=1; i<ia; i++) { c = c!=xp[i]; bitp_set(rp,i,c); } decG(x); return r; }
    }
    if (rtid==n_or) { // ∨
      if (xe==el_bit) { u64* xp=bitarr_ptr(x); u64* rp; B r=m_bitarrv(&rp,ia); bool c=0; for (usz i=0; i<ia; i++) { c|= bitp_get(xp,i);                       bitp_set(rp,i,c); } decG(x); return r; }
      if (xe==el_i8 ) { i8*  xp=i8any_ptr (x); u64* rp; B r=m_bitarrv(&rp,ia); bool c=0; for (usz i=0; i<ia; i++) { if ((xp[i]&1)!=xp[i])goto base; c|=xp[i]; bitp_set(rp,i,c); } decG(x); return r; }
      if (xe==el_i16) { i16* xp=i16any_ptr(x); u64* rp; B r=m_bitarrv(&rp,ia); bool c=0; for (usz i=0; i<ia; i++) { if ((xp[i]&1)!=xp[i])goto base; c|=xp[i]; bitp_set(rp,i,c); } decG(x); return r; }
      if (xe==el_i32) { i32* xp=i32any_ptr(x); u64* rp; B r=m_bitarrv(&rp,ia); bool c=0; for (usz i=0; i<ia; i++) { if ((xp[i]&1)!=xp[i])goto base; c|=xp[i]; bitp_set(rp,i,c); } decG(x); return r; }
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
  if (!reuse) decG(x);
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
      if (xe==el_bit) { u64* xp=bitarr_ptr(x); i32* rp; B r=m_i32arrv(&rp, ia); i64 c=wv; for (usz i=0; i<ia; i++) { c+= bitp_get(xp,i);          rp[i]=c; } decG(x); return r; }
      if (xe==el_i8 ) { i8*  xp=i8any_ptr (x); i32* rp; B r=m_i32arrv(&rp, ia); i32 c=wv; for (usz i=0; i<ia; i++) { if(addOn(c,xp[i]))goto base; rp[i]=c; } decG(x); return r; }
      if (xe==el_i16) { i16* xp=i16any_ptr(x); i32* rp; B r=m_i32arrv(&rp, ia); i32 c=wv; for (usz i=0; i<ia; i++) { if(addOn(c,xp[i]))goto base; rp[i]=c; } decG(x); return r; }
      if (xe==el_i32) { i32* xp=i32any_ptr(x); i32* rp; B r=m_i32arrv(&rp, ia); i32 c=wv; for (usz i=0; i<ia; i++) { if(addOn(c,xp[i]))goto base; rp[i]=c; } decG(x); return r; }
    }
    if (rtid==n_ceil) { // ⌈
      if (xe==el_i8  && wv==(i8 )wv) { i8*  xp=i8any_ptr (x); i8*  rp; B r=m_i8arrv (&rp, ia); i8  c=wv; for (usz i=0; i<ia; i++) { if (xp[i]>c)c=xp[i]; rp[i]=c; } decG(x); return r; }
      if (xe==el_i16 && wv==(i16)wv) { i16* xp=i16any_ptr(x); i16* rp; B r=m_i16arrv(&rp, ia); i16 c=wv; for (usz i=0; i<ia; i++) { if (xp[i]>c)c=xp[i]; rp[i]=c; } decG(x); return r; }
      if (xe==el_i32 && wv==(i32)wv) { i32* xp=i32any_ptr(x); i32* rp; B r=m_i32arrv(&rp, ia); i32 c=wv; for (usz i=0; i<ia; i++) { if (xp[i]>c)c=xp[i]; rp[i]=c; } decG(x); return r; }
    }
    if (rtid==n_ne) { // ≠
      if (!q_ibit(wv)) goto base; bool c=wv;
      if (xe==el_bit) return scan_ne(-(u64)wv, x, ia);
      if (xe==el_i8 ) { i8*  xp=i8any_ptr (x); u64* rp; B r=m_bitarrv(&rp, ia); for (usz i=0; i<ia; i++) { c^= xp[i]; bitp_set(rp,i,c); } decG(x); return r; }
      if (xe==el_i16) { i16* xp=i16any_ptr(x); u64* rp; B r=m_bitarrv(&rp, ia); for (usz i=0; i<ia; i++) { c^= xp[i]; bitp_set(rp,i,c); } decG(x); return r; }
      if (xe==el_i32) { i32* xp=i32any_ptr(x); u64* rp; B r=m_bitarrv(&rp, ia); for (usz i=0; i<ia; i++) { c^= xp[i]; bitp_set(rp,i,c); } decG(x); return r; }
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
    decG(w);
  } else {
    if (xr!=1) thrF("`: Shape of 𝕨 must match the cell of 𝕩 (%H ≡ ≢𝕨, %H ≡ ≢𝕩)", w, x);
    if (ia==0) return x;
    B pr = r.a[0] = fc2(f, w, xget(xa,0)); i++;
    for (; i < ia; i++) r.a[i] = pr = fc2(f, inc(pr), xget(xa,i));
  }
  if (!reuse) decG(x);
  return withFill(r.b, wf);
}

B fold_c1(Md1D* d, B x) { B f = d->f;
  if (isAtm(x) || rnk(x)!=1) thrF("´: Argument must be a list (%H ≡ ≢𝕩)", x);
  usz ia = a(x)->ia;
  if (ia==0) {
    decG(x);
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
      if (xe==el_bit) { B r = m_f64(bit_sum(bitarr_ptr(x), ia)); decG(x); return r; }
      if (xe==el_i8 ) { i8*  xp = i8any_ptr (x); i64 c=0; for (usz i=0; i<ia; i++) c+= xp[i];                    decG(x); return m_f64(c); } // won't worry about 64TB array sum float inaccuracy for now
      if (xe==el_i16) { i16* xp = i16any_ptr(x); i32 c=0; for (usz i=0; i<ia; i++) if (addOn(c,xp[i]))goto base; decG(x); return m_i32(c); }
      if (xe==el_i32) { i32* xp = i32any_ptr(x); i32 c=0; for (usz i=0; i<ia; i++) if (addOn(c,xp[i]))goto base; decG(x); return m_i32(c); }
      if (xe==el_f64) { f64* xp = f64any_ptr(x); f64 c=0; for (usz i=0; i<ia; i++) c+= xp[i];                    decG(x); return m_f64(c); }
    }
    if (rtid==n_mul | rtid==n_and) { // ×/∧
      if (xe==el_bit) { u64* xp = bitarr_ptr(x); bool r=1; for (usz i=0; i<(ia>>6); i++) if (~xp[i]){r=0;break;} if(~bitp_l1(xp,ia))r=0; decG(x);return m_i32(r); }
      if (xe==el_i8 ) { i8*  xp = i8any_ptr (x); i32 c=1; for (usz i=0; i<ia; i++) if (mulOn(c,xp[i]))goto base; decG(x); return m_f64(c); }
      if (xe==el_i16) { i16* xp = i16any_ptr(x); i32 c=1; for (usz i=0; i<ia; i++) if (mulOn(c,xp[i]))goto base; decG(x); return m_i32(c); }
      if (xe==el_i32) { i32* xp = i32any_ptr(x); i32 c=1; for (usz i=0; i<ia; i++) if (mulOn(c,xp[i]))goto base; decG(x); return m_i32(c); }
      if (xe==el_f64) { f64* xp = f64any_ptr(x); f64 c=1; for (usz i=0; i<ia; i++) c*= xp[i];                    decG(x); return m_f64(c); }
    }
    if (rtid==n_floor) { // ⌊
      if (xe==el_i8 ) { i8*  xp = i8any_ptr (x); i8  c=I8_MAX ; for (usz i=0; i<ia; i++) if (xp[i]<c) c=xp[i]; decG(x); return m_i32(c); }
      if (xe==el_i16) { i16* xp = i16any_ptr(x); i16 c=I16_MAX; for (usz i=0; i<ia; i++) if (xp[i]<c) c=xp[i]; decG(x); return m_i32(c); }
      if (xe==el_i32) { i32* xp = i32any_ptr(x); i32 c=I32_MAX; for (usz i=0; i<ia; i++) if (xp[i]<c) c=xp[i]; decG(x); return m_i32(c); }
    }
    if (rtid==n_ceil) { // ⌈
      if (xe==el_i8 ) { i8*  xp = i8any_ptr (x); i8  c=I8_MIN ; for (usz i=0; i<ia; i++) if (xp[i]>c) c=xp[i]; decG(x); return m_i32(c); }
      if (xe==el_i16) { i16* xp = i16any_ptr(x); i16 c=I16_MIN; for (usz i=0; i<ia; i++) if (xp[i]>c) c=xp[i]; decG(x); return m_i32(c); }
      if (xe==el_i32) { i32* xp = i32any_ptr(x); i32 c=I32_MIN; for (usz i=0; i<ia; i++) if (xp[i]>c) c=xp[i]; decG(x); return m_i32(c); }
    }
    if (rtid==n_or) { // ∨
      if (xe==el_bit) { u64* xp = bitarr_ptr(x); bool r=0; for (usz i=0; i<(ia>>6); i++) if(xp[i]){r=1;break;} if(bitp_l0(xp,ia))r=1;decG(x); return m_i32(r); }
      if (xe==el_i8 ) { i8*  xp = i8any_ptr (x); bool r=0; for (usz i=0; i<ia; i++) { i8  c=xp[i]; if (c!=0&&c!=1)goto base; r|=c; } decG(x); return m_i32(r); }
      if (xe==el_i16) { i16* xp = i16any_ptr(x); bool r=0; for (usz i=0; i<ia; i++) { i16 c=xp[i]; if (c!=0&&c!=1)goto base; r|=c; } decG(x); return m_i32(r); }
      if (xe==el_i32) { i32* xp = i32any_ptr(x); bool r=0; for (usz i=0; i<ia; i++) { i32 c=xp[i]; if (c!=0&&c!=1)goto base; r|=c; } decG(x); return m_i32(r); }
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
  decG(x);
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
      if (xe==el_bit) { B r = m_f64(wi + bit_sum(bitarr_ptr(x), ia)); decG(x); return r; }
      if (xe==el_i8 ) { i8*  xp = i8any_ptr (x); i64 c=wi; for (usz i=0; i<ia; i++) c+=xp[i];                     decG(x); return m_f64(c); }
      if (xe==el_i16) { i16* xp = i16any_ptr(x); i32 c=wi; for (usz i=0; i<ia; i++) if (addOn(c,xp[i]))goto base; decG(x); return m_i32(c); }
      if (xe==el_i32) { i32* xp = i32any_ptr(x); i32 c=wi; for (usz i=0; i<ia; i++) if (addOn(c,xp[i]))goto base; decG(x); return m_i32(c); }
    }
    if (rtid==n_mul | rtid==n_and) { // ×/∧
      if (xe==el_bit &&
          wi==(wi&1)) { u64* xp = bitarr_ptr(x); bool r=wi; if (r) { for (usz i=0; i<(ia>>6); i++) if (~xp[i]){r=0;break;} if(~bitp_l1(xp,ia))r=0; } decG(x); return m_i32(r); }
      if (xe==el_i8 ) { i8*  xp = i8any_ptr (x); i32 c=wi; for (usz i=0; i<ia; i++) if (mulOn(c,xp[i]))goto base; decG(x); return m_i32(c); }
      if (xe==el_i16) { i16* xp = i16any_ptr(x); i32 c=wi; for (usz i=0; i<ia; i++) if (mulOn(c,xp[i]))goto base; decG(x); return m_i32(c); }
      if (xe==el_i32) { i32* xp = i32any_ptr(x); i32 c=wi; for (usz i=0; i<ia; i++) if (mulOn(c,xp[i]))goto base; decG(x); return m_i32(c); }
    }
    if (rtid==n_floor) { // ⌊
      if (xe==el_i8 ) { i8*  xp = i8any_ptr (x); i32 c=wi; for (usz i=0; i<ia; i++) if (xp[i]<c) c=xp[i]; decG(x); return m_i32(c); }
      if (xe==el_i16) { i16* xp = i16any_ptr(x); i32 c=wi; for (usz i=0; i<ia; i++) if (xp[i]<c) c=xp[i]; decG(x); return m_i32(c); }
      if (xe==el_i32) { i32* xp = i32any_ptr(x); i32 c=wi; for (usz i=0; i<ia; i++) if (xp[i]<c) c=xp[i]; decG(x); return m_i32(c); }
    }
    if (rtid==n_ceil) { // ⌈
      if (xe==el_i8 ) { i8*  xp = i8any_ptr (x); i32 c=wi; for (usz i=0; i<ia; i++) if (xp[i]>c) c=xp[i]; decG(x); return m_i32(c); }
      if (xe==el_i16) { i16* xp = i16any_ptr(x); i32 c=wi; for (usz i=0; i<ia; i++) if (xp[i]>c) c=xp[i]; decG(x); return m_i32(c); }
      if (xe==el_i32) { i32* xp = i32any_ptr(x); i32 c=wi; for (usz i=0; i<ia; i++) if (xp[i]>c) c=xp[i]; decG(x); return m_i32(c); }
    }
    if (rtid==n_or && (wi&1)==wi) { // ∨
      if (xe==el_bit) { u64* xp = bitarr_ptr(x); bool r=wi; if (!r) { for (usz i=0; i<(ia>>6); i++) if(xp[i]){r=1;break;} if(bitp_l0(xp,ia))r=1; } decG(x); return m_i32(r); }
      if (xe==el_i8 ) { i8*  xp = i8any_ptr (x); bool q=wi; for (usz i=0; i<ia; i++) { i8  c=xp[i]; if (c!=0&&c!=1)goto base; q|=c; } decG(x); return m_i32(q); }
      if (xe==el_i16) { i16* xp = i16any_ptr(x); bool q=wi; for (usz i=0; i<ia; i++) { i16 c=xp[i]; if (c!=0&&c!=1)goto base; q|=c; } decG(x); return m_i32(q); }
      if (xe==el_i32) { i32* xp = i32any_ptr(x); bool q=wi; for (usz i=0; i<ia; i++) { i32 c=xp[i]; if (c!=0&&c!=1)goto base; q|=c; } decG(x); return m_i32(q); }
    }
  }
  base:;
  SLOW3("𝕨 F´ 𝕩", w, x, f);
  
  B c = w;
  SGet(x)
  BBB2B fc2 = c2fn(f);
  for (usz i = ia; i>0; i--) c = fc2(f, Get(x, i-1), c);
  decG(x);
  return c;
}

B const_c1(Md1D* d,      B x) {         dec(x); return inc(d->f); }
B const_c2(Md1D* d, B w, B x) { dec(w); dec(x); return inc(d->f); }

B swap_c1(Md1D* d,      B x) { return c2(d->f, inc(x), x); }
B swap_c2(Md1D* d, B w, B x) { return c2(d->f,     x , w); }


B timed_c2(Md1D* d, B w, B x) { B f = d->f;
  i64 am = o2i64(w);
  incBy(x, am);
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


static B m1c1(B t, B f, B x) { // consumes x
  B fn = m1_d(inc(t), inc(f));
  B r = c1(fn, x);
  decG(fn);
  return r;
}
static B m1c2(B t, B f, B w, B x) { // consumes w,x
  B fn = m1_d(inc(t), inc(f));
  B r = c2(fn, w, x);
  decG(fn);
  return r;
}

#define S_SLICES(X)                    \
  BSS2A X##_slc = TI(X,slice);         \
  usz X##_csz = 1;                     \
  usz X##_cr = rnk(X)-1;               \
  ShArr* X##_csh;                      \
  if (X##_cr>1) {                      \
    X##_csh = m_shArr(X##_cr);         \
    for (usz i = 0; i < X##_cr; i++) { \
      usz v = a(X)->sh[i+1];           \
      X##_csz*= v;                     \
      X##_csh->a[i] = v;               \
    }                                  \
  } else if (X##_cr!=0) X##_csz*= a(X)->sh[1];

#define SLICE(X, S) ({ Arr* r_ = X##_slc(inc(X), S, X##_csz); arr_shSetI(r_, X##_cr, X##_csh); taga(r_); })

#define E_SLICES(X) if (X##_cr>1) ptr_dec(X##_csh); decG(X);

#pragma GCC diagnostic push
#ifdef __clang__
  #pragma GCC diagnostic ignored "-Wsometimes-uninitialized"
  // no gcc case because gcc is gcc and does gcc things instead of doing what it's asked to do
#endif
  
extern B rt_cell;
B cell_c1(Md1D* d, B x) { B f = d->f;
  if (isAtm(x) || rnk(x)==0) {
    B r = c1(f, x);
    return isAtm(r)? m_atomUnit(r) : r;
  }
  
  if (Q_BI(f,lt) && a(x)->ia!=0 && rnk(x)>1) return toCells(x);
  
  usz cam = a(x)->sh[0];
  if (cam==0) {
    if (!isPureFn(f) || !CATCH_ERRORS) { decG(x); return emptyHVec(); }
    
    B xf = getFillQ(x);
    if (noFill(xf)) xf = m_f64(0);
    
    usz csz = 1;
    ur cr = rnk(x)-1;
    for (usz i=0; i<cr; i++) if (mulOn(csz, a(x)->sh[i+1])) thrF("˘: Empty argument too large (%H ≡ ≢𝕩)", x);
    MAKE_MUT(fc, csz);
    mut_fill(fc, 0, xf, csz); dec(xf);
    Arr* ca = mut_fp(fc);
    usz* csh = arr_shAlloc(ca, cr);
    if (cr>1) memcpy(csh, a(x)->sh+1, cr*sizeof(usz));
    decG(x);
    
    if (CATCH) return emptyHVec();
    B rc = c1(f, taga(ca));
    popCatch();
    
    u64 rr = isArr(rc)? rnk(rc)+1ULL : 1;
    if (rr>UR_MAX) thrM("˘: Result rank too large");
    B rf = getFillQ(rc);
    Arr* r = m_fillarrp(0);
    fillarr_setFill(r, rf);
    usz* rsh = arr_shAlloc(r, rr);
    if (rr>1) {
      rsh[0] = 0;
      memcpy(rsh+1, a(rc)->sh, (rr-1)*sizeof(usz));
    }
    dec(rc);
    return taga(r);
  }
  S_SLICES(x)
  M_HARR(r, cam);
  for (usz i=0,p=0; i<cam; i++,p+=x_csz) HARR_ADD(r, i, c1(f, SLICE(x, p)));
  E_SLICES(x)

  return bqn_merge(HARR_FV(r));
}

B cell_c2(Md1D* d, B w, B x) { B f = d->f;
  bool wr = isAtm(w)? 0 : rnk(w);
  bool xr = isAtm(x)? 0 : rnk(x);
  B r;
  if (wr==0 && xr==0) return isAtm(r = c2(f, w, x))? m_atomUnit(r) : r;
  if (wr==0) {
    usz cam = a(x)->sh[0]; if (cam==0) goto zero;
    S_SLICES(x)
    M_HARR(r, cam);
    for (usz i=0,p=0; i<cam; i++,p+=x_csz) HARR_ADD(r, i, c2iW(f, w, SLICE(x, p)));
    E_SLICES(x) dec(w);
    r = HARR_FV(r);
  } else if (xr==0) {
    usz cam = a(w)->sh[0]; if (cam==0) goto zero;
    S_SLICES(w)
    M_HARR(r, cam);
    for (usz i=0,p=0; i<cam; i++,p+=w_csz) HARR_ADD(r, i, c2iX(f, SLICE(w, p), x));
    E_SLICES(w) dec(x);
    r = HARR_FV(r);
  } else {
    usz cam = a(w)->sh[0];
    if (cam==0) goto zero;
    if (cam != a(x)->sh[0]) thrF("˘: Leading axis of arguments not equal (%H ≡ ≢𝕨, %H ≡ ≢𝕩)", w, x);
    S_SLICES(w) S_SLICES(x)
    M_HARR(r, cam);
    for (usz i=0,wp=0,xp=0; i<cam; i++,wp+=w_csz,xp+=x_csz) HARR_ADD(r, i, c2(f, SLICE(w, wp), SLICE(x, xp)));
    E_SLICES(w) E_SLICES(x)
    r = HARR_FV(r);
  }
  return bqn_merge(r);
  
  zero: return m1c2(rt_cell, f, w, x); // waaaaay too complicated to handle
}

extern B rt_insert;
B insert_c1(Md1D* d, B x) { B f = d->f;
  if (isAtm(x) || rnk(x)==0) thrM("˝: 𝕩 must have rank at least 1");
  usz xia = a(x)->ia;
  if (xia==0) return m1c1(rt_insert, f, x);
  
  S_SLICES(x)
  usz p = xia-x_csz;
  B r = SLICE(x, p);
  while(p!=0) {
    p-= x_csz;
    r = c2(f, SLICE(x, p), r);
  }
  E_SLICES(x)
  return r;
}
B insert_c2(Md1D* d, B w, B x) { B f = d->f;
  if (isAtm(x) || rnk(x)==0) thrM("˝: 𝕩 must have rank at least 1");
  usz xia = a(x)->ia;
  B r = w;
  if (xia!=0) {
    S_SLICES(x)
    usz p = xia;
    while(p!=0) {
      p-= x_csz;
      r = c2(f, SLICE(x, p), r);
    }
    E_SLICES(x)
  }
  return r;
}

#pragma GCC diagnostic pop

static void print_md1BI(FILE* f, B x) { fprintf(f, "%s", pm1_repr(c(Md1,x)->extra)); }
static B md1BI_im(Md1D* d,      B x) { return ((BMd1*)d->m1)->im(d,    x); }
static B md1BI_iw(Md1D* d, B w, B x) { return ((BMd1*)d->m1)->iw(d, w, x); }
static B md1BI_ix(Md1D* d, B w, B x) { return ((BMd1*)d->m1)->ix(d, w, x); }
void md1_init() {
  TIi(t_md1BI,print) = print_md1BI;
  TIi(t_md1BI,m1_im) = md1BI_im;
  TIi(t_md1BI,m1_iw) = md1BI_iw;
  TIi(t_md1BI,m1_ix) = md1BI_ix;
}
