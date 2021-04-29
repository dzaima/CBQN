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

B rt_undo;
void repeat_bounds(i64* bound, B g) { // doesn't consume
  if (isArr(g)) {
    BS2B xgetU = TI(g).getU;
    usz ia = a(g)->ia;
    for (usz i = 0; i < ia; i++) repeat_bounds(bound, xgetU(g, i));
  } else if (isNum(g)) {
    i64 i = o2i64(g);
    if (i<bound[0]) bound[0] = i;
    if (i>bound[1]) bound[1] = i;
  } else thrM("⍟: 𝔽 contained a non-number atom");
}
B repeat_replace(B g, B* q) { // doesn't consume
  if (isArr(g)) {
    BS2B ggetU = TI(g).getU;
    usz ia = a(g)->ia;
    HArr_p r = m_harrUc(g);
    for (usz i = 0; i < ia; i++) r.a[i] = repeat_replace(ggetU(g,i), q);
    return r.b;
  } else {
    return inc(q[o2i64u(g)]);
  }
}
#define REPEAT_T(CN, END, ...)                     \
  B g = CN(c(Md2D,d)->g, __VA_ARGS__ inc(x));      \
  B f = c(Md2D,d)->f;                              \
  if (isNum(g)) {                                  \
    i64 am = o2i64(g);                             \
    if (am>=0) {                                   \
      for (i64 i = 0; i < am; i++) x = CN(f, __VA_ARGS__ x); \
      END;                                         \
      return x;                                    \
    }                                              \
  }                                                \
  i64 bound[2] = {0,0};                            \
  repeat_bounds(bound, g);                         \
  u64 min = -bound[0]; u64 max = bound[1];         \
  B all[min+max+1];                                \
  B* q = all+min;                                  \
  q[0] = inc(x);                                   \
  if (min) {                                       \
    B x2 = inc(x);                                 \
    B fi = m1_d(inc(rt_undo), inc(f));             \
    for (i64 i = 0; i < min; i++) q[-1-i] = inc(x2 = CN(fi, __VA_ARGS__ x2)); \
    dec(x2);                                       \
    dec(fi);                                       \
  }                                                \
  for (i64 i = 0; i < max; i++) q[i+1] = inc(x = CN(f, __VA_ARGS__ x)); \
  dec(x);                                          \
  B r = repeat_replace(g, q);                      \
  dec(g);                                          \
  for (i64 i = 0; i < min+max+1; i++) dec(all[i]); \
  END;                                             \
  return r;

B repeat_c1(B d,      B x) { REPEAT_T(c1,{}              ); }
B repeat_c2(B d, B w, B x) { REPEAT_T(c2,dec(w), inc(w), ); }
#undef REPEAT_T

B before_c1(B d,      B x) { return c2(c(Md2D,d)->g, c1(c(Md2D,d)->f, inc(x)), x); }
B before_c2(B d, B w, B x) { return c2(c(Md2D,d)->g, c1(c(Md2D,d)->f,     w ), x); }
B after_c1(B d,      B x) { return c2(c(Md2D,d)->f, x, c1(c(Md2D,d)->g, inc(x))); }
B after_c2(B d, B w, B x) { return c2(c(Md2D,d)->f, w, c1(c(Md2D,d)->g,     x )); }
B atop_c1(B d,      B x) { return c1(c(Md2D,d)->f, c1(c(Md2D,d)->g,    x)); }
B atop_c2(B d, B w, B x) { return c1(c(Md2D,d)->f, c2(c(Md2D,d)->g, w, x)); }
B over_c1(B d,      B x) { return c1(c(Md2D,d)->f, c1(c(Md2D,d)->g,    x)); }
B over_c2(B d, B w, B x) { B xr=c1(c(Md2D,d)->g, x); return c2(c(Md2D,d)->f, c1(c(Md2D,d)->g, w), xr); }

B cond_c1(B d, B x) { B g=c(Md2D,d)->g;
  if (!isArr(g)||rnk(g)!=1) thrM("◶: 𝕘 must have rank 1");
  i64 fr = o2i64(c1(c(Md2D,d)->f, inc(x)));
  if (fr<0) fr+= a(g)->ia;
  if ((u64)fr >= a(g)->ia) thrM("◶: 𝔽 out of bounds of 𝕘");
  return c1(TI(g).getU(g, fr), x);
}
B cond_c2(B d, B w, B x) { B g=c(Md2D,d)->g;
  if (!isArr(g)||rnk(g)!=1) thrM("◶: 𝕘 must have rank 1");
  i64 fr = o2i64(c2(c(Md2D,d)->f, inc(w), inc(x)));
  if (fr<0) fr+= a(g)->ia;
  if ((u64)fr >= a(g)->ia) thrM("◶: 𝔽 out of bounds of 𝕘");
  return c2(TI(g).getU(g, fr), w, x);
}

#define ba(NAME) bi_##NAME = mm_alloc(sizeof(Md2), t_md2BI, ftag(MD2_TAG)); c(Md2,bi_##NAME)->c2 = NAME##_c2; c(Md2,bi_##NAME)->c1 = NAME##_c1;  c(Md2,bi_##NAME)->extra=pm2_##NAME; gc_add(bi_##NAME);
#define bd(NAME) bi_##NAME = mm_alloc(sizeof(Md2), t_md2BI, ftag(MD2_TAG)); c(Md2,bi_##NAME)->c2 = NAME##_c2; c(Md2,bi_##NAME)->c1 = c1_invalid; c(Md1,bi_##NAME)->extra=pm2_##NAME; gc_add(bi_##NAME);
#define bm(NAME) bi_##NAME = mm_alloc(sizeof(Md2), t_md2BI, ftag(MD2_TAG)); c(Md2,bi_##NAME)->c2 = c2_invalid;c(Md2,bi_##NAME)->c1 = NAME##_c1;  c(Md1,bi_##NAME)->extra=pm2_##NAME; gc_add(bi_##NAME);

void print_md2_def(B x) { printf("%s", format_pm2(c(Md1,x)->extra)); }

B                               bi_val, bi_repeat, bi_atop, bi_over, bi_before, bi_after, bi_cond, bi_fillBy, bi_catch;
static inline void md2_init() { ba(val) ba(repeat) ba(atop) ba(over) ba(before) ba(after) ba(cond) ba(fillBy) ba(catch)
  ti[t_md2BI].print = print_md2_def;
}

#undef ba
#undef bd
#undef bm
