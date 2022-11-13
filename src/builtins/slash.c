// Indices and Replicate (/)
// In the notes 𝕨 might indicate 𝕩 for Indices too

// Boolean 𝕨 (Where/Compress) general case based on result type width
// COULD use AVX-512
// Size 1: pext, or bit-at-a-time
//   SHOULD emulate pext if unavailable
//   COULD return boolean result from Where
// Size 8, 16: pdep/pext, or branchless
//   SHOULD try vector lookup-shuffle if unavailable or old AMD
// Size 32, 64: 16-bit indices from where_block_u16
// Other sizes: always used grouped code
// Adaptivity based on 𝕨 statistics
//   None for 8-bit Where, too short
//   COULD try per-block adaptivity for 16-bit Compress
//   Sparse if +´𝕨 is small, branchless unless it's very small
//     Chosen per-argument for 8, 16 and per-block for larger
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
// Boolean uses pdep, ≠`, or overwriting
//   SHOULD make a shift/mask replacement for pdep
// Others use +`, or lots of Singeli
//   Fixed shuffles, factorization, partial shuffles, self-overlapping

// SHOULD do something for odd cell widths in Replicate

// Indices inverse (/⁼), a lot like Group
// COULD always give a squeezed result, sometimes expensive
// SHOULD sort large-range 𝕩 to find minimum result type
// Boolean 𝕩: just count 1s
// Long i8 and i16 𝕩: count into zeroed buffer before anything else
//   Only zero positive part; if total is too small there were negatives
//   Cutoff is set so short 𝕩 gives a result of the same type
// Scan for strictly ascending 𝕩
//   COULD vectorize with find-compare
//   COULD find descending too
// Unsigned maximum for integers to avoid a separate negative check

#include "../core.h"
#include "../utils/mut.h"
#include "../utils/calls.h"
#include "../utils/talloc.h"
#include "../builtins.h"

#ifdef __BMI2__
  #include <immintrin.h>
  
  #if USE_VALGRIND
    #define DBG_VG_SLASH 0
    u64 loadMask(u64* p, u64 unk, u64 exp, u64 i, u64 pos) {
      // #if DBG_VG_SLASH
      //   if (pos==0) printf("index %2ld, got %016lx\n", i, p[i]);
      // #endif
      if (pos==0) return ~(p[i]^exp);
      u64 res =          loadMask(p, unk, exp, i,     pos<<1);
      if (unk&pos) res&= loadMask(p, unk, exp, i|pos, pos<<1);
      return res;
    }
    NOINLINE u64 vg_load64(u64* p, u64 i) {
      u64 unk = ~vg_getDefined_u64(i);
      u64 res = p[vg_withDefined_u64(i, ~0ULL)]; // result value will always be the proper indexing operation
      
      i32 undefCount = POPC(unk);
      if (undefCount>0) {
        if (undefCount>8) err("too many unknown bits in index of vg_load64");
        res = vg_withDefined_u64(res, loadMask(p, unk, res, i & ~unk, 1));
      }
      #if DBG_VG_SLASH
        vg_printDefined_u64("idx", i);
        vg_printDefined_u64("res", res);
      #endif
      return res;
    }
    NOINLINE u64 vg_pext_u64(u64 src, u64 mask) {
      u64 maskD = vg_getDefined_u64(mask);
      u64 r = vg_undef_u64(0);
      i32 ri = 0;
      u64 undefMask = 0;
      for (i32 i = 0; i < 64; i++) {
        u64 c = 1ull<<i;
        if (!(maskD&c) && undefMask==0) undefMask = (~0ULL)<<ri;
        if (vg_def_u64(mask&c)) r = vg_withBit_u64(r, ri++, (c&src)!=0);
      }
      if (ri<64) r = r & (1ULL<<ri)-1;
      r = vg_withDefined_u64(r, vg_getDefined_u64(r) & ~undefMask);
      #if DBG_VG_SLASH
        printf("pext:\n");
        vg_printDefined_u64("src", src);
        vg_printDefined_u64("msk", mask);
        vg_printDefined_u64("res", r);
        vg_printDefined_u64("exp", _pext_u64(src, mask));
      #endif
      return r;
    }
    NOINLINE u64 vg_pdep_u64(u64 src, u64 mask) {
      if (0 != ~vg_getDefined_u64(mask)) err("pdep impl assumes mask is defined everywhere");
      u64 c = src;
      u64 r = 0;
      for (i32 i = 0; i < 64; i++) {
        if ((mask>>i)&1) {
          r|= (c&1) << i;
          c>>= 1;
        }
      }
      #if DBG_VG_SLASH
        printf("pdep:\n");
        vg_printDefined_u64("src", src);
        vg_printDefined_u64("msk", mask);
        vg_printDefined_u64("res", r);
        vg_printDefined_u64("exp", _pdep_u64(src, mask));
      #endif
      return r;
    }
    NOINLINE u64 rand_popc64(u64 x) {
      u64 def = vg_getDefined_u64(x);
      if (def==~0ULL) return POPC(x);
      i32 min = POPC(x & def);
      i32 diff = POPC(~def);
      i32 res = min + vgRand64Range(diff);
      #if DBG_VG_SLASH
        printf("popc:\n");
        vg_printDefined_u64("x", x);
        printf("popc in %d-%d; res: %d\n", min, min+diff, res);
      #endif
      return res;
    }
    #define _pext_u32 vg_pext_u64
    #define _pext_u64 vg_pext_u64
    #define _pdep_u32 vg_pdep_u64
    #define _pdep_u64 vg_pdep_u64
  #else
    #define vg_load64(p, i) p[i]
    #define rand_popc64(X) POPC(X)
  #endif
  
  void storeu_u64(u64* p, u64 v) { memcpy(p, &v, 8); }
  u64 loadu_u64(u64* p) { u64 v; memcpy(&v, p, 8); return v; }
  #if SINGELI
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wunused-variable"
    #include "../singeli/gen/slash.c"
    #pragma GCC diagnostic pop
  #endif
