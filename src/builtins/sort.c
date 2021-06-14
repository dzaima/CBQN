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

B and_c1(B t, B x) {
  if (isAtm(x) || rnk(x)==0) thrM("∧: Argument cannot have rank 0");
  if (rnk(x)!=1) return bqn_merge(and_c1(t, toCells(x)));
  usz xia = a(x)->ia;
  if (TI(x).elType==el_i32) {
    i32* xp = i32any_ptr(x);
    i32* rp; B r = m_i32arrv(&rp, xia);
    memcpy(rp, xp, xia*4);
    iA_tim_sort(rp, xia);
    dec(x);
    return r;
  }
  B xf = getFillQ(x);
  HArr_p r = m_harrUv(xia);
  BS2B xget = TI(x).get;
  for (usz i = 0; i < xia; i++) r.a[i] = xget(x,i);
  bA_tim_sort(r.a, xia);
  dec(x);
  return withFill(r.b,xf);
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
  if (isAtm(x) || rnk(x)==0) thrM("∨: Argument cannot have rank 0");
  if (rnk(x)!=1) return bqn_merge(or_c1(t, toCells(x)));
  usz xia = a(x)->ia;
  if (TI(x).elType==el_i32) {
    i32* xp = i32any_ptr(x);
    i32* rp; B r = m_i32arrv(&rp, xia);
    memcpy(rp, xp, xia*4);
    iD_tim_sort(rp, xia);
    dec(x);
    return r;
  }
  B xf = getFillQ(x);
  HArr_p r = m_harrUv(xia);
  BS2B xget = TI(x).get;
  for (usz i = 0; i < xia; i++) r.a[i] = xget(x,i);
  bD_tim_sort(r.a, xia);
  dec(x);
  return withFill(r.b,xf);
}
