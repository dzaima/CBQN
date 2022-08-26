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

#define SORT_UD(U,D) U
#include "sort.h"
#define SORT_UD(U,D) D
#include "sort.h"
