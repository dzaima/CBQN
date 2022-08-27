#define SORT_C1 CAT(SORT_UD(and,or),c1)
#define LT SORT_UD(<,>)

#define TIM_B SORT_UD(bA,bD)
#define TIM_I SORT_UD(iA,iD)

#define SORT_CMP(W, X) SORT_UD(compare(W, X), compare(X, W))
#define SORT_NAME TIM_B
#define SORT_TYPE B
#include "sortTemplate.h"

#define SORT_CMP(W, X) (SORT_UD((W) - (i64)(X), (X) - (i64)(W)))
#define SORT_NAME TIM_I
#define SORT_TYPE i32
#include "sortTemplate.h"

#define FOR(I,MAX) SORT_UD(for (usz I=0; I<MAX; I++), for (usz I=MAX; I--; ))

#define PRE_UD(K,SL,SRE) \
  u64 p##K=s##K; s##K+=cw##K[j]; \
  s##K+=s##K SL 8; s##K+=s##K SL 16; s##K+=s##K SL 32; \
  cw##K[j] = p##K|(s##K SL 8); s##K SRE 56
#define PRE64(K) SORT_UD(PRE_UD(K,<<,>>=), PRE_UD(K,>>,<<=))

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
    SORT_UD(                                                 \
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

B SORT_C1(B t, B x) {
  if (isAtm(x) || RNK(x)==0) thrM(SORT_UD("∧","∨")": Argument cannot have rank 0");
  if (RNK(x)!=1) return bqn_merge(SORT_C1(t, toCells(x)));
  usz xia = IA(x);
  if (xia <= 1) return x;
  u8 xe = TI(x,elType);
  B r;
  if (xe==el_i8) {
    i8* xp = i8any_ptr(x); usz n=xia;
    i8* rp; r = m_i8arrv(&rp, n);
    if (n<16) {
      INSERTION_SORT(i8);
    } else if (n<=256) { // Radix/bucket sort
      TALLOC(u8, c0, 256); u8 *c0o=c0+128; // Offset for signedness
      for (usz j=0; j<256; j++) c0[j]=0;
      for (usz i=0; i<n; i++) c0o[xp[i]]++;
      u64 s=0; u64 *cw=(u64*)c0;
      FOR(j, 256/8) { PRE64(); } // Prefix sum
      for (usz i=0; i<n; i++) { i8 xi=xp[i]; u8 c=c0o[xi]++; rp[c]=xi; }
      TFREE(c0);
    } else {
      COUNTING_SORT(i8);
    }
  } else if (xe==el_i16) {
    i16* xp = i16any_ptr(x); usz n=xia;
    i16* rp; r = m_i16arrv(&rp, n);
    if (n < 24) {
      INSERTION_SORT(i16);
    } else if (n <= 256) { // Radix sort, 1-byte counts
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
        u64 s0=0; u64 s1=0; u64 *cw0=(u64*)c0; u64 *cw1=(u64*)c1;
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
    i32* rp; r = m_i32arrv(&rp, xia);
    memcpy(rp, xp, xia*4);
    CAT(TIM_I,tim_sort)(rp, xia);
  } else {
    B xf = getFillQ(x);
    HArr_p ra = m_harrUv(xia);
    SGet(x)
    for (usz i = 0; i < xia; i++) ra.a[i] = Get(x,i);
    CAT(TIM_B,tim_sort)(ra.a, xia);
    r = withFill(ra.b,xf);
  }
  decG(x);
  return FL_SET(r, CAT(fl,SORT_UD(asc,dsc)));
}
#undef SORT_C1
#undef LT
#undef TIM_B
#undef TIM_I
#undef FOR
#undef PRE_UD
#undef PRE64
#undef INSERTION_SORT
#undef COUNTING_SORT
#undef SORT_UD
