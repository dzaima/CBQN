// Ordering functions: Sort (∧∨), Grade (⍋⍒), Bins (⍋⍒)

// Sort and Grade
// Trivial results on empty, single-cell, or flagged-sorted 𝕩
//   COULD reverse to sort flat 𝕩 with flag in opposite direction
// Small ≠𝕩: insertion sort
// Long ≤2-byte sort (not grade): counting sort
//   Length threshold is around the type's maximum range
//   Count 1s for booleans
//   Large range/length ("sparse") uses scan: plus, or Singeli max/min
//   COULD range check 4-byte 𝕩 to try counting sort
// 4-byte grade checks range and sum (before even trying insertion sort)
//   If consistent with a permutation, grade as one and verify after
//   If small-range, grade with bucket sort
// Other 1-, 2-, 4-byte cases: radix sort
//   Singeli +` used if available, speeding up shorter cases
//   SHOULD skip steps where all bytes are equal
// General case: Timsort
// SHOULD scan for sortedness in all cases
// SHOULD use an adaptive quicksort for 4- and 8-byte arguments
// SHOULD widen odd cell sizes under 8 bytes in sort and grade

// Bins
// Length 0 or 1 𝕨: trivial, or comparison
// Stand-alone 𝕨 sortedness check, with Singeli SIMD
// Mixed integer and character arguments gives all 0 or ≠𝕨
// Non-Singeli, integers and characters:
//   4-byte branchless binary search, 4-byte output
// SHOULD support fast character searches
// Boolean 𝕨 or 𝕩: lookup table (single binary search on boolean 𝕨)
// Different widths: generally widen narrower argument
//   Narrow wider-type 𝕩 instead if it isn't much shorter
//   SHOULD trim wider-type 𝕨 and possibly narrow
// Same-width numbers:
//   Output type based on ≠𝕨
//   Short 𝕨: vector binary search (then linear on extra lanes)
//   1- or 2-byte type, long enough 𝕩: lookup table from ⌈`
//     Binary gallops to skip long repeated elements of 𝕨
//     1-byte, no duplicates or few uniques: vector bit-table lookup
//   General: interleaved branchless binary search
//   COULD start interleaved search with a vector binary round
// General case: branching binary search
// COULD trim 𝕨 based on range of 𝕩
// COULD optimize small-range 𝕨 with small-type methods
// SHOULD partition 𝕩 when 𝕨 is large
// COULD interpolation search for large 𝕩 and short 𝕨
// COULD use linear search and galloping for sorted 𝕩

#define GRADE_NAME GRADE_UD(gradeUp,gradeDown)
#define GRADE_CAT(N) CAT(GRADE_NAME,N)
#define GRADE_NEG GRADE_UD(,-)

// Timsort
#define SORT_CMP(W, X) GRADE_NEG compare(W, X)
#define SORT_NAME GRADE_UD(bA,bD)
#define SORT_TYPE B
#include "sortTemplate.h"

#define SORT_CMP(W, X) GRADE_NEG compare((W).k, (X).k)
#define SORT_NAME GRADE_CAT(BP)
#define SORT_TYPE BI32p
#include "sortTemplate.h"


#define LT GRADE_UD(<,>)
#define FOR(I,MAX) GRADE_UD(for (usz I=0; I<MAX; I++), \
                            for (usz I=MAX; I--; ))

