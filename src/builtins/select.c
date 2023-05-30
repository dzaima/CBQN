// First Cell and Select (⊏)

// First Cell is just a slice

// Complications in Select mostly come from range checks and negative 𝕨
// Atom or enclosed atom 𝕨 and rank-1 𝕩: make new array
// Atom or enclosed atom 𝕨 and high-rank 𝕩: slice
// Empty 𝕨: no selection
// Float or generic 𝕨: attempt to squeeze, go generic cell size path if stays float
// High-rank 𝕩 & boolean 𝕨: either widens 𝕨 to i8, or goes generic cell path
//   SHOULD go a bit select path for small cells
//   SHOULD reshape for 1=≠𝕩
// Boolean 𝕩 (cell size = 1 bit):
//   𝕨 larger than 𝕩: convert 𝕩 to i8, select, convert back
//   Otherwise: select/shift bytes, reversed for fast writing
//     TRIED pext, doesn't seem faster (mask built with shifts anyway)
// 𝕩 with cell sizes of 1, 2, 4, or 8 bytes:
//   Small 𝕩 and i8 𝕨 with Singeli: use shuffles
//   Boolean 𝕨: use bit_sel for blend or similar
//   Integer 𝕨 with Singeli: fused wrap, range-check, and gather
//     COULD try selecting from boolean with gather
//     COULD detect <Skylake where gather is slow
//   i32 𝕨: wrap, check, select one index at a time
//   i8 and i16 𝕨: separate range check in blocks to auto-vectorize
// Generic cell size 𝕩:
//   Computes a function that copies the necessary amount of bytes/bits
//   Specializes over i8/i16/i32 𝕨
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
#include "../utils/calls.h"
#include "../builtins.h"

#if SINGELI_AVX2
  #define SINGELI_FILE select
  #include "../utils/includeSingeli.h"
#endif

typedef void (*CFn)(void* r, ux rs, void* x, ux xs, ux data);
typedef struct {
  CFn fn;
  ux data;
  ux mul;
} CFRes;

static void cf_0(void* r, ux rs, void* x, ux xs, ux d) { }
static void cf_1(void* r, ux rs, void* x, ux xs, ux d) { r=rs+(u8*)r; x=xs+(u8*)x; memcpy(r, x, 1); }
static void cf_2(void* r, ux rs, void* x, ux xs, ux d) { r=rs+(u8*)r; x=xs+(u8*)x; memcpy(r, x, 2); }
static void cf_3(void* r, ux rs, void* x, ux xs, ux d) { r=rs+(u8*)r; x=xs+(u8*)x; memcpy(r, x, 3); }
static void cf_4(void* r, ux rs, void* x, ux xs, ux d) { r=rs+(u8*)r; x=xs+(u8*)x; memcpy(r, x, 4); }
static CFn cfs_0_4[] = {cf_0, cf_1, cf_2, cf_3, cf_4};
static void cf_8(void* r, ux rs, void* x, ux xs, ux d) { r=rs+(u8*)r; x=xs+(u8*)x; memcpy(r, x, 8); }
static void cf_16(void* r, ux rs, void* x, ux xs, ux d) { r=rs+(u8*)r; x=xs+(u8*)x; memcpy(r, x, 16); }
static void cf_5_7  (void* r, ux rs, void* x, ux xs, ux d) { r=rs+(u8*)r; x=xs+(u8*)x; memcpy(r, x, 4);  memcpy(r+d, x+d, 4); }
static void cf_9_16 (void* r, ux rs, void* x, ux xs, ux d) { r=rs+(u8*)r; x=xs+(u8*)x; memcpy(r, x, 8);  memcpy(r+d, x+d, 8); }
static void cf_17_24(void* r, ux rs, void* x, ux xs, ux d) { r=rs+(u8*)r; x=xs+(u8*)x; memcpy(r, x, 16); memcpy(r+d, x+d, 8); }
static void cf_25_32(void* r, ux rs, void* x, ux xs, ux d) { r=rs+(u8*)r; x=xs+(u8*)x; memcpy(r, x, 24); memcpy(r+d, x+d, 8); }
static void cf_arb(void* r, ux rs, void* x, ux xs, ux d) { r=rs+(u8*)r; x=xs+(u8*)x; memcpy(r, x, d); }

static void cfb_1(void* r, ux rs, void* x, ux xs, ux d) { bitp_set(r, rs, bitp_get(x, xs)); }
static void cfb_arb(void* r, ux rs, void* x, ux xs, ux d) { bit_cpy(r, rs, x, xs, d); }

