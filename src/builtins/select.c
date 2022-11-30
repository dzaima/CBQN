// First Cell and Select (⊏)

// First Cell is just a slice

// Complications in Select mostly come from range checks and negative 𝕨
// Atom 𝕨 and any rank 𝕩: slice
// Rank-1 𝕩:
//   Empty 𝕨: no selection
//   Small 𝕩 with Singeli: use shuffles
//   Boolean 𝕨: use bit_sel for blend or similar
//   Boolean 𝕩 and larger 𝕨: convert to i8, select, convert back
//   Boolean 𝕩 otherwise: select/shift bytes, reversed for fast writing
//     TRIED pext, doesn't seem faster (mask built with shifts anyway)
//   SHOULD squeeze 𝕨 if not ≤i32 to get to optimized cases
//   Integer 𝕨 with Singeli: fused wrap, range-check, and gather
//     COULD try selecting from boolean with gather
//     COULD detect <Skylake where gather is slow
//   i32 𝕨: wrap, check, select one index at a time
//   i8 and i16 𝕨: separate range check in blocks to auto-vectorize
// SHOULD optimize simple 𝕨 based on cell size for any rank 𝕩
// SHOULD implement nested 𝕨

// Under Select ⌾(i⊸⊏)
// Specialized for rank-1 numeric 𝕩
// SHOULD apply to characters as well
// No longer needs to range-check but indices can be negative
//   COULD convert negative indices before selection
// Must check collisions if CHECK_VALID; uses a byte set
//   Sparse initialization if 𝕨 is much smaller than 𝕩
//   COULD call Mark Firsts (∊) for very short 𝕨 to avoid allocation

#include "../core.h"
#include "../utils/talloc.h"
#include "../utils/mut.h"
#include "../builtins.h"

#if SINGELI
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wunused-variable"
  #include "../singeli/gen/select.c"
  #pragma GCC diagnostic pop
#endif

