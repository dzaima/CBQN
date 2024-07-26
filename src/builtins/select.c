// First Cell and Select (⊏)

// First Cell is just a slice

// Select - 𝕨 ⊏ 𝕩
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
//     COULD try to squeeze 𝕨 to i8 if small enough 𝕩
//   Boolean 𝕨: use bit_sel for blend or similar
//   Integer 𝕨 with Singeli: fused wrap, range-check, and gather
//     COULD try selecting from boolean with gather
//     COULD detect <Skylake where gather is slow
//   i32 𝕨: wrap, check, select one index at a time
//   i8 and i16 𝕨: separate range check in blocks to vectorize it
//     COULD wrap 𝕨 to a temp buffer
//     COULD copy 𝕩 to a buffer indexable directly by positive and negative indices
// Generic cell size 𝕩:
//   Computes a function that copies the necessary amount of bytes/bits
//   Specializes over i8/i16/i32 𝕨
// SHOULD implement nested 𝕨

// Under Select - F⌾(i⊸⊏) 𝕩
// Specialized for rank-1 numeric 𝕩
// SHOULD apply to characters as well
// No longer needs to range-check but indices can be negative
//   COULD convert negative indices before selection
// Must check collisions if CHECK_VALID; uses a byte set
//   Sparse initialization if 𝕨 is much smaller than 𝕩
//   COULD call Mark Firsts (∊) for very short 𝕨 to avoid allocation

// Select Cells - inds⊸⊏⎉1 x
// Squeeze indices if too wide for given x
// Boolean indices:
//   Short inds and short cells: Widen to i8
//   Otheriwse: bitsel call per cell
// 1, 2, 4 or 8-byte data elements & short cells & short index list:
//   Split indices to available native shuffle width (e.g. 2‿1⊸⊏˘ n‿5⥊i16 → 2‿3‿0‿1⊸⊏˘ n‿10⥊i8)
//   Repeat indices if using ≤0.5x of shuffle width (e.g. 0‿0‿2⊸⊏˘ n‿3⥊i8 → 0‿0‿2‿3‿3‿5⊸⊏˘ n‿6⥊i8)
//   SHOULD disregard actual cell width if index range is small
//   COULD merge indices ranges into wider element (e.g. 0‿1‿6‿7⊸⊏˘ n‿10⥊i16 → 0‿3⊸⊏˘ n‿5⥊i32)
//   COULD split into multiple indices blocks
// Long inds / long cells:
//   Direct call to select function per cell
//     COULD have a more direct call that avoids overflow checking & wrapping
//   COULD generate full list of indices via arith
// 1-element cells: use (≠inds)/⥊x after checking ∧´0=inds
// SHOULD use for atom⊸⊏⎉k, /⎉k, ⌽⎉k, ↑⎉k, ⍉⎉k, probably more

#include "../core.h"
#include "../utils/talloc.h"
#include "../utils/mut.h"
#include "../utils/calls.h"

#if SINGELI
  #define SINGELI_FILE select
  #include "../utils/includeSingeli.h"
  typedef bool (*SimdSelectFn)(void* w0, void* x0, void* r0, u64 wl, u64 xl);
  #define SIMD_SELECT(WE, XL) ({ AUTO we_=(WE); AUTO xl_=(XL); assert(we_>=el_i8 && we_<=el_i32 && xl_>=3 && xl_<=6); si_select_tab[4*(we_-el_i8)+xl_-3]; })
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
static CFn const cfs_0_4[] = {cf_0, cf_1, cf_2, cf_3, cf_4};
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