NOINLINE CFRes cf_get(usz count, usz cszBits) {
  if ((cszBits&7)==0) {
    ux cszBytes = cszBits/8;
    ux bytes = cszBytes * (ux)count;
    if (bytes<5)   return (CFRes){.mul=cszBytes, .fn = cfs_0_4[bytes]};
    if (bytes<8)   return (CFRes){.mul=cszBytes, .fn = cf_5_7,   .data=bytes-4};
    if (bytes==8)  return (CFRes){.mul=cszBytes, .fn = cf_8};
    if (bytes==16) return (CFRes){.mul=cszBytes, .fn = cf_16};
    if (bytes<=16) return (CFRes){.mul=cszBytes, .fn = cf_9_16,  .data=bytes-8};
    if (bytes<=24) return (CFRes){.mul=cszBytes, .fn = cf_17_24, .data=bytes-8};
    if (bytes<=32) return (CFRes){.mul=cszBytes, .fn = cf_25_32, .data=bytes-8};
    return                (CFRes){.mul=cszBytes, .fn = cf_arb,   .data=bytes};
  }
  ux bits = count*(ux)cszBits;
  if (bits==1) return (CFRes){.mul=cszBits, .fn = cfb_1};
  else         return (CFRes){.mul=cszBits, .fn = cfb_arb, .data=bits};
}

FORCE_INLINE void cf_call(CFRes f, void* r, ux rs, void* x, ux xs) {
  f.fn(r, rs, x, xs, f.data);
}

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

B select_c2(B t, B w, B x);
static NOINLINE NORETURN void select_properError(B w, B x) {
  select_c2(w, w, taga(cpyHArr(x)));
  fatal("select_properError");
}