extern B rt_select;
B select_c1(B t, B x) {
  if (isAtm(x)) thrM("⊏: Argument cannot be an atom");
  ur xr = RNK(x);
  if (xr==0) thrM("⊏: Argument cannot be rank 0");
  if (SH(x)[0]==0) thrF("⊏: Argument shape cannot start with 0 (%H ≡ ≢𝕩)", x);
  usz ia = shProd(SH(x), 1, xr);
  Arr* r = TI(x,slice)(incG(x), 0, ia);
  usz* sh = arr_shAlloc(r, xr-1);
  if (sh) shcpy(sh, SH(x)+1, xr-1);
  decG(x);
  return taga(r);
}
B select_c2(B t, B w, B x) {
  if (isAtm(x)) thrM("⊏: 𝕩 cannot be an atom");
  ur xr = RNK(x);
  if (isAtm(w)) {
    if (xr==0) thrM("⊏: 𝕩 cannot be a unit");
    usz csz = arr_csz(x);
    usz cam = SH(x)[0];
    usz wi = WRAP(o2i64(w), cam, thrF("⊏: Indexing out-of-bounds (𝕨≡%R, %s≡≠𝕩)", w, cam));
    Arr* r = TI(x,slice)(incG(x), wi*csz, csz);
    usz* sh = arr_shAlloc(r, xr-1);
    if (sh) shcpy(sh, SH(x)+1, xr-1);
    decG(x);
    return taga(r);
  }
  B xf = getFillQ(x);
  SGet(x)
  usz wia = IA(w);
  B r;
  
  if (xr==1) {
    if (wia==0) {
      decG(x);
      if (RNK(w)==1) {
        if (isNum(xf)) { r = emptyIVec(); goto ret; }
        if (isC32(xf)) { r = emptyCVec(); goto ret; }
      }
      Arr* ra;
      if (isNum(xf) || isC32(xf)) {
        ra = m_arr(sizeof(TyArr), isNum(xf)? t_i8arr : t_c8arr, 0);
      } else {
        ra = m_fillarrp(0);
        fillarr_setFill(ra, xf);
      }
      arr_shCopy(ra, w);
      r = taga(ra);
      ret:
      decG(w);
      return r;
    }
    usz xia = IA(x);
    if (xia==0) goto base; // can't just error immediately because depth 2 𝕨
    u8 xe = TI(x,elType);
    u8 we = TI(w,elType);
    #if SINGELI
      #define CPUSEL(W, NEXT) \
        if (!avx2_select_tab[4*(we-el_i8)+CTZ(xw)](wp, xp, rp, wia, xia)) thrM("⊏: Indexing out-of-bounds");
      #define BOOL_USE_SIMD (xia<=128)
      #define BOOL_SPECIAL(W) \
        if (sizeof(W)==1 && BOOL_USE_SIMD) { \
          if (!avx2_select_bool128(wp, xp, rp, wia, xia)) thrM("⊏: Indexing out-of-bounds"); \
          goto dec_ret;                                   \
        }
    #else
      #define CPUSEL(W, NEXT) \
        if (sizeof(W) >= 4) {                           \
          switch(xw) { default:UD; CASEW(1,u8); CASEW(2,u16); CASEW(4,u32); CASEW(8,f64); } \
        } else {                                        \
          W* wt = NULL;                                 \
          for (usz bl=(1<<14)/sizeof(W), i0=0, i1=0; i0<wia; i0=i1) { \
            i1+=bl; if (i1>wia) i1=wia;                 \
            W min=wp[i0], max=min; for (usz i=i0+1; i<i1; i++) { W e=wp[i]; if (e>max) max=e; if (e<min) min=e; } \
            if (min<-(i64)xia) thrF("⊏: Indexing out-of-bounds (%i∊𝕨, %s≡≠𝕩)", min, xia); \
            if (max>=(i64)xia) thrF("⊏: Indexing out-of-bounds (%i∊𝕨, %s≡≠𝕩)", max, xia); \
            W* ip=wp; usz off=xia;                      \
            if (max>=0) { off=0; if (RARE(min<0)) {     \
              if (RARE(xia > (1ULL<<(sizeof(W)*8-1)))) { w=taga(NEXT(w)); mm_free(v(r)); return select_c2(m_f64(0), w, x); } \
              if (!wt) {wt=TALLOCP(W,i1-i0);} ip=wt-i0; \
              for (usz i=i0; i<i1; i++) { W e=wp[i]; ip[i]=e+((W)xia & (W)-(e<0)); } \
            } }                                         \
            switch(xw) { default:UD; CASE(1,u8); CASE(2,u16); CASE(4,u32); CASE(8,f64); } \
          }                                             \
          if (wt) TFREE(wt);                            \
        }
      #define BOOL_USE_SIMD 0
      #define BOOL_SPECIAL(W)
    #endif
    #define CASE(S, E)  case S: for (usz i=i0; i<i1; i++) ((E*)rp)[i] = ((E*)xp+off)[ip[i]]; break
    #define CASEW(S, E) case S: for (usz i=0; i<wia; i++) ((E*)rp)[i] = ((E*)xp)[WRAP(wp[i], xia, thrF("⊏: Indexing out-of-bounds (%i∊𝕨, %s≡≠𝕩)", wp[i], xia))]; break
    #define TYPE(W, NEXT) { W* wp = W##any_ptr(w);      \
      if (xe==el_bit) { u64* xp=bitarr_ptr(x);          \
        u64* rp; r = m_bitarrc(&rp, w);                 \
        BOOL_SPECIAL(W)                                 \
        u64 b=0;                                        \
        for (usz i = wia; ; ) {                         \
          i--;                                          \
          usz n = WRAP(wp[i], xia, thrF("⊏: Indexing out-of-bounds (%i∊𝕨, %s≡≠𝕩)", wp[i], xia)); \
          b = 2*b + ((((u8*)xp)[n/8] >> (n%8)) & 1);    \
          if (i%64 == 0) { rp[i/64]=b; if (!i) break; } \
        }                                               \
        goto dec_ret;                                   \
      }                                                 \
      if (xe!=el_B) {                                   \
        usz xw = elWidth(xe);                           \
        void* rp = m_tyarrc(&r, xw, w, el2t(xe));       \
        void* xp = tyany_ptr(x);                        \
        CPUSEL(W, NEXT)                                 \
        goto dec_ret;                                   \
      }                                                 \
      M_HARR(r, wia);                                   \
      if (TY(x)==t_harr || TY(x)==t_hslice) {     \
        B* xp = hany_ptr(x);                      \
        for (usz i=0; i < wia; i++) HARR_ADD(r, i, inc(xp[WRAP(wp[i], xia, thrF("⊏: Indexing out-of-bounds (%i∊𝕨, %s≡≠𝕩)", wp[i], xia))])); \
        decG(x); return HARR_FCD(r, w);           \
      } SLOW2("𝕨⊏𝕩", w, x);                       \
      for (usz i=0; i < wia; i++) HARR_ADD(r, i, Get(x, WRAP(wp[i], xia, thrF("⊏: Indexing out-of-bounds (%i∊𝕨, %s≡≠𝕩)", wp[i], xia)))); \
      decG(x); return withFill(HARR_FCD(r,w),xf); \
    }
    if (xe==el_bit && wia>=256 && !BOOL_USE_SIMD && wia/4>=xia && we!=el_bit) {
      return taga(cpyBitArr(select_c2(m_f64(0), w, taga(cpyI8Arr(x)))));
    }
    if (we==el_bit) {
      SGetU(x)
      B x0 = GetU(x, 0);
      B x1;
      if (xia<2) {
        u64* wp=bitarr_ptr(w);
        usz i; for (i=0; i<wia/64; i++) if (wp[i]) break;
        if (i<wia/64 || bitp_l0(wp,wia)!=0) thrF("⊏: Indexing out-of-bounds (1∊𝕨, %s≡≠𝕩)", xia);
        x1 = x0;
      } else {
        x1 = GetU(x,1);
      }
      r = bit_sel(w, x0, x1);
      decG(x);
      return withFill(r, xf);
    }
    else if (we==el_i8) TYPE(i8,cpyI16Arr)
    else if (we==el_i16) TYPE(i16,cpyI32Arr)
    else if (we==el_i32) TYPE(i32,cpyF64Arr)
    else {
      SLOW2("𝕨⊏𝕩", w, x);
      M_HARR(r, wia)
      SGetU(w)
      for (usz i = 0; i < wia; i++) {
        B cw = GetU(w, i);
        if (!isNum(cw)) { HARR_ABANDON(r); goto base; }
        usz c = WRAP(o2i64(cw), xia, thrF("⊏: Indexing out-of-bounds (%R∊𝕨, %s≡≠𝕩)", cw, xia));
        HARR_ADD(r, i, Get(x, c));
      }
      decG(x);
      return withFill(HARR_FCD(r,w),xf);
    }
    #undef CASE
    #undef CASEW
  } else {
    if (!elNum(TI(w,elType))) { // 𝕨 could be depth 2, in which case allocating the buffer isn't acceptable even temporarily
      w = num_squeezeChk(w);
      if (!elNum(TI(w,elType))) goto base;
    }
    SLOW2("𝕨⊏𝕩", w, x);
    SGetU(w)
    ur wr = RNK(w);
    i32 rr = wr+xr-1;
    if (xr==0) thrM("⊏: 𝕩 cannot be a unit");
    if (rr>UR_MAX) thrF("⊏: Result rank too large (%i≡=𝕨, %i≡=𝕩)", wr, xr);
    usz csz = arr_csz(x);
    usz cam = SH(x)[0];
    MAKE_MUT(r, wia*csz); mut_init(r, TI(x,elType));
    MUTG_INIT(r);
    for (usz i = 0; i < wia; i++) {
      B cw = GetU(w, i); // assumed number from previous squeeze
      usz c = WRAP(o2i64(cw), cam, { mut_pfree(r, i*csz); thrF("⊏: Indexing out-of-bounds (%R∊𝕨, %s≡≠𝕩)", cw, cam); });
      mut_copyG(r, i*csz, x, csz*c, csz);
    }
    Arr* ra = mut_fp(r);
    usz* rsh = arr_shAlloc(ra, rr);
    if (rsh) {
      shcpy(rsh   , SH(w)  , wr  );
      shcpy(rsh+wr, SH(x)+1, xr-1);
    }
    decG(w); decG(x);
    return withFill(taga(ra),xf);
  }
  base:;
  dec(xf);
  return c2(rt_select, w, x);
  
  dec_ret:;
  decG(w); decG(x); return r;
}




