// Ordering functions: Sort (∧∨), Grade (⍋⍒), Bins (⍋⍒)

// Sort and Grade
// Small ≠𝕩 sort (not grade): insertion sort
//   SHOULD implement insertion grade
// Small range sort (not grade): counting sort
//   Count 1s for booleans
//   Large range/length ("sparse") uses scan: plus, or Singeli max/min
//   COULD range check 4-byte 𝕩 to try counting sort
// 4-byte grade checks range and sum
//   If consistent with a permutation, grade as one and verify after
//   If small-range, grade with bucket sort
// Other 1-, 2-, 4-byte cases: radix sort
//   Singeli +` used if available, speeding up shorter cases
//   SHOULD skip steps where all bytes are equal
// General case: Timsort
// SHOULD check for sorted flag and scan for sortedness in all cases
// SHOULD use an adaptive quicksort for 4- and 8-byte arguments
// SHOULD widen odd cell sizes under 8 bytes in sort and grade

// Bins
// Mixed integer and character arguments gives all 0 or ≠𝕨
// Integers and characters: 4-byte branchless binary search
// General case: branching binary search
// SHOULD implement f64 branchless binary search
// SHOULD interleave multiple branchless binary searches
// SHOULD specialize bins on equal types at least
// SHOULD implement table-based ⍋⍒ for small-range 𝕨
// SHOULD special-case short 𝕨
// SHOULD partition 𝕩 when 𝕨 is large

#define GRADE_CAT(N) CAT(GRADE_UD(gradeUp,gradeDown),N)
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

#define SORT_CMP(W, X) (GRADE_NEG ((W).k - (i64)(X).k))
#define SORT_NAME GRADE_CAT(IP)
#define SORT_TYPE I32I32p
#include "sortTemplate.h"


#define LT GRADE_UD(<,>)
#define FOR(I,MAX) GRADE_UD(for (usz I=0; I<MAX; I++), \
                            for (usz I=MAX; I--; ))

#define INSERTION_SORT(T) \
  rp[0] = xp[0];                                             \
  for (usz i=0; i<n; i++) {                                  \
    usz j=i, jn; T xi=xp[i], rj;                             \
    while (0<j && xi LT (rj=rp[jn=j-1])) { rp[j]=rj; j=jn; } \
    rp[j] = xi;                                              \
  }

#if SINGELI_AVX2
extern void (*const avx2_scan_max_i8)(int8_t* v0,int8_t* v1,uint64_t v2);
extern void (*const avx2_scan_min_i8)(int8_t* v0,int8_t* v1,uint64_t v2);
extern void (*const avx2_scan_max_i16)(int16_t* v0,int16_t* v1,uint64_t v2);
extern void (*const avx2_scan_min_i16)(int16_t* v0,int16_t* v1,uint64_t v2);
#define COUNT_THRESHOLD 32
#define WRITE_SPARSE_i8 \
  for (usz i=0; i<n; i++) rp[i]=j;                       \
  while (ij<n) { rp[ij]=GRADE_UD(++j,--j); ij+=c0o[j]; } \
  GRADE_UD(avx2_scan_max_i8,avx2_scan_min_i8)(rp,rp,n);
#define WRITE_SPARSE_i16 \
  usz b = 1<<10;                                              \
  for (usz k=0; ; ) {                                         \
    usz e = b<n-k? k+b : n;                                   \
    for (usz i=k; i<e; i++) rp[i]=j;                          \
    while (ij<e) { rp[ij]=GRADE_UD(++j,--j); ij+=c0o[j]; }    \
    GRADE_UD(avx2_scan_max_i16,avx2_scan_min_i16)(rp+k,rp+k,e-k); \
    if (e==n) {break;}  k=e;                                  \
  }
#define WRITE_SPARSE(T) WRITE_SPARSE_##T
extern i8 (*const avx2_count_i8)(usz*, i8*, u64, i8);
#define SINGELI_COUNT_OR(T) \
  if (1==sizeof(T)) avx2_count_i8(c0o, (i8*)xp, n, -128); else
#else
#define COUNT_THRESHOLD 16
#define WRITE_SPARSE(T) \
  for (usz i=0; i<n; i++) rp[i]=0;                       \
  usz js = j;                                            \
  while (ij<n) { rp[ij]GRADE_UD(++,--); ij+=c0o[GRADE_UD(++j,--j)]; } \
  for (usz i=0; i<n; i++) js=rp[i]+=js;
#define SINGELI_COUNT_OR(T)
#endif

