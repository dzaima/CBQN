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

#define PRE_UD(K,SL,SRE) \
  u64 p##K=s##K; s##K+=((u64*)c##K)[j];                \
  s##K+=s##K SL 8; s##K+=s##K SL 16; s##K+=s##K SL 32; \
  ((u64*)c##K)[j] = p##K|(s##K SL 8); s##K SRE 56
#define PRE64(K) GRADE_UD(PRE_UD(K,<<,>>=), PRE_UD(K,>>,<<=))

#define INSERTION_SORT(T) \
  rp[0] = xp[0];                                             \
  for (usz i=0; i<n; i++) {                                  \
    usz j=i, jn; T xi=xp[i], rj;                             \
    while (0<j && xi LT (rj=rp[jn=j-1])) { rp[j]=rj; j=jn; } \
    rp[j] = xi;                                              \
  }

#define COUNTING_SORT(T) \
  usz C=1<<(8*sizeof(T));                                    \
  TALLOC(usz, c0, C); usz *c0o=c0+C/2;                       \
  for (usz j=0; j<C; j++) c0[j]=0;                           \
  for (usz i=0; i<n; i++) c0o[xp[i]]++;                      \
  if (n <= C*8) { /* Sum-based */                            \
    for (usz i=1; i<n; i++) rp[i]=0;                         \
    GRADE_UD(                                                \
      rp[0]=-C/2;  usz e=C-1; while (c0[e]==0) e--;          \
      for (usz j=0  , i=0; j<e; j++) { i+=c0[j]; rp[i]++; }  \
    ,                                                        \
      rp[0]=C/2-1; usz e=0;   while (c0[e]==0) e++;          \
      for (usz j=C-1, i=0; j>e; j--) { i+=c0[j]; rp[i]--; }  \
    )                                                        \
    for (usz i=1; i<n; i++) rp[i]+=rp[i-1];                  \
  } else { /* Branchy */                                     \
    FOR(j,C) for (usz c=c0[j]; c--; ) *rp++ = j-C/2;         \
  }                                                          \
  TFREE(c0)

#define SORT_C1 CAT(GRADE_UD(and,or),c1)
B SORT_C1(B t, B x) {
  if (isAtm(x) || RNK(x)==0) thrM(GRADE_UD("∧","∨")": Argument cannot have rank 0");
  if (RNK(x)!=1) return bqn_merge(SORT_C1(t, toCells(x)));
  usz n = IA(x);
  if (n <= 1) return x;
  u8 xe = TI(x,elType);
  B r;
  if (xe==el_i8) {
    i8* xp = i8any_ptr(x);
    i8* rp; r = m_i8arrv(&rp, n);
    if (n<16) {
      INSERTION_SORT(i8);
    } else if (n<256) { // Radix/bucket sort
      TALLOC(u8, c0, 256); u8 *c0o=c0+128; // Offset for signedness
      for (usz j=0; j<256; j++) c0[j]=0;
      for (usz i=0; i<n; i++) c0o[xp[i]]++;
      u64 s0=0; FOR(j, 256/8) { PRE64(0); } // Prefix sum
      for (usz i=0; i<n; i++) { i8 xi=xp[i]; u8 c=c0o[xi]++; rp[c]=xi; }
      TFREE(c0);
    } else {
      COUNTING_SORT(i8);
    }
  } else if (xe==el_i16) {
    i16* xp = i16any_ptr(x);
    i16* rp; r = m_i16arrv(&rp, n);
    if (n < 24) {
      INSERTION_SORT(i16);
    } else if (n < 256) { // Radix sort, 1-byte counts
      #define RADIX2(T, PRE_SUM) \
        TALLOC(u8, alloc, 2*256*sizeof(T) + n*2);                                \
        T *c0=(T*)alloc; T *c1=c0+256; T *c1o=c1+128;                            \
        for (usz j=0; j<2*256; j++) c0[j]=0;                                     \
        for (usz i=0; i<n; i++) { i16 v=xp[i]; c0[(u8)v]++; c1o[(i8)(v>>8)]++; } \
        PRE_SUM;                                                                 \
        i16 *r0 = (i16*)(c0+2*256);                                              \
        for (usz i=0; i<n; i++) { i16 v=xp[i]; T c=c0 [(u8)v     ]++; r0[c]=v; } \
        for (usz i=0; i<n; i++) { i16 v=r0[i]; T c=c1o[(i8)(v>>8)]++; rp[c]=v; } \
        TFREE(alloc)
      RADIX2(u8,
        u64 s0=0; u64 s1=0;
        FOR(j, 256/8) { PRE64(0); PRE64(1); }
      );
    } else if (n < 1<<15) { // Radix sort
      RADIX2(u32,
        u32 s0=0; u32 s1=0;
        FOR(j, 256) {
          u32 p0=s0; s0+=c0[j]; c0[j]=p0;
          u32 p1=s1; s1+=c1[j]; c1[j]=p1;
        }
      );
      #undef RADIX2
    } else {
      COUNTING_SORT(i16);
    }
  } else if (xe==el_i32) {
    i32* xp = i32any_ptr(x);
    i32* rp; r = m_i32arrv(&rp, n);
    if (n < 40) {
      INSERTION_SORT(i32);
    } else if (n < 256) {
      #define RADIX4(T, PRE_SUM) \
        TALLOC(u8, alloc, 4*256*sizeof(T) + n*4);                                 \
        T *c0=(T*)alloc, *c1=c0+256, *c2=c1+256, *c3=c2+256, *c3o=c3+128;         \
        for (usz j=0; j<4*256; j++) c0[j]=0;                                      \
        for (usz i=0; i<n; i++) { i32 v=xp[i];                                    \
          c0 [(u8)v      ]++; c1 [(u8)(v>> 8)]++;                                 \
          c2 [(u8)(v>>16)]++; c3o[(i8)(v>>24)]++; }                               \
        PRE_SUM;                                                                  \
        i32 *r0 = (i32*)(c0+4*256);                                               \
        for (usz i=0; i<n; i++) { i32 v=xp[i]; T c=c0 [(u8)v      ]++; r0[c]=v; } \
        for (usz i=0; i<n; i++) { i32 v=r0[i]; T c=c1 [(u8)(v>> 8)]++; rp[c]=v; } \
        for (usz i=0; i<n; i++) { i32 v=rp[i]; T c=c2 [(u8)(v>>16)]++; r0[c]=v; } \
        for (usz i=0; i<n; i++) { i32 v=r0[i]; T c=c3o[(i8)(v>>24)]++; rp[c]=v; } \
        TFREE(alloc)
      RADIX4(u8,
        u64 s0=0; u64 s1=0; u64 s2=0; u64 s3=0;
        FOR(j, 256/8) { PRE64(0); PRE64(1); PRE64(2); PRE64(3); }
      );
    } else {
      RADIX4(usz,
        usz s0=0; usz s1=0; usz s2=0; usz s3=0;
        FOR(j, 256) {
          u32 p0=s0; s0+=c0[j]; c0[j]=p0;
          u32 p1=s1; s1+=c1[j]; c1[j]=p1;
          u32 p2=s2; s2+=c2[j]; c2[j]=p2;
          u32 p3=s3; s3+=c3[j]; c3[j]=p3;
        }
      );
      #undef RADIX4
    }
  } else {
    B xf = getFillQ(x);
    HArr_p ra = m_harrUv(n);
    SGet(x)
    for (usz i = 0; i < n; i++) ra.a[i] = Get(x,i);
    CAT(GRADE_UD(bA,bD),tim_sort)(ra.a, n);
    r = withFill(ra.b,xf);
  }
  decG(x);
  return FL_SET(r, CAT(fl,GRADE_UD(asc,dsc)));
}
#undef SORT_C1
#undef INSERTION_SORT
#undef COUNTING_SORT


#define GRADE_CHR GRADE_UD("⍋","⍒")
B GRADE_CAT(c1)(B t, B x) {
  if (isAtm(x) || RNK(x)==0) thrM(GRADE_CHR": Argument cannot be a unit");
  if (RNK(x)>1) x = toCells(x);
  usz ia = IA(x);
  if (ia>I32_MAX) thrM(GRADE_CHR": Argument too large");
  if (ia==0) { decG(x); return emptyIVec(); }
  
  u8 xe = TI(x,elType);
  i32* rp; B r = m_i32arrv(&rp, ia);
  if (xe==el_bit) {
    u64* xp = bitarr_ptr(x);
    u64 sum = bit_sum(xp, ia);
    u64 r0 = 0;
    u64 r1 = GRADE_UD(ia-sum, sum);
    for (usz i = 0; i < ia; i++) {
      if (bitp_get(xp,i)^GRADE_UD(0,1)) rp[r1++] = i;
      else                              rp[r0++] = i;
    }
    decG(x); return r;
  } else if (xe==el_i8) {
    i8* xp = i8any_ptr(x);
    i32 min=-128, range=256;
    TALLOC(usz, tmp, range+1);
    for (i64 i = 0; i < range+1; i++) tmp[i] = 0;
    GRADE_UD( // i8 range-based
      for (usz i = 0; i < ia; i++) (tmp-min+1)[xp[i]]++;
      for (i64 i = 1; i < range; i++) tmp[i]+= tmp[i-1];
      for (usz i = 0; i < ia; i++) rp[(tmp-min)[xp[i]]++] = i;
    ,
      for (usz i = 0; i < ia; i++) (tmp-min)[xp[i]]++;
      for (i64 i = range-2; i >= 0; i--) tmp[i]+= tmp[i+1];
      for (usz i = 0; i < ia; i++) rp[(tmp-min+1)[xp[i]]++] = i;
    )
    TFREE(tmp); decG(x);
    return r;
  }
  if (xe==el_i32 || xe==el_c32) { // safe to use the same comparison for i32 & c32 as c32 is 0≤x≤1114111
    i32* xp = tyany_ptr(x);
    i32 min=I32_MAX, max=I32_MIN;
    for (usz i = 0; i < ia; i++) {
      i32 c = xp[i];
      if (c<min) min=c;
      if (c>max) max=c;
    }
    i64 range = max - (i64)min + 1;
    if (range/2 < ia) {
      TALLOC(usz, tmp, range+1);
      for (i64 i = 0; i < range+1; i++) tmp[i] = 0;
      GRADE_UD( // i32 range-based
        for (usz i = 0; i < ia; i++) (tmp-min+1)[xp[i]]++;
        for (i64 i = 1; i < range; i++) tmp[i]+= tmp[i-1];
        for (usz i = 0; i < ia; i++) rp[(tmp-min)[xp[i]]++] = i;
      ,
        for (usz i = 0; i < ia; i++) (tmp-min)[xp[i]]++;
        for (i64 i = range-2; i >= 0; i--) tmp[i]+= tmp[i+1];
        for (usz i = 0; i < ia; i++) rp[(tmp-min+1)[xp[i]]++] = i;
      )
      TFREE(tmp); decG(x);
      return r;
    }
    
    TALLOC(I32I32p, tmp, ia);
    for (usz i = 0; i < ia; i++) {
      tmp[i].v = i;
      tmp[i].k = xp[i];
    }
    CAT(GRADE_CAT(IP),tim_sort)(tmp, ia);
    for (usz i = 0; i < ia; i++) rp[i] = tmp[i].v;
    TFREE(tmp); decG(x);
    return r;
  }
  
  SLOW1(GRADE_CHR"𝕩", x);
  TALLOC(BI32p, tmp, ia);
  SGetU(x)
  for (usz i = 0; i < ia; i++) {
    tmp[i].v = i;
    tmp[i].k = GetU(x,i);
  }
  CAT(GRADE_CAT(BP),tim_sort)(tmp, ia);
  for (usz i = 0; i < ia; i++) rp[i] = tmp[i].v;
  TFREE(tmp); decG(x);
  return r;
}


B GRADE_CAT(c2)(B t, B w, B x) {
  if (isAtm(w) || RNK(w)==0) thrM(GRADE_CHR": 𝕨 must have rank≥1");
  if (isAtm(x)) x = m_atomUnit(x);
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
  i32* rp; B r = m_i32arrc(&rp, x);
  
  u8 fl = GRADE_UD(fl_asc,fl_dsc);
  
  if (we<=el_i32 & xe<=el_i32) {
    w = toI32Any(w); i32* wi = i32any_ptr(w);
    x = toI32Any(x); i32* xi = i32any_ptr(x);
    if (CHECK_VALID && !FL_HAS(w,fl)) {
      for (i64 i = 0; i < (i64)wia-1; i++) if ((wi[i]-wi[i+1]) GRADE_UD(>,<) 0) thrM(GRADE_CHR": 𝕨 must be sorted"GRADE_UD(," in descending order"));
      FL_SET(w, fl);
    }
    
    for (usz i = 0; i < xia; i++) {
      i32 c = xi[i];
      usz s = 0, e = wia+1;
      while (e-s > 1) {
        usz m = (s+(i64)e)/2;
        if (c LT wi[m-1]) e = m;
        else s = m;
      }
      rp[i] = s;
    }
  } else {
    SGetU(x)
    SLOW2("𝕨"GRADE_CHR"𝕩", w, x);
    B* wp = arr_bptr(w);
    if (wp==NULL) {
      HArr* a = toHArr(w);
      wp = a->a;
      w = taga(a);
    }
    if (CHECK_VALID && !FL_HAS(w,fl)) {
      for (i64 i = 0; i < (i64)wia-1; i++) if (compare(wp[i], wp[i+1]) GRADE_UD(>,<) 0) thrM(GRADE_CHR": 𝕨 must be sorted"GRADE_UD(," in descending order"));
      FL_SET(w, fl);
    }
    
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
  decG(w);decG(x);
  return r;
}
#undef GRADE_CHR

#undef LT
#undef FOR
#undef PRE_UD
#undef PRE64
#undef GRADE_CAT
#undef GRADE_NEG
#undef GRADE_UD
