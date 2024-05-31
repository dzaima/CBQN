#include "../core.h"
#include "../utils/mut.h"
#include "../builtins.h"
#include <math.h>
#define F64_MIN -INFINITY
#define F64_MAX  INFINITY

#if !USE_VALGRIND
static u64 vg_rand(u64 x) { return x; }
#endif

#if SINGELI
  #define SINGELI_FILE scan
  #include "../utils/includeSingeli.h"
#endif


B scan_ne(B x, u64 p, u64 ia) { // consumes x
  u64* xp = bitarr_ptr(x);
  u64* rp; B r=m_bitarrv(&rp,ia);
#if SINGELI
  si_scan_ne(p, xp, rp, BIT_N(ia));
  #if USE_VALGRIND
  if (ia&63) rp[ia>>6] = vg_def_u64(rp[ia>>6]);
  #endif
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
  decG(x); return r;
}

static B scan_or(B x, u64 ia) { // consumes x
  u64* xp = bitarr_ptr(x);
  u64* rp; B r=m_bitarrv(&rp,ia);
  usz n=BIT_N(ia); u64 xi; usz i=0;
  while (i<n) if ((xi= vg_rand(xp[i]))!=0) { rp[i] = -(xi&-xi)  ; i++; while(i<n) rp[i++] = ~0LL; break; } else rp[i++]=0;
  decG(x); return FL_SET(r, fl_asc|fl_squoze);
}
static B scan_and(B x, u64 ia) { // consumes x
  u64* xp = bitarr_ptr(x);
  u64* rp; B r=m_bitarrv(&rp,ia);
  usz n=BIT_N(ia); u64 xi; usz i=0;
  while (i<n) if ((xi=~vg_rand(xp[i]))!=0) { rp[i] =  (xi&-xi)-1; i++; while(i<n) rp[i++] =  0  ; break; } else rp[i++]=~0LL;
  decG(x); return FL_SET(r, fl_dsc|fl_squoze);
}

B slash_c1(B t, B x);
B scan_add_bool(B x, u64 ia) { // consumes x
  u64* xp = bitarr_ptr(x);
  u64 xs = bit_sum(xp, ia);
  if (xs<=1) return xs==0? x : scan_or(x, ia);
  B r;
  u8 re = xs<=I8_MAX? el_i8 : xs<=I16_MAX? el_i16 : xs<=I32_MAX? el_i32 : el_f64;
  if (xs < ia/128) {
    B ones = C1(slash, x);
    MAKE_MUT_INIT(r0, ia, re); MUTG_INIT(r0);
    SGetU(ones)
    usz ri = 0;
    for (usz i = 0; i < xs; i++) {
      usz e = o2s(GetU(ones, i));
      mut_fillG(r0, ri, m_usz(i), e-ri);
      ri = e;
    }
    if (ri<ia) mut_fillG(r0, ri, m_usz(xs), ia-ri);
    decG(ones);
    r = mut_fv(r0);
  } else {
    void* rp = m_tyarrv(&r, elWidth(re), ia, el2t(re));
    #define SUM_BITWISE(T) { T c=0; for (usz i=0; i<ia; i++) { c+= bitp_get(xp,i); ((T*)rp)[i]=c; } }
    #if SINGELI
      #define SUM(W,T) si_bcs##W(xp, rp, ia);
    #else
      #define SUM(W,T) SUM_BITWISE(T)
    #endif
    #define CASE(W) case el_i##W: SUM(W, i##W) break;
    switch (re) { default:UD;
      CASE(8) CASE(16) CASE(32) case el_f64: SUM_BITWISE(f64) break;
    }
    #undef CASE
    #undef SUM
    #undef SUM_BITWISE
    decG(x);
  }
  return FL_SET(r, fl_asc|fl_squoze);
}

// min/max-scan
#if SINGELI
  #define MINMAX_SCAN(T,NAME,C,I) si_scan_##NAME##_init_##T(xp, rp, ia, I);
