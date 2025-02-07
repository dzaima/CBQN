// Self-search: Mark Firsts (∊), Occurrence Count (⊒), Classify (⊐), Deduplicate (⍷)

// Except for trivial cases, ⍷ is implemented as ∊⊸/
// Other functions use adaptations of the same set of methods

// Boolean cases (1-bit cells) all use special code, including ⍷
//   COULD vectorize boolean ⊒ with +`
// Sorted flags: start with r0⌾⊑»⊸≠𝕩 (r0←0 for ⊐, 1 otherwise)
//   ∊: ⊢; ⊐: +`; ⊒: ↕∘≠⊸(⊣-⌈`∘×)
//   COULD determine ⊒ result type by direct comparisons on 𝕩
// Brute force or all-pairs comparison for small lengths
//   Branchless, not vectorized (+´∧` structure for ⊐)
//   COULD use direct all-pairs filter, not ∊⊸/, for short ⍷
// Full-size table lookups for 1- and 2-byte 𝕩
//   1-byte ∊ and ⍷, and ⊐ via ⍷⊸⊐: SSSE3/AVX2 table
//     TRIED dedicated table constructor for ⊐, no significant speedup
//   2-byte table can be "sparse" initialized with an extra pass over 𝕩
//   4-byte ⊐ can use a small-range lookup table
//   COULD add small-range 4-byte tables for ∊ and ⊒
// Radix-assisted lookups are fallbacks for 4-byte ∊ and ⊒
//   COULD do radix-assisted ⊐ as ⍷⊸⊐ or similar
//   Specializes on constant top 1/2 bytes, but hashes make this rare
// Cells with 2≤n≤63 bits, ¬n∊8‿16‿32:
//   Pad to the nearest nice size (8, 16, 32, 64 bits)

// Specialized 4-byte and 8-byte hash tables
//   In-place resizing by factor of 4 based on measured collisions
//   Max collisions ensures bounded time spent here before giving up
//   First element used as sentinel (not good for ⊒)
//   COULD prefetch when table gets larger
// Generic hash table for other cases
//   Resizing is pretty expensive here

#include "../core.h"
#include "../utils/hash.h"
#include "../utils/talloc.h"
#include "../utils/calls.h"

extern B shape_c1(B, B);
extern B slash_c2(B, B, B);
extern B ud_c1(B, B);
extern B sub_c2(B, B, B);
extern B mul_c2(B, B, B);
extern B indexOf_c2(B, B, B);
extern B take_c2(B, B, B);
extern B scan_add_bool(B x, u64 ia);
extern B scan_max_num(B x, u8 xe, u64 ia);

// From search.c
extern NOINLINE void memset32(u32* p, u32 v, usz l);
extern NOINLINE void memset64(u64* p, u64 v, usz l);

