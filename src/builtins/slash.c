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

#define WHERE_SPARSE(X,R,S) do { \
    for (usz i=0, j=0; j<S; i++) \
      for (u64 v=X[i]; v; v&=v-1) R[j++] = i*64 + CTZ(v); \
  } while (0)

static B where(B x, usz xia, u64 s) {
  B r;
  u64* xp = bitarr_ptr(x);
  usz q=xia%64; if (q) xp[xia/64] &= ((u64)1<<q) - 1;
  if (xia <= 128) {
    #if SINGELI && defined(__BMI2__)
    i8* rp = m_tyarrvO(&r, 1, s, t_i8arr, 8);
    bmipopc_1slash8(xp, rp, xia);
    #else
    i8* rp; r=m_i8arrv(&rp,s); WHERE_SPARSE(xp,rp,s);
    #endif
  } else if (xia <= 32768) {
    #if SINGELI && defined(__BMI2__)
    if (s >= xia/16) {
      i16* rp = m_tyarrvO(&r, 2, s, t_i16arr, 16);
      bmipopc_1slash16(xp, rp, xia);
    }
    #else
    if (s >= xia/2) {
      i16* rp = m_tyarrvO(&r, 2, s, t_i16arr, 2);
      for (usz i=0; i<(xia+7)/8; i++) {
        u8 v = ((u8*)xp)[i];
        for (usz k=0; k<8; k++) { *rp=8*i+k; rp+=v&1; v>>=1; }
      }
    }
    #endif
    else {
      i16* rp; r=m_i16arrv(&rp,s); WHERE_SPARSE(xp,rp,s);
    }
  } else {
    assert(xia <= (usz)I32_MAX+1);
    #if SINGELI && defined(__BMI2__)
    i32* rp; r = m_i32arrv(&rp, s);
    #else
    i32* rp = m_tyarrvO(&r, 4, s, t_i32arr, 4);
    #endif
    usz b = 1<<11; // Maximum allowed for branchless sparse method
    TALLOC(i16, buf, b);
    i32* rq=rp; usz i=0;
    for (; i<xia; i+=b) {
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
        for (usz ii=0; ii<(b+7)/8; ii++) {
          u8 v = ((u8*)xp)[ii];
          i32* rs=rq;
          for (usz k=0; k<8; k++) { *rs=i+8*ii+k; rs+=v&1; v>>=1; }
        }
      }
      #endif
      else if (bs >= b/256) { // Branchless sparse
        for (usz j=0; j<bs; j++) rq[j]=0;
        u32 top = 1<<24;
        for (usz i=0, j=0; i<(b+63)/64; i++) {
          u64 u=xp[i], p;
          p=(u32)u&(top-1); rq[j]+=(2*top)|p; j+=POPC(p); u>>=24;
          p=(u32)u&(top-1); rq[j]+=(3*top)|p; j+=POPC(p); u>>=24;
          p=(u32)u        ; rq[j]+=(3*top)|p; j+=POPC(p);
        }
        u64 t=((u64)i<<21)-2*top;
        for (usz j=0; j<bs; j++) {
          t += (u32)rq[j];
          rq[j] = 8*(t>>24) + CTZ((u32)t);
          t &= t-1;
        }
      } else { // Branched very sparse
        for (usz ii=i/64, j=0; j<bs; ii++) {
          for (u64 v=xp[ii-i/64]; RARE(v); v&=v-1) rq[j++] = ii*64 + CTZ(v);
        }
      }
      rq+= bs;
      xp+= b/64;
    }
    TFREE(buf);
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
  if (RARE(xia>=I32_MAX)) {
    usz xia = IA(x);
    SGetU(x)
    f64* rp; B r = m_f64arrv(&rp, s); usz ri = 0;
    for (usz i = 0; i < xia; i++) {
      usz c = o2s(GetU(x, i));
      for (usz j = 0; j < c; j++) rp[ri++] = i;
    }
    decG(x);
    return r;
  }
  B r;
  u8 xe = TI(x,elType);
  if (xe==el_bit) {
    r = where(x, xia, s);
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
    
    usz ri = 0;
    if (TI(w,elType)==el_bit) {
      B r;
      u64* wp = bitarr_ptr(w);
      u8 xe = TI(x,elType);
      #ifdef __BMI2__
      usz wsum = bit_sum(wp, wia);
      switch(xe) {
        case el_bit: {
          u64* xp = bitarr_ptr(x); u64* rp; r = m_bitarrv(&rp,wsum+128); a(r)->ia = wsum;
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
          goto bit_ret;
        }
        #if SINGELI
        case el_i8: case el_c8:  { i8*  xp=tyany_ptr(x); i8*  rp=m_tyarrvO(&r,1,wsum,el2t(xe),  8); bmipopc_2slash8 (wp, xp, rp, wia); goto bit_ret; }
        case el_i16:case el_c16: { i16* xp=tyany_ptr(x); i16* rp=m_tyarrvO(&r,2,wsum,el2t(xe), 16); bmipopc_2slash16(wp, xp, rp, wia); goto bit_ret; }
        case el_i32:case el_c32: {
          i32* xp=tyany_ptr(x); i32* rp=m_tyarrv(&r,4,wsum,el2t(xe));
          usz b = 1<<7;
          TALLOC(i8, buf, b);
          i32* rq=rp; i32* end=xp+xia-b;
          while (xp < end) {
            bmipopc_1slash8(wp, buf, b);
            usz bs = bit_sum(wp, b);
            for (usz j=0; j<bs; j++) rq[j] = xp[buf[j]];
            rq+= bs;
            wp+= b/64;
            xp+= b;
          }
          bmipopc_1slash8(wp, buf, (end+b)-xp);
          for (usz j=0, bs=wsum-(rq-rp); j<bs; j++) rq[j] = xp[buf[j]];
          TFREE(buf);
          goto bit_ret;
        }
        #endif
      }
      #endif
      
      while (wia>0 && !bitp_get(wp,wia-1)) wia--;
      #ifndef __BMI2__
      usz wsum = bit_sum(wp, wia);
      #endif
      if (wsum==0) { decG(w); decG(x); return q_N(xf)? emptyHVec() : isF64(xf)? emptyIVec() : isC32(xf)? emptyCVec() : m_emptyFVec(xf); }
      switch(xe) { default: UD;
        
        #if !SINGELI
        case el_i8: case el_c8:  { i8*  xp=tyany_ptr(x); i8*  rp=m_tyarrv(&r,1,wsum,el2t(xe)); for (usz i=0; i<wia; i++) { *rp = xp[i]; rp+= bitp_get(wp,i); } break; }
        case el_i16:case el_c16: { i16* xp=tyany_ptr(x); i16* rp=m_tyarrv(&r,2,wsum,el2t(xe)); for (usz i=0; i<wia; i++) { *rp = xp[i]; rp+= bitp_get(wp,i); } break; }
        #ifndef __BMI2__
        case el_bit: { u64* xp = bitarr_ptr(x); u64* rp; r = m_bitarrv(&rp,wsum); for (usz i=0; i<wia; i++) { bitp_set(rp,ri,bitp_get(xp,i)); ri+= bitp_get(wp,i); } break; }
        #endif
        #endif
        
        case el_i32:case el_c32: { i32* xp=tyany_ptr(x); i32* rp=m_tyarrv(&r,4,wsum,el2t(xe)); for (usz i=0; i<wia; i++) { *rp = xp[i]; rp+= bitp_get(wp,i); } break; }
        case el_f64: { f64* xp = f64any_ptr(x); f64* rp; r = m_f64arrv(&rp,wsum); for (usz i=0; i<wia; i++) { *rp = xp[i]; rp+= bitp_get(wp,i); } break; }
        case el_B: {
          B* xp = arr_bptr(x);
          if (xp!=NULL) {
            HArr_p rp = m_harrUv(wsum);
            for (usz i=0; i<wia; i++) { rp.a[ri] = xp[i]; ri+= bitp_get(wp,i); }
            for (usz i=0; i<wsum; i++) inc(rp.a[i]);
            r = withFill(rp.b, xf);
          } else {
            SLOW2("𝕨/𝕩", w, x);
            M_HARR(rp, wsum) SGet(x)
            for (usz i = 0; i < wia; i++) if (bitp_get(wp,i)) HARR_ADDA(rp, Get(x,i));
            r = withFill(HARR_FV(rp), xf);
          }
          break;
        }
      }
      #if __BMI2__
      bit_ret:;
      #endif
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
