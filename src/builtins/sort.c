#include "../core.h"
#include "../utils/talloc.h"



#define CAT0(A,B) A##_##B
#define CAT(A,B) CAT0(A,B)
typedef struct BI32p { B k; i32 v; } BI32p;
typedef struct I32I32p { i32 k; i32 v; } I32I32p;

#define GRADE_UD(U,D) U
#define GRADE_NEG
#define GRADE_CHR "⍋"
#include "grade.h"
#define GRADE_UD(U,D) D
#define GRADE_NEG -
#define GRADE_CHR "⍒"
#include "grade.h"

#define SORT_CMP(W, X) compare(W, X)
#define SORT_NAME bA
#define SORT_TYPE B
#include "sortTemplate.h"

#define SORT_CMP(W, X) ((W) - (i64)(X))
#define SORT_NAME iA
#define SORT_TYPE i32
#include "sortTemplate.h"

#define INSERTION_SORT(T, CMP) \
  rp[0] = xp[0];                                              \
  for (usz i=0; i<n; i++) {                                   \
    usz j=i, jn; T xi=xp[i], rj;                              \
    while (0<j && xi CMP (rj=rp[jn=j-1])) { rp[j]=rj; j=jn; } \
    rp[j] = xi;                                               \
  }

#define COUNTING_SORT(T, COUNTS) \
  usz C=1<<(8*sizeof(T));                                           \
  TALLOC(usz, c0, C); usz *c0o=c0+C/2;                              \
  for (usz j=0; j<C; j++) c0[j]=0;                                  \
  for (usz i=0; i<n; i++) c0o[xp[i]]++;                             \
  COUNTS(T);                                                        \
  TFREE(c0)
#define COUNTS_UP(T) \
  if (n <= C*8) { /* Sum-based */                                   \
    usz e=C-1; while (c0[e]==0) e--;                                \
    rp[0]=-C/2; for (usz i=1; i<n; i++) rp[i]=0;                    \
    for (usz j=0, i=0; j<e; j++) { i+=c0[j]; rp[i]++; }             \
    for (usz i=1; i<n; i++) rp[i]+=rp[i-1];                         \
  } else { /* Branchy */                                            \
    for (usz j=0; j<C; j++) for (usz c=c0[j]; c--; ) *rp++ = j-C/2; \
  }
#define COUNTS_DOWN(T) \
  if (n <= C*8) { /* Sum-based */                                   \
    usz e=0; while (c0[e]==0) e++;                                  \
    rp[0]=C/2-1; for (usz i=1; i<n; i++) rp[i]=0;                   \
    for (usz j=C-1, i=0; j>e; j--) { i+=c0[j]; rp[i]--; }           \
    for (usz i=1; i<n; i++) rp[i]+=rp[i-1];                         \
  } else { /* Branchy */                                            \
    for (usz j=C; j-->0;  ) for (usz c=c0[j]; c--; ) *rp++ = j-C/2; \
  }

B and_c1(B t, B x) {
  if (isAtm(x) || RNK(x)==0) thrM("∧: Argument cannot have rank 0");
  if (RNK(x)!=1) return bqn_merge(and_c1(t, toCells(x)));
  usz xia = IA(x);
  if (xia <= 1) return x;
  u8 xe = TI(x,elType);
  if (xe<=el_i32) {
    if (xe==el_i8) {
      i8* xp = i8any_ptr(x); usz n=xia;
      i8* rp; B r = m_i8arrv(&rp, n);
      if (n<16) {
        INSERTION_SORT(i8,<);
      } else if (n<=256) {
        // Radix/bucket sort
        TALLOC(u8, c0, 256); u8 *c0o=c0+128; // Offset for signedness
        for (usz j=0; j<256; j++) c0[j]=0;
        for (usz i=0; i<n; i++) c0o[xp[i]]++;
        u64 s=0; u64 *cw=(u64*)c0; for (usz j=0; j<256/8; j++) {
          u64 p=s; s+=cw[j]; s+=s<<8; s+=s<<16; s+=s<<32;
          cw[j] = p|(s<<8); s>>=56;
        }
        for (usz i=0; i<n; i++) { i8 xi=xp[i]; u8 c=c0o[xi]++; rp[c]=xi; }
        TFREE(c0);
      } else {
        COUNTING_SORT(i8, COUNTS_UP);
      }
      decG(x);
      return r;
    }
    if (xe!=el_i32) x = taga(cpyI32Arr(x));
    i32* xp = i32any_ptr(x);
    i32* rp; B r = m_i32arrv(&rp, xia);
    memcpy(rp, xp, xia*4);
    iA_tim_sort(rp, xia);
    decG(x);
    return FL_SET(r, fl_asc);
  }
  B xf = getFillQ(x);
  HArr_p r = m_harrUv(xia);
  SGet(x)
  for (usz i = 0; i < xia; i++) r.a[i] = Get(x,i);
  bA_tim_sort(r.a, xia);
  decG(x);
  return FL_SET(withFill(r.b,xf), fl_asc);
}


#define SORT_CMP(W, X) compare(X, W)
#define SORT_NAME bD
#define SORT_TYPE B
#include "sortTemplate.h"

#define SORT_CMP(W, X) ((X) - (i64)(W))
#define SORT_NAME iD
#define SORT_TYPE i32
#include "sortTemplate.h"

B or_c1(B t, B x) {
  if (isAtm(x) || RNK(x)==0) thrM("∨: Argument cannot have rank 0");
  if (RNK(x)!=1) return bqn_merge(or_c1(t, toCells(x)));
  usz xia = IA(x);
  u8 xe = TI(x,elType);
  if (xe<=el_i32) {
    if (xe==el_i8) {
      i8* xp = i8any_ptr(x); usz n=xia;
      i8* rp; B r = m_i8arrv(&rp, n);
      if (n<16) {
        INSERTION_SORT(i8,>);
      } else if (n<=256) {
        // Radix/bucket sort
        TALLOC(u8, c0, 256); u8 *c0o=c0+128; // Offset for signedness
        for (usz j=0; j<256; j++) c0[j]=0;
        for (usz i=0; i<n; i++) c0o[xp[i]]++;
        u64 s=0; u64 *cw=(u64*)c0; for (usz j=256/8; j--; ) {
          u64 p=s; s+=cw[j]; s+=s>>8; s+=s>>16; s+=s>>32;
          cw[j] = p|(s>>8); s<<=56;
        }
        for (usz i=0; i<n; i++) { i8 xi=xp[i]; u8 c=c0o[xi]++; rp[c]=xi; }
        TFREE(c0);
      } else {
        COUNTING_SORT(i8, COUNTS_DOWN);
      }
      decG(x);
      return r;
    }
    if (xe!=el_i32) x = taga(cpyI32Arr(x));
    i32* xp = i32any_ptr(x);
    i32* rp; B r = m_i32arrv(&rp, xia);
    memcpy(rp, xp, xia*4);
    iD_tim_sort(rp, xia);
    decG(x);
    return FL_SET(r, fl_dsc);
  }
  B xf = getFillQ(x);
  HArr_p r = m_harrUv(xia);
  SGet(x)
  for (usz i = 0; i < xia; i++) r.a[i] = Get(x,i);
  bD_tim_sort(r.a, xia);
  decG(x);
  return FL_SET(withFill(r.b,xf), fl_dsc);
}