#endif

#if SINGELI
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wunused-variable"
  #include "../singeli/gen/constrep.c"
  #pragma GCC diagnostic pop

  extern void (*const avx2_scan_pluswrap_u8)(uint8_t* v0,uint8_t* v1,uint64_t v2,uint8_t v3);
  extern void (*const avx2_scan_pluswrap_u16)(uint16_t* v0,uint16_t* v1,uint64_t v2,uint16_t v3);
  extern void (*const avx2_scan_pluswrap_u32)(uint32_t* v0,uint32_t* v1,uint64_t v2,uint32_t v3);
  #define avx2_scan_pluswrap_u64(V0,V1,V2,V3) for (usz i=k; i<e; i++) js=rp[i]+=js;
  #define PLUS_SCAN(T) avx2_scan_pluswrap_##T(rp+k,rp+k,e-k,js); js=rp[e-1];
  extern void (*const avx2_scan_max32)(int32_t* v0,int32_t* v1,uint64_t v2);
#else
  #define PLUS_SCAN(T) for (usz i=k; i<e; i++) js=rp[i]+=js;
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
  TALLOC(u32, buf, bufsize);
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
  #if SINGELI && defined(__BMI2__)
  if (sum >=       len/8) bmipopc_1slash16(src, (i16*)dst, len);
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

static B compress_grouped(u64* wp, B x, usz wia, usz wsum, u8 xt) {
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
      xp = (u8*)arr_bptr(x);
      usz ria = wsum*csz;
      if (xp != NULL) {
        rh = m_harrUv(ria);
        rp = (u8*)rh.a;
      } else {
        SLOW2("𝕨/𝕩", taga(RFLD(wp,TyArr,a)), x);
        M_HARR(rp, ria) SGet(x)
        for (usz i = 0; i < wia; i++) if (bitp_get(wp,i)) {
          for (usz j = 0; j < csz; j++) HARR_ADDA(rp, Get(x,i*csz+j));
        }
        return withFill(HARR_FV(rp), getFillQ(x));
      }
    }
    COMPRESS_GROUP(MEM_CPY)
    if (is_B) {
      for (usz i = 0; i < wsum*csz; i++) inc(((B*)rp)[i]);
      r = withFill(rh.b, getFillQ(x));
      IA(r) = wsum; // Shape-setting code at end of compress expects this
    }
  } else { // Bits
    usz width = csz;
    u64* xp = tyany_ptr(x);
    u64* rp; r = m_bitarrv(&rp,wsum*width); IA(r) = wsum;
    COMPRESS_GROUP(bit_cpy)
  }
  return r;
}

