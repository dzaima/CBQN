// Indices and Replicate (/)
// In the notes 𝕨 might indicate 𝕩 for Indices too

// Boolean 𝕨 (Where/Compress) general case based on result type width
// Size 1: compress 64-bit units, possibly SIMD
//   pext if BMI2 is present
//   Pairwise combination, SIMD if AVX2
//   SIMD shift-by-offset if there's CLMUL but no AVX2
//     SHOULD use with polynomial multiply in NEON
//   COULD return boolean result from Where
// Size 8, 16, 32, 64: mostly table-based
//   Where: direct table lookup, widening for 16 and 32 if available
//   Compress: table lookup plus shuffle
//     AVX2 permutevar8x32 for 32 and 64 if available
//     Sparse method using table-based Where fills in if no shuffle
//   SHOULD implement for NEON
//   AVX-512: compress instruction, separate store not compressstore
// Size 32, 64: 16-bit indices from where_block_u16
// Other sizes: always used grouped code
// Adaptivity based on 𝕨 statistics
//   None for 8-bit Where, too short
//   COULD try per-block adaptivity for 16-bit Compress
//   Sparse if +´𝕨 is small, branchless unless it's very small
//     Chosen per-argument for 1, 8, 16 and per-block for larger
//     Careful when benchmarking, branch predictor has a long memory
//   Grouped if +´»⊸≠𝕨 is small, always branching
//     Chosen per-argument with a threshold that gives up early
//     SHOULD implement grouped Where

// Arbitrary 𝕨 is slower, squeezes if (+´<≠)𝕨 to avoid this
// Dense cases (large +´𝕨) use obvious loop, expand 𝕨 to i32
//   Boolean Replicate overwrites to avoid trailing boundary
// Sparse Indices uses ⌈` with Singeli and +` otherwise, i32 output only
//   COULD specialize on result type
// Sparse Replicate
//   ≠` for booleans, +` for CPU types
//   TRIED ≠` for CPU types; no better, and clmul would be worse
// COULD consolidate refcount updates for nested 𝕩

// Replicate by constant
// Boolean uses specialized small-𝕨 methods, ≠`, or overwriting
//   𝕨≤64: Singeli generic and SIMD methods
//     𝕨=2,4,8: Various shift, shuffle, and zip-based loops
//     odd 𝕨: Modular permutation
//       COULD use pdep or similar to avoid overhead on small results
//     Otherwise, factor into power of 2 times odd
//       COULD fuse 2×odd, since 2/odd/ has a larger intermediate
//   𝕨≤256, AVX2: Modular permutation with shift-based masks
// Other typed 𝕩 uses +`, or lots of Singeli
//   Fixed shuffles, factorization, partial shuffles, self-overlapping
// Otherwise, cell-by-cell copying
//   SHOULD better handle small odd cell widths
//   COULD do large copies for large 𝕨

// Indices inverse (/⁼), a lot like Group
// Always gives a squeezed result for integer 𝕩
// Boolean 𝕩: just count 1s
// Without SINGELI_SIMD, just write to large-type table and squeeze
//   COULD do many /⁼ optimizations without SIMD
// Scan for strictly ascending 𝕩
//   SHOULD vectorize, maybe with find-compare
// Sorted indices: i8 counter and index+count overflow
//   Work in blocks of 128, try galloping if one has start equal to end
//   Otherwise use runs-adaptive count (not sums, they're rarely better)
// Long i8 and i16 𝕩: allocate full-range to skip initial range check
// If (≠÷⌈´)𝕩 is small, detect u1 and i8 result with a sparse u8 table
// General-case i8 to i32 𝕩: dedicated SIMD functions
//   i8 and i16 𝕩: i16 counter and index overflow (implicit count 1<<15)
//     Flush to overflow every 1<<15 writes
//     Get range in 2KB blocks to enable count by compare and sum
//   Run detection used partly to mitigate write stalls from repeats
//   COULD also alternate writes to multiple tables if 𝕩 is long enough

#include "../core.h"
#include "../utils/mut.h"
#include "../utils/calls.h"
#include "../utils/talloc.h"
#include "../builtins.h"

#ifdef __BMI2__
  #if !SLOW_PDEP
    #define FAST_PDEP 1
  #endif
  #include <immintrin.h>
#endif

#if !USE_VALGRIND
  #define rand_popc64(X) POPC(X)
#endif

#if SINGELI
  extern void (*const si_scan_pluswrap_u8)(uint8_t* v0,uint8_t* v1,uint64_t v2,uint8_t v3);
  extern void (*const si_scan_pluswrap_u16)(uint16_t* v0,uint16_t* v1,uint64_t v2,uint16_t v3);
  extern void (*const si_scan_pluswrap_u32)(uint32_t* v0,uint32_t* v1,uint64_t v2,uint32_t v3);
  extern void (*const si_scan_max_i32)(int32_t* v0,int32_t* v1,uint64_t v2);
  #define SINGELI_FILE slash
  #include "../utils/includeSingeli.h"
  extern uint64_t* const si_spaced_masks;
  #define get_spaced_mask(i) si_spaced_masks[i-1]
  #define SINGELI_FILE replicate
  #include "../utils/includeSingeli.h"
#endif

#if SINGELI_SIMD
  #define SINGELI_FILE count
  #include "../utils/includeSingeli.h"
#endif

// Dense Where, still significantly worse than SIMD
// Assumes modifiable DST
#define WHERE_DENSE(SRC, DST, LEN, OFF) do { \
    for (usz ii=0; ii<(LEN+7)/8; ii++) {                            \
      u8 v = ((u8*)SRC)[ii];                                        \
      for (usz k=0; k<8; k++) { *DST=OFF+8*ii+k; DST+=v&1; v>>=1; } \
    }                                                               \
  } while (0)

// Sparse Where with branching
#define WHERE_SPARSE(X,R,S,I0,COND) do { \
    for (usz ii=I0, j=0; j<S; ii++)                                    \
      for (u64 v=(X)[ii]; COND(v!=0); v&=v-1) R[j++] = ii*64 + CTZ(v); \
  } while (0)

// Branchless sparse Where (bsp)
// Works on a buffer of 1<<11 32-bit values
static const usz bsp_max  = (1<<11) - 64;
static const u32 bsp_top  = 1<<24;
static const u32 bsp_mask = bsp_top - 1;
static usz bsp_fill(u64* src, u32* buf, usz len) {
  assert(len < 1<<11);
  usz j = 0;
  for (usz i=0; i<(len+63)/64; i++) {
    u64 u=src[i]; u32 p;
    p=u&bsp_mask; buf[j]+=(2*bsp_top)|p; j+=POPC(p); u>>=24;
    p=u&bsp_mask; buf[j]+=(3*bsp_top)|p; j+=POPC(p); u>>=24;
    p=u         ; buf[j]+=(3*bsp_top)|p; j+=POPC(p);
  }
  return j;
}
#define BSP_WRITE(BUF, DST, SUM, OFF, CLEAR) \
  u64 t=((u64)OFF<<21)-2*bsp_top;     \
  for (usz j=0; j<SUM; j++) {         \
    t += BUF[j]; CLEAR                \
    DST[j] = 8*(t>>24) + CTZ((u32)t); \
    t &= t-1;                         \
  }
