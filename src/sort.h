#pragma once
#include "h.h"

#define SORT_CMP(W, X) compare(W, X)
#define SORT_NAME b
#define SORT_TYPE B
#include "sortTemplate.c"

#define SORT_CMP(W, X) ((W) - (i64)(X))
#define SORT_NAME i
#define SORT_TYPE i32
#include "sortTemplate.c"

typedef struct BI32p { B k; i32 v; } BI32p;
#define SORT_CMP(W, X) compare((W).k, (X).k)
#define SORT_NAME bp
#define SORT_TYPE BI32p
#include "sortTemplate.c"

typedef struct I32I32p { i32 k; i32 v; } I32I32p;
#define SORT_CMP(W, X) ((W).k - (i64)(X).k)
#define SORT_NAME ip
#define SORT_TYPE I32I32p
#include "sortTemplate.c"
