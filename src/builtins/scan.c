#include "../core.h"
#include "../utils/mut.h"
#include "../builtins.h"
#include <math.h>
#define F64_MIN -INFINITY
#define F64_MAX  INFINITY

#if !USE_VALGRIND
static u64 vg_rand(u64 x) { return x; }
#endif

#if SINGELI_X86_64
  #define SINGELI_FILE scan
  #include "../utils/includeSingeli.h"
  #if __PCLMUL__
    #define SINGELI_FILE neq
    #include "../utils/includeSingeli.h"
  #endif
#endif


B scan_ne(B x, u64 p, u64 ia) { // consumes x
  u64* xp = bitarr_ptr(x);
  u64* rp; B r=m_bitarrv(&rp,ia);
#if SINGELI_X86_64 && __PCLMUL__
  clmul_scan_ne(p, xp, rp, BIT_N(ia));
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

B slash_c1(B f, B x);
B scan_add_bool(B x, u64 ia) { // consumes x
  u64* xp = bitarr_ptr(x);
  u64 xs = bit_sum(xp, ia);
  if (xs<=1) return xs==0? x : scan_or(x, ia);
  B r;
  u8 re = xs<=I8_MAX? el_i8 : xs<=I16_MAX? el_i16 : xs<=I32_MAX? el_i32 : el_f64;
  if (xs < ia/128) {
    B ones = C1(slash, x);
    MAKE_MUT(r0, ia) mut_init(r0, re); MUTG_INIT(r0);
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
    #if SINGELI_X86_64
      #define SUM(W,T) avx2_bcs##W(xp, rp, ia);
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
#if SINGELI_X86_64
  #define MINMAX_SCAN(T,NAME,C,I) avx2_scan_##NAME##_init_##T(xp, rp, ia, I);
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
  i32 wv=0; if (q_i32(w)) { wv=w.f; } else { x=taga(cpyF64Arr(x)); xe=el_f64; } \
  B r; switch (xe) { default:UD;           \
    case el_bit: if (wv C BI) r=C2(shape,m_f64(ia),w); else return scan_##BIT(x, ia); break; \
    MM2_ICASE(i8 ,NAME,C,I8_##INIT )       \
    MM2_ICASE(i16,NAME,C,I16_##INIT)       \
    MM_CASE(i32,NAME,C,wv)                 \
    MM_CASE(f64,NAME,C,w.f)                \
  }                                        \
  decG(x); return FL_SET(r, fl_##ORD);
static B scan2_min_num(B w, B x, u8 xe, usz ia) { MINMAX2(min,<,MAX,and,1,dsc) }
static B scan2_max_num(B w, B x, u8 xe, usz ia) { MINMAX2(max,>,MIN,or ,0,asc) }
#undef MINMAX2
#undef MM_CASE
#undef MM_CASE2
#undef MINMAX_SCAN

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
      if (rtid==n_add                              ) return scan_add_bool(x, ia);
      if (rtid==n_or  |               rtid==n_ceil ) return scan_or(x, ia);
      if (rtid==n_and | rtid==n_mul | rtid==n_floor) return scan_and(x, ia);
      if (rtid==n_ne                               ) return scan_ne(x, 0, ia);
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
    if (rtid==n_floor) return scan_min_num(x, xe, ia); // ⌊
    if (rtid==n_ceil ) return scan_max_num(x, xe, ia); // ⌈
    if (rtid==n_ne) { // ≠
      f64 x0 = IGetU(x,0).f; if (x0!=0 && x0!=1) goto base;
      if (xe==el_i8 ) { i8*  xp=i8any_ptr (x); u64* rp; B r=m_bitarrv(&rp,ia); bool c=x0; rp[0]=c; for (usz i=1; i<ia; i++) { c = c!=xp[i]; bitp_set(rp,i,c); } decG(x); return r; }
      if (xe==el_i16) { i16* xp=i16any_ptr(x); u64* rp; B r=m_bitarrv(&rp,ia); bool c=x0; rp[0]=c; for (usz i=1; i<ia; i++) { c = c!=xp[i]; bitp_set(rp,i,c); } decG(x); return r; }
      if (xe==el_i32) { i32* xp=i32any_ptr(x); u64* rp; B r=m_bitarrv(&rp,ia); bool c=x0; rp[0]=c; for (usz i=1; i<ia; i++) { c = c!=xp[i]; bitp_set(rp,i,c); } decG(x); return r; }
    }
    if (rtid==n_or) { x=num_squeezeChk(x); xe=TI(x,elType); if (xe==el_bit) return scan_or(x, ia); }
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
  if (xr==1 && xe<=el_f64 && isFun(f) && v(f)->flags) {
    u8 rtid = v(f)->flags-1;
    if (rtid==n_floor) return scan2_min_num(w, x, xe, ia); // ⌊
    if (rtid==n_ceil ) return scan2_max_num(w, x, xe, ia); // ⌈
    if (!q_i32(w)) goto base;
    i32 wv = o2iG(w);
    if (xe==el_bit) {
      u64* xp=bitarr_ptr(x);
      if (rtid==n_add) { i32* rp; B r=m_i32arrv(&rp, ia); i64 c=wv; for (usz i=0; i<ia; i++) { c+= bitp_get(xp,i); rp[i]=c; } decG(x); return r; }
      if (rtid==n_ne) return scan_ne(x, -(u64)(q_ibit(wv)?wv:1&~*xp), ia);
      goto base;
    }
    if (rtid==n_add) { // +
      if (xe==el_i8 ) { i8*  xp=i8any_ptr (x); i32* rp; B r=m_i32arrv(&rp, ia); i32 c=wv; for (usz i=0; i<ia; i++) { if(addOn(c,xp[i]))goto base; rp[i]=c; } decG(x); return r; }
      if (xe==el_i16) { i16* xp=i16any_ptr(x); i32* rp; B r=m_i32arrv(&rp, ia); i32 c=wv; for (usz i=0; i<ia; i++) { if(addOn(c,xp[i]))goto base; rp[i]=c; } decG(x); return r; }
      if (xe==el_i32) { i32* xp=i32any_ptr(x); i32* rp; B r=m_i32arrv(&rp, ia); i32 c=wv; for (usz i=0; i<ia; i++) { if(addOn(c,xp[i]))goto base; rp[i]=c; } decG(x); return r; }
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