static void bsp_block_u32(u64* src, u32* dst, usz len, usz sum, usz off) {
  for (usz j=0; j<sum; j++) dst[j]=0;
  bsp_fill(src, dst, len);
  BSP_WRITE(dst, dst, sum, off,);
}
static void bsp_u16(u64* src, u16* dst, usz len, usz sum) {
  usz b = bsp_max;
  usz bufsize = b<sum? b : sum;
  TALLOC(u32, buf, bufsize+1);
  for (usz j=0; j<bufsize; j++) buf[j]=0;
  for (usz i=0; i<len; i+=b) {
    if (b > len-i) b = len-i;
    usz bs = bsp_fill(src+i/64, buf, b);
    BSP_WRITE(buf, dst, bs, i, buf[j]=0;); buf[bs]=0;
    dst+= bs;
  }
  TFREE(buf);
}

static void where_block_u16(u64* src, u16* dst, usz len, usz sum) {
  assert(len <= bsp_max);
  #if SINGELI
  if (sum >= len/si_thresh_1slash16) si_1slash16(src, (i16*)dst, len, sum);
  #else
  if (sum >= len/4+len/8) WHERE_DENSE(src, dst, len, 0);
  #endif
  else if (sum >= len/128) {
    u32* buf = (u32*)dst; assert(sum*2 <= len);
    for (usz j=0; j<sum; j++) buf[j]=0;
    bsp_fill(src, buf, len);
    BSP_WRITE(buf, dst, sum, 0,);
  } else {
    WHERE_SPARSE(src, dst, sum, 0, RARE);
  }
}

static B compress_grouped(u64* wp, B x, usz wia, usz wsum, u8 xt) { // expected to return a refcount-1 array (at least when rank≥1), expects argument with non-empty cells
  B r;
  usz csz = arr_csz(x);
  u8 xl = arrTypeBitsLog(TY(x));
  #define COMPRESS_GROUP(CPY) \
    u64 ri = 0;                                                                  \
    u64 wv = wp[0]; usz i = 0, wn = (wia-1)/64+1;                                \
    for (u64 e=wsum*width; ri < e; ) {                                           \
      while (wv==      0) { wv=wp[++i];        } usz i0=64*i+CTZ( wv); wv|=wv-1; \
      while (wv==-(u64)1) { wv=++i<wn?wp[i]:0; } usz i1=64*i+CTZ(~wv); wv&=wv+1; \
      u64 l = (i1-i0) * width;                                                   \
      CPY(rp, ri, xp, i0*width, l); ri += l;                                     \
    }
  if (xl>0 || csz%8==0) { // Full bytes
    u64 width = xl==0 ? csz/8 : csz << (xl-3);
    u8* xp; u8* rp;
    bool is_B = TI(x,elType) == el_B; HArr_p rh;
    if (!is_B) {
      xp = tyany_ptr(x);
      rp = m_tyarrv(&r,width,wsum,xt);
    } else {
      ONLY_GCC(r = m_f64(0);)
      usz ria = wsum*csz;
      
      xp = (u8*)arr_bptr(x);
      if (xp == NULL) {
        SLOW2("𝕨/𝕩", taga(RFLD(wp,TyArr,a)), x);
        M_HARR(rp, ria) SGet(x)
        for (usz i = 0; i < wia; i++) if (bitp_get(wp,i)) {
          for (usz j = 0; j < csz; j++) HARR_ADDA(rp, Get(x,i*csz+j));
        }
        r = withFill(HARR_FV(rp), getFillR(x));
        goto b_res;
      }
      
      rh = m_harrUv(ria);
      rp = (u8*)rh.a;
    }
    COMPRESS_GROUP(MEM_CPY)
    if (is_B) {
      for (usz i = 0; i < wsum*csz; i++) inc(((B*)rp)[i]);
      NOGC_E;
      r = withFill(rh.b, getFillR(x));
      b_res:;
      a(r)->ia = wsum; // Shape-setting code at end of compress expects this
    }
  } else { // Bits
    usz width = csz;
    u64* xp = tyany_ptr(x);
    u64* rp; r = m_bitarrv(&rp,wsum*width); a(r)->ia = wsum;
    COMPRESS_GROUP(bit_cpyN)
  }
  return r;
}

static B where(B x, usz xia, u64 s) {
  B r;
  u64* xp = bitany_ptr(x);
  usz q=xia%64; if (q) xp[xia/64] &= ((u64)1<<q) - 1;
  if (xia <= 128) {
    #if SINGELI
    i8* rp = m_tyarrv(&r, 1, s, t_i8arr);
    si_1slash8(xp, rp, xia, s);
    #else
    i8* rp; r=m_i8arrv(&rp,s); WHERE_SPARSE(xp,rp,s,0,);
    #endif
  } else if (xia <= 32768) {
    #if SINGELI
    if (s >= xia/si_thresh_1slash16) {
      i16* rp = m_tyarrv(&r, 2, s, t_i16arr);
      si_1slash16(xp, rp, xia, s);
    }
    #else
    if (s >= xia/4+xia/8) {
      i16* rp = m_tyarrvO(&r, 2, s, t_i16arr, 2);
      WHERE_DENSE(xp, rp, xia, 0);
      FINISH_OVERALLOC_A(r, s*2, 2);
    }
    #endif
    else {
      i16* rp; r=m_i16arrv(&rp,s);
      if (s >= xia/128) {
        bsp_u16(xp, (u16*)rp, xia, s);
      } else {
        WHERE_SPARSE(xp, rp, s, 0, RARE);
      }
    }
  } else if (xia <= (usz)I32_MAX+1) {
    i32* rp = m_tyarrvO(&r, 4, s, t_i32arr, 4);
    usz b = bsp_max; TALLOC(i16, buf, b);
    i32* rq = rp;
    for (usz i=0; i<xia; i+=b) {
      usz bs;
      if (b>xia-i) {
        b = xia-i;
        bs = s-(rq-rp);
      } else {
        bs = bit_sum(xp,b);
      }
      #if SINGELI
      if (bs >= b/si_thresh_1slash32) {
        si_1slash32(xp, i, rq, b, bs);
      }
      #else
      if (bs >= b/2) {
        i32* rs=rq; WHERE_DENSE(xp, rs, b, i);
      }
      #endif
      else if (bs >= b/256) {
        bsp_block_u32(xp, (u32*)rq, b, bs, i);
      } else {
        WHERE_SPARSE(xp-i/64, rq, bs, i/64, RARE);
      }
      rq+= bs;
      xp+= b/64;
    }
    FINISH_OVERALLOC_A(r, s*4, 4);
    TFREE(buf);
  } else {
    f64* rp; r = m_f64arrv(&rp, s);
    usz b = bsp_max; TALLOC(u16, buf, b);
    f64* rp0 = rp;
    for (usz i=0; i<xia; i+=b) {
      usz bs;
      if (b>xia-i) { b=xia-i; bs=s-(rp-rp0); } else { bs=bit_sum(xp,b); }
      where_block_u16(xp, buf, b, bs);
      vfor (usz j=0; j<bs; j++) rp[j] = i+buf[j];
      rp+= bs;
      xp+= b/64;
    }
    TFREE(buf);
  }
  return r;
}

