// Dyadic search functions: Member Of (∊), Index of (⊐), Progressive Index of (⊒)

// 𝕨⊐unit or unit∊𝕩: SIMD shortcutting search
// 𝕨⊒𝕩 where 1≥≠𝕩: defer to 𝕨⊐𝕩
// High-rank inputs:
//   Convert to a typed numeric list if cells are ≤62 bits
//   COULD have hashing for equal-type >64 bit cells, to skip squeezing
//   COULD try squeezing ahead-of-time to skip it in bqn_hash
//   SHOULD have fast path when cell sizes don't match
// One input empty: fast not-found
// Character elements:
//   Character versus number array is fast not-found
//   Reinterpret as integer elements
//     Mixed c8 & c16 get widened to c16 to use in signed 16-bit tables
// COULD try p=⌜n when all arguments are short (may not be faster?)
// p⊐n & n∊p with short n: p⊸⊐¨n
// p⊐n & n∊p with boolean p: based on ⊑p and p⊐¬⊑p
// p⊐n & n∊p with short p:
//   AVX2 binary search when applicable
//     SHOULD apply vector binary search to characters (sort as ints)
//   n⊸=¨p otherwise
// ≤16-bit elements: lookup tables
//   8-bit ∊ and ⊐: SSSE3 table
//     SHOULD make 8-bit NEON table
//   SHOULD have branchy reverse 16-bit table search
// 32- or 64-bit elements: hash tables
//   Store hash in table and not element; Robin Hood ordering; resizing
//   Reverse hash if searched-for is shorter
//   Shortcutting for reverse hashes and non-reversed ⊒
//   SIMD lookup for 32-bit ∊ if chain length is small enough
//   SHOULD partition if hash table size gets large
// SHOULD handle unequal search types better
// Otherwise, generic hashtable

#include "../core.h"
#include "../utils/hash.h"
#include "../utils/talloc.h"
#include "../utils/calls.h"

extern NOINLINE void memset16(u16* p, u16 v, usz l) { for (usz i=0; i<l; i++) p[i]=v; }
extern NOINLINE void memset32(u32* p, u32 v, usz l) { for (usz i=0; i<l; i++) p[i]=v; }
extern NOINLINE void memset64(u64* p, u64 v, usz l) { for (usz i=0; i<l; i++) p[i]=v; }
#define memset_i8          memset
#define memset_i16(P,V,L)  memset16((u16*)(P), V, L)
#define memset_i32(P,V,L)  memset32((u32*)(P), V, L)

INIT_GLOBAL RangeFn getRange_fns[el_f64+1];
#if SINGELI
  extern RangeFn* const simd_getRangeRaw;
  #define SINGELI_FILE search
  #include "../utils/includeSingeli.h"
#else
  #define GETRANGE(T,CHK) bool getRange_##T(void* x0, i64* res, u64 ia) { \
    assert(ia>0); T* x=x0; T min=*x,max=min; \
    { T c=min; CHK; (void)c; }               \
    for (ux i=1; i<ia; i++) { T c=x[i]; CHK; \
      {if(c<min)min=c;} {if(c>max)max=c;}    \
    }                                        \
    res[0]=min; res[1]=max; return 1;        \
  }
  GETRANGE(i8,)
  GETRANGE(i16,)
  GETRANGE(i32,)
  GETRANGE(f64, if (!q_fi64(c)) return 0)
#endif
#if SINGELI_AVX2
  extern void (**const avx2_member_sort)(uint64_t*,void*,uint64_t,void*,uint64_t);
  extern void (**const avx2_indexOf_sort)(int8_t*,void*,uint64_t,void*,uint64_t);
#endif


#define C2i(F, W, X) C2(F, m_i32(W), X)
extern B and_c1(B,B);
extern B gradeDown_c1(B,B);
extern B reverse_c1(B,B);
extern B eq_c2(B,B,B);
extern B ne_c2(B,B,B);
extern B or_c2(B,B,B);
extern B add_c2(B,B,B);
extern B sub_c2(B,B,B);
extern B mul_c2(B,B,B);
extern B join_c2(B,B,B);
extern B select_c2(B,B,B);

B asNormalized(B x, usz n, bool nanBad);
SHOULD_INLINE bool canCompare64_norm2(B* w, usz wia, B* x, usz xia) {
  B wn=asNormalized(*w,wia,true); if (wn.u == m_f64(0).u) return 0; *w=wn;
  B xn=asNormalized(*x,xia,true); if (xn.u == m_f64(0).u) return 0; *x=xn;
  return 1;
}

