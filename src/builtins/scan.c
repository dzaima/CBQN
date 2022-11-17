#include "../core.h"
#include "../utils/mut.h"
#include "../builtins.h"

#if !USE_VALGRIND
static u64 vg_rand(u64 x) { return x; }
#endif

#if SINGELI
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wunused-variable"
  #include "../singeli/gen/scan.c"
  #pragma GCC diagnostic pop
#endif

#if SINGELI && __PCLMUL__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wunused-variable"
  #include "../singeli/gen/neq.c"
  #pragma GCC diagnostic pop
#endif
B scan_ne(u64 p, u64* xp, u64 ia) {
  u64* rp; B r=m_bitarrv(&rp,ia);
#if SINGELI && __PCLMUL__
  clmul_scan_ne(p, xp, rp, BIT_N(ia));
#else
  for (usz i = 0; i < BIT_N(ia); i++) {
    u64 c = xp[i];
    u64 r = c ^ (c<<1);
    r^= r<< 2; r^= r<< 4; r^= r<<8;
    r^= r<<16; r^= r<<32; r^=    p;
    rp[i] = r;
    p = -(r>>63); // repeat sign bit
  }
#endif
  return r;
}

B slash_c1(B f, B x);
B scan_bit_sum(B x, u64* xp, u64 ia, u64 xs) { // consumes x
  u8 re = xs<=I8_MAX? el_i8 : xs<=I16_MAX? el_i16 : el_i32;
  if (xs < ia/128) {
    B ones = slash_c1(m_f64(0), x);
    MAKE_MUT(r0, ia) mut_init(r0, re); MUTG_INIT(r0);
    SGetU(ones)
    usz ri = 0;
    for (usz i = 0; i < xs; i++) {
      usz e = o2s(GetU(ones, i));
      mut_fillG(r0, ri, m_i32(i), e-ri);
      ri = e;
    }
    if (ri<ia) mut_fillG(r0, ri, m_i32(xs), ia-ri);
    decG(ones);
    return mut_fv(r0);
  }
  B r;
  void* rp = m_tyarrv(&r, elWidth(re), ia, el2t(re));
  #if SINGELI
    #define SUM(W,T) avx2_bcs##W(xp, rp, ia);
  #else
    #define SUM(W,T) { T c=0; for (usz i=0; i<ia; i++) { c+= bitp_get(xp,i); ((T*)rp)[i]=c; } }
  #endif
  #define CASE(W) case el_i##W: SUM(W, i##W) break;
  switch (re) { default:UD; CASE(8) CASE(16) CASE(32) }
  #undef CASE
  #undef SUM
  decG(x); return r;
}