static B where(B x, usz xia, u64 s) {
  B r;
  u64* xp = bitarr_ptr(x);
  usz q=xia%64; if (q) xp[xia/64] &= ((u64)1<<q) - 1;
  if (xia <= 128) {
    #if SINGELI && defined(__BMI2__)
    i8* rp = m_tyarrvO(&r, 1, s, t_i8arr, 8);
    bmipopc_1slash8(xp, rp, xia);
    #else
    i8* rp; r=m_i8arrv(&rp,s); WHERE_SPARSE(xp,rp,s,0,);
    #endif
  } else if (xia <= 32768) {
    #if SINGELI && defined(__BMI2__)
    if (s >= xia/8) {
      i16* rp = m_tyarrvO(&r, 2, s, t_i16arr, 16);
      bmipopc_1slash16(xp, rp, xia);
    }
    #else
    if (s >= xia/4+xia/8) {
      i16* rp = m_tyarrvO(&r, 2, s, t_i16arr, 2);
      WHERE_DENSE(xp, rp, xia, 0);
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
    #if SINGELI && defined(__BMI2__)
    i32* rp; r = m_i32arrv(&rp, s);
    #else
    i32* rp = m_tyarrvO(&r, 4, s, t_i32arr, 4);
    #endif
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
      #if SINGELI && defined(__BMI2__)
      if (bs >= b/8+b/16) {
        bmipopc_1slash16(xp, buf, b);
        for (usz j=0; j<bs; j++) rq[j] = i+buf[j];
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
    TFREE(buf);
  } else {
    f64* rp; r = m_f64arrv(&rp, s);
    usz b = bsp_max; TALLOC(u16, buf, b);
    f64* rp0 = rp;
    for (usz i=0; i<xia; i+=b) {
      usz bs;
      if (b>xia-i) { b=xia-i; bs=s-(rp-rp0); } else { bs=bit_sum(xp,b); }
      where_block_u16(xp, buf, b, bs);
      for (usz j=0; j<bs; j++) rp[j] = i+buf[j];
      rp+= bs;
      xp+= b/64;
    }
    TFREE(buf);
  }
  return r;
}

// Is the number of values switches in w at most max?
static bool groups_lt(u64* wp, usz len, usz max) {
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

static NOINLINE B zeroCells(B x) { // doesn't consume
  u8 xe = TI(x,elType);
  B r; ur xr = RNK(x);
  if (xr==1) {
    if (xe==el_B) { B xf = getFillR(x); r = noFill(xf)? emptyHVec() : m_emptyFVec(xf); }
    else r = elNum(xe)? emptyIVec() : emptyCVec();
  } else {
    Arr* ra;
    if (xe==el_B) { B xf = getFillR(x); if (noFill(xf)) ra = (Arr*)m_harrUp(0).c; else { ra = m_fillarrp(0); fillarr_setFill(ra, xf); } }
    else m_tyarrp(&ra, 1, 0, elNum(xe)? t_bitarr : t_c8arr);
    usz* rsh = arr_shAlloc(ra, xr);
    shcpy(rsh+1, SH(x)+1, xr-1);
    rsh[0] = 0;
    r = taga(ra);
  }
  return r;
}

B not_c1(B t, B x);
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
  u64* xp = bitarr_ptr(x);
  u64 sum = bit_sum(xp, xia);
  u64 l0 = up? xia-sum : sum; // Length of first set of indices
  #if SINGELI && defined(__BMI2__)
  if (xia < 16) { BRANCHLESS_GRADE(i8) }
  else if (xia <= 1<<15) {
    B notx = not_c1(m_f64(0), inc(x));
    u64* xp0 = bitarr_ptr(notx);
    u64* xp1 = xp;
    if (!up) { u64* t=xp1; xp1=xp0; xp0=t; }
    #define BMI_GRADE(W) \
      i##W* rp = m_tyarrvO(&r, W/8, xia, t_i##W##arr, W); \
      bmipopc_1slash##W(xp0, rp   , xia);                 \
      bmipopc_1slash##W(xp1, rp+l0, xia);
    if (xia <= 128) { BMI_GRADE(8) } else { BMI_GRADE(16) }
    #undef BMI_GRADE
    decG(notx);
  }
  #else
  if      (xia <= 128)      { BRANCHLESS_GRADE(i8) }
  else if (xia <= 1<<15)    { BRANCHLESS_GRADE(i16) }
  #endif
  else if (xia <= 1ull<<31) { BRANCHLESS_GRADE(i32) }
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
  u64* wp = bitarr_ptr(w);
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
  if (wsum == wia0) return inc(x);
  
  B r;
  switch(xl) {
    default: r = compress_grouped(wp, x, wia, wsum, xt); break;
    case 0: {
      u64* xp = bitarr_ptr(x); u64* rp;
      #if SINGELI && defined(__BMI2__)
      r = m_bitarrv(&rp,wsum+128); a(r)->ia = wsum;
      u64 cw = 0; // current word
      u64 ro = 0; // offset in word where next bit should be written; never 64
      for (usz i=0; i<BIT_N(wia); i++) {
        u64 wv = wp[i];
        u64 v = _pext_u64(xp[i], wv);
        u64 c = rand_popc64(wv);
        cw|= v<<ro;
        u64 ro2 = ro+c;
        if (ro2>=64) {
          *(rp++) = cw;
          cw = ro? v>>(64-ro) : 0;
        }
        ro = ro2&63;
      }
      if (ro) *rp = cw;
      #else
      r = m_bitarrv(&rp,wsum);
      for (usz i=0, ri=0; i<wia; i++) { bitp_set(rp,ri,bitp_get(xp,i)); ri+= bitp_get(wp,i); }
      #endif
      break;
    }
    #define COMPRESS_BLOCK(T) \
      usz b = bsp_max; TALLOC(i16, buf, b);          \
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
    #define WITH_SPARSE(W, CUTOFF, DENSE) { \
      i##W *xp=tyany_ptr(x), *rp;           \
      if (wsum<wia/CUTOFF) { rp=m_tyarrv(&r,W/8,wsum,xt); COMPRESS_BLOCK(i##W); }      \
      else if (groups_lt(wp,wia, wia/128)) r = compress_grouped(wp, x, wia, wsum, xt); \
      else { DENSE; }                       \
      break; }
    #if SINGELI
    case 3: WITH_SPARSE( 8, 32, rp=m_tyarrvO(&r,1,wsum,xt,  8); bmipopc_2slash8 (wp, xp, rp, wia))
    case 4: WITH_SPARSE(16, 16, rp=m_tyarrvO(&r,2,wsum,xt, 16); bmipopc_2slash16(wp, xp, rp, wia))
    #else
    case 3: WITH_SPARSE( 8,  2, rp=m_tyarrv(&r,1,wsum,xt); for (usz i=0; i<wia; i++) { *rp = xp[i]; rp+= bitp_get(wp,i); })
    case 4: WITH_SPARSE(16,  2, rp=m_tyarrv(&r,2,wsum,xt); for (usz i=0; i<wia; i++) { *rp = xp[i]; rp+= bitp_get(wp,i); })
    #endif
    #undef WITH_SPARSE
    #define BLOCK_OR_GROUPED(T) \
      if (wsum>=wia/8 && groups_lt(wp,wia, wia/16)) r = compress_grouped(wp, x, wia, wsum, xt); \
      else { T* xp=tyany_ptr(x); T* rp=m_tyarrv(&r,sizeof(T),wsum,xt); COMPRESS_BLOCK(T); }
    case 5: BLOCK_OR_GROUPED(i32) break;
    case 6:
      if (TI(x,elType)!=el_B) { BLOCK_OR_GROUPED(u64) }
      else {
        B xf = getFillQ(x);
        B* xp = arr_bptr(x);
        if (xp!=NULL) {
          HArr_p rh = m_harrUv(wsum);
          B *rp = rh.a; COMPRESS_BLOCK(B);
          for (usz i=0; i<wsum; i++) inc(rh.a[i]);
          r = withFill(rh.b, xf);
        } else {
          SLOW2("𝕨/𝕩", w, x);
          M_HARR(rp, wsum) SGet(x)
          for (usz i = 0; i < wia; i++) if (bitp_get(wp,i)) HARR_ADDA(rp, Get(x,i));
          r = withFill(HARR_FV(rp), xf);
        }
      }
      break;
    #undef BLOCK_OR_GROUPED
    #undef COMPRESS_BLOCK
  }
  ur xr = RNK(x);
  if (xr > 1) {
    Arr* ra=a(r); SPRNK(ra,xr);
    usz* sh = PSH(ra) = m_shArr(xr)->a;
    sh[0] = PIA(ra); PIA(ra) *= arr_csz(x);
    shcpy(sh+1, SH(x)+1, xr-1);
  }
  return r;
}

// Replicate using plus/max/xor-scan
#define SCAN_CORE(WV, UPD, SET, SCAN) \
  usz b = 1<<10;                       \
  for (usz k=0, j=0, ij=WV; ; ) {      \
    usz e = b<s-k? k+b : s;            \
    for (usz i=k; i<e; i++) rp[i]=0;   \
    SET;                               \
    while (ij<e) { j++; UPD; ij+=WV; } \
    SCAN;                              \
    if (e==s) {break;}  k=e;           \
  }
#define SUM_CORE(T, WV, PREP, INC) \
  SCAN_CORE(WV, PREP; rp[ij]+=INC, , PLUS_SCAN(T))

#if SINGELI
  #define IND_BY_SCAN \
    SCAN_CORE(xp[j], rp[ij]=j, rp[k]=j, avx2_scan_max32(rp+k,rp+k,e-k))
#else
  #define IND_BY_SCAN usz js=0; SUM_CORE(i32, xp[j], , 1)
#endif

#define REP_BY_SCAN(T, WV) \
  T* xp = xv; T* rp = rv;                 \
  T js=xp[0], px=js;                      \
  SUM_CORE(T, WV, T sx=px, (px=xp[j])-sx)

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

extern B rt_slash;
B slash_c1(B t, B x) {
  if (RARE(isAtm(x)) || RARE(RNK(x)!=1)) thrF("/: Argument must have rank 1 (%H ≡ ≢𝕩)", x);
  u64 s = usum(x);
  if (s>=USZ_MAX) thrOOM();
  if (s==0) { decG(x); return emptyIVec(); }
  usz xia = IA(x);
  B r;
  u8 xe = TI(x,elType);
  if (xe!=el_bit && s<=xia) { x = num_squeezeChk(x); xe = TI(x,elType); }
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
    if (s/32 <= xia) { // Sparse case: type of x matters
      #define SPARSE_IND(T) T* xp = T##any_ptr(x); IND_BY_SCAN
      i32* rp; r = m_i32arrv(&rp, s);
      if      (xe == el_i8 ) { SPARSE_IND(i8 ); }
      else if (xe == el_i16) { SPARSE_IND(i16); }
      else                   { SPARSE_IND(i32); }
      #undef SPARSE_IND
    } else { // Dense case: only result type matters
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
  usz wia;
  if (isArr(w)) {
    if (depth(w)>1) goto base;
    ur wr = RNK(w);
    if (wr>1) thrF("/: Simple 𝕨 must have rank 0 or 1 (%i≡=𝕨)", wr);
    if (wr<1) { B v=IGet(w, 0); decG(w); w=v; goto atom; }
    wia = IA(w);
    if (wia==0) { decG(w); return isArr(x)? x : m_atomUnit(x); }
  } else {
    atom:
    if (!q_i32(w)) goto base;
    wv = o2iG(w);
    if (wv < 0) thrM("/: 𝕨 cannot be negative");
  }
  if (isAtm(x) || RNK(x)==0) thrM("/: 𝕩 must have rank at least 1 for simple 𝕨");
  ur xr = RNK(x);
  usz xlen = *SH(x);
  u8 xl = cellWidthLog(x);
  u8 xt = arrNewType(TY(x));
  
  B r;
  if (wv < 0) { // Array w
    if (RARE(wia!=xlen)) thrF("/: Lengths of components of 𝕨 must match 𝕩 (%s ≠ %s)", wia, xlen);
    
    u8 we = TI(w,elType);
    if (!elInt(we)) {
      w=any_squeeze(w); we=TI(w,elType);
      if (!elInt(we)) goto slow;
    }
    if (we==el_bit) {
      wbool:
      r = compress(w, x, wia, xl, xt);
      goto decWX_ret;
    }
    if (xl>6 || (xl<3 && xl!=0)) goto base;
    u64 s = usum(w);
    if (s<=wia) {
      w=num_squeezeChk(w); we=TI(w,elType);
      if (we==el_bit) goto wbool;
    }
    
    if (RARE(TI(x,elType)==el_B)) { // Slow case
      slow:
      if (xr > 1) goto base;
      SLOW2("𝕨/𝕩", w, x);
      B xf = getFillQ(x);
      MAKE_MUT(r0, s) mut_init(r0, el_B); MUTG_INIT(r0);
      SGetU(w) SGetU(x)
      usz ri = 0;
      for (usz i = 0; i < wia; i++) {
        usz c = o2s(GetU(w, i));
        if (c) {
          mut_fillG(r0, ri, GetU(x, i), c);
          ri+= c;
        }
      }
      r = withFill(mut_fv(r0), xf);
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
      u64* xp = bitarr_ptr(x);
      u64* rp; r = m_bitarrv(&rp, s); if (rsh) { SPRNK(a(r),xr); SH(r) = rsh; }
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
      void* rv = m_tyarrv(&r, 1<<xk, s, xt);
      if (rsh) { Arr* ra=a(r); SPRNK(ra,xr); PSH(ra) = rsh; PIA(ra) = s*arr_csz(x); }
      void* xv = tyany_ptr(x);
      if ((xk<3? s/64 : s/32) <= wia) { // Sparse case: use both types
        #define CASE(L,XT) case L: { REP_BY_SCAN(XT, wp[j]) break; }
        #define SPARSE_REP(WT) \
          WT* wp = WT##any_ptr(w);                \
          switch (xk) { default: UD; CASE(0,u8) CASE(1,u16) CASE(2,u32) CASE(3,u64) }
        if      (we == el_i8 ) { SPARSE_REP(i8 ); }
        else if (we == el_i16) { SPARSE_REP(i16); }
        else                   { SPARSE_REP(i32); }
        #undef SPARSE_REP
        #undef CASE
      } else { // Dense case: only type of x matters
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
    usz s = xlen * wv;
    if (xl>6 || (xl<3 && xl!=0) || TI(x,elType)==el_B) {
      if (xr != 1) goto base;
      SLOW2("𝕨/𝕩", w, x);
      B xf = getFillQ(x);
      HArr_p r0 = m_harrUv(s);
      SGetU(x)
      for (usz i = 0; i < xlen; i++) {
        B cx = incBy(GetU(x, i), wv);
        for (i64 j = 0; j < wv; j++) *r0.a++ = cx;
      }
      r = withFill(r0.b, xf);
      goto decX_ret;
    }
    if (xl == 0) {
      u64* xp = bitarr_ptr(x);
      u64* rp; r = m_bitarrv(&rp, s);
      #if __BMI2__
      if (wv <= 52) {
        u64 m = (u64)-1 / (((u64)1<<wv)-1); // TODO table lookup
        u64 xw = 0;
        usz d = POPC(m); // == 64/wv
        if (m & 1) {  // Power of two
          for (usz i=-1, j=0; j<BIT_N(s); j++) {
            xw >>= d;
            if ((j&(wv-1))==0) xw = xp[++i];
            u64 rw = _pdep_u64(xw, m);
            rp[j] = (rw<<wv)-rw;
          }
        } else {
          usz q = CTZ(m);  // == 64%wv
          m = m<<(wv-q) | 1;
          u64 mt = (u64)1<<(d+1);  // Bit d+1 may be needed, isn't pdep-ed
          usz tsh = d*wv-(d+1);
          for (usz xi=0, o=0, j=0; j<BIT_N(s); j++) {
            xw = *(u64*)((u8*)xp+xi/8) >> (xi%8);
            u64 ex = (xw&mt)<<tsh;
            u64 rw = _pdep_u64(xw, m);
            rp[j] = ((rw-ex)<<(wv-o))-(rw>>o|(xw&1));
            o += q;
            bool oo = o>=wv; xi+=d+oo; o-=wv&-oo;
          }
        }
      } else
      #endif
      if (wv <= 256) { BOOL_REP_XOR_SCAN(wv) }
      else           { BOOL_REP_OVER(wv, xlen) }
      goto decX_ret;
    } else {
      u8 xk = xl-3;
      void* rv = m_tyarrv(&r, 1<<xk, s, xt);
      void* xv = tyany_ptr(x);
      #if SINGELI
      #define CASE(L,T) case L: constrep_##T(wv, xv, rv, xlen); break;
      #else
      #define CASE(L,T) case L: { REP_BY_SCAN(T, wv) break; }
      #endif
      switch (xk) { default: UD; CASE(0,u8) CASE(1,u16) CASE(2,u32) CASE(3,u64) }
      #undef CASE
    }
    if (xr > 1) {
      usz* rsh = m_shArr(xr)->a;
      rsh[0] = s;
      shcpy(rsh+1, SH(x)+1, xr-1);
      Arr* ra=a(r); SPRNK(ra,xr); PSH(ra)=rsh; PIA(ra)=s*arr_csz(x);
    }
    goto decX_ret;
  }
  base:
  return c2(rt_slash, w, x);
}


B slash_im(B t, B x) {
  if (!isArr(x) || RNK(x)!=1) thrM("/⁼: Argument must be an array");
  u8 xe = TI(x,elType);
  usz xia = IA(x);
  if (xia==0) { decG(x); return emptyIVec(); }
  B r;
  switch(xe) { default: UD;
    case el_bit: {
      usz sum = bit_sum(bitarr_ptr(x), xia);
      usz ria = 1 + (sum>0);
      f64* rp; r = m_f64arrv(&rp, ria);
      rp[sum>0] = sum; rp[0] = xia - sum;
      r = num_squeeze(r); break;
    }
#define CASE_SMALL(N) \
    case el_i##N: {                                                              \
      i##N* xp = i##N##any_ptr(x);                                               \
      usz m=1<<N;                                                                \
      if (xia < m/2) {                                                           \
        if (xp[0]<0) thrM("/⁼: Argument cannot contain negative numbers");       \
        usz a=1; while (a<xia && xp[a]>xp[a-1]) a++;                             \
        u##N max=xp[a-1];                                                        \
        if (a==xia) { /* Sorted unique argument */                               \
          usz ria = max + 1;                                                     \
          u64* rp; r = m_bitarrv(&rp, ria);                                      \
          for (usz i=0; i<BIT_N(ria); i++) rp[i]=0;                              \
          for (usz i=0; i<xia; i++) bitp_set(rp, xp[i], 1);                      \
          break;                                                                 \
        }                                                                        \
        for (usz i=a; i<xia; i++) { u##N c=xp[i]; if (c>max) max=c; }            \
        if ((i##N)max<0) thrM("/⁼: Argument cannot contain negative numbers");   \
        usz ria = max+1;                                                         \
        i##N* rp; r = m_i##N##arrv(&rp, ria); for (usz i=0; i<ria; i++) rp[i]=0; \
        for (usz i = 0; i < xia; i++) rp[xp[i]]++;                               \
      } else {                                                                   \
        TALLOC(usz, t, m);                                                       \
        for (usz j=0; j<m/2; j++) t[j]=0;                                        \
        for (usz i=0; i<xia; i++) t[(u##N)xp[i]]++;                              \
        t[m/2]=xia; usz ria=0; for (u64 s=0; s<xia; ria++) s+=t[ria];            \
        if (ria>m/2) thrM("/⁼: Argument cannot contain negative numbers");       \
        i32* rp; r = m_i32arrv(&rp, ria); for (usz i=0; i<ria; i++) rp[i]=t[i];  \
        TFREE(t);                                                                \
      }                                                                          \
      r = num_squeeze(r); break;                                                 \
    }
    CASE_SMALL(8) CASE_SMALL(16)
#undef CASE_SMALL
    case el_i32: {
      i32* xp = i32any_ptr(x);
      usz i,j; i32 max=-1;
      for (i = 0; i < xia; i++) { i32 c=xp[i]; if (c<=max) break; max=c; }
      for (j = i; j < xia; j++) { i32 c=xp[j]; max=c>max?c:max; if (c<0) thrM("/⁼: Argument cannot contain negative numbers"); }
      usz ria = max+1;
      if (i==xia) {
        u64* rp; r = m_bitarrv(&rp, ria); for (usz i=0; i<BIT_N(ria); i++) rp[i]=0;
        for (usz i = 0; i < xia; i++) bitp_set(rp, xp[i], 1);
      } else {
        i32* rp; r = m_i32arrv(&rp, ria); for (usz i=0; i<ria; i++) rp[i]=0;
        for (usz i = 0; i < xia; i++) rp[xp[i]]++;
      }
      break;
    }
    case el_f64: {
      f64* xp = f64any_ptr(x);
      usz i,j; f64 max=-1;
      for (i = 0; i < xia; i++) { f64 c=xp[i]; if (c!=(usz)c) thrM("/⁼: Argument must consist of natural numbers"); if (c<=max) break; max=c; }
      for (j = i; j < xia; j++) { f64 c=xp[j]; if (c!=(usz)c) thrM("/⁼: Argument must consist of natural numbers"); max=c>max?c:max; if (c<0) thrM("/⁼: Argument cannot contain negative numbers"); }
      usz ria = max+1; if (ria==0) thrOOM();
      if (i==xia) {
        u64* rp; r = m_bitarrv(&rp, ria); for (usz i=0; i<BIT_N(ria); i++) rp[i]=0;
        for (usz i = 0; i < xia; i++) bitp_set(rp, xp[i], 1);
      } else {
        i32* rp; r = m_i32arrv(&rp, ria); for (usz i=0; i<ria; i++) rp[i]=0;
        for (usz i = 0; i < xia; i++) rp[(usz)xp[i]]++;
      }
      break;
    }
    case el_c8: case el_c16: case el_c32: case el_B: {
      SLOW1("/⁼", x);
      B* xp = arr_bptr(x);
      if (xp==NULL) { HArr* xa=cpyHArr(x); x=taga(xa); xp=xa->a; }
      usz i,j; i64 max=-1;
      for (i = 0; i < xia; i++) { i64 c=o2i64(xp[i]); if (c<=max) break; max=c; }
      for (j = i; j < xia; j++) { i64 c=o2i64(xp[j]); max=c>max?c:max; if (c<0) thrM("/⁼: Argument cannot contain negative numbers"); }
      if (max > USZ_MAX-1) thrOOM();
      usz ria = max+1;
      if (i==xia) {
        u64* rp; r = m_bitarrv(&rp, ria); for (usz i=0; i<BIT_N(ria); i++) rp[i]=0;
        for (usz i = 0; i < xia; i++) bitp_set(rp, o2i64G(xp[i]), 1);
      } else {
        i32* rp; r = m_i32arrv(&rp, ria); for (usz i=0; i<ria; i++) rp[i]=0;
        for (usz i = 0; i < xia; i++) rp[o2i64G(xp[i])]++;
      }
      break;
    }
  }
  decG(x); return r;
}

B slash_ucw(B t, B o, B w, B x) {
  if (isAtm(w) || isAtm(x) || RNK(w)!=1 || RNK(x)!=1 || IA(w)!=IA(x)) {
    base:
    return def_fn_ucw(t, o, w, x);
  }
  usz ia = IA(x);
  SGetU(w)
  if (TY(w) != t_bitarr) {
    w = num_squeezeChk(w);
    if (!elInt(TI(w,elType))) goto base;
  }
  B arg = slash_c2(t, inc(w), inc(x));
  usz argIA = IA(arg);
  B rep = c1(o, arg);
  if (isAtm(rep) || RNK(rep)!=1 || IA(rep) != argIA) thrF("𝔽⌾(a⊸/)𝕩: Result of 𝔽 must have the same shape as a/𝕩 (expected ⟨%s⟩, got %H)", argIA, rep);
  MAKE_MUT(r, ia); mut_init(r, el_or(TI(x,elType), TI(rep,elType)));
  SGet(x)
  SGet(rep)
  usz repI = 0;
  if (TY(w) == t_bitarr) {
    u64* d = bitarr_ptr(w);
    if (elInt(TI(x,elType)) && elInt(TI(rep,elType))) {
      if (r->fns->elType!=el_i32) mut_to(r, el_i32);
      i32* rp = r->ai32;
      x   = toI32Any(x);   i32* xp = i32any_ptr(x);
      rep = toI32Any(rep); i32* np = i32any_ptr(rep);
      for (usz i = 0; i < ia; i++) {
        bool v = bitp_get(d, i);
        i32 nc = np[repI];
        i32 xc = xp[i];
        rp[i] = v? nc : xc;
        repI+= v;
      }
    } else {
      MUTG_INIT(r);
      for (usz i = 0; i < ia; i++) mut_setG(r, i, bitp_get(d, i)? Get(rep,repI++) : Get(x,i));
    }
  } else {
    SGetU(rep)
    MUTG_INIT(r);
    for (usz i = 0; i < ia; i++) {
      i32 cw = o2iG(GetU(w, i));
      if (cw) {
        B cr = Get(rep,repI);
        if (CHECK_VALID) for (i32 j = 1; j < cw; j++) if (!equal(GetU(rep,repI+j), cr)) { mut_pfree(r,i); thrM("𝔽⌾(a⊸/): Incompatible result elements"); }
        mut_setG(r, i, cr);
        repI+= cw;
      } else mut_setG(r, i, Get(x,i));
    }
  }
  decG(w); decG(rep);
  return mut_fcd(r, x);
}

void slash_init() {
  c(BFn,bi_slash)->im = slash_im;
  c(BFn,bi_slash)->ucw = slash_ucw;
}