B select_c2(B t, B w, B x) {
  if (isAtm(x)) thrM("⊏: 𝕩 cannot be an atom");
  ur xr = RNK(x);
  if (isAtm(w)) {
    watom:;
    if (xr==0) thrM("⊏: 𝕩 cannot be a unit");
    usz xn = *SH(x);
    usz wi = WRAP(o2i64(w), xn, thrF("⊏: Indexing out-of-bounds (%R∊𝕨, %s≡≠𝕩)", w, xn));
    if (xr==1) {
      B xf = getFillR(x);
      B xv = IGet(x, wi);
      B rb;
      if (isNum(xf) || isC32(xf)) {
        rb = m_unit(xv);
      } else if (noFill(xf)) {
        rb = m_hunit(xv);
      } else {
        Arr* r = m_fillarrp(1);
        arr_shAtm(r);
        fillarr_ptr(r)[0] = xv;
        fillarr_setFill(r, xf);
        NOGC_E;
        rb = taga(r);
      }
      decG(x);
      return rb;
    }
    usz csz = arr_csz(x);
    Arr* r = TI(x,slice)(incG(x), wi*csz, csz);
    usz* sh = arr_shAlloc(r, xr-1);
    if (sh) shcpy(sh, SH(x)+1, xr-1);
    decG(x);
    return taga(r);
  }
  
  usz wia = IA(w);
  Arr* r;
  ur wr = RNK(w);
  if (wr==0) {
    B w0 = IGetU(w, 0);
    if (isAtm(w0)) {
      decG(w);
      w = inc(w0);
      goto watom;
    }
  }
  i32 rr = xr+wr-1;
  if (wia==0) {
    emptyRes:
    if (0 == *SH(x) && wr==1) {
      decG(w);
      return x;
    }
    r = emptyArr(x, rr);
    if (rr<=1) goto dec_ret;
    goto setsh;
  }
  
  B xf = getFillR(x);
  usz xn = *SH(x);
  if (xn==0) goto base;
  usz csz = arr_csz(x);
  u8 xl = cellWidthLog(x);
  usz ria = wia * csz;
  
  usz xia = IA(x);
  u8 xe = TI(x,elType);
  u8 we = TI(w,elType);
  
  
  #if SINGELI_AVX2
    #define CPUSEL(W, NEXT) /*assumes 3≤xl≤6*/ \
      if (RARE(!avx2_select_tab[4*(we-el_i8)+xl-3](wp, xp, rp, wia, xn))) select_properError(w, x);
    bool bool_use_simd = we==el_i8 && xl==0 && xia<=128;
    #define BOOL_SPECIAL(W) \
      if (sizeof(W)==1 && bool_use_simd) { \
        if (RARE(!avx2_select_bool128(wp, xp, rp, wia, xn))) select_properError(w, x); \
        goto setsh; \
      }
  #else
    #define CASE(S, E)  case S: for (usz i=i0; i<i1; i++) ((E*)rp)[i] = ((E*)xp+off)[ip[i]]; break
    #define CASEW(S, E) case S: for (usz i=0; i<wia; i++) ((E*)rp)[i] = ((E*)xp)[WRAP(wp[i], xn, thrF("⊏: Indexing out-of-bounds (%i∊𝕨, %s≡≠𝕩)", wp[i], xn))]; break
    #define CPUSEL(W, NEXT) /*assumes 3≤xl≤6*/ \
      if (sizeof(W) >= 4) {                          \
        switch(xl) { default:UD; CASEW(3,u8); CASEW(4,u16); CASEW(5,u32); CASEW(6,u64); } \
      } else {                                       \
        W* wt = NULL;                                \
        for (usz bl=(1<<14)/sizeof(W), i0=0, i1=0; i0<wia; i0=i1) { \
          i1+=bl; if (i1>wia) i1=wia;                \
          W min=wp[i0], max=min; for (usz i=i0+1; i<i1; i++) { W e=wp[i]; if (e>max) max=e; if (e<min) min=e; } \
          if (min<-(i64)xn) thrF("⊏: Indexing out-of-bounds (%i∊𝕨, %s≡≠𝕩)", min, xn); \
          if (max>=(i64)xn) thrF("⊏: Indexing out-of-bounds (%i∊𝕨, %s≡≠𝕩)", max, xn); \
          W* ip=wp; usz off=xn;                      \
          if (max>=0) { off=0; if (RARE(min<0)) {    \
            if (RARE(xn > (1ULL<<(sizeof(W)*8-1)))) { w=taga(NEXT(w)); mm_free((Value*)r); return C2(select, w, x); } \
            if (!wt) {wt=TALLOCP(W,i1-i0);} ip=wt-i0;\
            for (usz i=i0; i<i1; i++) { W e=wp[i]; ip[i]=e+((W)xn & (W)-(e<0)); } \
          } }                                        \
          switch(xl) { default:UD; CASE(3,u8); CASE(4,u16); CASE(5,u32); CASE(6,u64); } \
        }                                            \
        if (wt) TFREE(wt);                           \
      }
    bool bool_use_simd = 0;
    #define BOOL_SPECIAL(W)
  #endif
  
  if (!bool_use_simd && xe==el_bit && (csz&7)!=0 && (xl==0? wia>=256 : wia>=4) && csz<128 && TI(w,arrD1)) {
    // test widen/narrow on bitarr input
    // ShArr* sh = RNK(x)==1? NULL : ptr_inc(shObj(x));
    // B t = C2(select, w, widenBitArr(x, 1));
    // B r = narrowWidenedBitArr(t, wr, xr-1, sh==NULL? &xn : sh->a+1);
    // if (sh!=NULL) ptr_dec(sh);
    // return r;
    if (csz==1) {
      if (wia/4>=xia) return taga(cpyBitArr(C2(select, w, taga(cpyI8Arr(x)))));
    } else if (csz>64? wia/2>=xn : wia>=xn/2) {
      ShArr* sh = ptr_inc(shObj(x));
      B t = C2(select, w, widenBitArr(x, 1));
      B r = narrowWidenedBitArr(t, wr, xr-1, sh->a+1);
      ptr_dec(sh);
      return r;
    }
  }
  
  
  #define TYPE(W, NEXT) { W* wp = W##any_ptr(w);      \
    if (xl==0) { u64* xp=bitarr_ptr(x);               \
      u64* rp; r = m_bitarrp(&rp, ria);               \
      BOOL_SPECIAL(W)                                 \
      u64 b=0;                                        \
      for (usz i = wia; ; ) {                         \
        i--;                                          \
        usz n = WRAP(wp[i], xn, thrF("⊏: Indexing out-of-bounds (%i∊𝕨, %s≡≠𝕩)", wp[i], xn)); \
        b = 2*b + ((((u8*)xp)[n/8] >> (n%8)) & 1);    \
        if (i%64 == 0) { rp[i/64]=b; if (!i) break; } \
      }                                               \
      goto setsh;                                     \
    }                                                 \
    if (xe!=el_B) {                                   \
      if (xl<3 || xl==7) goto generic_l;              \
      void* rp = m_tyarrlp(&r, xl-3, ria, arrNewType(TY(x))); \
      void* xp = tyany_ptr(x);                        \
      CPUSEL(W, NEXT)                                 \
      goto setsh;                                     \
    }                                                 \
    if (xl!=6) goto generic_l;                        \
    M_HARR(ra, wia); B* xp = arr_bptr(x);             \
    SLOWIF(xp==NULL) SLOW2("𝕨⊏𝕩", w, x);              \
    if (xp!=NULL) { for (usz i=0; i<wia; i++) HARR_ADD(ra, i, inc(xp[WRAP(wp[i], xia, thrF("⊏: Indexing out-of-bounds (%i∊𝕨, %s≡≠𝕩)", wp[i], xn))])); } \
    else { SGet(x); for (usz i=0; i<wia; i++) HARR_ADD(ra, i, Get(x, WRAP(wp[i], xia, thrF("⊏: Indexing out-of-bounds (%i∊𝕨, %s≡≠𝕩)", wp[i], xn)) )); } \
    r = a(withFill(HARR_FV(ra), xf)); goto setsh;     \
  }
  
  retry:
  switch (we) { default: UD;
    case el_bit: {
      if (xr!=1) {
        if (xe!=el_B && (csz<<elWidthLogBits(xe)) < 128) {
          dec(xf);
          return C2(select, taga(cpyI8Arr(w)), x);
        } else {
          goto generic_l;
        }
      }
      SGetU(x)
      B x0 = GetU(x, 0);
      B x1;
      if (xia<2) {
        u64* wp=bitarr_ptr(w);
        usz i; for (i=0; i<wia/64; i++) if (wp[i]) break;
        if (i<wia/64 || bitp_l0(wp,wia)!=0) thrF("⊏: Indexing out-of-bounds (1∊𝕨, %s≡≠𝕩)", xn);
        x1 = x0;
      } else {
        x1 = GetU(x,1);
      }
      B r = bit_sel(w, x0, x1);
      decG(x);
      return withFill(r, xf);
    }
    case el_i8:  TYPE(i8, cpyI16Arr)
    case el_i16: TYPE(i16,cpyI32Arr)
    case el_i32: TYPE(i32,cpyF64Arr)
    case el_f64: {
      if (FL_HAS(w, fl_squoze)) goto generic_l; // either has non-integers (i.e. error, thus don't care about speed) or very large (i.e. will hit memory bandwidth anyway)
      // else fallthrough - want to do integer 𝕨 if possible
    }
    case el_B: case el_c8: case el_c16: case el_c32: {
      w = num_squeezeChk(w);
      we = TI(w,elType);
      if (elNum(we)) goto retry;
      goto base;
    }
  }
  #undef CASE
  #undef CASEW
  
  base:;
  dec(xf);
  return c2rt(select, w, x);
  
  generic_l: {
    if (xia==0) goto emptyRes;
    SLOW2("𝕨⊏𝕩", w, x);
    SGetU(w)
    usz csz = arr_csz(x);
    CFRes f = cf_get(1, csz<<elWidthLogBits(xe));
    
    MAKE_MUT_INIT(rm, ria, xe);
    usz i = 0; f64 badw;
    if (xe<el_B && elInt(we)) {
      void* wp = tyany_ptr(w);
      void* xp = tyany_ptr(x);
      ux ri = 0;
      switch(we) { default: UD;
        case el_bit: for (; i<wia; i++) { i8  c =bitp_get(wp,i); if (c>=xn)          { badw=c;  goto bad1; }   cf_call(f, rm->a, ri, xp, c*f.mul); ri+= f.mul; } // TODO something better
        case el_i8:  for (; i<wia; i++) { i8  c0=((i8* )wp)[i]; usz c = WRAP(c0, xn, { badw=c0; goto bad1; }); cf_call(f, rm->a, ri, xp, c*f.mul); ri+= f.mul; }
        case el_i16: for (; i<wia; i++) { i16 c0=((i16*)wp)[i]; usz c = WRAP(c0, xn, { badw=c0; goto bad1; }); cf_call(f, rm->a, ri, xp, c*f.mul); ri+= f.mul; }
        case el_i32: for (; i<wia; i++) { i32 c0=((i32*)wp)[i]; usz c = WRAP(c0, xn, { badw=c0; goto bad1; }); cf_call(f, rm->a, ri, xp, c*f.mul); ri+= f.mul; }
      }
      
      assert(!isVal(xf));
      r = a(mut_fv(rm));
    } else {
      MUTG_INIT(rm);
      for (; i < wia; i++) {
        B cw = GetU(w, i); // assumed number from previous squeeze
        usz c = WRAP(o2i64(cw), xn, { badw=o2fG(cw); goto bad1; });
        mut_copyG(rm, i*csz, x, csz*c, csz);
      }
      r = a(withFill(mut_fv(rm), xf));
    }
    goto setsh;
    
    bad1:;
    mut_pfree(rm, i*csz);
    thrF("⊏: Indexing out-of-bounds (%f∊𝕨, %s≡≠𝕩)", badw, xn);
  }
  
  
  
  setsh:
  if (rr>1) {
    if (rr > UR_MAX) thrF("⊏: Result rank too large (%i≡=𝕨, %i≡=𝕩)", wr, xr);
    ShArr* sh = m_shArr(rr);
    shcpy(sh->a, SH(w), wr);
    shcpy(sh->a+wr, SH(x)+1, xr-1);
    arr_shSetUG(r, rr, sh);
  } else {
    arr_shVec(r);
  }
  
  dec_ret:;
  decG(w); decG(x); return taga(r);
}