B scan_c1(Md1D* d, B x) { B f = d->f;
  if (isAtm(x) || RNK(x)==0) thrM("`: Argument cannot have rank 0");
  ur xr = RNK(x);
  usz ia = IA(x);
  if (ia==0) return x;
  B xf = getFillQ(x);
  u8 xe = TI(x,elType);
  if (xr==1 && xe<=el_f64 && isFun(f) && v(f)->flags) {
    u8 rtid = v(f)->flags-1;
    if (xe==el_bit) {
      u64* xp=bitarr_ptr(x);
      if (rtid==n_add) {
        u64 xs = bit_sum(xp, ia);
        if (xs>I32_MAX) goto base;
        if (xs<=1) { if (xs==0) return x; goto bit_or; }
        return FL_SET(scan_bit_sum(x, xp, ia, xs), fl_asc|fl_squoze);
      }
      if (rtid==n_or  |               rtid==n_ceil ) { bit_or:; u64* rp; B r=m_bitarrv(&rp,ia); usz n=BIT_N(ia); u64 xi; usz i=0; while(i<n) if ((xi= vg_rand(xp[i]))!=0) { rp[i] = -(xi&-xi)  ; i++; while(i<n) rp[i++] = ~0LL; break; } else rp[i++]= 0  ; decG(x); return r; }
      if (rtid==n_and | rtid==n_mul | rtid==n_floor) {          u64* rp; B r=m_bitarrv(&rp,ia); usz n=BIT_N(ia); u64 xi; usz i=0; while(i<n) if ((xi=~vg_rand(xp[i]))!=0) { rp[i] =  (xi&-xi)-1; i++; while(i<n) rp[i++] =  0  ; break; } else rp[i++]=~0LL; decG(x); return r; }
      if (rtid==n_ne) { B r=scan_ne(0, xp, ia); decG(x); return r; }
      if (rtid==n_lt) {
        u64* rp; B r=m_bitarrv(&rp,ia); usz n=BIT_N(ia);
        u64 m10 = 0x5555555555555555;
        u64 p = 0;
        for (usz i=0; i<n; i++) {
          u64 x = xp[i];
          u64 c  = (m10 & ~(x<<1)) & ~(p>>63);
          rp[i] = p = x & (m10 ^ (x + c));
        }
        decG(x); return r;
      }
      goto base;
    }
    if (rtid==n_add) { // +
      if (xe==el_i8 ) { i8*  xp=i8any_ptr (x); i32* rp; B r=m_i32arrv(&rp, ia); i32 c=0; for (usz i=0; i<ia; i++) { if(addOn(c,xp[i]))goto base; rp[i]=c; } decG(x); return r; }
      if (xe==el_i16) { i16* xp=i16any_ptr(x); i32* rp; B r=m_i32arrv(&rp, ia); i32 c=0; for (usz i=0; i<ia; i++) { if(addOn(c,xp[i]))goto base; rp[i]=c; } decG(x); return r; }
      if (xe==el_i32) { i32* xp=i32any_ptr(x); i32* rp; B r=m_i32arrv(&rp, ia); i32 c=0; for (usz i=0; i<ia; i++) { if(addOn(c,xp[i]))goto base; rp[i]=c; } decG(x); return r; }
    }
    if (rtid==n_floor) { // ⌊
      #if SINGELI
        if (xe==el_i8 ) { i8*  rp; B r=m_i8arrv (&rp, ia); avx2_scan_min8 (i8any_ptr (x), rp, ia); decG(x); return r; }
        if (xe==el_i16) { i16* rp; B r=m_i16arrv(&rp, ia); avx2_scan_min16(i16any_ptr(x), rp, ia); decG(x); return r; }
        if (xe==el_i32) { i32* rp; B r=m_i32arrv(&rp, ia); avx2_scan_min32(i32any_ptr(x), rp, ia); decG(x); return r; }
      #else
        if (xe==el_i8 ) { i8*  xp=i8any_ptr (x); i8*  rp; B r=m_i8arrv (&rp, ia); i8  c=I8_MAX ; for (usz i=0; i<ia; i++) { if (xp[i]<c)c=xp[i]; rp[i]=c; } decG(x); return r; }
        if (xe==el_i16) { i16* xp=i16any_ptr(x); i16* rp; B r=m_i16arrv(&rp, ia); i16 c=I16_MAX; for (usz i=0; i<ia; i++) { if (xp[i]<c)c=xp[i]; rp[i]=c; } decG(x); return r; }
        if (xe==el_i32) { i32* xp=i32any_ptr(x); i32* rp; B r=m_i32arrv(&rp, ia); i32 c=I32_MAX; for (usz i=0; i<ia; i++) { if (xp[i]<c)c=xp[i]; rp[i]=c; } decG(x); return r; }
      #endif
    }
    if (rtid==n_ceil) { // ⌈
      #if SINGELI
        if (xe==el_i8 ) { i8*  rp; B r=m_i8arrv (&rp, ia); avx2_scan_max8 (i8any_ptr (x), rp, ia); decG(x); return r; }
        if (xe==el_i16) { i16* rp; B r=m_i16arrv(&rp, ia); avx2_scan_max16(i16any_ptr(x), rp, ia); decG(x); return r; }
        if (xe==el_i32) { i32* rp; B r=m_i32arrv(&rp, ia); avx2_scan_max32(i32any_ptr(x), rp, ia); decG(x); return r; }
      #else
        if (xe==el_i8 ) { i8*  xp=i8any_ptr (x); i8*  rp; B r=m_i8arrv (&rp, ia); i8  c=I8_MIN ; for (usz i=0; i<ia; i++) { if (xp[i]>c)c=xp[i]; rp[i]=c; } decG(x); return r; }
        if (xe==el_i16) { i16* xp=i16any_ptr(x); i16* rp; B r=m_i16arrv(&rp, ia); i16 c=I16_MIN; for (usz i=0; i<ia; i++) { if (xp[i]>c)c=xp[i]; rp[i]=c; } decG(x); return r; }
        if (xe==el_i32) { i32* xp=i32any_ptr(x); i32* rp; B r=m_i32arrv(&rp, ia); i32 c=I32_MIN; for (usz i=0; i<ia; i++) { if (xp[i]>c)c=xp[i]; rp[i]=c; } decG(x); return r; }
      #endif
    }
    if (rtid==n_ne) { // ≠
      f64 x0 = IGetU(x,0).f; if (x0!=0 && x0!=1) goto base;
      if (xe==el_i8 ) { i8*  xp=i8any_ptr (x); u64* rp; B r=m_bitarrv(&rp,ia); bool c=x0; rp[0]=c; for (usz i=1; i<ia; i++) { c = c!=xp[i]; bitp_set(rp,i,c); } decG(x); return r; }
      if (xe==el_i16) { i16* xp=i16any_ptr(x); u64* rp; B r=m_bitarrv(&rp,ia); bool c=x0; rp[0]=c; for (usz i=1; i<ia; i++) { c = c!=xp[i]; bitp_set(rp,i,c); } decG(x); return r; }
      if (xe==el_i32) { i32* xp=i32any_ptr(x); u64* rp; B r=m_bitarrv(&rp,ia); bool c=x0; rp[0]=c; for (usz i=1; i<ia; i++) { c = c!=xp[i]; bitp_set(rp,i,c); } decG(x); return r; }
    }
    if (rtid==n_or) { // ∨
      if (xe==el_i8 ) { i8*  xp=i8any_ptr (x); u64* rp; B r=m_bitarrv(&rp,ia); bool c=0; for (usz i=0; i<ia; i++) { if ((xp[i]&1)!=xp[i])goto base; c|=xp[i]; bitp_set(rp,i,c); } decG(x); return r; }
      if (xe==el_i16) { i16* xp=i16any_ptr(x); u64* rp; B r=m_bitarrv(&rp,ia); bool c=0; for (usz i=0; i<ia; i++) { if ((xp[i]&1)!=xp[i])goto base; c|=xp[i]; bitp_set(rp,i,c); } decG(x); return r; }
      if (xe==el_i32) { i32* xp=i32any_ptr(x); u64* rp; B r=m_bitarrv(&rp,ia); bool c=0; for (usz i=0; i<ia; i++) { if ((xp[i]&1)!=xp[i])goto base; c|=xp[i]; bitp_set(rp,i,c); } decG(x); return r; }
    }
  }
  base:;
  SLOW2("𝕎` 𝕩", f, x);
  
  bool reuse = TY(x)==t_harr && reusable(x);
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
  if (isAtm(x) || RNK(x)==0) thrM("`: 𝕩 cannot have rank 0");
  ur xr = RNK(x); usz* xsh = SH(x); usz ia = IA(x);
  B wf = getFillQ(w);
  u8 xe = TI(x,elType);
  if (xr==1 && q_i32(w) && elInt(xe) && isFun(f) && v(f)->flags) {
    u8 rtid = v(f)->flags-1;
    i32 wv = o2iG(w);
    if (xe==el_bit) {
      u64* xp=bitarr_ptr(x);
      if (rtid==n_add) { i32* rp; B r=m_i32arrv(&rp, ia); i64 c=wv; for (usz i=0; i<ia; i++) { c+= bitp_get(xp,i); rp[i]=c; } decG(x); return r; }
      if (rtid==n_ne) { B r=scan_ne(-(u64)(q_ibit(wv)?wv:1&~*xp), xp, ia); decG(x); return r; }
      goto base;
    }
    if (rtid==n_add) { // +
      if (xe==el_i8 ) { i8*  xp=i8any_ptr (x); i32* rp; B r=m_i32arrv(&rp, ia); i32 c=wv; for (usz i=0; i<ia; i++) { if(addOn(c,xp[i]))goto base; rp[i]=c; } decG(x); return r; }
      if (xe==el_i16) { i16* xp=i16any_ptr(x); i32* rp; B r=m_i32arrv(&rp, ia); i32 c=wv; for (usz i=0; i<ia; i++) { if(addOn(c,xp[i]))goto base; rp[i]=c; } decG(x); return r; }
      if (xe==el_i32) { i32* xp=i32any_ptr(x); i32* rp; B r=m_i32arrv(&rp, ia); i32 c=wv; for (usz i=0; i<ia; i++) { if(addOn(c,xp[i]))goto base; rp[i]=c; } decG(x); return r; }
    }
    if (rtid==n_floor) { // ⌊
      if (xe==el_i8  && wv==(i8 )wv) { i8*  xp=i8any_ptr (x); i8*  rp; B r=m_i8arrv (&rp, ia); i8  c=wv; for (usz i=0; i<ia; i++) { if (xp[i]<c)c=xp[i]; rp[i]=c; } decG(x); return r; }
      if (xe==el_i16 && wv==(i16)wv) { i16* xp=i16any_ptr(x); i16* rp; B r=m_i16arrv(&rp, ia); i16 c=wv; for (usz i=0; i<ia; i++) { if (xp[i]<c)c=xp[i]; rp[i]=c; } decG(x); return r; }
      if (xe==el_i32 && wv==(i32)wv) { i32* xp=i32any_ptr(x); i32* rp; B r=m_i32arrv(&rp, ia); i32 c=wv; for (usz i=0; i<ia; i++) { if (xp[i]<c)c=xp[i]; rp[i]=c; } decG(x); return r; }
    }
    if (rtid==n_ceil) { // ⌈
      if (xe==el_i8  && wv==(i8 )wv) { i8*  xp=i8any_ptr (x); i8*  rp; B r=m_i8arrv (&rp, ia); i8  c=wv; for (usz i=0; i<ia; i++) { if (xp[i]>c)c=xp[i]; rp[i]=c; } decG(x); return r; }
      if (xe==el_i16 && wv==(i16)wv) { i16* xp=i16any_ptr(x); i16* rp; B r=m_i16arrv(&rp, ia); i16 c=wv; for (usz i=0; i<ia; i++) { if (xp[i]>c)c=xp[i]; rp[i]=c; } decG(x); return r; }
      if (xe==el_i32 && wv==(i32)wv) { i32* xp=i32any_ptr(x); i32* rp; B r=m_i32arrv(&rp, ia); i32 c=wv; for (usz i=0; i<ia; i++) { if (xp[i]>c)c=xp[i]; rp[i]=c; } decG(x); return r; }
    }
    if (rtid==n_ne) { // ≠
      if (!q_ibit(wv)) { goto base; }  bool c=wv;
      if (xe==el_i8 ) { i8*  xp=i8any_ptr (x); u64* rp; B r=m_bitarrv(&rp, ia); for (usz i=0; i<ia; i++) { c^= xp[i]; bitp_set(rp,i,c); } decG(x); return r; }
      if (xe==el_i16) { i16* xp=i16any_ptr(x); u64* rp; B r=m_bitarrv(&rp, ia); for (usz i=0; i<ia; i++) { c^= xp[i]; bitp_set(rp,i,c); } decG(x); return r; }
      if (xe==el_i32) { i32* xp=i32any_ptr(x); u64* rp; B r=m_bitarrv(&rp, ia); for (usz i=0; i<ia; i++) { c^= xp[i]; bitp_set(rp,i,c); } decG(x); return r; }
    }
  }
  base:;
  SLOW3("𝕨 F` 𝕩", w, x, f);
  
  bool reuse = (TY(x)==t_harr && reusable(x)) | !ia;
  usz i = 0;
  HArr_p r = reuse? harr_parts(REUSE(x)) : m_harr0c(x);
  AS2B xget = reuse? TI(x,getU) : TI(x,get); Arr* xa = a(x);
  BBB2B fc2 = c2fn(f);
  
  if (isArr(w)) {
    ur wr = RNK(w); usz* wsh = SH(w); SGet(w)
    if (wr+1!=xr || !eqShPart(wsh, xsh+1, wr)) thrF("`: Shape of 𝕨 must match the cell of 𝕩 (%H ≡ ≢𝕨, %H ≡ ≢𝕩)", w, x);
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