extern GLOBAL B rt_select;
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
  if (xr==0) thrM("⊏: 𝕩 cannot be a unit");
  if (isAtm(w)) {
    watom:;
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
  
  
  #if SINGELI_AVX2 || SINGELI_NEON
    #define CPUSEL(W, NEXT) /*assumes 3≤xl≤6*/ \
      if (RARE(!SIMD_SELECT(we, xl)(wp, xp, rp, wia, xn))) select_properError(w, x);
    
  #else
    #define CASE(S, E)  case S: for (usz i=i0; i<i1; i++) ((E*)rp)[i] = ((E*)xp+off)[ip[i]]; break
    #define CASEW(S, E) case S: for (usz i=0; i<wia; i++) ((E*)rp)[i] = ((E*)xp)[WRAP(wp[i], xn, thrF("⊏: Indexing out-of-bounds (%i∊𝕨, %s≡≠𝕩)", wp[i], xn))]; break
    #define CPUSEL(W, NEXT) /*assumes 3≤xl≤6*/ \
      if (sizeof(W) >= 4) {                           \
        switch(xl) { default:UD; CASEW(3,u8); CASEW(4,u16); CASEW(5,u32); CASEW(6,u64); } \
      } else {                                        \
        W* wt = NULL;                                 \
        for (usz bl=(1<<14)/sizeof(W), i0=0, i1=0; i0<wia; i0=i1) { \
          i1+=bl; if (i1>wia) i1=wia;                 \
          W min=wp[i0], max=min; for (usz i=i0+1; i<i1; i++) { W e=wp[i]; if (e>max) max=e; if (e<min) min=e; } \
          if (min<-(i64)xn) select_properError(w, x); \
          if (max>=(i64)xn) select_properError(w, x); \
          W* ip=wp; usz off=xn;                       \
          if (max>=0) { off=0; if (RARE(min<0)) {     \
            if (RARE(xn > (1ULL<<(sizeof(W)*8-1)))) { w=taga(NEXT(w)); mm_free((Value*)r); return C2(select, w, x); } \
            if (!wt) {wt=TALLOCP(W,i1-i0);} ip=wt-i0; \
            for (usz i=i0; i<i1; i++) { W e=wp[i]; ip[i]=e+((W)xn & (W)-(e<0)); } \
          } }                                         \
          switch(xl) { default:UD; CASE(3,u8); CASE(4,u16); CASE(5,u32); CASE(6,u64); } \
        }                                             \
        if (wt) TFREE(wt);                            \
      }
  #endif
    
  #if SINGELI_AVX2 || SINGELI_NEON
    bool bool_use_simd = we==el_i8 && xl==0 && xia<=128;
    
    #define BOOL_SPECIAL(W) \
      if (sizeof(W)==1 && bool_use_simd) { \
        if (RARE(!simd_select_bool128(wp, xp, rp, wia, xn))) select_properError(w, x); \
        goto setsh; \
      }
  #else
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
        usz n = WRAP(wp[i], xn, select_properError(w, x)); \
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
        if (xe!=el_B && (csz<<elwBitLog(xe)) < 128) {
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
    CFRes f = cf_get(1, csz<<elwBitLog(xe));
    
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
        if (!q_i64(cw)) { bad_cw: badw=o2fG(cw); goto bad1; }
        usz c = WRAP(o2i64G(cw), xn, goto bad_cw; );
        mut_copyG(rm, i*csz, x, csz*c, csz);
      }
      r = a(withFill(mut_fv(rm), xf));
    }
    goto setsh;
    
    bad1:;
    mut_pfree(rm, i*csz);
    if (!q_fi64(badw)) expI_f64(badw);
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



