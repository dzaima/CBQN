#include "../core.h"
#include "../utils/mut.h"
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
static const usz bsp_max  = 1<<11;
static const u32 bsp_top  = 1<<24;
static const u32 bsp_mask = bsp_top - 1;
static usz bsp_fill(u64* src, u32* buf, usz len) {
  assert(len <= 1<<11);
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

static B compress(B w, B x, usz wia, B xf) {
  u64* wp = bitarr_ptr(w);
  u64 we = 0;
  usz ie = wia/64;
  usz q=wia%64; if (q) we = wp[ie] &= ((u64)1<<q) - 1;
  while (!we) {
    if (RARE(ie==0)) { return q_N(xf)? emptyHVec() : isF64(xf)? emptyIVec() : isC32(xf)? emptyCVec() : m_emptyFVec(xf); }
    we = wp[--ie];
  }
  usz wia0 = wia;
  wia = 64*(ie+1) - CLZ(we);
  usz wsum = bit_sum(wp, wia);
  if (wsum == wia0) { dec(xf); return inc(x); }

  B r;
  u8 xe = TI(x,elType);
  switch(xe) { default: UD;
    case el_bit: {
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
      if (wsum>=wia/CUTOFF) { DENSE; }      \
      else { rp=m_tyarrv(&r,W/8,wsum,el2t(xe)); COMPRESS_BLOCK(i##W); } \
      break; }
    #if SINGELI
    case el_i8: case el_c8:  WITH_SPARSE( 8, 32, rp=m_tyarrvO(&r,1,wsum,el2t(xe),  8); bmipopc_2slash8 (wp, xp, rp, wia))
    case el_i16:case el_c16: WITH_SPARSE(16, 16, rp=m_tyarrvO(&r,2,wsum,el2t(xe), 16); bmipopc_2slash16(wp, xp, rp, wia))
    #else
    case el_i8: case el_c8:  WITH_SPARSE( 8,  2, rp=m_tyarrv(&r,1,wsum,el2t(xe)); for (usz i=0; i<wia; i++) { *rp = xp[i]; rp+= bitp_get(wp,i); })
    case el_i16:case el_c16: WITH_SPARSE(16,  2, rp=m_tyarrv(&r,2,wsum,el2t(xe)); for (usz i=0; i<wia; i++) { *rp = xp[i]; rp+= bitp_get(wp,i); })
    #endif
    #undef WITH_SPARSE
    case el_i32:case el_c32: { i32* xp= tyany_ptr(x); i32* rp=m_tyarrv(&r,4,wsum,el2t(xe)); COMPRESS_BLOCK(i32); break; }
    case el_f64:             { f64* xp=f64any_ptr(x); f64* rp; r = m_f64arrv(&rp,wsum);     COMPRESS_BLOCK(f64); break; }
    case el_B: {
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
      break;
    }
    #undef COMPRESS_BLOCK
  }
  return r;
}

extern B rt_slash;
B slash_c1(B t, B x) {
  if (RARE(isAtm(x)) || RARE(RNK(x)!=1)) thrF("/: Argument must have rank 1 (%H ≡ ≢𝕩)", x);
  u64 s = usum(x);
  if (s>=USZ_MAX) thrOOM();
  if (s==0) { decG(x); return emptyIVec(); }
  usz xia = IA(x);
  B r;
  u8 xe = TI(x,elType);
  if (xe==el_bit) {
    r = where(x, xia, s);
  } else if (RARE(xia>=I32_MAX)) {
    SGetU(x)
    f64* rp; r = m_f64arrv(&rp, s); usz ri = 0;
    for (usz i = 0; i < xia; i++) {
      usz c = o2s(GetU(x, i));
      for (usz j = 0; j < c; j++) rp[ri++] = i;
    }
  } else {
    i32* rp; r = m_i32arrv(&rp, s);
    if (xe==el_i8) {
      i8* xp = i8any_ptr(x);
      while (xia>0 && !xp[xia-1]) xia--;
      for (u64 i = 0; i < xia; i++) {
        i32 c = xp[i];
        if (LIKELY(c==0 || c==1)) {
          *rp = i;
          rp+= c;
        } else {
          for (i32 j = 0; j < c; j++) *rp++ = i;
        }
      }
    } else if (xe==el_i32) {
      i32* xp = i32any_ptr(x);
      while (xia>0 && !xp[xia-1]) xia--;
      for (u64 i = 0; i < xia; i++) {
        i32 c = xp[i];
        if (LIKELY(c==0 || c==1)) {
          *rp = i;
          rp+= c;
        } else {
          for (i32 j = 0; j < c; j++) *rp++ = i;
        }
      }
    } else {
      SLOW1("/𝕩", x);
      SGetU(x)
      for (u64 i = 0; i < xia; i++) {
        usz c = o2s(GetU(x, i));
        for (u64 j = 0; j < c; j++) *rp++ = i;
      }
    }
  }
  decG(x);
  return r;
}

B slash_c2(B t, B w, B x) {
  if (isArr(x) && RNK(x)==1 && isArr(w) && RNK(w)==1 && depth(w)==1) {
    usz wia = IA(w);
    usz xia = IA(x);
    if (RARE(wia!=xia)) {
      if (wia==0) { decG(w); return x; }
      thrF("/: Lengths of components of 𝕨 must match 𝕩 (%s ≠ %s)", wia, xia);
    }
    B xf = getFillQ(x);
    
    if (TI(w,elType)==el_bit) {
      B r = compress(w, x, wia, xf);
      decG(w); decG(x); return r;
    }
    #define CASE(WT,XT) if (TI(x,elType)==el_##XT) { \
      XT* xp = XT##any_ptr(x);                       \
      XT* rp; B r = m_##XT##arrv(&rp, wsum);         \
      if (or<2) for (usz i = 0; i < wia; i++) {      \
        *rp = xp[i];                                 \
        rp+= wp[i];                                  \
      } else for (usz i = 0; i < wia; i++) {         \
        WT cw = wp[i]; XT cx = xp[i];                \
        for (i64 j = 0; j < cw; j++) *rp++ = cx;     \
      }                                              \
      decG(w); decG(x); return r;                    \
    }
    #define TYPED(WT,SIGN) { \
      WT* wp = WT##any_ptr(w);           \
      while (wia>0 && !wp[wia-1]) wia--; \
      i64 wsum = 0;                      \
      u32 or = 0;                        \
      for (usz i = 0; i < wia; i++) {    \
        wsum+= wp[i];                    \
        or|= (u32)wp[i];                 \
      }                                  \
      if (or>>SIGN) thrM("/: 𝕨 must consist of natural numbers"); \
      if (TI(x,elType)==el_bit) {                  \
        u64* xp = bitarr_ptr(x); u64 ri=0;         \
        u64* rp; B r = m_bitarrv(&rp, wsum);       \
        if (or<2) for (usz i = 0; i < wia; i++) {  \
          bitp_set(rp, ri, bitp_get(xp,i));        \
          ri+= wp[i];                              \
        } else for (usz i = 0; i < wia; i++) {     \
          WT cw = wp[i]; bool cx = bitp_get(xp,i); \
          for (i64 j = 0; j < cw; j++) bitp_set(rp, ri++, cx); \
        }                                          \
        decG(w); decG(x); return r;                \
      }                                            \
      CASE(WT,i8) CASE(WT,i16) CASE(WT,i32) CASE(WT,f64) \
      SLOW2("𝕨/𝕩", w, x);                    \
      M_HARR(r, wsum) SGetU(x)               \
      for (usz i = 0; i < wia; i++) {        \
        i32 cw = wp[i]; if (cw==0) continue; \
        B cx = incBy(GetU(x, i), cw);        \
        for (i64 j = 0; j < cw; j++) HARR_ADDA(r, cx);\
      }                                      \
      decG(w); decG(x);                      \
      return withFill(HARR_FV(r), xf);       \
    }
    if (TI(w,elType)==el_i8 ) TYPED(i8,7);
    if (TI(w,elType)==el_i32) TYPED(i32,31);
    #undef TYPED
    #undef CASE
    SLOW2("𝕨/𝕩", w, x);
    u64 ria = usum(w);
    if (ria>=USZ_MAX) thrOOM();
    M_HARR(r, ria) SGetU(w) SGetU(x)
    for (usz i = 0; i < wia; i++) {
      usz c = o2s(GetU(w, i));
      if (c) {
        B cx = incBy(GetU(x, i), c);
        for (usz j = 0; RARE(j < c); j++) HARR_ADDA(r, cx);
      }
    }
    decG(w); decG(x);
    return withFill(HARR_FV(r), xf);
  }
  if (isArr(x) && RNK(x)==1 && q_i32(w)) {
    usz xia = IA(x);
    i32 wv = o2i(w);
    if (wv<=0) {
      if (wv<0) thrM("/: 𝕨 cannot be negative");
      return taga(arr_shVec(TI(x,slice)(x, 0, 0)));
    }
    if (TI(x,elType)==el_i32) {
      i32* xp = i32any_ptr(x);
      i32* rp; B r = m_i32arrv(&rp, xia*wv);
      for (usz i = 0; i < xia; i++) {
        for (i64 j = 0; j < wv; j++) *rp++ = xp[i];
      }
      decG(x);
      return r;
    } else {
      SLOW2("𝕨/𝕩", w, x);
      B xf = getFillQ(x);
      HArr_p r = m_harrUv(xia*wv);
      SGetU(x)
      for (usz i = 0; i < xia; i++) {
        B cx = incBy(GetU(x, i), wv);
        for (i64 j = 0; j < wv; j++) *r.a++ = cx;
      }
      decG(x);
      return withFill(r.b, xf);
    }
  }
  return c2(rt_slash, w, x);
}


B slash_im(B t, B x) {
  if (!isArr(x) || RNK(x)!=1) thrM("/⁼: Argument must be an array");
  u8 xe = TI(x,elType);
  usz xia = IA(x);
  if (xia==0) { decG(x); return emptyIVec(); }
  switch(xe) { default: UD;
    case el_bit: {
      usz sum = bit_sum(bitarr_ptr(x), xia);
      usz ria = 1 + (sum>0);
      f64* rp; B r = m_f64arrv(&rp, ria);
      rp[sum>0] = sum; rp[0] = xia - sum;
      decG(x); return num_squeeze(r);
    }
#define CASE_SMALL(N) \
    case el_i##N: {                                                              \
      i##N* xp = i##N##any_ptr(x);                                               \
      usz m=1<<N;                                                                \
      B r;                                                                       \
      if (xia < m/2) {                                                           \
        usz a=1; u##N max=xp[0];                                                 \
        if (xp[0]<0) thrM("/⁼: Argument cannot contain negative numbers");       \
        if (xia < m/2) {                                                         \
          a=1; while (a<xia && xp[a]>xp[a-1]) a++;                               \
          max=xp[a-1];                                                           \
          if (a==xia) { /* Sorted unique argument */                             \
            usz ria = max + 1;                                                   \
            u64* rp; r = m_bitarrv(&rp, ria);                                    \
            for (usz i=0; i<BIT_N(ria); i++) rp[i]=0;                            \
            for (usz i=0; i<xia; i++) bitp_set(rp, xp[i], 1);                    \
            decG(x); return r;                                                   \
          }                                                                      \
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
      decG(x); return num_squeeze(r);                                            \
    }
    CASE_SMALL(8) CASE_SMALL(16)
#undef CASE_SMALL
    case el_i32: {
      i32* xp = i32any_ptr(x);
      usz i,j; B r; i32 max=-1;
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
      decG(x); return r;
    }
    case el_f64: {
      f64* xp = f64any_ptr(x);
      usz i,j; B r; f64 max=-1;
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
      decG(x); return r;
    }
    case el_c8: case el_c16: case el_c32: case el_B: {
      SLOW1("/⁼", x);
      B* xp = arr_bptr(x);
      if (xp==NULL) { HArr* xa=cpyHArr(x); x=taga(xa); xp=xa->a; }
      usz i,j; B r; i64 max=-1;
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
      decG(x); return r;
    }
  }
}

B slash_ucw(B t, B o, B w, B x) {
  if (isAtm(w) || isAtm(x) || RNK(w)!=1 || RNK(x)!=1 || IA(w)!=IA(x)) return def_fn_ucw(t, o, w, x);
  usz ia = IA(x);
  SGetU(w)
  if (!elInt(TI(w,elType))) for (usz i = 0; i < ia; i++) if (!q_i32(GetU(w,i))) return def_fn_ucw(t, o, w, x);
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
