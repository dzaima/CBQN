#include "h.h"



B val_c1(B t, B f, B g,      B x) { return c1(f,  x); }
B val_c2(B t, B f, B g, B w, B x) { return c2(g,w,x); }

B fillBy_c1(B t, B f, B g,      B x) { return c1(f,  x); }
B fillBy_c2(B t, B f, B g, B w, B x) { return c2(f,w,x); }

#define ba(NAME) bi_##NAME = mm_alloc(sizeof(Md2), t_md2_def, ftag(MD2_TAG)); c(Md2,bi_##NAME)->c2 = NAME##_c2; c(Md2,bi_##NAME)->c1 = NAME##_c1;
#define bd(NAME) bi_##NAME = mm_alloc(sizeof(Md2), t_md2_def, ftag(MD2_TAG)); c(Md2,bi_##NAME)->c2 = NAME##_c2; c(Md2,bi_##NAME)->c1 = c1_invalid;
#define bm(NAME) bi_##NAME = mm_alloc(sizeof(Md2), t_md2_def, ftag(MD2_TAG)); c(Md2,bi_##NAME)->c2 = c2_invalid;c(Md2,bi_##NAME)->c1 = NAME##_c1;

B                 bi_val, bi_fillBy;
void md2_init() { ba(val) ba(fillBy) }

#undef ba
#undef bd
#undef bm
