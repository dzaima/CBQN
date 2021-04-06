#include "h.h"



B val_c1(B d,      B x) { return c1(c(Md2D,d)->f,   x); }
B val_c2(B d, B w, B x) { return c2(c(Md2D,d)->g, w,x); }

B fillBy_c1(B d,      B x) { return c1(c(Md2D,d)->f,   x); }
B fillBy_c2(B d, B w, B x) { return c2(c(Md2D,d)->f, w,x); }

#define ba(NAME) bi_##NAME = mm_alloc(sizeof(Md2), t_md2_def, ftag(MD2_TAG)); c(Md2,bi_##NAME)->c2 = NAME##_c2; c(Md2,bi_##NAME)->c1 = NAME##_c1;
#define bd(NAME) bi_##NAME = mm_alloc(sizeof(Md2), t_md2_def, ftag(MD2_TAG)); c(Md2,bi_##NAME)->c2 = NAME##_c2; c(Md2,bi_##NAME)->c1 = c1_invalid;
#define bm(NAME) bi_##NAME = mm_alloc(sizeof(Md2), t_md2_def, ftag(MD2_TAG)); c(Md2,bi_##NAME)->c2 = c2_invalid;c(Md2,bi_##NAME)->c1 = NAME##_c1;

B                 bi_val, bi_fillBy;
void md2_init() { ba(val) ba(fillBy) }

#undef ba
#undef bd
#undef bm