#if SINGELI
  #define SINGELI_FILE selfsearch
  #include "../utils/includeSingeli.h"

  #define TRY_HASHTAB_RET(NAME, W, RET) \
    if (NAME##_c1_hash##W(rp, (u##W*)xv, n)) { decG(x); return RET; }
#else
  #define TRY_HASHTAB_RET(NAME, W, RET) (void)0;
#endif

static u8 fast_sameness(B x, u8 xe) { // 0: unknown; 1: all same; 2: all different
  assert(IA(x)>0);
  if (!FL_HAS_ALL(x, fl_asc|fl_dsc)) return 0;
  B x0 = IGetU(x,0);
  return equal(x0, x0)? 1 : 2;
}

static inline bool use_sorted(B x, u8 logw) {
  if (!FL_HAS(x, fl_asc|fl_dsc)) return 0;
  if (logw==6) return TI(x, elType) == el_f64;
  return 3<=logw & logw<=5;
}
static inline B shift_ne(B x, usz n, u8 lw, bool r0) { // consumes x
  u64* rp; B r = m_bitarrv(&rp, n);
  u8* xp = tyany_ptr(x);
  u8 lb = lw - 3;
  CMP_AA_IMM(ne, el_i8+lb, rp, xp-(1<<lb), xp, n);
  bitp_set(rp, 0, r0);
  decG(x); return r;
}

B asNormalized(B x, usz n, bool nanBad); // from search.c
SHOULD_INLINE bool canCompare64_norm(B* x, void** xp, usz n) {
  u8 e = TI(*x,elType);
  if (e == el_B) return 0;
  if (e == el_f64) {
    B r = asNormalized(*x, n, true);
    if (r.u == m_f64(0).u) return 0;
    *x = r;
    *xp = tyany_ptr(r);
  }
  return 1;
}
static bool shouldWidenBitarr(B x, usz csz) { // assumes cells won't anymore have sizes of 0, 8, or 16 bits
  u8 xe = TI(x,elType);
  ux bcsz = ((ux)csz)<<elwBitLog(xe);
  assert(csz!=0 && bcsz!=1 && bcsz!=8 && bcsz!=16);
  if (bcsz<64 && bcsz!=32) { assert(xe!=el_B); return true; } // not el_B because csz>0 → csz*sizeof(B) >= 64
  return false;
}

#define GRADE_UD(U,D) U
#include "radix.h"
u8 radix_offsets_2_usz(usz* c0, u32* v0, usz n) {
  usz rx = 256;
  usz* c1 = c0 + rx;
  // Count keys
  for (usz j=0; j<2*rx+1; j++) c0[j] = 0;
  for (usz i=0; i<n; i++) { u32 v=v0[i]; (c0+1)[(u8)(v>>16)]++; (c1+1)[(u8)(v>>24)]++; }
  u32 v=v0[0];
  // Inclusive prefix sum; note c offsets above
  if ((c1+1)[(u8)(v>>24)] < n) { c1[0]-=n; RADIX_SUM_2_usz; return 2; }
  if ((c0+1)[(u8)(v>>16)] < n) {           RADIX_SUM_1_usz; return 1; }
  return 0;
}
#undef GRADE_UD
#define RADIX_LOOKUP_32(INIT, SETTAB) \
  u8 bytes = radix_offsets_2_usz(c0, v0, n);                                                    \
  usz tim = tn/(64/sizeof(*tab)); /* sparse table init max */                                   \
  if (bytes==0) {                                                                               \
    if (n<tim) for (usz i=0; i< n; i++) tab[(u16)v0[i]]=INIT;                                   \
    else       for (usz j=0; j<tn; j++) tab[j]=INIT;                                            \
    for (usz i=0; i<n; i++) { u32 j=(u16)v0[i]; r0[i]=tab[j]; tab[j]SETTAB; }                   \
  } else {                                                                                      \
    if (bytes==1) { v1=v2; r1=r2; }                                                             \
    for (usz i=0; i<n; i++) { u32 v=v0[i]; u8 k=k0[i]=(u8)(v>>16); usz c=c0[k]++; v1[c]=v; }    \
    if (bytes==1) {                                                                             \
      /* Table lookup, getting radix boundaries from c0 */                                      \
      for (usz i=0; i<n; ) {                                                                    \
        usz l=c0[(u8)(v1[i]>>16)];                                                              \
        if (l-i < tim) for (usz ii=i; ii<l; ii++) tab[(u16)v1[ii]]=INIT;                        \
        else           for (usz  j=0; j<tn;  j++) tab[j]=INIT;                                  \
        for (; i<l; i++) { u32 j=(u16)v1[i]; r1[i]=tab[j]; tab[j]SETTAB; }                      \
      }                                                                                         \
    } else {                                                                                    \
      /* Radix move */                                                                          \
      for (usz i=0; i<n; i++) { u32 v=v1[i]; u8 k=k1[i]=(u8)(v>>24); usz c=c1[k]++; v2[c]=v; }  \
      /* Table lookup */                                                                        \
      u32 tv=v2[0]>>16; v2[n]=~v2[n-1];                                                         \
      for (usz l=0, i=0; l<n; ) {                                                               \
        for (;    ; l++) { u32 v=v2[l], t0=tv; tv=v>>16; if (tv!=t0) break; tab[(u16)v]=INIT; } \
        for (; i<l; i++) { u32 j=(u16)v2[i]; r2[i]=tab[j]; tab[j]SETTAB; }                      \
      }                                                                                         \
      /* Radix unmove; back up c0 to account for increments in radix step  */                   \
      *--c1=0; for (usz i=0; i<n; i++) { r1[i]=r2[c1[k1[i]]++]; }                               \
    }                                                                                           \
    *--c0=0; for (usz i=0; i<n; i++) { r0[i]=r1[c0[k0[i]]++]; }                                 \
  }                                                                                             \
  decG(x); TFREE(alloc);

extern void (*const simd_mark_firsts_u8)(void*,uint64_t,void*,void*);
extern u64  (*const simd_deduplicate_u8)(void*,uint64_t,void*,void*);

#define PRUP ptr_roundUpToEl
B memberOf_c1(B t, B x) {
  if (isAtm(x) || RNK(x)==0) thrM("∊𝕩: 𝕩 cannot have rank 0");
  u64 n = *SH(x);
  if (n<=1) { decG(x); return n ? taga(arr_shVec(allOnes(1))) : emptyIVec(); }
  
  usz csz = arr_csz(x);
  u8 lw = cellWidthLog(x);
  void* xv = tyany_ptr(x);
  if (lw==0 || csz==0) {
    usz i = csz==0? n : bit_find(xv, n, 1 &~ *(u64*)xv); decG(x);
    B r = taga(arr_shVec(allZeroes(n)));
    u64* rp = tyany_ptr(r);
    rp[0]=1; if (i<n) bitp_set(rp, i, 1);
    return r;
  }
  
  u8 sameness = fast_sameness(x, TI(x,elType));
  if (sameness == 1) goto allSame;
  if (RARE(sameness == 2)) {
    decG(x);
    return taga(arr_shVec(allOnesFl(n)));
  }
  
  if (use_sorted(x, lw)) {
    return shift_ne(x, n, lw, 1);
  }
  #define BRUTE(T) assert(n <= 64); \
    i##T* xp = xv;                                                     \
    u64 rv = 1;                                                        \
    for (usz i=1; i<n; i++) {                                          \
      bool c=1; i##T xi=xp[i];                                         \
      PLAINLOOP for (usz j=0; j<i; j++) c &= xi!=xp[j];                \
      rv |= c<<i;                                                      \
    }                                                                  \
    decG(x); u64* rp; B r = m_bitarrv(&rp, n); rp[0] = rv;             \
    return r;
  #define LOOKUP(T) \
    usz tn = 1<<T;                                                     \
    u##T* xp = (u##T*)xv;                                              \
    i8* rp; B r = m_i8arrv(&rp, n);                                    \
    TALLOC(u8, tab, tn);                                               \
    if (T>8 && n<tn/64) for (usz i=0; i<n;  i++) tab[xp[i]]=1;         \
    else                for (usz j=0; j<tn; j++) tab[j]=1;             \
    for (usz i=0; i<n; i++) { u##T j=xp[i]; rp[i]=tab[j]; tab[j]=0; }  \
    decG(x); TFREE(tab);                                               \
    return taga(cpyBitArr(r))
  if (lw==3) { if (n<8) { BRUTE(8); } else {
    #if SINGELI
    TALLOC(u8, tab, 256);
    u64* rp; B r = m_bitarrv(&rp, n);
    simd_mark_firsts_u8(xv, n, rp, tab);
    TFREE(tab); decG(x);
    return r;
    #else
    LOOKUP(8);
    #endif
  } }
  if (lw==4) { if (n<8) { BRUTE(16); } else { LOOKUP(16); } }
  #undef LOOKUP
  #define TRY_HASHTAB(W) TRY_HASHTAB_RET(memberOf, W, taga(cpyBitArr(r)))
  if (lw==5) {
    if (n<12) { BRUTE(32); }
    i8* rp; B r = m_i8arrv(&rp, n);
    TRY_HASHTAB(32)

    // Radix-assisted lookup when hash table gives up
    usz rx = 256, tn = 1<<16; // Radix; table length
    u32* v0 = (u32*)xv;
    i8* r0 = rp;
    
    TALLOC(u8, alloc, 6*n + (4 + (tn>3*n?tn:3*n) + (2*rx+1)*sizeof(usz)) + sizeof(u32));
    //                                         timeline
    // Allocations               len  count radix hash deradix     bytes  layout:
    usz *c0 =      (usz*)(alloc)+1; // rx   [+++................]     c0   rx  #
    usz *c1 =      (usz*)(c0+rx);   // rx    [++................]     c1   rx   #
    u8  *k0 =      (u8 *)(c1+rx);   //  n        [+.............]     k0    n    ##
    u32 *v2 = PRUP((u32*)(k0+n));   //  n+1       [+.......]          v2  4*n+4    ########
    u8  *k1 =      (u8 *)(v2+n+1);  //  n         [+............]     k1    n              ##
    u32 *v1 =      (u32*)(k1);      //  n        [+-]                 v1  4*n              ########
    u8  *r2 =      (u8 *)(v2);      //  n              [+.....]       r2    n      ##
    u8  *r1 =      (u8 *)(k1+n);    //  n                   [+..]     r1    n                ##
    u8  *tab=      (u8 *)(r1);      // tn              [+]            tab  tn                #####
   
    RADIX_LOOKUP_32(1, =0)
    return taga(cpyBitArr(r));
  }
  if (lw==6 && canCompare64_norm(&x, &xv, n)) {
    if (n<20) { BRUTE(64); }
    i8* rp; B r = m_i8arrv(&rp, n);
    TRY_HASHTAB(64)
    decG(r); // Fall through
  }
  #undef TRY_HASHTAB
  #undef BRUTE
  
  if (RNK(x)>1) {
    if (shouldWidenBitarr(x, csz)) return C1(memberOf, widenBitArr(x, 1));
    x = toCells(x);
  }
  
  assert(n > 0);
  SGetU(x)
  usz i = 0;
  {
    B x0 = GetU(x, 0);
    while (i < n) {
      B c = GetU(x, i);
      if (!equal(c, x0)) {
        u64* rp; B r = m_bitarrv(&rp, n);
        memset(rp, 0, (i+7)>>3);
        rp[0] = 1;
        
        H_Sb* set = m_Sb(64);
        ins_Sb(&set, x0);
        for (; i < n; i++) bitp_set(rp, i, !ins_Sb(&set, GetU(x,i)));
        free_Sb(set); decG(x);
        return r;
      }
      i++;
    }
    allSame:;
    if (v(x)->refc > 1) {
      FL_SET(x, fl_asc|fl_dsc);
      incByG(x, -1);
    } else {
      decG(x);
    }
    Arr* r = allZeroes(n);
    bitarrv_ptr((TyArr*) r)[0] = 1;
    return taga(arr_shVec(r));
  }
}

B count_c1(B t, B x) {
  if (isAtm(x) || RNK(x)==0) thrM("⊒𝕩: 𝕩 cannot have rank 0");
  u64 n = *SH(x);
  if (n<=1) { decG(x); return n ? taga(arr_shVec(allZeroes(1))) : emptyIVec(); }
  if (n>(usz)I32_MAX+1) thrM("⊒𝕩: 𝕩 length >2⋆31 not supported");
  
  usz csz = arr_csz(x);
  if (csz==0) { decG(x); return C1(ud, m_f64(n)); }
  u8 lw = cellWidthLog(x);
  if (lw==0) {
    u64* xp = bitany_ptr(x);
    B r;
    #define COUNT_BOOL(T) \
      T* rp; r = m_##T##arrv(&rp, n);         \
      usz n1 = 0;                             \
      for (usz i=0; i<n; ) {                  \
        u64 bb = xp[i/64];                    \
        for (usz e=n-i<64?n:i+64; i<e; i++) { \
          bool b = bb&1; bb>>=1;              \
          rp[i] = b? n1 : i-n1;               \
          n1 += b;                            \
        }                                     \
      }
    if      (n <= 128)   { COUNT_BOOL(i8) }
    else if (n <= 1<<15) { COUNT_BOOL(i16) }
    else                 { COUNT_BOOL(i32) }
    decG(x); return r;
    #undef COUNT_BOOL
  }
  if (use_sorted(x, lw) && n>16 && (lw>4 || n<1<<16)) { // ↕∘≠(⊣-⌈`∘×)∊
    B c = shift_ne(x, n, lw, 1);
    B i = C1(ud, m_f64(n));
    B m = C2(mul, c, incG(i));
    return C2(sub, i, scan_max_num(m, TI(m,elType), n));
  }
  void* xv = tyany_ptr(x);
  #define BRUTE(T) \
    i##T* xp = xv;                                             \
    i8* rp; B r = m_i8arrv(&rp, n); rp[0]=0;                   \
    for (usz i=1; i<n; i++) {                                  \
      usz c=0; i##T xi=xp[i];                                  \
      PLAINLOOP for (usz j=0; j<i; j++) c += xi==xp[j];        \
      rp[i] = c;                                               \
    }                                                          \
    decG(x); return r;
  #define LOOKUP(T) \
    usz tn = 1<<T;                                             \
    u##T* xp = (u##T*)xv;                                      \
    i32* rp; B r = m_i32arrv(&rp, n);                          \
    TALLOC(i32, tab, tn);                                      \
    if (T>8 && n<tn/16) for (usz i=0; i<n;  i++) tab[xp[i]]=0; \
    else                for (usz j=0; j<tn; j++) tab[j]=0;     \
    for (usz i=0; i<n;  i++) rp[i]=tab[xp[i]]++;               \
    decG(x); TFREE(tab);                                       \
    return num_squeeze(r)
  if (lw==3) { if (n<12) { BRUTE(8); } else { LOOKUP(8); } }
  if (lw==4) { if (n<12) { BRUTE(16); } else { LOOKUP(16); } }
  #undef LOOKUP
  #define TRY_HASHTAB(W) TRY_HASHTAB_RET(count, W, num_squeeze(r))
  if (lw==5) {
    if (n<20) { BRUTE(32); }
    i32* rp; B r = m_i32arrv(&rp, n);
    TRY_HASHTAB(32)
    // Radix-assisted lookup
    usz rx = 256, tn = 1<<16; // Radix; table length
    u32* v0 = (u32*)xv;
    i32* r0 = rp;
    
    TALLOC(u8, alloc, 6*n + (4 + 4*(tn>n?tn:n) + (2*rx+1)*sizeof(usz)) + sizeof(u32));
    //                                                       timeline
    // Allocations                             len  count radix hash deradix     bytes  layout:
    usz *c0 =    (usz*)(alloc)+1; // rx   [+++................]    c0    rx  #
    usz *c1 =    (usz*)(c0+rx);   // rx    [++................]    c1    rx   #
    u8  *k0 =    (u8 *)(c1+rx);   //  n        [+.............]    k0     n    ##
    u8  *k1 =    (u8 *)(k0+n);    //  n         [+............]    k1     n      ##
    u32 *v2=PRUP((u32*)(k1+n));   //  n+1       [+....-]           v2   4*n        ########
    u32 *v1 =    (u32*)(v2+n+1);  //  n        [+..]               v1   4*n                ########
    u32 *r2 =    (u32*)v2;        //  n              [+.....]      r2   4*n        ########
    u32 *r1 =    (u32*)v1;        //  n                   [+..]    r1   4*n                ########
    u32 *tab=    (u32*)v1;        // tn              [+]           tab 4*tn                ###########
    
    RADIX_LOOKUP_32(0, ++)
    return num_squeeze(r);
  }
  if (lw==6 && canCompare64_norm(&x, &xv, n)) {
    if (n<20) { BRUTE(64); }
    i32* rp; B r = m_i32arrv(&rp, n);
    TRY_HASHTAB(64)
    decG(r); // Fall through
  }
  #undef TRY_HASHTAB
  #undef BRUTE
  
  if (RNK(x)>1) {
    if (shouldWidenBitarr(x, csz)) return C1(count, widenBitArr(x, 1));
    x = toCells(x);
  }
  i32* rp; B r = m_i32arrv(&rp, n);
  H_b2i* map = m_b2i(64);
  SGetU(x)
  for (usz i = 0; i < n; i++) {
    bool had; u64 p = mk_b2i(&map, GetU(x,i), &had);
    rp[i] = had? ++map->a[p].val : (map->a[p].val = 0);
  }
  decG(x); free_b2i(map);
  return num_squeeze(r);
}

static B reduceI32WidthBelow(B r, usz after) {
  return after<=2? taga(cpyBitArr(r)) : after<=I8_MAX+1? taga(cpyI8Arr(r)) : after<=I16_MAX+1? taga(cpyI16Arr(r)) : r;
}
B find_c1(B, B);

B indexOf_c1(B t, B x) {
  if (isAtm(x) || RNK(x)==0) thrM("⊐𝕩: 𝕩 cannot have rank 0");
  u64 n = *SH(x);
  if (n<=1) { zeroRes: decG(x); return n? taga(arr_shVec(allZeroesFl(n))) : emptyIVec(); }
  if (n>(usz)I32_MAX+1) thrM("⊐𝕩: 𝕩 length >2⋆31 not supported");
  
  usz csz = arr_csz(x);
  if (csz==0) goto zeroRes;
  u8 lw = cellWidthLog(x);
  void* xv = tyany_ptr(x);
  if (lw==0) {
    B r = 1&*(u64*)xv ? bit_negate(x) : x;
    return C1(shape, r);
  }
  if (use_sorted(x, lw) && n>8) {
    return scan_add_bool(shift_ne(x, n, lw, 0), n);
  }
  #define BRUTE(T) \
    i##T* xp = xv;                                             \
    i8* rp; B r = m_i8arrv(&rp, n); rp[0]=0;                   \
    TALLOC(i##T, uniq, n); uniq[0]=xp[0];                      \
    for (usz i=1, u=1; i<n; i++) {                             \
      bool c=1; usz s=0; i##T xi=uniq[u]=xp[i];                \
      for (usz j=0; j<u; j++) s += (c &= xi!=uniq[j]);         \
      rp[i]=s; u+=u==s;                                        \
    }                                                          \
    decG(x); TFREE(uniq); return r;
  #define DOTAB(T) /* leaves 1+max in u */ \
    i32 u=0;                                                   \
    for (usz i=0; i<n; i++) {                                  \
      T j=xp[i]; i32 t=tab[j];                                 \
      if (t==n) tab[j]=u++;                                    \
      rp[i]=tab[j];                                            \
    }
  #define LOOKUP(T) \
    usz tn = 1<<T;                                             \
    u##T* xp = (u##T*)xv;                                      \
    i32* rp; B r = m_i32arrv(&rp, n);                          \
    TALLOC(i32, tab, tn);                                      \
    if (T>8 && n<tn/16) for (usz i=0; i<n;  i++) tab[xp[i]]=n; \
    else                for (usz j=0; j<tn; j++) tab[j]=n;     \
    DOTAB(u##T)                                                \
    decG(x); TFREE(tab);                                       \
    return reduceI32WidthBelow(r, u)
  if (lw==3) {
    if (n<12) { BRUTE(8); }
    // If bit-table Mark Firsts is available
    #if SINGELI && defined(__SSSE3__)
    else if (n > 1<<12) {
      B u = C1(find, incG(x));
      return C2(indexOf, u, x);
    }
    #endif
    else { LOOKUP(8); }
  }
  if (lw==4) { if (n<12) { BRUTE(16); } else { LOOKUP(16); } }
  #undef LOOKUP

  #if SINGELI
    #define TRY_HASHTAB(W) \
      u32 ctr = indexOf_c1_hash##W(rp, (u##W*)xv, n); \
      if (ctr>0) { decG(x); return reduceI32WidthBelow(r, ctr); }
  #else
    #define TRY_HASHTAB(W) (void)0;
  #endif
  if (lw==5) {
    if (n<12) { BRUTE(32); }
    B r;
    i32* rp; r = m_i32arrv(&rp, n);
    i32* xp = xv;
    i32 min=I32_MAX, max=I32_MIN;
    for (usz i = 0; i < n; i++) {
      i32 c = xp[i];
      if (c<min) min = c;
      if (c>max) max = c;
    }
    i64 dst = 1 + (max-(i64)min);
    if (dst<n*5 || dst<50) {
      TALLOC(i32, tmp, dst); i32* tab = tmp-min;
      for (i64 i = 0; i < dst; i++) tmp[i] = n;
      DOTAB(i32)
      TFREE(tmp);
      decG(x);
      return reduceI32WidthBelow(r, u);
    }
    TRY_HASHTAB(32)
    decG(r); // Fall through
  }
  if (lw==6 && canCompare64_norm(&x, &xv, n)) {
    if (n<16) { BRUTE(64); }
    i32* rp; B r = m_i32arrv(&rp, n);
    TRY_HASHTAB(64)
    decG(r); // Fall through
  }
  #undef TRY_HASHTAB
  #undef BRUTE
  #undef DOTAB
  
  if (RNK(x)>1) {
    if (shouldWidenBitarr(x, csz)) return C1(indexOf, widenBitArr(x, 1));
    x = toCells(x);
  }
  i32* rp; B r = m_i32arrv(&rp, n);
  H_b2i* map = m_b2i(64);
  SGetU(x)
  i32 ctr = 0;
  for (usz i = 0; i < n; i++) {
    bool had; u64 p = mk_b2i(&map, GetU(x,i), &had);
    if (had) rp[i] = map->a[p].val;
    else     rp[i] = map->a[p].val = ctr++;
  }
  free_b2i(map); decG(x);
  return reduceI32WidthBelow(r, ctr);
}

B find_c1(B t, B x) {
  if (isAtm(x) || RNK(x)==0) thrM("⍷𝕩: 𝕩 cannot have rank 0");
  usz n = *SH(x);
  if (n<=1) return x;
  u8 xe = TI(x,elType);
  u8 sameness = IA(x)==0? 1 : fast_sameness(x, xe);
  if (sameness == 1) return C2(take, m_i32(1), x);
  if (RARE(sameness == 2)) return x;
  if (xe==el_bit && RNK(x)==1) {
    u64* xp = bitany_ptr(x);
    u64 x0 = 1 & *xp;
    usz i = bit_find(xp, n, !x0); decG(x);
    u64* rp; B r = m_bitarrv(&rp, 1 + (i<n));
    rp[0] = 2 ^ -x0;
    return r;
  }
  #if SINGELI
  if (elWidth(xe)==1 && RNK(x)==1 && !FL_HAS(x, fl_asc|fl_dsc)) {
    TALLOC(u8, tab, 513); u8* res = tab+256;
    usz ria = simd_deduplicate_u8(tyany_ptr(x), n, res, tab);
    B r; i8* rp = m_tyarrv(&r, 1, ria, el2t(xe));
    memcpy(rp, res, ria);
    TFREE(tab); decG(x);
    return r;
  }
  #endif
  return C2(slash, C1(memberOf, incG(x)), x);
}