extern INIT_GLOBAL u8 reuseElType[t_COUNT];
B select_replace(u32 chr, B w, B x, B rep, usz wia, usz cam, usz csz) { // consumes all; (⥊rep)⌾(⥊w⊏cam‿csz⥊⊢) x; assumes csz>0, that w is a typed (elNum) list of valid indices (squeeze already attempted on el_f64), and that rep has the proper element count
  assert(csz > 0);
  #if CHECK_VALID
    TALLOC(bool, set, cam);
    bool sparse = wia < cam/64;
    if (!sparse) for (i64 i = 0; i < cam; i++) set[i] = false;
    #define SPARSE_INIT(WI) \
      if (sparse) for (usz i = 0; i < wia; i++) {                   \
        i64 cw = WI; if (RARE(cw<0)) cw+= (i64)cam; set[cw] = false; \
      }
    #define EQ(ITER,F) if (set[cw]) ITER if (F) thrF("𝔽⌾(a⊸%c): Incompatible result elements", chr); set[cw] = true;
    #define EQ1(F) EQ(,F)
    #define FREE_CHECK TFREE(set)
  #else
    #define SPARSE_INIT(GET)
    #define EQ(ITER,F)
    #define EQ1(F)
    #define FREE_CHECK
  #endif
  
  #define READ_W(N,I) i64 N = (i64)wp[I]; if (RARE(N<0)) N+= (i64)cam
  u8 we = TI(w,elType); assert(elNum(we));
  u8 xe = TI(x,elType);
  u8 re = el_or(xe, TI(rep,elType));
  Arr* ra;
  // w = taga(cpyF64Arr(w)); we = el_f64; // test the float path
  if (we==el_f64) {
    f64* wp = f64any_ptr(w);
    SPARSE_INIT((i64)wp[i])
    
    MAKE_MUT(r, cam*csz);
    mut_init_copy(r, x, re);
    NOGC_E;
    MUTG_INIT(r); SGet(rep)
    if (csz==1) {
      for (usz i = 0; i < wia; i++) {
        READ_W(cw, i);
        B cn = Get(rep, i);
        EQ1(!equal(mut_getU(r, cw), cn));
        mut_rm(r, cw);
        mut_setG(r, cw, cn);
      }
    } else {
      for (usz i = 0; i < wia; i++) {
        READ_W(cw, i);
        EQ(for (usz j = 0; j < csz; j++), !equal(mut_getU(r, cw*csz + j), Get(rep, i*csz + j)));
        for (usz j = 0; j < csz; j++) mut_rm(r, cw*csz + j);
        mut_copyG(r, cw*csz, rep, i*csz, csz);
      }
    }
    ra = mut_fp(r);
    goto dec_ret_ra;
  }
  assert(elInt(we));
  
  w = toI32Any(w);
  i32* wp = i32any_ptr(w);
  SPARSE_INIT(wp[i])
  bool reuse = reusable(x) && re==reuseElType[TY(x)];
  SLOWIF(!reuse && cam>100 && wia<cam/50) SLOW2("⌾(𝕨⊸⊏)𝕩 or ⌾(𝕨⊸⊑)𝕩 because not reusable", w, x);
  switch (re) { default: UD;
    case el_i8:  rep = toI8Any(rep);  ra = reuse? a(REUSE(x)) : cpyI8Arr(x);  goto do_u8;
    case el_c8:  rep = toC8Any(rep);  ra = reuse? a(REUSE(x)) : cpyC8Arr(x);  goto do_u8;
    case el_i16: rep = toI16Any(rep); ra = reuse? a(REUSE(x)) : cpyI16Arr(x); goto do_u16;
    case el_c16: rep = toC16Any(rep); ra = reuse? a(REUSE(x)) : cpyC16Arr(x); goto do_u16;
    case el_i32: rep = toI32Any(rep); ra = reuse? a(REUSE(x)) : cpyI32Arr(x); goto do_u32;
    case el_c32: rep = toC32Any(rep); ra = reuse? a(REUSE(x)) : cpyC32Arr(x); goto do_u32;
    case el_f64: rep = toF64Any(rep); ra = reuse? a(REUSE(x)) : cpyF64Arr(x); goto do_u64;
    case el_bit: {                    ra = reuse? a(REUSE(x)) : cpyBitArr(x);
      TyArr* na = toBitArr(rep); rep = taga(na);
      u64* np = bitarrv_ptr(na);
      u64* rp = (void*)((TyArr*)ra)->a;
      if (csz==1) {
        for (usz i = 0; i < wia; i++) {
          READ_W(cw, i);
          bool cn = bitp_get(np, i);
          EQ1(cn != bitp_get(rp, cw));
          bitp_set(rp, cw, cn);
        }
      } else {
        for (usz i = 0; i < wia; i++) {
          READ_W(cw, i);
          EQ(for (usz j = 0; j < csz; j++), bitp_get(np, i*csz + j) != bitp_get(rp, cw*csz + j));
          COPY_TO(rp, el_bit, cw*csz, rep, i*csz, csz);
        }
      }
      goto dec_ret_ra;
    }
    case el_B: {
      ra = reuse? a(REUSE(x)) : cpyHArr(x);
      B* rp = harrP_parts((HArr*)ra).a;
      SGet(rep)
      if (csz==1) {
        for (usz i = 0; i < wia; i++) {
          READ_W(cw, i);
          B cn = Get(rep, i);
          EQ1(!equal(cn,rp[cw]));
          dec(rp[cw]);
          rp[cw] = cn;
        }
      } else {
        for (usz i = 0; i < wia; i++) {
          READ_W(cw, i);
          EQ(for (usz j = 0; j < csz; j++), !equal(Get(rep, i*csz + j), rp[cw*csz + j]));
          for (usz j = 0; j < csz; j++) dec(rp[cw*csz + j]);
          COPY_TO(rp, el_B, cw*csz, rep, i*csz, csz);
        }
      }
      goto dec_ret_ra;
    }
  }
  
  #define IMPL(T) do {              \
    if (csz!=1) goto do_tycell;     \
    T* rp = (void*)((TyArr*)ra)->a; \
    T* np = tyany_ptr(rep);         \
    for (usz i = 0; i < wia; i++) { \
      READ_W(cw, i);                \
      T cn = np[i];                 \
      EQ1(cn != rp[cw]);            \
      rp[cw] = cn;                  \
    }                               \
    goto dec_ret_ra;                \
  } while(0)
  
  do_u8:  IMPL(u8);
  do_u16: IMPL(u16);
  do_u32: IMPL(u32);
  do_u64: IMPL(u64);
  #undef IMPL
  
  do_tycell:;
  u8 cwidth = csz * elWidth(re);
  u8* rp = (u8*) ((TyArr*)ra)->a;
  u8* np = tyany_ptr(rep);
  EqFnObj eq = EQFN_GET(re,re);
  for (usz i = 0; i < wia; i++) {
    READ_W(cw, i);
    EQ1(!EQFN_CALL(eq, rp + cw*cwidth, np + i*cwidth, csz));
    COPY_TO(rp, re, cw*csz, rep, i*csz, csz);
  }
  goto dec_ret_ra;
  
  
  
  dec_ret_ra:;
  decG(w); decG(rep);
  FREE_CHECK;
  return taga(ra);
  
  #undef SPARSE_INIT
  #undef EQ
  #undef EQ1
  #undef FREE_CHECK
}