B select_replace(u32 chr, B w, B x, B rep, usz wia, usz xia) { // rep⌾(w⊏⥊) x
  #if CHECK_VALID
    TALLOC(bool, set, xia);
    bool sparse = wia < xia/64;
    if (!sparse) for (i64 i = 0; i < xia; i++) set[i] = false;
    #define SPARSE_INIT(WI) \
      if (sparse) for (usz i = 0; i < wia; i++) {                    \
        i64 cw = WI; if (RARE(cw<0)) cw+= (i64)xia; set[cw] = false; \
      }
    #define EQ(F) if (set[cw] && (F)) thrF("𝔽⌾(a⊸%c): Incompatible result elements", chr); set[cw] = true;
    #define FREE_CHECK TFREE(set)
    SLOWIF(xia>100 && wia<xia/20) SLOW2("⌾(𝕨⊸⊏)𝕩 or ⌾(𝕨⊸⊑)𝕩 because CHECK_VALID", w, x);
  #else
    #define SPARSE_INIT(GET)
    #define EQ(F)
    #define FREE_CHECK
  #endif
  
  u8 we = TI(w,elType);
  u8 xe = TI(x,elType);
  u8 re = TI(rep,elType);
  SLOWIF(!reusable(x) && xia>100 && wia<xia/50) SLOW2("⌾(𝕨⊸⊏)𝕩 or ⌾(𝕨⊸⊑)𝕩 because not reusable", w, x);
  if (elInt(we)) {
    w = toI32Any(w);
    i32* wp = i32any_ptr(w);
    SPARSE_INIT(wp[i])
    if (elNum(re) && elNum(xe)) {
      u8 me = xe>re?xe:re;
      bool reuse = reusable(x);
      if (me==el_i32) {
        I32Arr* xn = reuse? toI32Arr(REUSE(x)) : (I32Arr*)cpyI32Arr(x);
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
        I8Arr* xn = reuse? toI8Arr(REUSE(x)) : (I8Arr*)cpyI8Arr(x);
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
        I16Arr* xn = reuse? toI16Arr(REUSE(x)) : (I16Arr*)cpyI16Arr(x);
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
        BitArr* xn = reuse? toBitArr(REUSE(x)) : (BitArr*)cpyBitArr(x);
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
        F64Arr* xn = reuse? toF64Arr(REUSE(x)) : (F64Arr*)cpyF64Arr(x);
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
    MAKE_MUT_INIT(r, xia, el_or(xe, re));
    MUTG_INIT(r);
    mut_copyG(r, 0, x, 0, xia);
    NOGC_E;
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
  MAKE_MUT_INIT(r, xia, el_or(xe, re));
  MUTG_INIT(r);
  mut_copyG(r, 0, x, 0, xia);
  NOGC_E;
  SGet(rep) SGetU(w)
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

B select_ucw(B t, B o, B w, B x) {
  if (isAtm(x) || RNK(x)!=1 || isAtm(w)) { def: return def_fn_ucw(t, o, w, x); }
  usz xia = IA(x);
  usz wia = IA(w);
  u8 we = TI(w,elType);
  if (!elInt(we) && IA(w)!=0) {
    w = num_squeezeChk(w); we = TI(w,elType);
    if (!elNum(we)) goto def;
  }
  B rep;
  if (isArr(o)) {
    i64 buf[2];
    if (wia!=0 && (!getRange_fns[we](tyany_ptr(w), buf, wia) || buf[0]<-(i64)xia || buf[1]>=xia)) thrF("𝔽⌾(a⊸⊏)𝕩: Indexing out-of-bounds (%l∊a, %H≡≢𝕩)", buf[1]>=xia?buf[1]:buf[0], x);
    rep = incG(o);
  } else {
    rep = c1(o, C2(select, incG(w), incG(x)));
  }
  if (isAtm(rep) || !eqShape(w, rep)) thrF("𝔽⌾(a⊸⊏)𝕩: 𝔽 must return an array with the same shape as its input (expected %H, got %H)", w, rep);
  return select_replace(U'⊏', w, x, rep, wia, xia);
}