#else
  #define MINMAX_SCAN(T,NAME,C,I) T c=I; for (usz i=0; i<ia; i++) { if (xp[i] C c)c=xp[i]; rp[i]=c; }
#endif
#define MM_CASE(T,N,C,I) \
  case el_##T : { T* xp=T##any_ptr(x); T* rp; r=m_##T##arrv(&rp, ia); MINMAX_SCAN(T,N,C,I); break; }
#define MINMAX(NAME,C,INIT,BIT,ORD) \
  B r; switch (xe) { default:UD;           \
    case el_bit: return scan_##BIT(x, ia); \
    MM_CASE(i8 ,NAME,C,I8_##INIT )         \
    MM_CASE(i16,NAME,C,I16_##INIT)         \
    MM_CASE(i32,NAME,C,I32_##INIT)         \
    MM_CASE(f64,NAME,C,F64_##INIT)         \
  }                                        \
  decG(x); return FL_SET(r, fl_##ORD);
B scan_min_num(B x, u8 xe, u64 ia) { MINMAX(min,<,MAX,and,dsc) }
B scan_max_num(B x, u8 xe, u64 ia) { MINMAX(max,>,MIN,or ,asc) }
#undef MINMAX
// Initialized: try to convert 𝕨 to type of 𝕩
// (could do better for out-of-range floats)
B shape_c2(B, B, B);
#define MM2_ICASE(T,N,C,I) \
  case el_##T : { \
    if (wv!=(T)wv) { if (wv C 0) { r=C2(shape,m_f64(ia),w); break; } else wv=I; } \
    T* xp=T##any_ptr(x); T* rp; r=m_##T##arrv(&rp, ia); MINMAX_SCAN(T,N,C,wv); \
  break; }
#define MINMAX2(NAME,C,INIT,BIT,BI,ORD) \
  i32 wv=0; if (q_i32(w)) { wv=o2fG(w); } else { x=taga(cpyF64Arr(x)); xe=el_f64; } \
  B r; switch (xe) { default:UD;           \
    case el_bit: if (wv C BI) r=C2(shape,m_f64(ia),w); else return scan_##BIT(x, ia); break; \
    MM2_ICASE(i8 ,NAME,C,I8_##INIT )       \
    MM2_ICASE(i16,NAME,C,I16_##INIT)       \
    MM_CASE(i32,NAME,C,wv)                 \
    MM_CASE(f64,NAME,C,o2fG(w))            \
  }                                        \
  decG(x); return FL_SET(r, fl_##ORD);
SHOULD_INLINE B scan2_min_num(B w, B x, u8 xe, usz ia) { MINMAX2(min,<,MAX,and,1,dsc) }
SHOULD_INLINE B scan2_max_num(B w, B x, u8 xe, usz ia) { MINMAX2(max,>,MIN,or ,0,asc) }
#undef MINMAX2
#undef MM_CASE
#undef MM_CASE2
#undef MINMAX_SCAN

static B scan_lt(B x, u64 p, usz ia) {
  u64* xp = bitarr_ptr(x);
  u64* rp; B r=m_bitarrv(&rp,ia); usz n=BIT_N(ia);
  u64 m10 = 0x5555555555555555;
  for (usz i=0; i<n; i++) {
    u64 x = xp[i];
    u64 c  = (m10 & ~(x<<1)) & ~(p>>63);
    rp[i] = p = x & (m10 ^ (x + c));
  }
  decG(x); return r;
}

static B scan_plus(f64 r0, B x, u8 xe, usz ia) {
  assert(xe!=el_bit && elNum(xe));
  B r; void* rp = m_tyarrv(&r, xe==el_f64? sizeof(f64) : sizeof(i32), ia, xe==el_f64? t_f64arr : t_i32arr);
  #if SINGELI
    switch(xe) { default:UD;
      case el_i8:  { if (!q_fi32(r0) || si_scan_plus_i8_i32 (i8any_ptr(x),  r0, rp, ia)!=ia) goto cs_i8_f64;  decG(x); return r; }
      case el_i16: { if (!q_fi32(r0) || si_scan_plus_i16_i32(i16any_ptr(x), r0, rp, ia)!=ia) goto cs_i16_f64; decG(x); return r; }
      case el_i32: { if (!q_fi32(r0) || si_scan_plus_i32_i32(i32any_ptr(x), r0, rp, ia)!=ia) goto cs_i32_f64; decG(x); return r; }
      case el_f64: { f64* xp=f64any_ptr(x); f64 c=r0; for (usz i=0; i<ia; i++) { c+= xp[i];  ((f64*)rp)[i]=c; } decG(x); return r; }
    }
    cs_i8_f64: { x=taga(cpyI16Arr(x)); goto cs_i16_f64; }
    cs_i16_f64: { decG(r); f64* rp; r = m_f64arrv(&rp, ia); si_scan_plus_i16_f64(i16any_ptr(x), r0, rp, ia); decG(x); return r; }
    cs_i32_f64: { decG(r); f64* rp; r = m_f64arrv(&rp, ia); si_scan_plus_i32_f64(i32any_ptr(x), r0, rp, ia); decG(x); return r; }
  #else
    if (xe==el_i8  && q_fi32(r0)) { i8*  xp=i8any_ptr (x); i32 c=r0; for (usz i=0; i<ia; i++) { if (addOn(c,xp[i])) goto base; ((i32*)rp)[i]=c; } decG(x); return r; }
    if (xe==el_i16 && q_fi32(r0)) { i16* xp=i16any_ptr(x); i32 c=r0; for (usz i=0; i<ia; i++) { if (addOn(c,xp[i])) goto base; ((i32*)rp)[i]=c; } decG(x); return r; }
    if (xe==el_i32 && q_fi32(r0)) { i32* xp=i32any_ptr(x); i32 c=r0; for (usz i=0; i<ia; i++) { if (addOn(c,xp[i])) goto base; ((i32*)rp)[i]=c; } decG(x); return r; }
    if (xe==el_f64) {   res_float:; f64* xp=f64any_ptr(x); f64 c=r0; for (usz i=0; i<ia; i++) { c+= xp[i];                     ((f64*)rp)[i]=c; } decG(x); return r; }
    base:;
    decG(r);
    f64* rp2; r = m_f64arrv(&rp2, ia); rp = rp2;
    x = toF64Any(x);
    goto res_float;
  #endif
}

B fne_c1(B, B);
B shape_c2(B, B, B);

extern B scan_arith(B f, B w, B x, usz* xsh); // from cells.c
B scan_c1(Md1D* d, B x) { B f = d->f;
  if (isAtm(x) || RNK(x)==0) thrM("`: Argument cannot have rank 0");
  ur xr = RNK(x);
  usz ia = IA(x);
  if (*SH(x)<=1 || ia==0) return x;
  if (RARE(!isFun(f))) {
    if (isMd(f)) thrM("Calling a modifier");
    B xf = getFillR(x);
    MAKE_MUT(rm, ia);
    usz csz = arr_csz(x);
    mut_copy(rm, 0, x, 0, csz);
    mut_fill(rm, csz, f, ia-csz);
    return withFill(mut_fcd(rm, x), xf);
  }
  u8 xe = TI(x,elType);
  if (v(f)->flags) {
    u8 rtid = v(f)->flags-1;
    if (rtid==n_rtack) return x;
    if (rtid==n_ltack) {
      usz csz = arr_csz(x);
      B s = C1(fne, incG(x));
      Arr* r = TI(x,slice)(x, 0, csz);
      return C2(shape, s, taga(r));
    }
    if (!(xr==1 && xe<=el_f64)) goto base;
    
    if (xe==el_bit) {
      if (rtid==n_add                              ) return scan_add_bool(x, ia); // +
      if (rtid==n_or  |               rtid==n_ceil ) return scan_or(x, ia);       // ∨⌈
      if (rtid==n_and | rtid==n_mul | rtid==n_floor) return scan_and(x, ia);      // ∧×⌊
      if (rtid==n_ne                               ) return scan_ne(x, 0, ia);    // ≠
      if (rtid==n_lt)                                return scan_lt(x, 0, ia);    // <
      goto base;
    }
    if (rtid==n_add) return scan_plus(0, x, xe, ia); // +
    if (rtid==n_floor) return scan_min_num(x, xe, ia); // ⌊
    if (rtid==n_ceil ) return scan_max_num(x, xe, ia); // ⌈
    if (rtid==n_ne) { // ≠
      if (!elInt(xe)) goto base;
      f64 x0 = o2fG(IGetU(x,0));
      if (!q_fbit(x0)) goto base;
      u64* rp; B r = m_bitarrv(&rp, ia);
      bool c = x0;
      rp[0] = c;
      if (xe==el_i8 ) { i8*  xp=i8any_ptr (x); for (usz i=1; i<ia; i++) { c = c!=xp[i]; bitp_set(rp,i,c); } decG(x); return r; }
      if (xe==el_i16) { i16* xp=i16any_ptr(x); for (usz i=1; i<ia; i++) { c = c!=xp[i]; bitp_set(rp,i,c); } decG(x); return r; }
      if (xe==el_i32) { i32* xp=i32any_ptr(x); for (usz i=1; i<ia; i++) { c = c!=xp[i]; bitp_set(rp,i,c); } decG(x); return r; }
      UD;
    }
    if (rtid==n_or) { x=num_squeezeChk(x); xe=TI(x,elType); if (xe==el_bit) return scan_or(x, ia); }
  }
  base:;
  if (xr>1 && ia >= 6 * (u64)*SH(x) && isPervasiveDy(f)) return scan_arith(f, m_f64(0), x, SH(x));
  SLOW2("𝕎` 𝕩", f, x);
  B xf = getFillR(x);
  
  HArr_p r = m_harr0c(x);
  SGet(x)
  FC2 fc2 = c2fn(f);
  
  if (xr==1) {
    r.a[0] = Get(x,0);
    for (usz i=1; i<ia; i++) r.a[i] = fc2(f, inc(r.a[i-1]), Get(x,i));
  } else {
    usz csz = arr_csz(x);
    usz i = 0;
    for (; i<csz; i++) r.a[i] = Get(x,i);
    for (; i<ia; i++) r.a[i] = fc2(f, inc(r.a[i-csz]), Get(x,i));
  }
  decG(x);
  return withFill(r.b, xf);
}

B add_c2(B, B, B);
B scan_c2(Md1D* d, B w, B x) { B f = d->f;
  if (isAtm(x) || RNK(x)==0) thrM("`: 𝕩 cannot have rank 0");
  ur xr = RNK(x); usz* xsh = SH(x); usz ia = IA(x);
  if (isArr(w)? !ptr_eqShape(SH(w), RNK(w), xsh+1, xr-1) : xr!=1) thrF("`: Shape of 𝕨 must match the cell of 𝕩 (%H ≡ ≢𝕨, %H ≡ ≢𝕩)", w, x);
  if (ia==0) { dec(w); return x; }
  if (RARE(!isFun(f))) {
    if (isMd(f)) thrM("Calling a modifier");
    B xf = getFillR(x);
    MAKE_MUT(rm, ia);
    mut_fill(rm, 0, f, ia);
    return withFill(mut_fcd(rm, x), xf);
  }
  u8 xe = TI(x,elType);
  if (v(f)->flags) {
    u8 rtid = v(f)->flags-1;
    if (rtid==n_rtack) { dec(w); return x; }
    if (rtid==n_ltack) return C2(shape, C1(fne, x), w);
    if (!(xr==1 && elNum(xe) && xe<=el_f64)) goto base;
    if (!isF64(w)) goto base;
    
    if (rtid==n_floor) return scan2_min_num(w, x, xe, ia); // ⌊
    if (rtid==n_ceil ) return scan2_max_num(w, x, xe, ia); // ⌈
    
    if (rtid==n_add) { // +
      if (xe==el_bit) {
        if (!q_i64(w)) goto base;
        i64 wv = o2i64G(w);
        if (wv<=(-(1LL<<53)) || wv>=(1LL<<53) || wv+(i64)ia >= (1LL<<53)) goto base;
        B t = scan_add_bool(x, ia);
        return wv==0? t : C2(add, w, t);
      }
      
      if (elInt(xe)) return scan_plus(o2fG(w), x, xe, ia);
    }
    
    if (rtid==n_ne) { // ≠
      bool wBit = q_bit(w);
      if (xe==el_bit) return scan_ne(x, -(u64)(wBit? o2bG(w) : 1&~*bitarr_ptr(x)), ia);
      if (!wBit || !elInt(xe)) goto base;
      bool c = o2bG(w);
      u64* rp; B r = m_bitarrv(&rp, ia);
      if (xe==el_i8 ) { i8*  xp=i8any_ptr (x); for (usz i=0; i<ia; i++) { c^= xp[i]; bitp_set(rp,i,c); } decG(x); return r; }
      if (xe==el_i16) { i16* xp=i16any_ptr(x); for (usz i=0; i<ia; i++) { c^= xp[i]; bitp_set(rp,i,c); } decG(x); return r; }
      if (xe==el_i32) { i32* xp=i32any_ptr(x); for (usz i=0; i<ia; i++) { c^= xp[i]; bitp_set(rp,i,c); } decG(x); return r; }
      UD;
    }
    
    if (xe==el_bit && q_bit(w)) {
      // ⌊ & ⌈ handled above
      if (rtid==n_or               ) return !o2bG(w)? scan_or(x, ia)  : i64EachDec(1, x); // ∨
      if (rtid==n_and | rtid==n_mul) return  o2bG(w)? scan_and(x, ia) : i64EachDec(0, x); // ∧×
      if (rtid==n_lt)                return scan_lt(x, bitx(w), ia); // <
    }
  }
  base:;
  if (xr>1 && ia >= 6 * (u64)*SH(x) && isPervasiveDy(f)) return scan_arith(f, w, x, SH(x));
  SLOW3("𝕨 F` 𝕩", w, x, f);
  B wf = getFillR(w);
  
  usz i = 0;
  HArr_p r = m_harr0c(x);
  SGet(x)
  FC2 fc2 = c2fn(f);
  
  if (isArr(w)) {
    usz csz = arr_csz(x);
    SGet(w)
    for (; i < csz; i++) r.a[i] = fc2(f, Get(w,i), Get(x,i));
    for (; i < ia; i++) r.a[i] = fc2(f, inc(r.a[i-csz]), Get(x,i));
    decG(w);
  } else {
    B pr = r.a[0] = fc2(f, w, Get(x,0)); i++;
    for (; i < ia; i++) r.a[i] = pr = fc2(f, inc(pr), Get(x,i));
  }
  decG(x);
  return withFill(r.b, wf);
}

B scan_rows_bit(Md1D* fd, B x) {
  assert(isArr(x) && RNK(x)==2 && TI(x,elType)==el_bit);
  #if SINGELI
  if (!v(fd->f)->flags) return bi_N;
  u8 rtid = v(fd->f)->flags-1;
  if (rtid==n_and|rtid==n_or) {
    usz *sh = SH(x); usz n = sh[0]; usz m = sh[1];
    u64* xp = bitarr_ptr(x);
    u64* rp; B r = m_bitarrc(&rp, x);
    if (rtid==n_and) si_scan_rows_and(xp, rp, n, m);
    else             si_scan_rows_or (xp, rp, n, m);
    decG(x); return r;
  }
  #endif
  return bi_N;
}