static void* m_tyarrv_same(B* r, usz ia, B src) { // makes a new typed array with same element type as src, but new ia
  u8 se = TI(src,elType); assert(se!=el_bit && se!=el_B);
  return m_tyarrlv(r, arrTypeWidthLog(TY(src)), ia, arrNewType(TY(src)));
}

B slash_c2(B, B, B);
Arr* customizeShape(B x); // from cells.c

B select_cells_base(B inds, B x0, ux csz, ux cam);

#define CLZC(X) (64-(CLZ((u64)(X))))

#ifdef SELECT_ROWS_PRINTF
  #undef SELECT_ROWS_PRINTF
  #define SELECT_ROWS_PRINTF(...) printf(__VA_ARGS__)
#else
  #define SELECT_ROWS_PRINTF(...)
#endif

#define INDS_BUF_MAX 64 // only need 32 bytes for AVX2 & 16 for NEON, but have more for past-the-end pointers and writes
B select_rows_typed(B x, ux csz, ux cam, void* inds, ux indn, u8 ie, bool shouldBoundsCheck) { // ⥊ (indn↑inds As ie)⊸⊏˘ cam‿csz⥊z; xe cannot be el_bit or el_B, unless csz==1; ie must be ≤el_i8 if csz≤128
  assert(csz!=0 && cam!=0);
  assert(csz*cam == IA(x));
  assert(ie<=el_i32);
  
  u8 inds_buf[INDS_BUF_MAX]; (void)inds_buf;
  bool generic_allowed = true; // whether required interpretation of x hasn't changed from its real one
  if (csz==1) { // TODO maybe move to select_rows_B and require csz>=2 here?
    i64 bounds[2];
    if (!getRange_fns[ie](inds, bounds, indn) || bounds[0]<-1 || bounds[1]>0) goto generic; // could put under shouldBoundsCheck but ideally things setting that to false should handle size-1 cells themselves
    return C2(slash, m_f64(indn), taga(arr_shVec(customizeShape(x))));
  }
  
  u8 xe = TI(x,elType);
  assert(xe!=el_bit && xe!=el_B);
  assert(csz>=2);
  
  B r;
  u8 lb = arrTypeWidthLog(TY(x));
  ux xbump = csz<<lb;
  ux rbump = indn<<lb;
  ux ria = indn * cam;
  u8* xp = tyany_ptr(x);
  bool fast; (void) fast;
  i64 bounds[2];
  
  if (ie==el_bit) {
    if (csz>32 || indn>32 || indn>INDS_BUF_MAX) { // TODO properly tune
      u8* rp = m_tyarrv_same(&r, ria, x);
      for (ux i = 0; i < cam; i++) {
        bitselFns[lb](rp, inds, loadu_u64(xp), loadu_u64(xp + (1<<lb)), indn);
        xp+= xbump;
        rp+= rbump;
      }
      goto decG_ret;
    } else {
      assert(inds_buf != inds);
      bitselFns[0](inds_buf, inds, 0, 1, indn); // TODO use a widening copy
      inds = inds_buf;
      ie = el_i8;
      bounds[0] = 0;
      bounds[1] = 1; // might be an over-estimate, hopefully doesn't matter (and csz≥2 anyway)
      #if SINGELI
      fast = true;
      goto skip_bounds_check;
      #endif
    }
  }
  
  #if SINGELI
  assert(INDS_BUF_MAX_COPY == INDS_BUF_MAX);
  {
    fast = ie==el_i8;
    
    // TODO under shouldBoundsCheck (and probably rename that)
    if (!getRange_fns[ie](inds, bounds, indn)) goto generic;
    if (bounds[1] >= (i64)csz) goto generic;
    if (bounds[0] < 0) {
      if (bounds[0] < -(i64)csz) goto generic;
      if (csz < 128 && indn < INDS_BUF_MAX) {
        assert(ie == el_i8);
        si_wrap_inds[ie-el_i8](inds, inds_buf, indn, csz);
        inds = inds_buf;
      } else {
        fast = false;
      }
    }
    skip_bounds_check:;
    
    #if SINGELI_AVX2 || SINGELI_NEON
      if (fast) {
        generic_allowed = false;
        ux sh = select_rows_widen[lb](inds, inds_buf, indn); // TODO null element in table for guaranteed-zero
        if (sh!=0) {
          SELECT_ROWS_PRINTF("widening indices by factor of %d:\n", 1<<sh);
          SELECT_ROWS_PRINTF("  src: lb=%d, ie=%d, csz=%zu, indn=%zu\n", lb, ie, csz, indn);
          inds = inds_buf;
          lb-= sh;
          csz<<= sh;
          indn<<= sh;
          SELECT_ROWS_PRINTF("  dst: lb=%d, ie=%d, csz=%zu, indn=%zu\n", lb, ie, csz, indn);
        }
      }
    #endif
    
    u8* rp = m_tyarrv_same(&r, ria, x);
    
    ux slow_cam = cam;
    #if SINGELI_AVX2 || SINGELI_NEON
    ux lnt = CLZC(csz-1); // ceil-log2 of number of elements in table
    if (fast && lnt < select_rows_tab_h) {
      u8 max_indn = select_rows_max_indn[lb];
      if (indn > max_indn) goto no_fast;
      u8 min_lnt = select_rows_min_logcsz[lb];
      ux used_lnt;
      SELECT_ROWS_PRINTF("csz: %zu/%d; inds: %d/%d\n", csz, 1<<min_lnt, (int)indn, max_indn);
      
      ux indn_real = indn;
      ux rep;
      if (indn*2 <= max_indn) {
        assert(max_indn<=32); // otherwise inds_buf hard-coded size may need to change
        rep = simd_repeat_inds(inds, inds_buf, indn, csz);
        indn_real = rep*indn;
        SELECT_ROWS_PRINTF("rep: %zu; inds: %zu→%zu; csz: %zu→%zu - raw repeat\n", rep, indn, indn*rep, csz, csz*rep);
        PLAINLOOP while (rep*indn > max_indn) rep--; // simd_repeat_inds over-estimates
        SELECT_ROWS_PRINTF("rep: %zu; inds: %zu→%zu; csz: %zu→%zu - valid inds\n", rep, indn, indn*rep, csz, csz*rep);
        
        used_lnt = min_lnt;
        ux fine_csz = 1ULL<<(min_lnt+1); // TODO have a proper per-element-type LUT of "target LUT size"
        if (csz < fine_csz) {
          ux cap = fine_csz / csz;
          if (rep > cap) rep = cap;
        } else rep = 1;
        
        ux new_lnt = CLZC(csz*rep-1);
        if (new_lnt > used_lnt) used_lnt = new_lnt;
        
        SELECT_ROWS_PRINTF("rep: %zu; inds: %zu→%zu; csz: %zu→%zu - valid table\n", rep, indn, indn*rep, csz, csz*rep);
        inds = inds_buf;
        
      } else {
        rep = 1;
        used_lnt = lnt<min_lnt? min_lnt : lnt;
      }
      
      assert(indn*rep <= max_indn);
      AUTO fn = select_rows_tab[used_lnt*4 + lb];
      if (fn == null_fn) goto no_fast;
      ux done = fn(inds, xp, csz*rep, rp, indn*rep, rp + cam*rbump) * rep;
      ux left = cam - done;
      SELECT_ROWS_PRINTF("done_rows: %zu; left_rows: %zu; left_els: %zu; left_max: %zu\n", done, left, indn*left, indn_real);
      if (left) {
        xp+= done * xbump;
        rp+= done * rbump;
        if (left*csz <= 127) {
          assert(indn*left <= indn_real);
          bool ok = SIMD_SELECT(ie, lb+3)(inds, xp, rp, indn*left, I64_MAX); assert(ok);
        } else {
          slow_cam = left;
          goto no_fast;
        }
      }
      
      goto decG_ret;
    }
    no_fast:;
    #endif
    
    SimdSelectFn fn = SIMD_SELECT(ie, lb+3);
    for (ux i = 0; i < slow_cam; i++) {
      fn(inds, xp, rp, indn, csz);
      xp+= xbump;
      rp+= rbump;
    }
    goto decG_ret;
  }
  #endif
  
  generic:;
  assert(generic_allowed);
  B indo = taga(arr_shVec(m_tyslice(inds, a(emptyIVec()), ie, indn)));
  r = select_cells_base(indo, x, csz, cam);
  return r;
  
  decG_ret:;
  decG(x);
  return r;
}

