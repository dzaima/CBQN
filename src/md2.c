#include "h.h"



B val_c1(B d,      B x) { return c1(c(Md2D,d)->f,   x); }
B val_c2(B d, B w, B x) { return c2(c(Md2D,d)->g, w,x); }


#ifdef CATCH_ERRORS
B fillBy_c1(B d, B x) { 
  B xf=getFill(inc(x));
  B r = c1(c(Md2D,d)->f, x);
  if(!isArr(r) || noFill(xf)) { dec(xf); return r; }
  if (CATCH) { dec(catchMessage); return r; }
  B fill = asFill(c1(c(Md2D,d)->g, xf));
  popCatch();
  return withFill(r, fill);
}
B fillBy_c2(B d, B w, B x) {
  B wf=getFill(inc(w)); B xf=getFill(inc(x));
  B r = c2(c(Md2D,d)->f, w,x);
  if(!isArr(r) || noFill(xf)) { dec(xf); dec(wf); return r; }
  if (CATCH) { dec(catchMessage); return r; }
  if (noFill(wf)) wf = inc(bi_asrt);
  B fill = asFill(c2(c(Md2D,d)->g, wf, xf));
  popCatch();
  return withFill(r, fill);
}
B catch_c1(B d,      B x) { if(CATCH) return c1(c(Md2D,d)->g,   x); B r = c1(c(Md2D,d)->f,   x); popCatch(); return r; }
B catch_c2(B d, B w, B x) { if(CATCH) return c2(c(Md2D,d)->g, w,x); B r = c2(c(Md2D,d)->f, w,x); popCatch(); return r; }
#else
B fillBy_c1(B d,      B x) { return c1(c(Md2D,d)->f,   x); }
B fillBy_c2(B d, B w, B x) { return c2(c(Md2D,d)->f, w,x); }
B catch_c1 (B d,      B x) { return c1(c(Md2D,d)->f,   x); }
B catch_c2 (B d, B w, B x) { return c2(c(Md2D,d)->f, w,x); }
#endif

#define ba(NAME) bi_##NAME = mm_alloc(sizeof(Md2), t_md2_def, ftag(MD2_TAG)); c(Md2,bi_##NAME)->c2 = NAME##_c2; c(Md2,bi_##NAME)->c1 = NAME##_c1;  c(Md2,bi_##NAME)->extra=pm2_##NAME; gc_add(bi_##NAME);
#define bd(NAME) bi_##NAME = mm_alloc(sizeof(Md2), t_md2_def, ftag(MD2_TAG)); c(Md2,bi_##NAME)->c2 = NAME##_c2; c(Md2,bi_##NAME)->c1 = c1_invalid; c(Md1,bi_##NAME)->extra=pm2_##NAME; gc_add(bi_##NAME);
#define bm(NAME) bi_##NAME = mm_alloc(sizeof(Md2), t_md2_def, ftag(MD2_TAG)); c(Md2,bi_##NAME)->c2 = c2_invalid;c(Md2,bi_##NAME)->c1 = NAME##_c1;  c(Md1,bi_##NAME)->extra=pm2_##NAME; gc_add(bi_##NAME);

void print_md2_def(B x) { printf("%s", format_pm2(c(Md1,x)->extra)); }

B                 bi_val, bi_fillBy, bi_catch;
void md2_init() { ba(val) ba(fillBy) ba(catch)
  ti[t_md2_def].print = print_md2_def;
}

#undef ba
#undef bd
#undef bm