B select_ucw(B t, B o, B w, B x) {
  if (isAtm(x) || RNK(x)!=1 || isAtm(w)) return def_fn_ucw(t, o, w, x);
  usz xia = IA(x);
  usz wia = IA(w);
  SGetU(w)
  if (TI(w,elType)!=el_i32) for (usz i = 0; i < wia; i++) if (!q_i64(GetU(w,i))) return def_fn_ucw(t, o, w, x);
  B arg = select_c2(t, inc(w), inc(x));
  B rep = c1(o, arg);
  if (isAtm(rep) || !eqShape(w, rep)) thrF("𝔽⌾(a⊸⊏)𝕩: Result of 𝔽 must have the same shape as 'a' (expected %H, got %H)", w, rep);
  #if CHECK_VALID
    TALLOC(bool, set, xia);
    bool sparse = wia < xia/64;
    if (!sparse) for (i64 i = 0; i < xia; i++) set[i] = false;
    #define SPARSE_INIT(WI) \
      if (sparse) for (usz i = 0; i < wia; i++) {                    \
        i64 cw = WI; if (RARE(cw<0)) cw+= (i64)xia; set[cw] = false; \
      }
    #define EQ(F) if (set[cw] && (F)) thrM("𝔽⌾(a⊸⊏): Incompatible result elements"); set[cw] = true;
    #define FREE_CHECK TFREE(set)
    SLOWIF(xia>100 && wia<xia/20) SLOW2("⌾(𝕨⊸⊏)𝕩 because CHECK_VALID", w, x);
  #else
    #define SPARSE_INIT(GET)
    #define EQ(F)
    #define FREE_CHECK
  #endif
  
  u8 we = TI(w,elType);
  u8 xe = TI(x,elType);
  u8 re = TI(rep,elType);
  SLOWIF(!reusable(x) && xia>100 && wia<xia/50) SLOW2("⌾(𝕨⊸⊏)𝕩 because not reusable", w, x);
  if (elInt(we)) {
    w = toI32Any(w);
    i32* wp = i32any_ptr(w);
    SPARSE_INIT(wp[i])
    if (elNum(re) && elNum(xe)) {
      u8 me = xe>re?xe:re;
      bool reuse = reusable(x);
      if (me==el_i32) {
        I32Arr* xn = reuse? toI32Arr(REUSE(x)) : cpyI32Arr(x);
        i32* xp = i32arrv_ptr(xn);
        rep = toI32Any(rep); i32* rp = i32any_ptr(rep);
        for (usz i = 0; i < wia; i++) {
          i64 cw = wp[i]; if (RARE(cw<0)) cw+= (i64)xia; // we're free to assume w is valid
          i32 cr = rp[i];
          EQ(cr != xp[cw]);
          xp[cw] = cr;
        }
        decG(w); decG(rep); FREE_CHECK; return taga(xn);
      } else if (me==el_i8) {
        I8Arr* xn = reuse? toI8Arr(REUSE(x)) : cpyI8Arr(x);
        i8* xp = i8arrv_ptr(xn);
        rep = toI8Any(rep); i8* rp = i8any_ptr(rep);
        for (usz i = 0; i < wia; i++) {
          i64 cw = wp[i]; if (RARE(cw<0)) cw+= (i64)xia;
          i8 cr = rp[i];
          EQ(cr != xp[cw]);
          xp[cw] = cr;
        }
        decG(w); decG(rep); FREE_CHECK; return taga(xn);
      } else if (me==el_i16) {
        I16Arr* xn = reuse? toI16Arr(REUSE(x)) : cpyI16Arr(x);
        i16* xp = i16arrv_ptr(xn);
        rep = toI16Any(rep); i16* rp = i16any_ptr(rep);
        for (usz i = 0; i < wia; i++) {
          i64 cw = wp[i]; if (RARE(cw<0)) cw+= (i64)xia;
          i16 cr = rp[i];
          EQ(cr != xp[cw]);
          xp[cw] = cr;
        }
        decG(w); decG(rep); FREE_CHECK; return taga(xn);
      } else if (me==el_bit) {
        BitArr* xn = reuse? toBitArr(REUSE(x)) : cpyBitArr(x);
        u64* xp = bitarrv_ptr(xn);
        rep = taga(toBitArr(rep)); u64* rp = bitarr_ptr(rep);
        for (usz i = 0; i < wia; i++) {
          i64 cw = wp[i]; if (RARE(cw<0)) cw+= (i64)xia;
          bool cr = bitp_get(rp, i);
          EQ(cr != bitp_get(xp,cw));
          bitp_set(xp,cw,cr);
        }
        decG(w); decG(rep); FREE_CHECK; return taga(xn);
      } else if (me==el_f64) {
        F64Arr* xn = reuse? toF64Arr(REUSE(x)) : cpyF64Arr(x);
        f64* xp = f64arrv_ptr(xn);
        rep = toF64Any(rep); f64* rp = f64any_ptr(rep);
        for (usz i = 0; i < wia; i++) {
          i64 cw = wp[i]; if (RARE(cw<0)) cw+= (i64)xia;
          f64 cr = rp[i];
          EQ(cr != xp[cw]);
          xp[cw] = cr;
        }
        decG(w); decG(rep); FREE_CHECK; return taga(xn);
      } else UD;
    }
    if (reusable(x) && xe==re) {
      if (TY(x)==t_harr) {
        B* xp = harr_ptr(REUSE(x));
        SGet(rep)
        for (usz i = 0; i < wia; i++) {
          i64 cw = wp[i]; if (RARE(cw<0)) cw+= (i64)xia;
          B cr = Get(rep, i);
          EQ(!equal(cr,xp[cw]));
          dec(xp[cw]);
          xp[cw] = cr;
        }
        decG(w); decG(rep); FREE_CHECK;
        return x;
      }
    }
    MAKE_MUT(r, xia); mut_init(r, el_or(xe, re));
    MUTG_INIT(r);
    mut_copyG(r, 0, x, 0, xia);
    SGet(rep)
    for (usz i = 0; i < wia; i++) {
      i64 cw = wp[i]; if (RARE(cw<0)) cw+= (i64)xia;
      B cr = Get(rep, i);
      EQ(!equal(mut_getU(r, cw), cr));
      mut_rm(r, cw);
      mut_setG(r, cw, cr);
    }
    decG(w); decG(rep); FREE_CHECK;
    return mut_fcd(r, x);
  }
  MAKE_MUT(r, xia); mut_init(r, el_or(xe, re));
  MUTG_INIT(r);
  mut_copyG(r, 0, x, 0, xia);
  SGet(rep)
  SPARSE_INIT(o2i64G(GetU(w, i)))
  for (usz i = 0; i < wia; i++) {
    i64 cw = o2i64G(GetU(w, i)); if (RARE(cw<0)) cw+= (i64)xia;
    B cr = Get(rep, i);
    EQ(!equal(mut_getU(r, cw), cr));
    mut_rm(r, cw);
    mut_setG(r, cw, cr);
  }
  decG(w); decG(rep); FREE_CHECK;
  return mut_fcd(r, x);
  #undef SPARSE_INIT
  #undef EQ
  #undef FREE_CHECK
}