B select_rows_B(B x, ux csz, ux cam, B inds) { // consumes inds,x; ⥊ inds⊸⊏˘ cam‿csz⥊x
  assert(csz*cam == IA(x));
  if (csz==0) goto generic;
  if (cam<=1) {
    if (cam==0) return taga(emptyArr(x, 1));
    return C2(select, inds, taga(arr_shVec(TI(x,slice)(x, 0, IA(x)))));
  }
  
  ux in = IA(inds);
  u8 ie = TI(inds,elType);
  if (csz<=2? ie!=el_bit : csz<128? ie>el_i8 : !elInt(ie)) {
    inds = num_squeeze(inds);
    ie = TI(inds,elType);
    if (!elInt(ie)) goto generic;
  }
  void* ip = tyany_ptr(inds);
  
  u8 xe = TI(x,elType);
  if ((xe!=el_bit && xe!=el_B) || csz==1) {
    B r = select_rows_typed(x, csz, cam, (u8*)ip, in, ie, 1);
    decG(inds);
    return r;
  }
  
  generic:;
  return select_cells_base(inds, x, csz, cam);
}

B select_ucw(B t, B o, B w, B x) {
  if (isAtm(x) || isAtm(w)) { def: return def_fn_ucw(t, o, w, x); }
  usz xia = IA(x);
  usz wia = IA(w);
  u8 we = TI(w,elType);
  if (!elInt(we) && IA(w)!=0) {
    w = num_squeezeChk(w); we = TI(w,elType);
    if (!elNum(we)) goto def;
  }
  B rep;
  if (isArr(o) && RNK(x)>0) {
    i64 buf[2];
    if (wia!=0 && (!getRange_fns[we](tyany_ptr(w), buf, wia) || buf[0]<-(i64)xia || buf[1]>=xia)) thrF("𝔽⌾(a⊸⊏)𝕩: Indexing out-of-bounds (%l∊a, %H≡≢𝕩)", buf[1]>=xia?buf[1]:buf[0], x);
    rep = incG(o);
  } else {
    rep = c1(o, C2(select, incG(w), incG(x)));
  }
  usz xr = RNK(x);
  usz wr = RNK(w);
  usz rr = RNK(rep);
  bool ok = !isAtm(rep) && xr+wr==rr+1 && eqShPart(SH(w),SH(rep),wr) && eqShPart(SH(x)+1,SH(rep)+wr,xr-1);
  if (!ok) thrF("𝔽⌾(a⊸⊏)𝕩: 𝔽 must return an array with the same shape as its input (%H ≡ shape of a, %2H ≡ shape of ⊏𝕩, %H ≡ shape of result of 𝔽)", w, xr-1, SH(x)+1, rep);
  usz csz = arr_csz(x);
  if (csz == 0) { decG(rep); decG(w); return x; }
  return select_replace(U'⊏', w, x, rep, wia, *SH(x), csz);
}
