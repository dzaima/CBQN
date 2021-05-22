#include "h.h"


#define CAT0(A,B) A##_##B
#define CAT(A,B) CAT0(A,B)
typedef struct BI32p { B k; i32 v; } BI32p;
typedef struct I32I32p { i32 k; i32 v; } I32I32p;

#define GRADE_N gradeUp
#define GRADE_CHR "⍋"
#define GRADE_NEG
#define GRADE_UP 1
#define GRADE_UD(U,D) U
#include "grade.c"
#define GRADE_N gradeDown
#define GRADE_CHR "⍒"
#define GRADE_NEG -
#define GRADE_UP 0
#define GRADE_UD(U,D) D
#include "grade.c"

#define SORT_CMP(W, X) compare(W, X)
#define SORT_NAME b
#define SORT_TYPE B
#include "sortTemplate.c"

#define SORT_CMP(W, X) ((W) - (i64)(X))
#define SORT_NAME i
#define SORT_TYPE i32
#include "sortTemplate.c"



int sort_icmp(const void* w, const void* x) { return *(int*)w - *(int*)x; }
int sort_bcmp(const void* w, const void* x) { return compare(*(B*)w, *(B*)x); }
B rt_sortAsc;
B and_c1(B t, B x) {
  if (isAtm(x) || rnk(x)==0) thrM("∧: Argument cannot have rank 0");
  if (rnk(x)!=1) return bqn_merge(and_c1(t, toCells(x)));
  usz xia = a(x)->ia;
  if (TI(x).elType==el_i32) {
    i32* xp = i32any_ptr(x);
    i32* rp; B r = m_i32arrv(&rp, xia);
    memcpy(rp, xp, xia*4);
    i_tim_sort(rp, xia);
    dec(x);
    return r;
  }
  B xf = getFillQ(x);
  HArr_p r = m_harrUv(xia);
  BS2B xget = TI(x).get;
  for (usz i = 0; i < xia; i++) r.a[i] = xget(x,i);
  b_tim_sort(r.a, xia);
  dec(x);
  return withFill(r.b,xf);
}

#define F(A,M,D) A(gradeUp) A(gradeDown)
BI_FNS0(F);
static inline void sort_init() { BI_FNS1(F) }
#undef F