// Is the number of values switches in w at most max?
static NOINLINE bool groups_lt(u64* wp, usz len, usz max) {
  usz r = 0;
  u64 prev = 0;
  usz l = (len-1)/64+1;  // assume trailing bits are zeroed out
  usz b = 1<<8;
  for (usz i = 0; i < l; ) {
    for (usz e = l-i<b?l:i+b; i < e; i++) {
      u64 v=wp[i]; r+= POPC(v ^ (v<<1 | prev)); prev = v>>63;
    }
    if (r > max) return 0;
  }
  return 1;
}

static NOINLINE B reshapeToEmpty(B x, usz leading) { // doesn't consume; changes leading axis to the given one, assumes result is empty
  u8 xe = TI(x,elType);
  B r; ur xr = RNK(x);
  if (xr==1) {
    if (xe==el_B) { B xf = getFillR(x); r = noFill(xf)? emptyHVec() : taga(arr_shVec(m_fillarrpEmpty(xf))); }
    else r = elNum(xe)? emptyIVec() : emptyCVec();
  } else {
    assert(xr > 1);
    Arr* ra;
    if (xe==el_B) { B xf = getFillR(x); ra = noFill(xf)? (Arr*)m_harrUp(0).c : m_fillarrpEmpty(xf); }
    else m_tyarrp(&ra, 1, 0, elNum(xe)? t_bitarr : t_c8arr);
    usz* rsh = arr_shAlloc(ra, xr);
    shcpy(rsh+1, SH(x)+1, xr-1);
    rsh[0] = leading;
    r = taga(ra);
  }
  return r;
}
static B zeroCells(B x) { // doesn't consume
  return reshapeToEmpty(x, 0);
}

B grade_bool(B x, usz xia, bool up) {
  #define BRANCHLESS_GRADE(T) \
    T* rp; r = m_##T##arrv(&rp, xia);  \
    u64 r0 = 0;                        \
    u64 r1 = l0;                       \
    if (up) BG_LOOP(!) else BG_LOOP()
  #define BG_LOOP(OP) \
    for (usz i = 0; i < xia; i++) {    \
      bool b = OP bitp_get(xp,i);      \
      rp[b?r0:r1-r0] = i;              \
      r0+=b; r1++;                     \
    }
  B r;
  u64* xp = bitany_ptr(x);
  u64 sum = bit_sum(xp, xia);
  u64 l0 = up? xia-sum : sum; // Length of first set of indices
  #if SINGELI
  if (xia < 16) { BRANCHLESS_GRADE(i8) }
  else if (xia <= 1<<15) {
    B notx = bit_negate(incG(x));
    u64* xp0 = bitarr_ptr(notx);
    u64* xp1 = xp;
    u64 q=xia%64; if (q) { usz e=xia/64; u64 m=((u64)1<<q)-1; xp0[e]&=m; xp1[e]&=m; }
    if (!up) { u64* t=xp1; xp1=xp0; xp0=t; }
    #define SI_GRADE(W) \
      i##W* rp = m_tyarrv(&r, W/8, xia, t_i##W##arr); \
      si_1slash##W(xp0, rp   , xia, l0    );          \
      si_1slash##W(xp1, rp+l0, xia, xia-l0);
    if (xia <= 128) { SI_GRADE(8) } else { SI_GRADE(16) }
    #undef SI_GRADE
    decG(notx);
  } else if (xia <= 1ull<<31) {
    i32* rp0; r = m_i32arrv(&rp0, xia);
    i32* rp1 = rp0 + l0;
    if (!up) { i32* t=rp1; rp1=rp0; rp0=t; }
    usz b = 256;
    u64 xp0[4]; // 4 ≡ b/64
    u64* xp1 = xp;
    for (usz i=0; i<xia; i+=b) {
      vfor (usz j=0; j<BIT_N(b); j++) xp0[j] = ~xp1[j];
      usz b2 = b>xia-i? xia-i : b;
      if (b2<b) { u64 q=b2%64; usz e=b2/64; u64 m=((u64)1<<q)-1; xp0[e]&=m; xp1[e]&=m; }
      usz s0=bit_sum(xp0,b2); si_1slash32(xp0, i, rp0, b2, s0); rp0+=s0;
      usz s1=b2-s0;           si_1slash32(xp1, i, rp1, b2, s1); rp1+=s1;
      xp1+= b2/64;
    }
  }
  #else
  if      (xia <= 128)      { BRANCHLESS_GRADE(i8) }
  else if (xia <= 1<<15)    { BRANCHLESS_GRADE(i16) }
  else if (xia <= 1ull<<31) { BRANCHLESS_GRADE(i32) }
  #endif
  else                      { BRANCHLESS_GRADE(f64) }
  #undef BRANCHLESS_GRADE
  #undef BG_LOOP
  decG(x); return r;
}

void filter_ne_i32(i32* rp, i32* xp, usz len, usz sum, i32 val) {
  usz b = bsp_max; TALLOC(i16, buf, b + b/16);
  u64* wp = (u64*)(buf + b);
  i32* rp0=rp;
  CmpASFn cmp = CMP_AS_FN(ne, el_i32); B c = m_i32(val);
  for (usz i=0; i<len; i+=b) {
    bool last = b>len-i; if (last) b=len-i;
    CMP_AS_CALL(cmp, wp, xp, c, b);
    usz bs = last? sum-(rp-rp0) : bit_sum(wp,b);
    where_block_u16(wp, (u16*)buf, b, bs);
    for (usz j=0; j<bs; j++) rp[j] = xp[buf[j]];
    rp+= bs; xp+= b;
  }
  TFREE(buf)
}