#define COUNTING_SORT(T) \
  usz C=1<<(8*sizeof(T));                              \
  TALLOC(usz, c0, C); usz *c0o=c0+C/2;                 \
  for (usz j=0; j<C; j++) c0[j]=0;                     \
  SINGELI_COUNT_OR(T) for (usz i=0; i<n; i++) c0o[xp[i]]++; \
  if (n/(COUNT_THRESHOLD*sizeof(T)) <= C) { /* Scan-based */ \
    T j=GRADE_UD(-C/2,C/2-1);                          \
    usz ij; while ((ij=c0o[j])==0) GRADE_UD(j++,j--);  \
    WRITE_SPARSE(T)                                    \
  } else { /* Branchy */                               \
    FOR(j,C) for (usz c=c0[j]; c--; ) *rp++ = j-C/2;   \
  }                                                    \
  TFREE(c0)

// Radix sorting
#include "radix.h"
#define INC(P,I) GRADE_UD((P+1)[I]++,P[I]--)
#define ROFF GRADE_UD(1,0) // Radix offset

#define CHOOSE_SG_SORT(S,G) S
#define CHOOSE_SG_GRADE(S,G) G

#define RADIX_SORT_i8(T, TYP) \
  TALLOC(T, c0, 256+ROFF); T* c0o=c0+128;  \
  for (usz j=0; j<256; j++) c0[j]=0;       \
  GRADE_UD(,c0[0]=n;)                      \
  for (usz i=0; i<n; i++) INC(c0o,xp[i]);  \
  RADIX_SUM_1_##T;                         \
  for (usz i=0; i<n; i++) { i8 xi=xp[i];   \
    rp[c0o[xi]++]=CHOOSE_SG_##TYP(xi,i); } \
  TFREE(c0)

#define RADIX_SORT_i16(T, TYP, I) \
  TALLOC(u8, alloc, (2*256+ROFF)*sizeof(T) + n*(2 + CHOOSE_SG_##TYP(0,sizeof(I)))); \
  T* c0=(T*)alloc; T* c1=c0+256; T* c1o=c1+128;                              \
  for (usz j=0; j<2*256; j++) c0[j]=0;                                       \
  c1[0]=GRADE_UD(-n,c0[0]=n);                                                \
  for (usz i=0; i<n; i++) { i16 v=xp[i]; INC(c0,(u8)v); INC(c1o,(i8)(v>>8)); } \
  RADIX_SUM_2_##T;                                                           \
  i16 *r0 = (i16*)(c0+2*256);                                                \
  CHOOSE_SG_##TYP(                                                           \
  for (usz i=0; i<n; i++) { i16 v=xp[i]; r0[c0 [(u8)v     ]++]=v; }          \
  for (usz i=0; i<n; i++) { i16 v=r0[i]; rp[c1o[(i8)(v>>8)]++]=v; }          \
  ,                                                                          \
  I *g0 = (i32*)(r0+n);                                                      \
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
  if (isAtm(x) || RNK(x)==0) thrM(GRADE_UD("∧","∨")": Argument cannot have rank 0");
  usz n = *SH(x);
  if (n <= 1) return x;
  if (RNK(x)!=1) return IA(x)<=1? x : bqn_merge(SORT_C1(t, toCells(x)), 0);
  u8 xe = TI(x,elType);
  B r;
  if (xe==el_bit) {
    u64* xp = bitarr_ptr(x);
    u64* rp; r = m_bitarrv(&rp, n);
    usz sum = bit_sum(xp, n);
    u64 n0 = GRADE_UD(n-sum, sum);
    u64 ones = -1ull;
    u64 v0 = GRADE_UD(0, ones);
    usz i=0, e=(n+63)/64;
    for (; i<n0/64; i++) rp[i]=v0;
    if (i<e) rp[i++]=v0^(ones<<(n0%64));
    for (; i<e; i++) rp[i]=~v0;
  } else if (xe==el_i8) {
    i8* xp = i8any_ptr(x);
    i8* rp; r = m_i8arrv(&rp, n);
    if (n < 16) {
      INSERTION_SORT(i8);
    } else if (n < 256) {
      RADIX_SORT_i8(u8, SORT);
    } else {
      COUNTING_SORT(i8);
    }
  } else if (xe==el_i16) {
    i16* xp = i16any_ptr(x);
    i16* rp; r = m_i16arrv(&rp, n);
    if (n < 20) {
      INSERTION_SORT(i16);
    } else if (n < 256) {
      RADIX_SORT_i16(u8, SORT,);
    } else if (n < 1<<15) {
      RADIX_SORT_i16(u32, SORT,);
    } else {
      COUNTING_SORT(i16);
    }
  } else if (xe==el_i32) {
    i32* xp = i32any_ptr(x);
    i32* rp; r = m_i32arrv(&rp, n);
    if (n < 32) {
      INSERTION_SORT(i32);
    } else if (n < 256) {
      RADIX_SORT_i32(u8, SORT,);
    } else {
      RADIX_SORT_i32(u32, SORT,);
    }
  } else {
    B xf = getFillR(x);
    HArr* r0 = (HArr*)cpyHArr(incG(x));
    CAT(GRADE_UD(bA,bD),tim_sort)(r0->a, n);
    r = withFill(taga(r0), xf);
  }
  decG(x);
  return FL_SET(r, CAT(fl,GRADE_UD(asc,dsc)));
}
#undef SORT_C1
#undef INSERTION_SORT
#undef COUNTING_SORT
#undef SINGELI_COUNT_OR
#if SINGELI_AVX2
#undef WRITE_SPARSE_i8
#undef WRITE_SPARSE_i16
#endif


extern Arr* bitUD[3]; // from fns.c
extern B bit2x[2];
extern B grade_bool(B x, usz ia, bool up); // slash.c

#define GRADE_CHR GRADE_UD("⍋","⍒")
B GRADE_CAT(c1)(B t, B x) {
  if (isAtm(x) || RNK(x)==0) thrM(GRADE_CHR": Argument cannot be a unit");
  if (RNK(x)>1) x = toCells(x);
  usz ia = IA(x);
  B r;
  if (ia<=2) {
    if (ia==2) { SGetU(x); r = incG(bit2x[!(compare(GetU(x,0), GetU(x,1)) GRADE_UD(<=,>=) 0)]); }
    else if (ia==1) r = taga(ptr_inc(bitUD[1]));
    else r = emptyIVec();
    decG(x);
    return r;
  }
  
  u8 xe = TI(x,elType);
  if (xe==el_bit) return grade_bool(x, ia, GRADE_UD(1,0));
  if (ia>I32_MAX) thrM(GRADE_CHR": Argument too large");
  i32* rp; r = m_i32arrv(&rp, ia);
  if (xe==el_i8 && ia>8) {
    i8* xp = i8any_ptr(x); usz n=ia;
    RADIX_SORT_i8(usz, GRADE);
    goto decG_sq;
  } else if (xe==el_i16 && ia>16) {
    i16* xp = i16any_ptr(x); usz n = ia;
    RADIX_SORT_i16(usz, GRADE, i32);
    goto decG_sq;
  }
  if (xe==el_i32 || xe==el_c32) { // safe to use the same comparison for i32 & c32 as c32 is 0≤x≤1114111
    el32:;
    i32* xp = tyany_ptr(x);
    i32 min=I32_MAX, max=I32_MIN;
    u32 sum=0;
    for (usz i = 0; i < ia; i++) {
      i32 c = xp[i];
      sum += (u32)c;
      if (c<min) min=c;
      if (c>max) max=c;
    }
    u64 range = max - (i64)min + 1;
    if (range/2 < ia) {
      // First try to invert it as a permutation
      if (range==ia && sum==(u32)(i32)((i64)ia*(min+max)/2)) {
        for (usz i = 0; i < ia; i++) rp[i]=ia;
        for (usz i = 0; i < ia; i++) { i32 v=xp[i]; GRADE_UD(rp[v-min],rp[max-v])=i; }
        bool done=1; for (usz i = 0; i < ia; i++) done &= rp[i]!=ia;
        if (done) goto decG_sq;
      }
      TALLOC(usz, c0, range); usz *c0o=c0-min;
      for (usz i = 0; i < range; i++) c0[i] = 0;
      for (usz i = 0; i < ia; i++) c0o[xp[i]]++;
      usz s=0; FOR (i, range) { usz p=s; s+=c0[i]; c0[i]=p; }
      for (usz i = 0; i < ia; i++) rp[c0o[xp[i]]++] = i;
      TFREE(c0); goto decG_sq;
    }
    if (ia > 40) {
      usz n=ia;
      RADIX_SORT_i32(usz, GRADE, i32);
      goto decG_sq;
    }
    
    TALLOC(I32I32p, tmp, ia);
    for (usz i = 0; i < ia; i++) {
      tmp[i].v = i;
      tmp[i].k = xp[i];
    }
    CAT(GRADE_CAT(IP),tim_sort)(tmp, ia);
    for (usz i = 0; i < ia; i++) rp[i] = tmp[i].v;
    TFREE(tmp);
    goto decG_sq;
  }
  if (elChr(xe)) { x = taga(cpyC32Arr(x)); goto el32; }
  
  SLOW1(GRADE_CHR"𝕩", x);
  generic_grade(x, ia, r, rp, CAT(GRADE_CAT(BP),tim_sort));
  goto decG_sq;
  
  decG_sq:;
  if (ia<=(I8_MAX+1)) r = taga(cpyI8Arr(r));
  else if (ia<=(I16_MAX+1)) r = taga(cpyI16Arr(r));
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
  #define CASE(T) case el_##T: { \
    T* xp = T##any_ptr(x); CMP(xp[i-1] GRADE_UD(>,<) xp[i]) }
  switch (TI(x,elType)) { default: UD;
    CASE(i8) CASE(i16) CASE(i32) CASE(f64)
    CASE(c8) CASE(c16) CASE(c32)
    case el_bit: {
      #define HI GRADE_UD(1,0)
      u64* xp = bitarr_ptr(x);
      u64 i = bit_find(xp, xia, HI);
      usz iw = i/64;
      u64 m = ~(u64)0;
      u64 d = GRADE_UD(,~)xp[iw] ^ (m<<(i%64));
      if (iw == xia/64) return (d &~ (m<<(xia%64))) == 0;
      if (d) return 0;
      usz o = iw + 1;
      usz l = xia - 64*o;
      return (bit_find(xp+o, l, !HI) == l);
      #undef HI
    }
    case el_B: {
      B* xp = TO_BPTR(x);
      CMP(compare(xp[i-1], xp[i]) GRADE_UD(>,<) 0)
    }
  }
  #undef CASE
  #undef CMP
}

B GRADE_CAT(c2)(B t, B w, B x) {
  if (isAtm(w) || RNK(w)==0) thrM(GRADE_CHR": 𝕨 must have rank≥1");
  if (isAtm(x)) x = m_unit(x);
  ur wr = RNK(w);
  ur xr = RNK(x);
  
  if (wr > 1) {
    if (wr > xr+1) thrM(GRADE_CHR": =𝕨 cannot be greater than =𝕩");
    i32 nxr = xr-wr+1;
    x = toKCells(x, nxr); xr = nxr;
    w = toCells(w);       xr = 1;
  }
  
  u8 we = TI(w,elType); usz wia = IA(w);
  u8 xe = TI(x,elType); usz xia = IA(x);
  
  if (wia>I32_MAX-10) thrM(GRADE_CHR": 𝕨 too big");
  
  u8 fl = GRADE_UD(fl_asc,fl_dsc);
  if (CHECK_VALID && !FL_HAS(w,fl)) {
    if (!CAT(isSorted,GRADE_UD(Up,Down))(w)) thrM(GRADE_CHR": 𝕨 must be sorted"GRADE_UD(," in descending order"));
    FL_SET(w, fl);
  }

  i32* rp; B r = m_i32arrc(&rp, x);
  
  if (LIKELY(we<el_B & xe<el_B)) {
    if (elNum(we)) { // number
      if (elNum(xe)) {
        if (!elInt(we) | !elInt(xe)) {
          #if SINGELI
          if (we<=el_f64 && xe<=el_f64) {
            w=toF64Any(w); x=toF64Any(x);
            f64* wi = tyany_ptr(w);
            f64* xi = tyany_ptr(x);
            si_bins[3*2 + GRADE_UD(0,1)](wi, wia, xi, xia, rp);
            goto done;
          }
          #endif
          goto gen;
        }
        w=toI32Any(w); x=toI32Any(x);
      } else {
        for (u64 i=0; i<xia; i++) rp[i]=wia;
        goto done;
      }
    } else { // character
      if (elNum(xe)) {
        Arr* ra=allZeroes(xia); arr_shVec(ra);
        decG(r); r=taga(ra); goto done;
      } else {
        w=toC32Any(w); x=toC32Any(x);
      }
    }
    i32* wi = tyany_ptr(w);
    i32* xi = tyany_ptr(x);

    #if SINGELI
    si_bins[2*2 + GRADE_UD(0,1)](wi, wia, xi, xia, rp);
    #else
    for (usz i = 0; i < xia; i++) {
      i32 c = xi[i];
      i32 *s = wi-1;
      for (usz l = wia+1, h; (h=l/2)>0; l-=h) { i32* m = s+h; if (!(c LT *m)) s = m; }
      rp[i] = s - (wi-1);
    }
    #endif
  } else {
    gen:;
    SGetU(x)
    SLOW2("𝕨"GRADE_CHR"𝕩", w, x);
    B* wp = TO_BPTR(w);
    
    for (usz i = 0; i < xia; i++) {
      B c = GetU(x,i);
      usz s = 0, e = wia+1;
      while (e-s > 1) {
        usz m = (s+e) / 2;
        if (compare(c, wp[m-1]) LT 0) e = m;
        else s = m;
      }
      rp[i] = s;
    }
  }
done:
  decG(w);decG(x);
  return r;
}
#undef GRADE_CHR

#undef LT
#undef FOR
#undef PRE
#undef INC
#undef ROFF
#undef PRE64
#undef CHOOSE_SG_SORT
#undef CHOOSE_SG_GRADE
#undef RADIX_SORT_i8
#undef RADIX_SORT_i16
#undef RADIX_SORT_i32
#undef GRADE_CAT
#undef GRADE_NEG
#undef GRADE_UD