// Insertion sort
#define INS_SORT( J, I, V) rp[J]=V;
#define INS_GRADE(J, I, V) xs[J]=V; rp[J]=I;
#define INSERTION_SORT(T, TYP) \
  INS_##TYP(0, 0, xp[0])                               \
  for (usz i=0; i<n; i++) {                            \
    usz j=i, jn; T xi=xp[i], rj;                       \
    while (0<j && xi LT (rj=CHOOSE_SG_##TYP(rp,xs)[jn=j-1])) { \
      INS_##TYP(j, rp[jn], rj) j=jn;                   \
    }                                                  \
    INS_##TYP(j, i, xi)                                \
  }


// Counting sort
#define COUNTING_SORT(T) \
  usz C=1<<(8*sizeof(T));                              \
  TALLOC(usz, c0, C); usz *c0o=c0+C/2;                 \
  for (usz j=0; j<C; j++) c0[j]=0;                     \
  for (usz i=0; i<n; i++) c0o[xp[i]]++;                \
  if (!ch && n/(COUNT_THRESHOLD*sizeof(T)) <= C) { /* Scan-based */ \
    T j=GRADE_UD(-C/2,C/2-1);                          \
    usz ij; while ((ij=c0o[j])==0) GRADE_UD(j++,j--);  \
    WRITE_SPARSE(T)                                    \
  } else { /* Branchy */                               \
    WRITE_DENSE(c0)                                    \
  }                                                    \
  TFREE(c0)
#define WRITE_DENSE(CP) \
  if (!ch) {                                           \
    FOR(j,C) for (usz c=CP[j]; c--; ) *rp++ = j-C/2;   \
  } else {                                             \
    usz h=GRADE_UD(C/2,0), g=C/2-h;                    \
    FOR(j,C/2) for (usz c=CP[j+h]; c--; ) *rp++ = j+g; \
    FOR(j,C/2) for (usz c=CP[j+g]; c--; ) *rp++ = j+h; \
  }

#if SINGELI_SIMD
extern void (*const si_scan_max_i8)(int8_t* v0,int8_t* v1,uint64_t v2);
extern void (*const si_scan_min_i8)(int8_t* v0,int8_t* v1,uint64_t v2);
extern void (*const si_scan_max_i16)(int16_t* v0,int16_t* v1,uint64_t v2);
extern void (*const si_scan_min_i16)(int16_t* v0,int16_t* v1,uint64_t v2);
#define COUNT_THRESHOLD 32
#define WRITE_SPARSE_i8 \
  for (usz i=0; i<n; i++) rp[i]=j;                       \
  while (ij<n) { rp[ij]=GRADE_UD(++j,--j); ij+=c0o[j]; } \
  GRADE_UD(si_scan_max_i8,si_scan_min_i8)(rp,rp,n);
#define WRITE_SPARSE_i16 \
  usz b = 1<<10;                                              \
  for (usz k=0; ; ) {                                         \
    usz e = b<n-k? k+b : n;                                   \
    for (usz i=k; i<e; i++) rp[i]=j;                          \
    while (ij<e) { rp[ij]=GRADE_UD(++j,--j); ij+=c0o[j]; }    \
    GRADE_UD(si_scan_max_i16,si_scan_min_i16)(rp+k,rp+k,e-k); \
    if (e==n) {break;}  k=e;                                  \
  }
#define WRITE_SPARSE(T) WRITE_SPARSE_##T
extern i8 (*const simd_count_i8)(u16*, u16*, void*, u64, i8);
#define COUNTING_SORT_i8 \
  usz C=1<<8;                                          \
  TALLOC(u16, c0, C+(n>>15)+1);                        \
  u16 *c0o=c0+C/2; u16 *ov=c0+C;                       \
  for (usz j=0; j<C; j++) c0[j]=0;                     \
  simd_count_i8(c0o, ov, xp, n, -128);                 \
  if (!ch && n/COUNT_THRESHOLD <= C) { /* Scan-based */\
    i8 j=GRADE_UD(-C/2,C/2-1);                         \
    usz ij; while ((ij=c0o[j])==0) GRADE_UD(j++,j--);  \
    WRITE_SPARSE(i8)                                   \
    TFREE(c0)                                          \
  } else { /* Branchy, and ov may have entries */      \
    TALLOC(usz, cw, C);                                \
    NOUNROLL for (usz i=0; i<C; i++) cw[i]=c0[i];      \
    u16 oe=-1;                                         \
    for (usz i=0; ov[i]!=oe; i++) cw[ov[i]]+= 1<<15;   \
    TFREE(c0)                                          \
    WRITE_DENSE(cw)                                    \
    TFREE(cw)                                          \
  }
#else
#define COUNT_THRESHOLD 16
#define WRITE_SPARSE(T) \
  for (usz i=0; i<n; i++) rp[i]=0;                       \
  usz js = j;                                            \
  while (ij<n) { rp[ij]GRADE_UD(++,--); ij+=c0o[GRADE_UD(++j,--j)]; } \
  for (usz i=0; i<n; i++) js=rp[i]+=js;
#define COUNTING_SORT_i8 COUNTING_SORT(i8)
#endif

// Radix sorting
#include "radix.h"
#define INC(P,I) GRADE_UD((P+1)[I]++,P[I]--)
#define ROFF GRADE_UD(1,0) // Radix offset

#define CHOOSE_SG_SORT(S,G) S
#define CHOOSE_SG_GRADE(S,G) G

#define MAKE_FLIP(T, FOR) \
  NOINLINE void GRADE_CAT(flip_sum_##T)(T* c, usz n) {  \
    T* co=c+128; T o=co[GRADE_UD(0,-1)], p=n-o;         \
    FOR (usz j=0; j<128; j++) { GRADE_UD(co,c)[j]-=o;   \
                                GRADE_UD(c,co)[j]+=p; } \
  }
MAKE_FLIP(u8, for) MAKE_FLIP(u32, NOUNROLL for) MAKE_FLIP(usz, NOUNROLL for)
#undef MAKE_FLIP

#define RADIX_SORT_i8(T, TYP) \
  TALLOC(T, c0, 256+ROFF); T* c0o=c0+128;  \
  for (usz j=0; j<256; j++) c0[j]=0;       \
  GRADE_UD(,c0[0]=n;)                      \
  for (usz i=0; i<n; i++) INC(c0o,xp[i]);  \
  RADIX_SUM_1_##T;                         \
  if (RARE(ch)) GRADE_CAT(flip_sum_##T)(c0,n); \
  for (usz i=0; i<n; i++) { i8 xi=xp[i];   \
    rp[c0o[xi]++]=CHOOSE_SG_##TYP(xi,i); } \
  TFREE(c0)

#define RADIX_SORT_i16(T, TYP, I) \
  TALLOC(u8, alloc, (2*256+ROFF)*sizeof(T) + n*(2 + CHOOSE_SG_##TYP(0,sizeof(I))) + sizeof(i32)); \
  T* c0=(T*)alloc; T* c1=c0+256; T* c1o=c1+128;                              \
  for (usz j=0; j<2*256; j++) c0[j]=0;                                       \
  c1[0]=GRADE_UD(-n,c0[0]=n);                                                \
  for (usz i=0; i<n; i++) { i16 v=xp[i]; INC(c0,(u8)v); INC(c1o,(i8)(v>>8)); } \
  RADIX_SUM_2_##T;                                                           \
  if (RARE(ch)) GRADE_CAT(flip_sum_##T)(c1,n);                               \
  i16 *r0 = (i16*)(c0+2*256);                                                \
  CHOOSE_SG_##TYP(                                                           \
  for (usz i=0; i<n; i++) { i16 v=xp[i]; r0[c0 [(u8)v     ]++]=v; }          \
  for (usz i=0; i<n; i++) { i16 v=r0[i]; rp[c1o[(i8)(v>>8)]++]=v; }          \
  ,                                                                          \
  I *g0 = PTR_ROUND_UP_TO_ELT((i32*)(r0+n));                                     \
  for (usz i=0; i<n; i++) { i16 v=xp[i]; T c=c0[(u8)v     ]++; r0[c]=v; g0[c]=i; } \
  for (usz i=0; i<n; i++) { i16 v=r0[i]; rp[c1o[(i8)(v>>8)]++]=g0[i]; }      \
  )                                                                          \
  TFREE(alloc)

#define RADIX_SORT_i32(T, TYP, I) \
  TALLOC(u8, alloc, (4*256+ROFF)*sizeof(T) + n*(4 + CHOOSE_SG_##TYP(0,4+sizeof(I)))); \
  T *c0=(T*)alloc, *c1=c0+256, *c2=c1+256, *c3=c2+256, *c3o=c3+128;          \
  for (usz j=0; j<4*256; j++) c0[j]=0;                                       \
  c1[0]=c2[0]=c3[0]=GRADE_UD(-n,c0[0]=n);                                    \
  for (usz i=0; i<n; i++) { i32 v=xp[i];                                     \
    INC(c0 ,(u8)v      ); INC(c1 ,(u8)(v>> 8));                              \
    INC(c2 ,(u8)(v>>16)); INC(c3o,(i8)(v>>24)); }                            \
  RADIX_SUM_4_##T;                                                           \
  i32 *r0 = (i32*)(c0+4*256);                                                \
  CHOOSE_SG_##TYP(                                                           \
  for (usz i=0; i<n; i++) { i32 v=xp[i]; T c=c0 [(u8)v      ]++; r0[c]=v; }  \
  for (usz i=0; i<n; i++) { i32 v=r0[i]; T c=c1 [(u8)(v>> 8)]++; rp[c]=v; }  \
  for (usz i=0; i<n; i++) { i32 v=rp[i]; T c=c2 [(u8)(v>>16)]++; r0[c]=v; }  \
  for (usz i=0; i<n; i++) { i32 v=r0[i]; T c=c3o[(i8)(v>>24)]++; rp[c]=v; }  \
  ,                                                                          \
  i32 *r1 = r0+n; I *g0 = (i32*)(r1+n);                                      \
  for (usz i=0; i<n; i++) { i32 v=xp[i]; T c=c0 [(u8)v      ]++; r0[c]=v; g0[c]=i;     } \
  for (usz i=0; i<n; i++) { i32 v=r0[i]; T c=c1 [(u8)(v>> 8)]++; r1[c]=v; rp[c]=g0[i]; } \
  for (usz i=0; i<n; i++) { i32 v=r1[i]; T c=c2 [(u8)(v>>16)]++; r0[c]=v; g0[c]=rp[i]; } \
  for (usz i=0; i<n; i++) { i32 v=r0[i]; T c=c3o[(i8)(v>>24)]++;          rp[c]=g0[i]; } \
  )                                                                          \
  TFREE(alloc)


#define SORT_C1 CAT(GRADE_UD(and,or),c1)
B SORT_C1(B t, B x) {
  if (isAtm(x) || RNK(x)==0) thrM(GRADE_UD("∧","∨")"𝕩: 𝕩 cannot have rank 0");
  usz n = IA(x);
  if (n <= 1 || FL_HAS(x,GRADE_UD(fl_asc,fl_dsc))) return x;
  if (RNK(x)!=1 && n!=*SH(x)) return *SH(x)<=1? x : bqn_merge(SORT_C1(t, toCells(x)), 0);
  B r;
  u8 xe = TI(x,elType);
  usz xw = elWidth(xe);
  if (xe==el_bit) {
    u64* xp = bitany_ptr(x);
    u64* rp; r = m_bitarrc(&rp, x);
    usz sum = bit_sum(xp, n);
    u64 n0 = GRADE_UD(n-sum, sum);
    u64 ones = -1ull;
    u64 v0 = GRADE_UD(0, ones);
    usz i=0, e=(n+63)/64;
    for (; i<n0/64; i++) rp[i]=v0;
    if (i<e) rp[i++]=v0^(ones<<(n0%64));
    for (; i<e; i++) rp[i]=~v0;
  } else if (xw <= 4) {
    bool ch = elChr(xe);
    void* xv = tyany_ptr(x);
    void* rv = m_tyarrc(&r, xw, x, el2t(xe));
    if (xw == 1) {
      i8 *xp=xv, *rp=rv;
      if (n < 16) {
        if (!ch) { INSERTION_SORT(i8, SORT); }
        else     { INSERTION_SORT(u8, SORT); }
      } else if (n < 256) {
        RADIX_SORT_i8(u8, SORT);
      } else {
        COUNTING_SORT_i8;
      }
    } else if (xw == 2) {
      i16 *xp=xv, *rp=rv;
      if (n < 20) {
        if (!ch) { INSERTION_SORT(i16, SORT); }
        else     { INSERTION_SORT(u16, SORT); }
      } else if (n < 256) {
        RADIX_SORT_i16(u8, SORT,);
      } else if (n < 1<<15 || (ch && n < 1<<18)) {
        RADIX_SORT_i16(u32, SORT,);
      } else {
        COUNTING_SORT(i16);
      }
    } else if (xw == 4) { // Assume signed: max c32 is 1114111 < 2^31
      i32 *xp=xv, *rp=rv;
      if (n < 32) {
        INSERTION_SORT(i32, SORT);
      } else if (n < 256) {
        RADIX_SORT_i32(u8, SORT,);
      } else {
        if (MAY_T(n>U32_MAX)) { tyarr_freeF(v(r)); goto generic; }
        RADIX_SORT_i32(u32, SORT,);
      }
    }
  } else {
    generic:;
    B xf = getFillR(x);
    HArr* r0 = (HArr*)cpyHArr(incG(x));
    CAT(GRADE_UD(bA,bD),tim_sort)(r0->a, n);
    r = withFill(taga(r0), xf);
  }
  decG(x);
  return FL_SET(r, CAT(fl,GRADE_UD(asc,dsc)));
}
#undef SORT_C1
#undef COUNTING_SORT
#undef COUNTING_SORT_i8
#undef WRITE_DENSE
#if SINGELI_AVX2
#undef WRITE_SPARSE_i8
#undef WRITE_SPARSE_i16
#endif


extern GLOBAL Arr* bitUD[3]; // from fns.c
extern GLOBAL B bit2x[2]; // from fns.c
extern GLOBAL B int2x[2]; // from sort.c
extern B grade_bool(B x, usz n, bool up); // slash.c
extern B ud_c1(B,B);

#define GRADE_CHR GRADE_UD("⍋","⍒")
B GRADE_CAT(c1)(B t, B x) {
  if (isAtm(x)) unit: thrM(GRADE_CHR"𝕩: 𝕩 cannot be a unit");
  usz n = IA(x);
  ur xr = RNK(x); // xr==0 implies n==1
  if (n <= 1 || FL_HAS(x,GRADE_UD(fl_asc,fl_dsc))) {
    if (xr!=1) { if (xr==0) goto unit; n=*SH(x); }
    sorted: decG(x); return n<=2? taga(ptr_inc(bitUD[n])) : C1(ud,m_usz(n));
  }
  if (xr > 1) {
    usz ia = n;
    n = *SH(x); if (n<=1) goto sorted;
    if (ia!=n) x = toCells(x);
  }
  if (n == 2) {
    SGetU(x);
    B r = incG(bit2x[!(compare(GetU(x,0), GetU(x,1)) GRADE_UD(<=,>=) 0)]);
    decG(x); return r;
  }
  
  u8 xe = TI(x,elType);
  if (xe==el_bit) return grade_bool(x, n, GRADE_UD(1,0));
  if (n>I32_MAX) thrM(GRADE_CHR"𝕩: 𝕩 too large");
  i32* rp; B r = m_i32arrv(&rp, n);
  usz xw = elWidth(xe);
  bool ch = elChr(xe);
  if (xw == 1) {
    #define INSERTION_GRADES(W) \
      TALLOC(i##W, xsi, n);                                          \
      if (!ch) { i##W* xs=       xsi; INSERTION_SORT(i##W, GRADE); } \
      else     { u##W* xs=(u##W*)xsi; INSERTION_SORT(u##W, GRADE); } \
      TFREE(xsi);
    i8* xp = tyany_ptr(x);
    if (n < 24) {
      INSERTION_GRADES(8);
    } else {
      RADIX_SORT_i8(usz, GRADE);
    }
  } else if (xw == 2) {
    i16* xp = tyany_ptr(x);
    if (n < 48) {
      INSERTION_GRADES(16);
    } else {
      RADIX_SORT_i16(usz, GRADE, i32);
    }
    #undef INSERTION_GRADES
  } else if (xw == 4) { // Assume signed: max c32 is 1114111 < 2^31
    i32* xp = tyany_ptr(x);
    i32 min=I32_MAX, max=I32_MIN;
    u32 sum=0;
    for (usz i = 0; i < n; i++) {
      i32 c = xp[i];
      sum += (u32)c;
      if (c<min) min=c;
      if (c>max) max=c;
    }
    u64 range = max - (i64)min + 1;
    if (range/2 < n) {
      // First try to invert it as a permutation
      if (range == n && sum == (u32)(n * (min+(i64)max)/2)) {
        for (usz i = 0; i < n; i++) rp[i]=n;
        for (usz i = 0; i < n; i++) { i32 v=xp[i]; GRADE_UD(rp[v-min],rp[max-v])=i; }
        bool done=1; for (usz i = 0; i < n; i++) done &= rp[i]!=n;
        if (done) goto decG_sq;
      }
      TALLOC(usz, c0, range); usz *c0o=c0-min;
      for (usz i = 0; i < range; i++) c0[i] = 0;
      for (usz i = 0; i < n; i++) c0o[xp[i]]++;
      usz s=0; FOR (i, range) { usz p=s; s+=c0[i]; c0[i]=p; }
      for (usz i = 0; i < n; i++) rp[c0o[xp[i]]++] = i;
      TFREE(c0);
    } else if (n < 64) {
      TALLOC(i32, xs, n);
      INSERTION_SORT(i32, GRADE);
      TFREE(xs);
    } else {
      RADIX_SORT_i32(usz, GRADE, i32);
    }
  } else {
    SLOW1(GRADE_CHR"𝕩", x);
    generic_grade(x, n, r, rp, CAT(GRADE_CAT(BP),tim_sort));
  }
  
  decG_sq:;
  if (n<=(I8_MAX+1)) r = taga(cpyI8Arr(r));
  else if (n<=(I16_MAX+1)) r = taga(cpyI16Arr(r));
  decG(x);
  return r;
}

bool CAT(isSorted,GRADE_UD(Up,Down))(B x) {
  assert(isArr(x) && RNK(x)==1); // TODO extend to >=1
  usz xia = IA(x);
  if (xia <= 1) return 1;

  #define CMP(TEST) \
    for (usz i=1; i<xia; i++) if (TEST) return 0; \
    return 1;
  if (0) { elB:;
    B* xp = arr_bptr(x);
    if (xp!=NULL) {
      CMP(compare(xp[i-1], xp[i]) GRADE_UD(>,<) 0)
    } else {
      SGetU(x)
      CMP(compare(GetU(x,i-1), GetU(x,i)) GRADE_UD(>,<) 0)
    }
  }
  #if SINGELI_SIMD
    u8 xe = TI(x,elType);
    if (xe == el_B) goto elB;
    return is_sorted[xe](tyany_ptr(x), GRADE_UD(0,1), xia-1);
  #else
    #define CASE(T) case el_##T: { \
      T* xp = T##any_ptr(x); CMP(xp[i-1] GRADE_UD(>,<) xp[i]) }
    switch (TI(x,elType)) { default: UD;
      CASE(i8) CASE(i16) CASE(i32)
      CASE(c8) CASE(c16) CASE(c32)
      case el_bit: return bit_isSorted(bitany_ptr(x), GRADE_UD(0,1), xia-1);
      case el_f64: {
        f64* xp = f64any_ptr(x);
        CMP(floatCompare(xp[i-1], xp[i]) GRADE_UD(>,<) 0);
      }
      case el_B: goto elB;
    }
    #undef CASE
  #endif
  #undef CMP
}

// Location of first 1 (ascending) or 0 (descending), by binary search
u64 CAT(bit_boundary,GRADE_UD(up,dn))(u64* x, u64 n) {
  u64 c = GRADE_UD(,~)(u64)0;
  u64 *s = x-1;
  for (usz l = BIT_N(n)+1, h; (h=l/2)>0; l-=h) {
    u64* m = s+h; if (!(c LT *m)) s = m;
  }
  ++s; // Word containing boundary
  u64 b = 64*(s-x);
  if (b >= n) return n;
  u64 v = GRADE_UD(~,) *s;
  if (b+63 >= n) v &= ~(u64)0 >> ((-n)%64);
  return b + POPC(v);
}

#define LE_FN GRADE_UD(le,ge)
extern B lt_c2(B,B,B);
extern B le_c2(B,B,B);
extern B gt_c2(B,B,B);
extern B ge_c2(B,B,B);
extern B ne_c2(B,B,B);
extern B select_c2(B,B,B);
extern B mul_c2(B,B,B);

B GRADE_CAT(c2)(B t, B w, B x) {
  if (isAtm(w) || RNK(w)==0) thrM("𝕨"GRADE_CHR"𝕩: 𝕨 must have rank≥1");
  if (isAtm(x)) x = m_unit(x);
  ur wr = RNK(w);
  
  if (wr > 1) {
    ur xr = RNK(x);
    if (wr > xr+1) thrM("𝕨"GRADE_CHR"𝕩: =𝕨 cannot be greater than =𝕩");
    i32 nxr = xr-wr+1;
    x = toKCells(x, nxr);
    w = toCells(w);
  }
  
  u8 we = TI(w,elType); usz wia = IA(w);
  u8 xe = TI(x,elType); usz xia = IA(x);
  
  B r; Arr* ra;
  
  if (wia==0 | xia==0) {
    ra = allZeroesFl(xia);
    goto copysh_done;
  }
  if (wia==1) {
    B c = IGet(w, 0);
    if (LIKELY(we<el_B & xe<el_B)) {
      decG(w);
      if (we==el_f64 && elNum(xe) && q_nan(c)) return GRADE_UD(
        C2(ne, incG(x), x),
        i64EachDec(1, x)
      );
      if (GRADE_UD(1,0) && xe==el_f64) return bit_negate(C2(lt, x, c)); // handle NaNs in x properly
      return C2(LE_FN, c, x);
    } else {
      SLOW2("𝕨"GRADE_CHR"𝕩", w, x); // Could narrow for mixed types
      u64* rp; r = m_bitarrc(&rp, x);
      B* xp = TO_BPTR(x);
      u64 b = 0;
      for (usz i = xia; ; ) {
        i--;
        b = 2*b + !(compare(xp[i], c) LT 0);
        if (i%64 == 0) { rp[i/64]=b; if (!i) break; }
      }
      dec(c);
    }
    goto done;
  }
  if (wia>I32_MAX-10) thrM("𝕨"GRADE_CHR"𝕩: 𝕨 too big");
  
  u8 fl = GRADE_UD(fl_asc,fl_dsc);
  if (CHECK_VALID && !FL_HAS(w,fl)) {
    if (!CAT(isSorted,GRADE_UD(Up,Down))(w)) thrM("𝕨"GRADE_CHR"𝕩: 𝕨 must be sorted"GRADE_UD(," in descending order"));
    FL_SET(w, fl);
  }
  
  ux idxOfMax = GRADE_UD(wia-1, 0);
  #if SINGELI
  B mult = bi_z;
  #endif
  
  if (LIKELY(we<el_B & xe<el_B)) {
    if (elNum(we)) {
      if (elNum(xe)) {
        if (RARE(we==el_bit | xe==el_bit)) goto bit_cases;
        if (we==el_f64 && q_nan(IGetU(w,idxOfMax))) goto generic;
        goto nums;
      } else { // num F chr
        ra = GRADE_UD(reshape_one(xia, m_f64(wia)), allZeroesFl(xia));
        goto copysh_done;
      }
    } else { // chr F x
      if (elNum(xe)) { // chr F num
        ra = GRADE_UD(allZeroesFl(xia), reshape_one(xia, m_f64(wia)));
        goto copysh_done;
      }
      // chr F chr
      we = el_c32;
      w=toC32Any(w); x=toC32Any(x);
      goto signed32;
    }
  } else {
    goto generic;
  }
  
  
  
  if (0) generic: {
    i32* rp; r = m_i32arrc(&rp, x);
    SLOW2("𝕨"GRADE_CHR"𝕩", w, x);
    SGetU(w) SGetU(x)
    for (usz i = 0; i < xia; i++) {
      B c = GetU(x,i);
      usz s = 0, e = wia+1;
      while (e-s > 1) {
        usz m = (s+e) / 2;
        if (compare(c, GetU(w,m-1)) LT 0) e = m;
        else s = m;
      }
      rp[i] = s;
    }
    goto done;
  }
  
  if (0) bit_cases: {
    if (we==el_bit) {
      usz c1 = CAT(bit_boundary,GRADE_UD(up,dn))(bitany_ptr(w), wia);
      decG(w); // c1 and wia contain all information in w
      if (xe==el_bit) {
        r = bit_sel(x, m_f64(GRADE_UD(c1,wia)), m_f64(GRADE_UD(wia,c1)));
      } else {
        B i = C2(GRADE_NAME, incG(int2x[GRADE_UD(0,1)]), x);
        f64* c; B rw = m_f64arrv(&c, 3); c[0]=0; c[1]=c1; c[2]=wia;
        r = C2(select, i, squeeze_numNewTy(el_f64,rw));
      }
    } else { // xe==el_bit: 2-element lookup table
      B i = C2(GRADE_NAME, w, incG(int2x[0]));
      SGetU(i)
      r = bit_sel(x, GetU(i,0), GetU(i,1));
      decG(i);
    }
    return r;
  }
  
  if (0) nums: {
    #if SINGELI
      #define WIDEN(E, X) switch (E) { default:UD; case el_i16:X=toI16Any(X);break; case el_i32:X=toI32Any(X);break; case el_f64:X=toF64Any(X);break; }
      if (xe > we) {
        if (HEURISTIC(xia/4 < wia)) { // Narrow x
          assert(el_i8 <=we && we<=el_i32);
          assert(el_i16<=xe && xe<=el_f64);
          i32 pre = (i32) (U32_MAX << ((8<<(we-el_i8))-1));
          pre = GRADE_UD(pre,-1-pre); // Smallest value of w's type
          i32 w0 = o2iG(IGetU(w,0));
          // Saturation is correct except it can move low values past
          // pre. Post-adjust with mult×r
          if (w0 == pre) mult = C2(LE_FN, m_i32(pre), incG(x));
          // Narrow x with saturating conversion
          B xn; void *xp = m_tyarrc(&xn, elWidth(we), x, el2t(we));
          u8 ind = xe<el_f64 ? (we-el_i8)+(xe-el_i16)
                 : 3 + 2*(we-el_i8) + GRADE_UD(0,1);
          si_saturate[ind](xp, tyany_ptr(x), xia);
          decG(x); x = xn;
        } else {
          WIDEN(xe, w)
          we = xe;
        }
      } else {
        if (we > xe) WIDEN(we, x)
      }
      goto signed_matching;
      #undef WIDEN
    #else
      if (!elInt(we) | !elInt(xe)) goto generic;
      we = el_i32;
      w=toI32Any(w); x=toI32Any(x);
      goto signed32;
    #endif
  }
  
  if (0) signed32: {
    assert(we==el_c32 || we==el_i32);
    #if SINGELI
      signed_matching:;
      u8 k = elwByteLog(we);
      u8 rl = wia<128 ? 0 : wia<(1<<15) ? 1 : wia<(1U<<31) ? 2 : 3;
      void *rp = m_tyarrc(&r, 1<<rl, x, el2t(el_i8+rl));
      si_bins[k*2 + GRADE_UD(0,1)](tyany_ptr(w), wia, tyany_ptr(x), xia, rp, rl);
      if (!q_z(mult)) r = C2(mul, mult, r);
    #else
      i32* rp; r = m_i32arrc(&rp, x);
      i32* wi = tyany_ptr(w);
      i32* xi = tyany_ptr(x);
      for (usz i = 0; i < xia; i++) {
        i32 c = xi[i];
        i32 *s = wi-1;
        for (usz l = wia+1, h; (h=l/2)>0; l-=h) { i32* m = s+h; if (!(c LT *m)) s = m; }
        rp[i] = s - (wi-1);
      }
    #endif
    goto done;
  }
  
  if (0) {
    copysh_done:;
    r = taga(arr_shCopy(ra, x));
    
    done:
    decG(w);decG(x);
    return r;
  }
}
#undef GRADE_CHR
#undef LE_FN

#undef LT
#undef FOR
#undef PRE
#undef INC
#undef ROFF
#undef PRE64
#undef INSERTION_SORT
#undef INS_SORT
#undef INS_GRADE
#undef CHOOSE_SG_SORT
#undef CHOOSE_SG_GRADE
#undef RADIX_SORT_i8
#undef RADIX_SORT_i16
#undef RADIX_SORT_i32
#undef GRADE_CAT
#undef GRADE_NAME
#undef GRADE_NEG
#undef GRADE_UD