extern B take_c2(B, B, B);
static B compress(B w, B x, usz wia, u8 xl, u8 xt) {
  u64* wp = bitany_ptr(w);
  u64 we = 0;
  usz ie = wia/64;
  usz q=wia%64; if (q) we = wp[ie] &= ((u64)1<<q) - 1;
  while (!we) {
    if (RARE(ie==0)) return zeroCells(x);
    we = wp[--ie];
  }
  usz wia0 = wia;
  wia = 64*(ie+1) - CLZ(we);
  usz wsum = bit_sum(wp, wia);
  if (wsum == wia0) return incG(x);
  assert(wsum > 0);
  
  B r;
  switch(xl) {
    case 7:
      if (IA(x)==0) return reshapeToEmpty(x, wsum); // handle empty cell case
      // fallthrough
    default: r = compress_grouped(wp, x, wia, wsum, xt); break;
    case 0: {
      u64* xp = bitany_ptr(x);
      u64* rp; r = m_bitarrv(&rp,wsum);
      #if SINGELI
      if (wsum>=wia/si_thresh_compress_bool) {
        si_compress_bool(wp, xp, rp, wia); break;
      }
      #endif
      u64 o = 0;
      usz j = 0;
      for (usz i=0; i<BIT_N(wia); i++) {
        for (u64 v=wp[i], x=xp[i]; v; v&=v-1) {
          o = o>>1 | (x>>CTZ(v))<<63;
          ++j; if (j%64==0) rp[j/64-1] = o;
        }
      }
      usz q=(-j)%64; if (q) rp[j/64] = o>>q;
      break;
    }
    
    #define COMPRESS_BLOCK_PREP(T, PREP) \
      usz b = bsp_max; TALLOC(i16, buf, b); PREP;    \
      T* rp0=rp;                                     \
      for (usz i=0; i<wia; i+=b) {                   \
        usz bs;                                      \
        if (b>wia-i) { b=wia-i; bs=wsum-(rp-rp0); }  \
        else { bs=bit_sum(wp,b); }                   \
        where_block_u16(wp, (u16*)buf, b, bs);       \
        for (usz j=0; j<bs; j++) rp[j] = xp[buf[j]]; \
        rp+= bs; wp+= b/64; xp+= b;                  \
      }                                              \
      TFREE(buf)
    #define COMPRESS_BLOCK(T) COMPRESS_BLOCK_PREP(T, )
    #define WITH_SPARSE(W, CUTOFF, DENSE) { \
      i##W *xp=tyany_ptr(x), *rp;                                                        \
      if (CUTOFF!=1) {                                                                   \
        if (wsum<wia/CUTOFF) { rp=m_tyarrv(&r,W/8,wsum,xt); COMPRESS_BLOCK(i##W); }      \
        else if (groups_lt(wp,wia, wia/128)) r = compress_grouped(wp, x, wia, wsum, xt); \
        else { DENSE; }                                                                  \
      } else {                                                                           \
        if (wsum>=wia/8 && groups_lt(wp,wia, wia/W)) r = compress_grouped(wp, x, wia, wsum, xt); \
        else { rp=m_tyarrv(&r,W/8,wsum,xt); COMPRESS_BLOCK(i##W); }                      \
      }                                                                                  \
      break; }
    
    #if SINGELI
      #define DO(W) WITH_SPARSE(W, si_thresh_2slash##W, rp=m_tyarrv(&r,W/8,wsum,xt); si_2slash##W(wp, xp, rp, wia, wsum))
      case 3: DO(8) case 4: DO(16) case 5: DO(32)
      case 6: if (TI(x,elType)!=el_B) { DO(64) } // else follows
      #undef DO
    #else
      case 3: WITH_SPARSE( 8,  2, rp=m_tyarrv(&r,1,wsum,xt); for (usz i=0; i<wia; i++) { *rp = xp[i]; rp+= bitp_get(wp,i); })
      case 4: WITH_SPARSE(16,  2, rp=m_tyarrv(&r,2,wsum,xt); for (usz i=0; i<wia; i++) { *rp = xp[i]; rp+= bitp_get(wp,i); })
      #define BLOCK_OR_GROUPED(T) \
        if (wsum>=wia/8 && groups_lt(wp,wia, wia/16)) r = compress_grouped(wp, x, wia, wsum, xt); \
        else { T* xp=tyany_ptr(x); T* rp=m_tyarrv(&r,sizeof(T),wsum,xt); COMPRESS_BLOCK(T); }
      case 5: BLOCK_OR_GROUPED(i32) break;
      case 6:
        if (TI(x,elType)!=el_B) { BLOCK_OR_GROUPED(u64) } // else follows
      #undef BLOCK_OR_GROUPED
    #endif
    
    #undef WITH_SPARSE
      else {
        B xf = getFillR(x);
        B* xp = arr_bptr(x);
        if (xp!=NULL) {
          COMPRESS_BLOCK_PREP(B, HArr_p rh = m_harrUv(wsum); B *rp = rh.a;);
          for (usz i=0; i<wsum; i++) inc(rh.a[i]);
          NOGC_E;
          r = withFill(rh.b, xf);
        } else {
          SLOW2("𝕨/𝕩", w, x);
          M_HARR(rp, wsum) SGet(x)
          for (usz i = 0; i < wia; i++) if (bitp_get(wp,i)) HARR_ADDA(rp, Get(x,i));
          r = withFill(HARR_FV(rp), xf);
        }
      }
      break;
    #undef COMPRESS_BLOCK
  }
  ur xr = RNK(x);
  if (xr > 1) {
    a(r)->ia*= arr_csz(x);
    usz* sh = arr_shAlloc(a(r), xr);
    sh[0] = wsum;
    shcpy(sh+1, SH(x)+1, xr-1);
  }
  return r;
}

#define BOOL_REP_XOR_SCAN(WV) \
  usz b = 1<<12;                                       \
  u64 xx=xp[0], xs=xx>>63, js=-(xx&1); xx^=xx<<1;      \
  for (usz k=0, j=0, ij=WV; ; ) {                      \
    usz e = b<s-k? k+b : s;                            \
    usz eb = (e-1)/64+1;                               \
    for (usz i=k/64; i<eb; i++) rp[i]=0;               \
    while (ij<e) {                                     \
      xx>>=1; j++; if (j%64==0) { u64 v=xp[j/64]; xx=v^(v<<1)^xs; xs=v>>63; } \
      rp[ij/64]^=(-(xx&1))<<(ij%64); ij+=WV;           \
    }                                                  \
    for (usz i=k/64; i<eb; i++) js=-((rp[i]^=js)>>63); \
    if (e==s) {break;} k=e;                            \
  }

// Basic boolean loop with overwriting
#define BOOL_REP_OVER(WV, LEN) \
  u64 ri=0, rc=0, xc=0; usz j=0;  \
  for (usz i = 0; i < LEN; i++) { \
    u64 v = -(u64)bitp_get(xp,i); \
    rc ^= (v^xc) << (ri%64);      \
    xc = v;                       \
    ri += WV; usz e = ri/64;      \
    if (j < e) {                  \
      rp[j++] = rc;               \
      while (j < e) rp[j++] = v;  \
      rc = v;                     \
    }                             \
  }                               \
  if (ri%64) rp[j] = rc;

extern GLOBAL B rt_slash;
B slash_c1(B t, B x) {
  if (RARE(isAtm(x)) || RARE(RNK(x)!=1)) thrF("/𝕩: 𝕩 must have rank 1 (%H ≡ ≢𝕩)", x);
  u64 s = usum(x);
  if (s>=USZ_MAX) thrOOM();
  if (s==0) { decG(x); return emptyIVec(); }
  usz xia = IA(x);
  B r;
  u8 xe = TI(x,elType);
  if (xe!=el_bit && s<=xia) x = squeeze_numTry(x, &xe, SQ_NUM);
  if (xe==el_bit) {
    r = where(x, xia, s);
  } else if (RARE(xia > (usz)I32_MAX+1)) {
    SGetU(x)
    f64* rp; r = m_f64arrv(&rp, s); usz ri = 0;
    for (usz i = 0; i < xia; i++) {
      usz c = o2s(GetU(x, i));
      for (usz j = 0; j < c; j++) rp[ri++] = i;
    }
  } else if (RARE(xe > el_i32)) {
    i32* rp; r = m_i32arrv(&rp, s);
    SLOW1("/𝕩", x);
    SGetU(x)
    for (u64 i = 0; i < xia; i++) {
      usz c = o2s(GetU(x, i));
      for (u64 j = 0; j < c; j++) *rp++ = i;
    }
  } else {
    #if SINGELI
    if (s/32 <= xia) { // Sparse case: type of x matters
      i32* rp; r = m_i32arrv(&rp, s);
      si_indices_scan_i32[elwByteLog(xe)](tyany_ptr(x), rp, s);
    } else
    #endif
    { // Dense case: only result type matters
      #define DENSE_IND(T) \
        T* rp; r = m_##T##arrv(&rp, s);          \
        for (u64 i = 0; i < xia; i++) {          \
          i32 c = xp[i];                         \
          for (i32 j = 0; j < c; j++) *rp++ = i; \
        }
      if (xe < el_i32) x = taga(cpyI32Arr(x));
      i32* xp = i32any_ptr(x);
      while (xia>0 && !xp[xia-1]) xia--;
      if      (xia <=   128) { DENSE_IND(i8 ); }
      else if (xia <= 32768) { DENSE_IND(i16); }
      else                   { DENSE_IND(i32); }
      #undef DENSE_IND
    }
  }
  decG(x);
  return r;
}

B slash_c2(B t, B w, B x) {
  i32 wv = -1;
  usz wia ONLY_GCC(=0);
  u8 we ONLY_GCC(=0);
  
  if (isArr(w)) {
    we = TI(w,elType);
    if (!elInt(we)) {
      w = squeeze_numTry(w, &we, SQ_MSGREQ(SQ_NUM));
      if (!elNum(we)) goto base;
    }
    ur wr = RNK(w);
    if (wr>1) thrF("𝕨/𝕩: Simple 𝕨 must have rank 0 or 1 (%i≡=𝕨)", wr);
    if (wr<1) { w = TO_GET(w, 0); goto atom; }
    wia = IA(w);
    if (wia==0) { decG(w); return isArr(x)? x : m_unit(x); }
  } else {
    atom:
    if (!q_i32(w)) goto base;
    wv = o2iG(w);
    if (wv < 0) thrM("𝕨/𝕩: 𝕨 cannot be negative");
  }
  
  if (isAtm(x) || RNK(x)==0) thrM("𝕨/𝕩: 𝕩 must have rank at least 1 for simple 𝕨");
  ur xr = RNK(x);
  usz xlen = *SH(x);
  u8 xl = cellWidthLog(x);
  u8 xt = arrNewType(TY(x));
  
  B r;
  if (wv < 0) { // Array w
    if (RARE(wia!=xlen)) thrF("𝕨/𝕩: Lengths of components of 𝕨 must match 𝕩 (%s ≠ %s)", wia, xlen);
    
    u64 s;
    if (we==el_bit) {
      wbool:
      r = compress(w, x, wia, xl, xt);
      goto decWX_ret;
    }
    if (!elInt(we)) {
      s = usum(w);
      goto arrW_base;
    }
    s = usum(w);
    if (s>=USZ_MAX) thrOOM();
    if (xl>6 || (xl<3 && xl!=0)) goto arrW_base;
    if (s<=wia) {
      if (s==0) { r = zeroCells(x); goto decWX_ret; }
      w = squeeze_numTry(w, &we, SQ_NUM);
      if (we==el_bit) goto wbool;
    }
    // s≠0 now
    
    if (RARE(TI(x,elType)==el_B)) { // Slow case
      arrW_base:
      SLOW2("𝕨/𝕩", w, x);
      B xf = getFillR(x);
      ux ria = s;
      usz csz = arr_csz(x);
      mulOn(ria, csz);
      if (s>=USZ_MAX) thrOOM();
      MAKE_MUT_INIT(r0, ria, TI(x,elType)); MUTG_INIT(r0);
      SGetU(w)
      B wc; usz ri=0;
      if (csz!=1) {   for (ux i=0; i<wia; i++) { if (!q_usz(wc=GetU(w,i))) goto pfree; usz c=o2sG(wc); for(ux j=0;j<c;j++) { mut_copyG(r0, ri, x, i*csz, csz); ri+= csz; } } }
      else { SGetU(x) for (ux i=0; i<wia; i++) { if (!q_usz(wc=GetU(w,i))) goto pfree; usz c=o2sG(wc); if (c)              { mut_fillG(r0, ri, GetU(x, i), c); ri+= c;   } } }
      if (0) { pfree: mut_pfree(r0, ri); expI_B(wc); }
      Arr* ra = mut_fp(r0);
      if (xr == 1) {
        arr_shVec(ra);
      } else {
        usz* rsh = arr_shAlloc(ra, xr);
        rsh[0] = s;
        shcpy(rsh+1, SH(x)+1, xr-1);
      }
      r = withFill(taga(ra), xf);
      decWX_ret: decG(w);
      decX_ret: decG(x);
      return r;
    }
    
    // Make shape if needed; all cases below use it
    usz* rsh = NULL;
    if (xr > 1) {
      rsh = m_shArr(xr)->a;
      rsh[0] = s;
      shcpy(rsh+1, SH(x)+1, xr-1);
    }
    
    if (xl == 0) {
      u64* xp = bitany_ptr(x);
      u64* rp; r = m_bitarrv(&rp, s); if (rsh) { SPRNK(a(r),xr); a(r)->sh = rsh; }
      if (s/1024 <= wia) {
        #define SPARSE_REP(T) T* wp=T##any_ptr(w); BOOL_REP_XOR_SCAN(wp[j])
        if      (we==el_i8 ) { SPARSE_REP(i8 ); }
        else if (we==el_i16) { SPARSE_REP(i16); }
        else                 { SPARSE_REP(i32); }
        #undef SPARSE_REP
      } else {
        if (we < el_i32) w = taga(cpyI32Arr(w));
        i32* wp = i32any_ptr(w);
        BOOL_REP_OVER(wp[i], wia)
      }
    } else {
      u8 xk = xl-3;
      void* rv = m_tyarrlv(&r, xk, s, xt);
      if (rsh) { Arr* ra=a(r); SPRNK(ra,xr); ra->sh = rsh; ra->ia = s*arr_csz(x); }
      void* xv = tyany_ptr(x);
      #if SINGELI
      if ((xk<3? s/64 : s/32) <= wia) { // Sparse case: use both types
        si_replicate_scan[4*elwByteLog(we) + xk](tyany_ptr(w), xv, rv, s);
      } else
      #endif
      { // Dense case: only type of x matters
        #define CASE(L,T) case L: { \
          T* xp = xv; T* rp = rv;                    \
          for (usz i = 0; i < wia; i++) {            \
            i32 cw = wp[i]; T cx = xp[i];            \
            for (i64 j = 0; j < cw; j++) *rp++ = cx; \
          } break; }
        if (we < el_i32) w = taga(cpyI32Arr(w));
        i32* wp = i32any_ptr(w);
        while (wia>0 && !wp[wia-1]) wia--;
        switch (xk) { default: UD; CASE(0,u8) CASE(1,u16) CASE(2,u32) CASE(3,u64) }
        #undef CASE
      }
    }
    goto decWX_ret;
  } else {
    if (wv <= 1) {
      if (wv || xlen==0) return x;
      r = zeroCells(x);
      goto decX_ret;
    }
    if (xlen == 0) return x;
    usz s = xlen;
    if (mulOn(s, wv)) thrOOM();
    if (xl>6 || (xl<3 && xl!=0) || TI(x,elType)==el_B) {
      B xf = getFillR(x);
      if (xr!=1) {
        MAKE_MUT_INIT(r0, IA(x) * wv, TI(x,elType)); MUTG_INIT(r0);
        usz csz = arr_csz(x);
        ux ri = 0;
        for (ux i = 0; i < xlen; i++) for (ux j = 0; j < wv; j++) {
          mut_copyG(r0, ri, x, i*csz, csz);
          ri+= csz;
        }
        r = withFill(mut_fv(r0), xf);
        r = taga(TI(r,slice)(r, 0, IA(r)));
        goto atmW_setsh;
      }
      SLOW2("𝕨/𝕩", w, x);
      HArr_p r0 = m_harrUv(s);
      SGetU(x)
      for (ux i = 0; i < xlen; i++) {
        B cx = incBy(GetU(x, i), wv);
        for (ux j = 0; j < wv; j++) *r0.a++ = cx;
      }
      NOGC_E;
      r = withFill(r0.b, xf);
      goto decX_ret;
    }
    if (xl == 0) {
      u64* xp = bitany_ptr(x);
      u64* rp; r = m_bitarrv(&rp, s);
      #if SINGELI_AVX2
      if (wv <= 256) si_constrep_bool(wv, xp, rp, s);
      #elif SINGELI
      if (wv <= 128) si_constrep_bool(wv, xp, rp, s);
      #else
      if (wv <= 64) { BOOL_REP_XOR_SCAN(wv) }
      #endif
      else {
        // Like BOOL_REP_OVER but predictable
        u64 ri=0, c=0; usz j=0;
        usz n=wv/64-1;
        for (usz i = 0; i < xlen; i++) {
          u64 v = -(1 & xp[i/64]>>(i%64));
          u64 r0 = c ^ ((v^c) << (ri%64));
          c = v;
          ri += wv; usz e = ri/64;
          rp[e-1] = v; // This allows the loop to be constant-length
          rp[j] = r0;
          for (usz k=0; k<n; k++) rp[j+1+k] = v;
          j = e;
        }
        if (ri%64) rp[j] = c;
      }
      goto atmW_maybesh;
    } else {
      u8 xk = xl-3;
      void* rv = m_tyarrv(&r, 1<<xk, s, xt);
      void* xv = tyany_ptr(x);
      #if SINGELI
      si_constrep[xk](wv, xv, rv, xlen);
      #else
      #define CASE(L,T) case L: {                   \
          T* xp = xv; T* rp = rv;                   \
          for (usz i = 0; i < xlen; i++) {          \
            T e = xp[i];                            \
            for (i64 j = 0; j < wv; j++) *rp++ = e; \
          }                                         \
        } break;
      switch (xk) { default: UD; CASE(0,u8) CASE(1,u16) CASE(2,u32) CASE(3,u64) }
      #undef CASE
      #endif
    }
    
    atmW_maybesh:;
    if (xr > 1) {
      atmW_setsh:;
      a(r)->ia = s*arr_csz(x);
      usz* rsh = arr_shAlloc(a(r), xr);
      rsh[0] = s;
      shcpy(rsh+1, SH(x)+1, xr-1);
    }
    goto decX_ret;
  }
  base:
  return c2rt(slash, w, x);
}

#if SINGELI_SIMD
static B finish_small_count(B r, u16* ov) {
  // Need to add 1<<15 to r at i for each index i in ov
  u16 e = -1; // ov end marker
  if (*ov == e) {
    r = squeeze_numNew(r);
  } else {
    r = taga(cpyI32Arr(r)); i32* rp = i32arr_ptr(r);
    usz on = 0; u16 ovi;
    for (usz i=0; (ovi=ov[i])!=e; i++) {
      i32 rv = (rp[ovi]+= 1<<15);
      if (RARE(rv < 0)) {
        rp[ovi] = rv ^ (1<<31);
        ov[on++] = ovi;
      }
    }
    if (RARE(on > 0)) { // Overflowed i32!
      r = taga(cpyF64Arr(r)); f64* rp = f64arr_ptr(r);
      for (usz i=0; i<on; i++) rp[ov[i]]+= 1U<<31;
    }
    FL_SET(r, fl_squoze);
  }
  return r;
}
static B finish_sorted_count(B r, usz* ov, usz* oc, usz on) {
  // Overflow values in ov are sorted but not unique
  // Set mo to the greatest sum of oc for equal ov values
  usz mo = 0, pv = 0, c = 0;
  for (usz i=0; i<on; i++) {
    usz sv = pv; pv = ov[i];
    c = c*(sv==pv) + oc[i];
    if (c>mo) mo=c;
  }
  // Since mo is a multiple of 128 and all of r is less than 128,
  // values in r can't affect the result type
  #define RESIZE(T, UT) \
    r = taga(cpy##UT##Arr(r)); T* rp = tyany_ptr(r); \
    for (usz i=0; i<on; i++) rp[ov[i]]+= oc[i];
  if (mo == 0); // No overflow, r is correct already
  else if (mo < I16_MAX) { RESIZE(i16, I16) }
  else if (mo < I32_MAX) { RESIZE(i32, I32) }
  else                   { RESIZE(f64, F64) }
  #undef RESIZE
  return FL_SET(r, fl_squoze); // Relies on having checked for boolean
}
#endif

static NORETURN void slash_im_bad(f64 c) {
  if (c < 0) thrM("/⁼𝕩: 𝕩 cannot contain negative numbers");
  else thrM("/⁼𝕩: 𝕩 must consist of natural numbers");
}
B slash_im(B t, B x) {
  if (!isArr(x) || RNK(x)!=1) thrM("/⁼𝕩: 𝕩 must be a list");
  u8 xe = TI(x,elType);
  usz xia = IA(x);
  if (xia==0) { decG(x); return emptyIVec(); }
  B r;
  retry:
  switch(xe) { default: UD;
    case el_bit: {
      usz sum = bit_sum(bitany_ptr(x), xia);
      usz ria = 1 + (sum>0);
      f64* rp; r = m_f64arrv(&rp, ria);
      rp[sum>0] = sum; rp[0] = xia - sum;
      r = squeeze_numNewTy(el_f64,r); break;
    }
#if SINGELI_SIMD
  #define INIT_RES(N,RIA) \
    i##N* rp; r = m_i##N##arrv(&rp, RIA); \
    for (usz i=0; i<RIA; i++) rp[i]=0;
  #define TRY_SMALL_OUT(N) \
    if (xp[0]<0) thrM("/⁼𝕩: 𝕩 cannot contain negative numbers");             \
    usz a=1; while (a<xia && xp[a]>xp[a-1]) a++;                             \
    u##N max=xp[a-1];                                                        \
    usz rmax=xia;                                                            \
    if (a<xia) {                                                             \
      if (FL_HAS(x,fl_asc)) {                                                \
        usz ria = xp[xia-1] + 1;                                             \
        usz os = xia/128;                                                    \
        INIT_RES(8,ria)                                                      \
        TALLOC(usz, ov, 2*os); usz* oc = ov+os;                              \
        usz on = si_count_sorted_i##N((u8*)rp, ov, oc, xp, xia);             \
        r = finish_sorted_count(r, ov, oc, on);                              \
        TFREE(ov);                                                           \
        break;                                                               \
      }                                                                      \
      for (usz i=a; i<xia; i++) { u##N c=xp[i]; if (c>max) max=c; }          \
      if ((i##N)max<0) thrM("/⁼𝕩: 𝕩 cannot contain negative numbers");       \
      usz ria = max + 1;                                                     \
      if (xia < ria/8) {                                                     \
        u8 maxcount = 0;                                                     \
        TALLOC(u8, tab, ria);                                                \
        for (usz i=0; i<xia; i++)             tab[xp[i]]=0;                  \
        for (usz i=0; i<xia; i++) maxcount|=++tab[xp[i]];                    \
        TFREE(tab);                                                          \
        if (maxcount<=1) a=xia;                                              \
        else if (N>=16 && maxcount<128) rmax=127;                            \
      }                                                                      \
    }                                                                        \
    usz ria = (usz)max + 1;                                                  \
    if (a==xia) { /* Unique argument */                                      \
      u64* rp; r = m_bitarrv(&rp, ria);                                      \
      for (usz i=0; i<BIT_N(ria); i++) rp[i]=0;                              \
      for (usz i=0; i<xia; i++) bitp_set(rp, xp[i], 1);                      \
      break;                                                                 \
    }                                                                        \
    if (rmax<128) { /* xia<128 or xia<ria/8, fine to process x slowly */     \
      INIT_RES(8,ria)                                                        \
      for (usz i = 0; i < xia; i++) rp[xp[i]]++;                             \
      r = squeeze_numNewTy(el_i8,r); break;                                  \
    }
  #define CASE_SMALL(N) \
    case el_i##N: {                                                          \
      i##N* xp = i##N##any_ptr(x);                                           \
      usz sa = 1<<(N-1);                                                     \
      if (xia < sa || FL_HAS(x,fl_asc)) {                                    \
        TRY_SMALL_OUT(N)                                                     \
        assert(N != 8);                                                      \
        sa = ria;                                                            \
      }                                                                      \
      INIT_RES(16,sa)                                                        \
      usz os = xia>>15;                                                      \
      TALLOC(u16, ov, os+1);                                                 \
      i##N max = simd_count_i##N((u16*)rp, (u16*)ov, xp, xia, 0);            \
      if (max < 0) thrM("/⁼𝕩: 𝕩 cannot contain negative numbers");           \
      usz ria = (usz)max + 1;                                                \
      if (ria < sa) r = C2(take, m_f64(ria), r);                             \
      r = finish_small_count(r, ov);                                         \
      TFREE(ov);                                                             \
      break;                                                                 \
    }
    CASE_SMALL(8) CASE_SMALL(16)
  #undef CASE_SMALL
    case el_i32: {
      i32* xp = i32any_ptr(x);
      TRY_SMALL_OUT(32)
      if (xia>I32_MAX) thrM("/⁼𝕩: 𝕩 too large");
      INIT_RES(32,ria)
      simd_count_i32_i32(rp, xp, xia);
      r = squeeze_numNewTy(el_i32,r); break;
    }
  #undef TRY_SMALL_OUT
  #undef INIT_RES
#else
  #define CASE(N) case el_i##N: { \
      i##N* xp = i##N##any_ptr(x);                                             \
      u##N max=xp[0];                                                          \
      for (usz i=1; i<xia; i++) { u##N c=xp[i]; if (c>max) max=c; }            \
      if ((i##N)max<0) thrM("/⁼𝕩: 𝕩 cannot contain negative numbers");         \
      usz ria = max + 1;                                                       \
      TALLOC(usz, t, ria);                                                     \
      for (usz j=0; j<ria; j++) t[j]=0;                                        \
      for (usz i = 0; i < xia; i++) t[xp[i]]++;                                \
      if (xia<=I32_MAX) { i32* rp; r = m_i32arrv(&rp, ria); vfor (usz i=0; i<ria; i++) rp[i]=t[i]; } \
      else              { f64* rp; r = m_f64arrv(&rp, ria); vfor (usz i=0; i<ria; i++) rp[i]=t[i]; } \
      TFREE(t);                                                                \
      r = squeeze_numNew(r); break; }
    CASE(8) CASE(16) CASE(32)
  #undef CASE
#endif
    case el_f64: {
      f64* xp = f64any_ptr(x);
      usz i,j; f64 max=-1;
      for (i = 0; i < xia; i++) { f64 c=xp[i]; if (!q_fusz(c)) slash_im_bad(c); if (c<=max) break; max=c; }
      for (j = i; j < xia; j++) { f64 c=xp[j]; if (!q_fusz(c)) slash_im_bad(c); max=c>max?c:max; }
      usz ria = max+1; if (ria==0) thrOOM();
      if (i==xia) {
        u64* rp; r = m_bitarrv(&rp, ria); for (usz i=0; i<BIT_N(ria); i++) rp[i]=0;
        for (usz i = 0; i < xia; i++) bitp_set(rp, xp[i], 1);
      } else {
        if (xia>I32_MAX) thrM("/⁼𝕩: 𝕩 too large");
        i32* rp; r = m_i32arrv(&rp, ria); for (usz i=0; i<ria; i++) rp[i]=0;
        for (usz i = 0; i < xia; i++) rp[(usz)xp[i]]++;
      }
      break;
    }
    case el_c8: case el_c16: case el_c32: case el_B: {
      x = squeeze_numTry(x, &xe, SQ_NUM);
      if (elNum(xe)) goto retry;
      B* xp = TO_BPTR(x);
      for (usz i=0; i<xia; i++) o2i64(xp[i]);
      UD;
    }
  }
  decG(x); return r;
}

#if SINGELI_SIMD
  typedef void (*BlendArrScalarFn)(void* r, void* zero, u64 one, void* mask, u64 n);
  extern INIT_GLOBAL BlendArrScalarFn* blendArrScalarFns;
#endif


#define BY_MASK(F, ELTS, C) ({ \
  u64 c = (C);        \
  B* elts_ = (ELTS);  \
  while (c) {         \
    F(elts_[CTZ(c)]); \
    c&=c-1;           \
  }                   \
})

static void decByMask(u64* mask, B* elts, ux ia, bool inv) {
  u64 xor = inv? U64_MAX : 0;
  ux e = ia>>6;
  for (ux i=0; i<e; i++) BY_MASK(dec, elts + i*64,  xor^mask[i]);
  if (ia&63)             BY_MASK(dec, elts + e*64, (xor^mask[e]) & ((1ULL<<(ia&63)) - 1));
}
static void incByMask(u64* mask, B* elts, ux ia, bool inv) {
  u64 xor = inv? U64_MAX : 0;
  ux e = ia>>6;
  for (ux i=0; i<e; i++) BY_MASK(inc, elts + i*64,  xor^mask[i]);
  if (ia&63)             BY_MASK(inc, elts + e*64, (xor^mask[e]) & ((1ULL<<(ia&63)) - 1));
}
#undef BY_MASK

B ne_c2(B,B,B);
B slash_ucw(B t, B o, B w, B x) {
  if (isAtm(w) || isAtm(x) || RNK(w)!=1 || RNK(x)!=1 || IA(w)!=IA(x)) {
    base:
    return def_fn_ucw(t, o, w, x);
  }
  usz ia = IA(x);
  u8 we = TI(w,elType);
  if (we != el_bit) {
    w = squeeze_numTry(w, &we, SQ_NUM);
    if (we != el_bit) {
      if (!elNum(we)) goto base;
      i64 bounds[2];
      if (!getRange_fns[we](tyany_ptr(w), bounds, IA(w)) || bounds[0]<0) {
        usum(w);
        thrOOM();
      }
    }
  }
  
  // (c ; C˙)¨⌾(w⊸/) x
  #if SINGELI_SIMD
  if (MAY_F(isFun(o) && TY(o)==t_md1D)) {
    u8 xe = TI(x,elType);
    
    Md1D* od = c(Md1D,o);
    if (PRTID(od->m1) != n_each) goto notConstEach;
    B c;
    if (!toConstant(od->f, &c)) goto notConstEach;
    
    if (we != el_bit) {
      // relies on compatible(c,c) always being true
      w = C2(ne, w, m_f64(0));
      assert(TI(w,elType)==el_bit);
      we = el_bit;
    }
    
    u8 ce = selfElType(c);
    u8 re = el_or(ce,xe);
    
    u64 sum = U64_MAX;
    if (isVal(c)) {
      sum = bit_sum(bitany_ptr(w), ia);
      if (sum==0) decG(c); // TODO could return x; fills?
      else incByG(c, sum-1);
    }
    
    ConvArr r = toEltypeArrX(x, re);
    assert((xe==el_B) == (r.refState!=0));
    if (r.refState != 0) {
      assert(re == el_B);
      if (r.refState == 1) decByMask(bitany_ptr(w), r.xp, ia, 0);
      else                 incByMask(bitany_ptr(w), r.xp, ia, 1);
    }
    
    u64 cv;
    if (elInt(re)) cv = o2iG(c);
    else if (elChr(re)) cv = o2cG(c);
    else {
      assert(re==el_B || re==el_f64);
      cv = re==el_f64? r_f64u(o2fG(c)) : r_Bu(c);
    }
    
    blendArrScalarFns[elwBitLog(re)](r.rp, r.xp, cv, bitany_ptr(w), ia);
    NOGC_E;
    decG(w); decG(x);
    return r.res;
  }
  #endif
  goto notConstEach; notConstEach:;
  
  B arg = C2(slash, incG(w), incG(x));
  usz argIA = IA(arg);
  B rep = c1(o, arg); // TODO special-case non-callable rep
  if (isAtm(rep) || RNK(rep)!=1 || IA(rep) != argIA) thrF("𝔽⌾(a⊸/)𝕩: 𝔽 must return an array with the same shape as its input (expected ⟨%s⟩, got %H)", argIA, rep);
  u8 re = el_or(TI(x,elType), TI(rep,elType));
  
  if (MAY_F(we==el_bit)) {
    bit_w:;
    ConvArr r = toEltypeArrX(x, re);
    ux ni = 0;
    u64* d = bitany_ptr(w);
    switch (re) { default: UD;
      case el_i8:  rep = toI8Any(rep);  goto bit_u8;
      case el_c8:  rep = toC8Any(rep);  goto bit_u8;
      case el_i16: rep = toI16Any(rep); goto bit_u16;
      case el_c16: rep = toC16Any(rep); goto bit_u16;
      case el_i32: rep = toI32Any(rep); goto bit_u32;
      case el_c32: rep = toC32Any(rep); goto bit_u32;
      case el_f64: rep = toF64Any(rep); goto bit_u64;
      case el_bit: {
        // TODO BMI2 pext
        void* np = bitany_ptr(rep);
        NOUNROLL for (usz i = 0; i < ia; i++) {
          bool v = bitp_get(d, i);
          bitp_set(r.rp, i, bitp_get(v? np : r.xp, v? ni : i));
          ni+= v;
        }
        goto done_v2;
      }
      case el_B: {
        B* np = arr_bptr(rep);
        if (np != NULL) {
          if (r.refState == 1) {
            NOUNROLL for (usz i = 0; i < ia; i++) {
              bool v = bitp_get(d, i);
              B xv = ((B*)r.xp)[i];
              if (v) dec(xv);
              ((B*)r.rp)[i] = v? inc(np[ni]) : xv;
              ni+= v;
            }
          } else {
            NOUNROLL for (usz i = 0; i < ia; i++) {
              bool v = bitp_get(d, i);
              ((B*)r.rp)[i] = inc(*(v? np+ni : ((B*)r.xp)+i));
              ni+= v;
            }
          }
        } else {
          if (r.refState == 1) decByMask(bitany_ptr(w), r.xp, ia, 0);
          else                 incByMask(bitany_ptr(w), r.xp, ia, 1);
          SGet(rep)
          NOUNROLL for (usz i = 0; i < ia; i++) {
            bool v = bitp_get(d, i);
            ((B*)r.rp)[i] = v? Get(rep,ni) : ((B*)r.xp)[i];
            ni+= v;
          }
        }
        NOGC_E;
        goto done_v2;
      }
    }
    
    #define IMPL(T) do {         \
      T* np = tyany_ptr(rep);    \
      NOUNROLL for (usz i = 0; i < ia; i++) { \
        bool v = bitp_get(d, i); \
        ((T*)r.rp)[i] = *(v? np+ni : ((T*)r.xp)+i); \
        ni+= v;                  \
      }                          \
      goto done_v2;              \
    } while(0)
    bit_u8:  IMPL(u8);
    bit_u16: IMPL(u16);
    bit_u32: IMPL(u32);
    bit_u64: IMPL(u64);
    #undef IMPL
    
    done_v2:;
    decG(rep); decG(w); decG(x);
    return r.res;
  } else {
    ux wia = IA(w);
    SGetU(w) SGetU(rep)
    B w2 = C2(ne, incG(w), m_f64(0));
    ux nz = bit_sum(bitany_ptr(w2), wia);
    
    ux ni = 0;
    M_HARR(rp, nz)
    for (ux i = 0; i < wia; i++) {
      ux c = o2sG(GetU(w, i));
      if (c != 0) {
        B cr = GetU(rep, ni);
        for (ux j = 1; j < c; j++) if (!compatible(GetU(rep,ni+j), cr)) thrM("𝔽⌾(a⊸/): Incompatible result elements");
        HARR_ADDA(rp, inc(cr));
      }
      ni+= c;
    }
    assert(ni == IA(rep));
    
    decG(w);
    w = w2;
    decG(rep);
    rep = toEltypeArr(HARR_FV(rp), re).obj;
    goto bit_w;
  }
}

void slash_init(void) {
  c(BFn,bi_slash)->im = slash_im;
  c(BFn,bi_slash)->ucw = slash_ucw;
}