static u64 elRange(u8 eltype) { return 1ull<<(1<<elwBitLog(eltype)); }

#define TABLE(IN, FOR, TY, INIT, SET) \
  usz it = elRange(IN##e);   /* Range of writes        */                    \
  usz ft = elRange(FOR##e);  /* Range of lookups       */                    \
  usz t = it>ft? it : ft;    /* Table allocation width */                    \
  TALLOC(TY, tab0, t); TY* tab = tab0 + t/2;                                 \
  usz m=IN##ia, n=FOR##ia;                                                   \
  void* ip = tyany_ptr(IN);                                                  \
  void* fp = tyany_ptr(FOR);                                                 \
  /* Initialize */                                                           \
  if (IN.u != FOR.u) {                                                       \
    if (FOR##e==el_i16 && n<ft/(64/sizeof(TY)))                              \
         { for (usz i=0; i<n; i++) tab[((i16*)fp)[i]]=INIT; }                \
    else { memset_##TY(tab-(ft/2-(ft==2)), INIT, ft); }                      \
  }                                                                          \
  /* Set */                                                                  \
  if (IN##e==el_i8) { for (usz i=m; i--;    ) tab[((i8 *)ip)[i]]=SET; }      \
  else              { for (usz i=m; i--;    ) tab[((i16*)ip)[i]]=SET; }      \
  decG(IN);                                                                  \
  /* Lookup */                                                               \
  if (FOR##e==el_bit) {                                                      \
    r = bit_sel(FOR, m_i32(tab[0]), m_i32(tab[1]));                          \
  } else {                                                                   \
    TY* rp; r = m_##TY##arrc(&rp, FOR);                                      \
    if (FOR##e==el_i8){ for (usz i=0; i<n; i++) rp[i]=tab[((i8 *)fp)[i]]; }  \
    else              { for (usz i=0; i<n; i++) rp[i]=tab[((i16*)fp)[i]]; }  \
    decG(FOR);                                                               \
  }                                                                          \
  TFREE(tab0);

typedef struct { B n, p; } B2;
static NOINLINE B toIntCell(B x, ux csz0, ur co) {
  assert(TI(x,elType)!=el_B);
  usz ria = shProd(SH(x), 0, co);
  ShArr* rsh;
  if (co>1) { rsh=m_shArr(co); shcpy(rsh->a,SH(x),co); }
  B r0 = widenBitArr(x, co);
  usz csz = shProd(SH(r0),co,RNK(r0)) << elwBitLog(TI(r0,elType));
  u8 t;
  if      (csz==8)  t = t_i8slice;
  else if (csz==16) t = t_i16slice;
  else if (csz==32) t = t_i32slice;
  else if (csz==64) t = t_f64slice;
  else UD;
  TySlice* r = m_arr(sizeof(TySlice), t, ria);
  r->p = a(r0);
  r->a = tyany_ptr(r0);
  if (co>=1) arr_shSetUO((Arr*)r, co, rsh);
  else arr_shVec((Arr*)r);
  return taga(r);
}
static NOINLINE B cpyToElLog(B x, u8 xe, u8 lb) {
  switch(lb) { default: UD;
    case 0: return taga(cpyBitArr(x));
    case 3: return taga(elNum(xe)? cpyI8Arr(x) : cpyC8Arr(x));
    case 4: return taga(elNum(xe)? cpyI16Arr(x) : cpyC16Arr(x));
    case 5: return taga(elNum(xe)? cpyI32Arr(x) : cpyC32Arr(x));
    case 6: return taga(cpyF64Arr(x));
  }
}
static NOINLINE B2 splitCells(B n, B p, u8 mode) { // 0:∊ 1:⊐ 2:⊒
  #define SYMB (mode==0? "∊" : mode==1? "⊐" : "⊒")
  #define ARG_N (mode? "𝕩" : "𝕨")
  #define ARG_P (mode? "𝕨" : "𝕩")
  if (isAtm(p) || RNK(p)==0) thrF("%U: %U cannot have rank 0", SYMB, ARG_P);
  ur pr = RNK(p);
  if (isAtm(n)) n = m_unit(n);
  ur nr = RNK(n);
  if (nr < pr-1) thrF("%U: Rank of %U must be at least the cell rank of %U (%H ≡ ≢𝕨, %H ≡ ≢𝕩)", SYMB, ARG_N, ARG_P, mode?p:n, mode?n:p);
  ur pcr = pr-1;
  ur nco = nr-pcr;
  if (nco>0 && eqShPart(SH(n)+nco, SH(p)+1, pcr)) {
    u8 ne = TI(n,elType);
    u8 pe = TI(p,elType);
    if (ne<el_B && pe<el_B && elNum(ne)==elNum(pe)) {
      usz csz = arr_csz(p);
      u8 neb = elwBitLog(ne);
      u8 peb = elwBitLog(pe);
      u8 meb = neb>peb? neb : peb;
      ux rb = csz<<meb;
      if (rb!=0 && rb<=62) {
        if (n.u == p.u) { decG(p); n=toIntCell(n,rb,1); return (B2){.n=n, .p=incG(n)}; }
        if      (neb!=meb) n = cpyToElLog(n, ne, meb);
        else if (peb!=meb) p = cpyToElLog(p, pe, meb);
        return (B2){.n=toIntCell(n,rb,nco), .p=toIntCell(p,rb,1)};
      }
    }
  }
  return (B2){.n=toKCells(n,nco), .p=toCells(p)};
  
  #undef ARG_N
  #undef ARG_P
  #undef SYMB
}

static B reduceI32Width(B r, usz count) {
  return count<=I8_MAX? taga(cpyI8Arr(r)) : count<=I16_MAX? taga(cpyI16Arr(r)) : r;
}

static NOINLINE usz indexOfOne(B l, B e) {
    void* lp = tyany_ptr(l);
    usz wia = IA(l);
    u8 v8; u16 v16; u32 v32; f64 v64f;
    switch(TI(l,elType)) { default: UD;
      case el_bit: if (!q_bit(e)) return wia;  return bit_find(lp,wia,o2bG(e));
      case el_i8:  if (!q_i8 (e)) return wia;  v8  = ( u8)( i8)o2iG(e); goto chk8;
      case el_i16: if (!q_i16(e)) return wia;  v16 = (u16)(i16)o2iG(e); goto chk16;
      case el_i32: if (!q_i32(e)) return wia;  v32 = (u32)(i32)o2iG(e); goto chk32;
      case el_f64: if (!q_f64(e)) return wia;  v64f=           o2fG(e); goto chk64f;
      case el_c8:  if (!q_c8 (e)) return wia;  v8  = ( u8)     o2cG(e); goto chk8;
      case el_c16: if (!q_c16(e)) return wia;  v16 = (u16)     o2cG(e); goto chk16;
      case el_c32: if (!q_c32(e)) return wia;  v32 = (u32)     o2cG(e); goto chk32;
      case el_B: {
        if (isF64(e) && sizeof(B)==sizeof(f64)) { // TODO could also have character atom case
          if ((lp = arr_bptr(l)) == NULL) goto generic;
          v64f = o2fG(e);
          goto chk64f;
        }
        generic:;
        SGetU(l)
        for (usz i = 0; i < wia; i++) if (equal(GetU(l,i), e)) return i;
        return wia;
      }
    }
    
    #if SINGELI_SIMD
      chk8:   return simd_search_u8 (lp, v8,   wia);
      chk16:  return simd_search_u16(lp, v16,  wia);
      chk32:  return simd_search_u32(lp, v32,  wia);
      chk64f: return simd_search_f64(lp, v64f, wia);
    #else
      chk8:   for (ux i=0; i<wia; i++) { if ((( u8*)lp)[i]== v8 ) return i; } return wia;
      chk16:  for (ux i=0; i<wia; i++) { if (((u16*)lp)[i]==v16 ) return i; } return wia;
      chk32:  for (ux i=0; i<wia; i++) { if (((u32*)lp)[i]==v32 ) return i; } return wia;
      chk64f: for (ux i=0; i<wia; i++) { if (((f64*)lp)[i]==v64f) return i; } return wia;
    #endif
}

#define CHECK_CHRS_ELSE /* runs block if arguments are numerical; goes to chrEls if arguments are char arrs, updating we/xe to integers; widens mixed c8,c16 to c16,c16 */ \
  if (!elNum(we)) {                      \
    if (elChr(we)) {                     \
      if (elNum(xe)) goto none_found;    \
      if (we!=xe && we<=el_c16 && xe<=el_c16) { /* LUT uses signed integers, so needs equal-width args if we're gonna give unsigned ones */ \
        if (we>xe) x=taga(cpyC16Arr(x)); \
        else       w=taga(cpyC16Arr(w)); \
        we = xe = el_i16;                \
        goto chrEls;                     \
      }                                  \
      if (elChr(xe)) {                   \
        we-= el_c8-el_i8;                \
        xe-= el_c8-el_i8; goto chrEls;   \
      }                                  \
    }                                    \
  } else if (!elNum(xe)) {               \
    if (elChr(xe)) goto none_found;      \
  } else

B indexOf_c2(B t, B w, B x) {
  bool split = 0; (void) split;
  if (RARE(!isArr(w) || RNK(w)!=1)) {
    split = 1;
    B2 t = splitCells(x, w, 1);
    w = t.p;
    x = t.n;
  }
  if (!isArr(x) || RNK(x)==0) {
    B el = isArr(x)? IGetU(x,0) : x;
    usz res = indexOfOne(w, el);
    decG(w); dec(x);
    B r = m_vec1(m_f64(res));
    arr_shAtm(a(r)); // replaces shape
    return r;
  } else {
    u8 we = TI(w,elType); usz wia = IA(w);
    u8 xe = TI(x,elType); usz xia = IA(x);
    if (wia==0 || xia==0) {
      none_found:
      decG(w); return i64EachDec(wia, x);
    }

    CHECK_CHRS_ELSE { chrEls:
      if (wia>32 && xia<=(we<=el_i8?1:3)) {
        SGetU(x);
        B r;
        #define IND(T) \
          T* rp; r = m_##T##arrc(&rp, x); \
          for (usz i=0; i<xia; i++) rp[i] = indexOfOne(w, GetU(x,i))
        if (xia<I32_MAX) { IND(i32); r=reduceI32Width(r, wia); }
        else { IND(f64); }
        #undef IND
        decG(w); decG(x); return r;
      }

      if (we==el_bit) {
        u64* wp = bitarr_ptr(w);
        u64 w0 = 1 & wp[0];
        u64 i = bit_find(wp, wia, !w0); decG(w);
        if (i!=wia) incG(x);
        B r =                         C2i(mul, wia  , C2i(ne,  w0, x)) ;
        return i==wia? r : C2(sub, r, C2i(mul, wia-i, C2i(eq, !w0, x)));
      }
      
      #if SINGELI_AVX2
      if (xia>=512 && wia>1 && el_i8<=xe && xe<=el_i32 && wia<(xe==el_i8?64:16) && we<=xe && !elChr(TI(x,elType))) {
        B g = C1(reverse, C1(gradeDown, incG(w)));
        w = C2(select, incG(g), w);
        switch (xe) { default:UD; case el_i8:w=toI8Any(w);break; case el_i16:w=toI16Any(w);break; case el_i32:w=toI32Any(w);break; }
        i8* rp; B r = m_i8arrc(&rp, x);
        avx2_indexOf_sort[xe-el_i8](rp, tyany_ptr(w), wia, tyany_ptr(x), xia);
        r = C2(select, r, C2(join, g, m_i8(wia)));
        decG(w); decG(x); return r;
      }
      #endif
      if (wia<=4 && xia>16) {
        SGetU(w);
        #define XEQ(I) C2(ne, GetU(w,I), incG(x))
        B r = XEQ(wia-1);
        for (usz i=wia-1; i--; ) r = C2(mul, XEQ(i), C2i(add, 1, r));
        #undef XEQ
        decG(w); decG(x); return r;
      }
      
      if (xia+wia>20 && we<=el_i16 && xe<=el_i16) {
        B r;
        #if SINGELI
        if (wia>256 && we==el_i8 && xe==el_i8) {
          TALLOC(u8, tab, 256*(1+sizeof(usz))); usz* ind = (usz*)(tab+256);
          void* fp = tyany_ptr(x);
          simd_index_tab_u8(tyany_ptr(w), wia, fp, xia, tab, ind);
          decG(w);
          #define LOOKUP(TY) \
            TY* rp; r = m_##TY##arrc(&rp, x); \
            for (usz i=0; i<xia; i++) rp[i]=ind[((u8*)fp)[i]];
          if (wia<=INT16_MAX) { LOOKUP(i16) } else { LOOKUP(i32) }
          #undef LOOKUP
          TFREE(tab); decG(x);
          return r;
        }
        #endif
        if      (wia<= INT8_MAX) { TABLE(w, x,  i8, wia, i) }
        else if (wia<=INT16_MAX) { TABLE(w, x, i16, wia, i) }
        else                     { TABLE(w, x, i32, wia, i) }
        return r;
      }
      #if SINGELI
      if (we==xe && wia<=INT32_MAX && (we==el_i32 || (we==el_f64 && (split || canCompare64_norm2(&w,wia,&x,xia))))) {
        i32* rp; B r = m_i32arrc(&rp, x);
        if (si_indexOf_c2_hash[we-el_i32](rp, tyany_ptr(w), wia, tyany_ptr(x), xia)) {
          decG(w); decG(x); return reduceI32Width(r, wia);
        }
        decG(r);
      }
      #endif
    }
    
    i32* rp; B r = m_i32arrc(&rp, x);
    H_b2i* map = m_b2i(64);
    SGetU(x)
    SGetU(w)
    for (usz i = 0; i < wia; i++) {
      bool had; u64 p = mk_b2i(&map, GetU(w,i), &had);
      if (!had) map->a[p].val = i;
    }
    for (usz i = 0; i < xia; i++) rp[i] = getD_b2i(map, GetU(x,i), wia);
    free_b2i(map); decG(w); decG(x);
    return reduceI32Width(r, wia);
  }
}

GLOBAL B enclosed_0, enclosed_1;
B memberOf_c2(B t, B w, B x) {
  bool split = 0; (void) split;
  if (isAtm(x) || RNK(x)!=1) {
    split = 1;
    B2 t = splitCells(w, x, false);
    w = t.n;
    x = t.p;
  }
  if (isAtm(w)) goto single;
  
  ur wr = RNK(w);
  if (wr>0) goto many;
  
  B w0 = IGet(w, 0);
  dec(w);
  w = w0;
  goto single;
  
  B r;
  single: {
    r = incG(indexOfOne(x,w)==IA(x)? enclosed_0 : enclosed_1);
    dec(w);
    goto dec_x;
  }
  
  
  many: {
    u8 we = TI(w,elType); usz wia = IA(w);
    u8 xe = TI(x,elType); usz xia = IA(x);
    if (wia==0 || xia==0) {
      none_found:
      decG(x); return i64EachDec(0, w);
    }

    CHECK_CHRS_ELSE { chrEls:
      if (xia>32 && wia<=(xe<=el_i8?1:xe==el_i32?4:6)) {
        SGetU(w);
        i8* rp; r = m_i8arrc(&rp, w);
        for (usz i=0; i<wia; i++) rp[i] = indexOfOne(x, GetU(w,i)) < xia;
        decG(w); decG(x); return taga(cpyBitArr(r));
      }

      #define WEQ(V) C2(eq, incG(w), V)
      if (xe==el_bit) {
        u64* xp = bitarr_ptr(x);
        u64 x0 = 1 & xp[0];
        r = WEQ(m_usz(x0));
        if (bit_has(xp, xia, !x0)) r = C2(or, r, WEQ(m_usz(!x0)));
        decG(w); goto dec_x;
      }
      
      #if SINGELI_AVX2
      if (we>=el_i16 && wia>=32>>(we-el_i8) && xia>1 && ((we==el_i16 && xia<32) || (we==el_i32 && xia<16)) && xe<=we && !elChr(TI(x,elType))) {
        x = C1(and, x); // sort
        if (xe<we) switch (we) { default:UD; case el_i16:x=toI16Any(x);break; case el_i32:x=toI32Any(x);break; }
        u64* rp; r = m_bitarrc(&rp, w);
        avx2_member_sort[we-el_i16](rp, tyany_ptr(x), xia, tyany_ptr(w), wia);
        decG(w); goto dec_x;
      }
      #endif
      if (xia<=(we==el_i8?1:we==el_i16?4:8) && wia>16) {
        SGetU(x);
        r = WEQ(GetU(x,0));
        for (usz i=1; i<xia; i++) r = C2(or, r, WEQ(GetU(x,i)));
        decG(w); goto dec_x;
      }
      #undef WEQ
      
      if (xia+wia>20 && we<=el_i16 && xe<=el_i16) {
        #if SINGELI
        if (we==el_i8 && xe==el_i8) {
          TALLOC(u8, tab, 256);
          u64* rp; r = m_bitarrc(&rp, w);
          simd_member_u8(tyany_ptr(x), xia, tyany_ptr(w), wia, rp, tab);
          TFREE(tab); decG(w);
          goto dec_x;
        }
        #endif
        TABLE(x, w, i8, 0, 1)
        return taga(cpyBitArr(r));
      }
      #if SINGELI
      if (we==xe && (we==el_i32 || (we==el_f64 && (split || canCompare64_norm2(&w,wia,&x,xia))))) {
        i8* rp; B r = m_i8arrc(&rp, w);
        if (si_memberOf_c2_hash[we-el_i32](rp, tyany_ptr(x), xia, tyany_ptr(w), wia)) {
          decG(w); decG(x); return taga(cpyBitArr(r));
        }
        decG(r);
      }
      #endif
    }
    
    H_Sb* set = m_Sb(64);
    SGetU(x) SGetU(w)
    bool had;
    for (usz i = 0; i < xia; i++) mk_Sb(&set, GetU(x,i), &had);
    u64* rp; r = m_bitarrc(&rp, w);
    for (usz i = 0; i < wia; i++) bitp_set(rp, i, has_Sb(set, GetU(w,i)));
    free_Sb(set); decG(w);
    goto dec_x;
  }
  
  dec_x:;
  decG(x);
  return r;
}

B count_c2(B t, B w, B x) {
  bool split = 0; (void) split;
  if (RARE(!isArr(w) || RNK(w)!=1)) {
    split = 1;
    B2 t = splitCells(x, w, 2);
    w = t.p;
    x = t.n;
  }
  
  if (!isArr(x) || IA(x)<=1 || IA(w)==0) return C2(indexOf, w, x);
  u8 we = TI(w,elType); usz wia = IA(w);
  u8 xe = TI(x,elType); usz xia = IA(x);
  i32* rp; B r = m_i32arrc(&rp, x);
  TALLOC(usz, wnext, wia+1);
  wnext[wia] = wia;
  if (wia==0 || xia==0) {
    none_found:
    TFREE(wnext); decG(r);
    decG(w); return i64EachDec(wia, x);
  }
  CHECK_CHRS_ELSE { chrEls:
    if (we<=el_i16 && xe<=el_i16) {
      if (we==el_bit) { w = toI8Any(w); we = el_i8; }
      if (xe==el_bit) { x = toI8Any(x); xe = el_i8; }
      usz it = elRange(we);    // Range of writes
      usz ft = elRange(xe);    // Range of lookups
      usz t = it>ft? it : ft;  // Table allocation width
      TALLOC(i32, tab0, t); i32* tab = tab0 + t/2;
      usz m=wia, n=xia;
      void* ip = tyany_ptr(w);
      void* fp = tyany_ptr(x);
      // Initialize
      if (xe==el_i16 && n<ft/(64/sizeof(i32)))
           { for (usz i=0; i<n; i++) tab[((i16*)fp)[i]]=wia; }
      else { for (i64 i=0; i<ft; i++) tab[i-ft/2]=wia; }
      // Set
      #define SET(T) for (usz i=m; i--; ) { i32* p=tab+((T*)ip)[i]; wnext[i]=*p; *p=i; }
      if (we==el_i8) { SET(i8) } else { SET(i16) }
      #undef SET
      // Lookup
      #define GET(T) for (usz i=0; i<n; i++) { i32* p=tab+((T*)fp)[i]; *p=wnext[rp[i]=*p]; }
      if (xe==el_i8) { GET(i8) } else { GET(i16) }
      #undef GET
      TFREE(tab0);
      goto dec_nwx;
    }
    #if SINGELI
    else if (we==xe && wia<=INT32_MAX && (we==el_i32 || (we==el_f64 && (split || canCompare64_norm2(&w,wia,&x,xia)))) &&
        si_count_c2_hash[we-el_i32](rp, tyany_ptr(w), wia, tyany_ptr(x), xia, (u32*)wnext)) {
      goto dec_nwx;
    }
    #endif
  }
  
  H_b2i* map = m_b2i(64);
  SGetU(x)
  SGetU(w)
  for (usz i = wia; i--; ) {
    bool had; u64 p = mk_b2i(&map, GetU(w,i), &had);
    wnext[i] = had ? map->a[p].val : wia;
    map->a[p].val = i;
  }
  for (usz i = 0; i < xia; i++) {
    bool had; u64 p = getQ_b2i(map, GetU(x,i), &had);
    usz j = wia;
    if (had) { j = map->a[p].val; map->a[p].val = wnext[j]; }
    rp[i] = j;
  }
  free_b2i(map);
  
  dec_nwx:;
  TFREE(wnext); decG(w); decG(x);
  return reduceI32Width(r, wia);
}
#undef CHECK_CHRS_ELSE

// if nanBad and input contains a NaN, doesn't consume and returns m_f64(0)
// otherwise, consumes and returns an array with -0 (and NaNs if !nanBad) normalized
B asNormalized(B x, usz n, bool nanBad) {
  f64* fp = f64any_ptr(x);
  ux i = 0;
  #if SINGELI_SIMD
    i = simd_search_normalizable(fp, n);
    if (i!=n) goto some;
  #else
    for (; i < n; i++) if (r_f64u(fp[i])==r_f64u(-0.0) || fp[i]!=fp[i]) goto some;
  #endif
  return x;
  
  some:;
  f64* rp;
  B r;
  if (TY(x)==t_f64arr && reusable(x)) {
    rp = fp;
    r = x;
  } else {
    r = m_f64arrc(&rp, x);
    COPY_TO(rp, el_f64, 0, x, 0, i);
  }
  
  if (nanBad) {
    #if SINGELI_SIMD
      if (RARE(simd_copy_ordered(rp+i, fp+i, n-i))) goto bad;
    #else
      for (; i < n; i++) {
        if (RARE(fp[i]!=fp[i])) goto bad;
        rp[i] = fp[i]+0.0;
      }
    #endif
  } else {
    vfor (; i < n; i++) rp[i] = normalizeFloat(fp[i]);
  }
  
  if (r.u!=x.u) decG(x);
  return r;
  
  bad:
  if (r.u!=x.u) mm_free(v(r));
  return m_f64(0);
}

bool getRangeBool(void* xp, i64* res, u64 ia) {
  assert(ia>0);
  u64 x0 = 1 & ((u64*)xp)[0];
  if (bit_has(xp, ia, !x0)) { res[0]=0; res[1]=1; }
  else                      { res[0]=res[1]=x0; }
  return true;
}
void search_init(void) {
  { u64* p; Arr* a=m_bitarrp(&p, 1); arr_shAtm(a); *p= 0;    gc_add(enclosed_0=taga(a)); }
  { u64* p; Arr* a=m_bitarrp(&p, 1); arr_shAtm(a); *p=~0ULL; gc_add(enclosed_1=taga(a)); }
  getRange_fns[0] = getRangeBool;
  #if SINGELI
    for (i32 i=0; i<4; i++) getRange_fns[i+1] = simd_getRangeRaw[i];
  #else
    getRange_fns[1] = getRange_i8;
    getRange_fns[2] = getRange_i16;
    getRange_fns[3] = getRange_i32;
    getRange_fns[4] = getRange_f64;
  #endif
}


// •HashMap implementation
typedef struct HashMap {
  struct Value;
  u64 pop; // count of defined entries
  u64 sh;  // shift to turn hash into index
  u64 sz;  // count of allocated entries, a power of 2
  u64 a[]; // lower 32 bits: index into keys/vals; upper 32 bits: upper 32 bits of hash
} HashMap;
static u64 hashmap_size(usz sh) { return ((u64)1 << (64-sh)) + 32; }
static const u64 empty = ~(u64)0;
static const u64 hmask = ~(u32)0;

usz hashmap_count(B hash) { return c(HashMap, hash)->pop; }

static HashMap* hashmap_alloc(usz pop, usz sh, u64 sz) {
  HashMap* map = mm_alloc(fsizeof(HashMap,a,u64,sz), t_hashmap);
  map->pop = pop; map->sh = sh; map->sz = sz;
  memset64(map->a, empty, sz);
  return map;
}

static NOINLINE HashMap* hashmap_resize(HashMap* old_map) {
  usz sh = old_map->sh - 1;
  if (sh < 32) thrM("•HashMap: hash size maximum exceeded");
  u64 sz = hashmap_size(sh);
  HashMap* map = hashmap_alloc(old_map->pop, sh, sz);
  for (u64 i=0, j=-(u64)1; i<old_map->sz; i++) {
    u64 h = old_map->a[i];
    if (h == empty) continue;
    j++; u64 jh = h>>sh; if (j < jh) j = jh;
    map->a[j] = h;
  }
  mm_free((Value*)old_map);
  return map->a[sz-1]==empty ? map : hashmap_resize(map);
}

static NOINLINE void hashmap_compact(B* vars) {
  B k = vars[0]; B* keys = harr_ptr(k);
  B v = vars[1]; B* vals = harr_ptr(v);
  assert(reusable(k) && reusable(v));
  usz l0 = IA(k); usz l = l0;
  while (l>0 && q_N(keys[l-1])) l--;
  TALLOC(usz, ind_map, l+1);
  ind_map[0] = -1; // Index by i+1 to maintain empty entries
  usz j = 0;       // Number of entries seen
  for (usz i=0; i<l; i++) {
    B ki = keys[i];
    keys[j] = ki;
    vals[j] = vals[i];
    ind_map[i+1] = j;
    j += !q_N(ki);
  }
  IA(k) = IA(v) = j;
  FINISH_OVERALLOC_A(k, j*sizeof(B), (l0-j)*sizeof(B));
  FINISH_OVERALLOC_A(v, j*sizeof(B), (l0-j)*sizeof(B));
  if (j > 0) {
    HashMap* map = c(HashMap, vars[2]);
    assert(j == map->pop);
    u64 sz = map->sz;
    u64* hp = map->a;
    for (u64 i=0; i<sz; i++) {
      u64 h = hp[i];
      hp[i] = (h&~hmask) | ind_map[(u32)h + 1];
    }
  }
  TFREE(ind_map);
}
B hashmap_keys_or_vals(B* vars, usz which) {
  assert(which < 2);
  B r = vars[which];
  if (c(HashMap, vars[2])->pop == IA(r)) return r;
  if (!reusable(vars[0])) vars[0] = taga(cpyHArr(vars[0]));
  if (!reusable(vars[1])) vars[1] = taga(cpyHArr(vars[1]));
  hashmap_compact(vars);
  return vars[which];
}

// Expects already defined: u64* hp, u64 sh, B* keys
#define HASHMAP_FIND(X, FOUND) \
  u64 h = bqn_hash(X, wy_secret);    \
  u64 m = hmask;                     \
  u64 v = h &~ m;                    \
  u64 j = h >> sh;                   \
  u64 u; while ((u=hp[j]) < v) j++;  \
  while (u < (v|m)) {                \
    usz i = u&m;                     \
    if (equal(X, keys[i])) { FOUND } \
    u = hp[++j];                     \
  }
// Expects usz i to be the index if new (i in FOUND is index where found)
#define HASHMAP_INSERT(X, FOUND) \
  HASHMAP_FIND(X, FOUND)                                            \
  u64 je=j; while (u!=empty) { u64 s=u; je++; u=hp[je]; hp[je]=s; } \
  hp[j] = v | i;

B hashmap_build(B key_arr, usz n) {
  usz sh = CLZ(n|16)-1;
  u64 sz = hashmap_size(sh);
  HashMap* map = hashmap_alloc(n, sh, sz);
  u64* hp = map->a;
  B* keys = harr_ptr(key_arr);
  for (usz i=0; i<n; i++) {
    B key = keys[i];
    HASHMAP_INSERT(key, thrM("•HashMap: 𝕨 contained duplicate keys");)
    if (RARE(je == sz-1)) {
      map=hashmap_resize(map);
      hp=map->a; sh=map->sh; sz=map->sz;
    }
  }
  return tag(map, OBJ_TAG);
}

B hashmap_lookup(B* vars, B w, B x) {
  HashMap* map = c(HashMap, vars[2]);
  u64* hp = map->a; u64 sh = map->sh;
  B* keys = harr_ptr(vars[0]);
  HASHMAP_FIND(x, dec(w); dec(x); return inc(harr_ptr(vars[1])[i]);)
  if (q_N(w)) thrM("(hashmap).Get: key not found");
  dec(x); return w;
}

void hashmap_set(B* vars, B w, B x) {
  HashMap* map = c(HashMap, vars[2]);
  u64* hp = map->a; u64 sh = map->sh;
  usz i = IA(vars[0]);
  B* keys = harr_ptr(vars[0]);
  HASHMAP_INSERT(
    w,
    B* s = harr_ptr(vars[1])+i; dec(*s); dec(w); *s=x; return;
  )
  map->pop++;
  if (map->pop>>(64-3-sh)>7 || je==map->sz-1) { // keep load <= 7/8
    vars[2] = bi_N; // hashmap_resize might free then alloc
    map = hashmap_resize(map);
    vars[2] = tag(map, OBJ_TAG);
    hp=map->a; sh=map->sh;
  }
  vars[0] = vec_addN(vars[0], w);
  vars[1] = vec_addN(vars[1], x);
}

void hashmap_delete(B* vars, B x) {
  HashMap* map = c(HashMap, vars[2]);
  u64* hp = map->a; u64 sh = map->sh;
  B kb = vars[0]; B* keys = harr_ptr(kb);
  HASHMAP_FIND(x,
    dec(x);
    do {
      u64 jp=j; j++;
      u=hp[j]; if (u>>sh==j) u=empty;
      hp[jp]=u;
    } while (u!=empty);
    B vb = vars[1];
    if (!reusable(kb)) { vars[0]=kb=taga(cpyHArr(kb)); keys=harr_ptr(kb); }
    if (!reusable(vb)) { vars[1]=vb=taga(cpyHArr(vb)); }
    usz p = --(map->pop); dec(keys[i]); keys[i]=bi_N;
    B* s = harr_ptr(vb)+i; dec(*s); *s=bi_N;
    if (p==0 || p+32 < IA(kb)/2) hashmap_compact(vars);
    return;
  )
  thrM("(hashmap).Delete: key not found");
}
