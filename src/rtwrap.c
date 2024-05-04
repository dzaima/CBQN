#include "core.h"
#include "vm.h"
#if RT_PERF
#include "builtins.h"
#endif
#include "utils/time.h"

#if defined(RT_WRAP) || defined(WRAP_NNBI)


typedef struct WFun WFun;
typedef struct WMd1 WMd1;
typedef struct WMd2 WMd2;
struct WFun {
  struct Fun;
  B v;
  WFun* prev;
  #if RT_PERF
    u64 c1t, c2t;
    u32 c1a, c2a;
  #elif RT_VERIFY
    B r1;
  #endif
};
struct WMd1 {
  struct Md1;
  u64 c1t, c2t;
  u32 c1a, c2a;
  B v;
  WMd1* prev;
};
struct WMd2 {
  struct Md2;
  u64 c1t, c2t;
  u32 c1a, c2a;
  B v;
  WMd2* prev;
};


WFun* lastWF;
WMd1* lastWM1;
WMd2* lastWM2;
void wfn_visit(Value* x) { mm_visit(((WFun*)x)->v); }
void wm1_visit(Value* x) { mm_visit(((WMd1*)x)->v); }
void wm2_visit(Value* x) { mm_visit(((WMd2*)x)->v); }

B wfn_identity(B x) {
  B f = c(WFun,x)->v;
  return TI(f,identity)(f);
}


// rtverify
#ifndef RT_VERIFY_ARGS
  #define RT_VERIFY_ARGS 1
#endif
#if RT_VERIFY
  B info_c2(B t, B w, B x);
  #define CHK(EXP,GOT,W,X) { if (!eequal(EXP,GOT)) { \
    printI(f); printf(": failed RT_VERIFY\n"); fflush(stdout); \
    if (RT_VERIFY_ARGS) {  \
      if(!q_N(W)){printf("𝕨:"); printI(W); printf(" / "); printsB(C2(info, m_i32(1), inc(W))); printf("\n"); fflush(stdout); } \
      {           printf("𝕩:"); printI(X); printf(" / "); printsB(C2(info, m_i32(1), inc(X))); printf("\n"); fflush(stdout); } \
      {       printf("got:"); printI(GOT); printf(" / "); printsB(C2(info, m_i32(1), inc(GOT))); printf("\n"); fflush(stdout); } \
      {       printf("exp:"); printI(EXP); printf(" / "); printsB(C2(info, m_i32(1), inc(EXP))); printf("\n"); fflush(stdout); } \
    }                      \
    vm_pstLive(); exit(1); \
  }}
#endif

// rtperf
u64 fwTotal;


B wfn_c1(B t, B x) {
  WFun* c = c(WFun,t);
  B f = c->v;
  FC1 fi = c(Fun,f)->c1;
  #if RT_PERF
    u64 s = nsTime();
    B r = fi(f, x);
    u64 e = nsTime();
    c->c1a++;
    c->c1t+= e-s;
    fwTotal+= e-s+20;
  #elif RT_VERIFY
    B exp = c1(c->r1, inc(x));
    #if RT_VERIFY_ARGS
      B r = fi(f, inc(x));
      CHK(exp, r, bi_N, x);
      dec(x);
    #else
      B r = fi(f, x);
      CHK(exp, r, bi_N, x);
    #endif
    dec(exp);
  #else
    B r = fi(f, x);
  #endif
  return r;
}
B wfn_c2(B t, B w, B x) {
  WFun* c = c(WFun,t);
  B f = c->v;
  FC2 fi = c(Fun,f)->c2;
  #if RT_PERF
    u64 s = nsTime();
    B r = fi(f, w, x);
    u64 e = nsTime();
    c->c2a++;
    c->c2t+= e-s;
    fwTotal+= e-s+20;
  #elif RT_VERIFY
    B exp = c2(c->r1, inc(w), inc(x));
    #if RT_VERIFY_ARGS
      B r = fi(f, inc(w), inc(x));
      CHK(exp, r, w, x);
      dec(w); dec(x);
    #else
      B r = fi(f, w, x);
      CHK(exp, r, w, x);
    #endif
    dec(exp);
  #else
    B r = fi(f, w, x);
  #endif
  return r;
}
#undef CHK

B wm1_c1(Md1D* d, B x) { B f = d->f; WMd1* t = (WMd1*)d->m1;
  #if RT_PERF
    u64 pfwt=fwTotal; fwTotal = 0;
    u64 s = nsTime();
  #endif
  B om = t->v;
  B fn = m1_d(incG(om), inc(f));
  B r = c1(fn, x);
  dec(fn);
  #if RT_PERF
    u64 e = nsTime();
    t->c1a++;
    t->c1t+= e-s - fwTotal;
    fwTotal = pfwt + e-s;
  #endif
  return r;
}
B wm1_c2(Md1D* d, B w, B x) { B f = d->f; WMd1* t = (WMd1*)d->m1;
  #if RT_PERF
    u64 pfwt=fwTotal; fwTotal = 0;
    u64 s = nsTime();
  #endif
  B om = t->v;
  B fn = m1_d(incG(om), inc(f));
  B r = c2(fn, w, x);
  dec(fn);
  #if RT_PERF
    u64 e = nsTime();
    t->c2a++;
    t->c2t+= e-s - fwTotal;
    fwTotal = pfwt + e-s;
  #endif
  return r;
}

B wm2_c1(Md2D* d, B x) { B f = d->f; B g = d->g; WMd2* t = (WMd2*)d->m2;
  #if RT_PERF
    u64 pfwt=fwTotal; fwTotal = 0;
    u64 s = nsTime();
  #endif
  B om = t->v;
  B fn = m2_d(incG(om), inc(f), inc(g));
  B r = c1(fn, x);
  dec(fn);
  #if RT_PERF
    u64 e = nsTime();
    t->c1a++;
    t->c1t+= e-s - fwTotal;
    fwTotal = pfwt + e-s;
  #endif
  return r;
}
B wm2_c2(Md2D* d, B w, B x) { B f = d->f; B g = d->g; WMd2* t = (WMd2*)d->m2;
  #if RT_PERF
    u64 pfwt=fwTotal; fwTotal = 0;
    u64 s = nsTime();
  #endif
  B om = t->v;
  B fn = m2_d(incG(om), inc(f), inc(g));
  B r = c2(fn, w, x);
  dec(fn);
  #if RT_PERF
    u64 e = nsTime();
    t->c2a++;
    t->c2t+= e-s - fwTotal;
    fwTotal = pfwt + e-s;
  #endif
  return r;
}





B rtWrap_wrap(B t, bool nnbi) {
  #if !defined(RT_WRAP)
    if (!nnbi) return t;
  #endif
  if (isFun(t)) {
    #if RT_VERIFY
      if(v(t)->flags==0) return t;
    #endif
    WFun* r = mm_alloc(sizeof(WFun), t_funWrap);
    r->extra = v(t)->extra;
    r->flags = v(t)->flags;
    r->c1 = wfn_c1;
    r->c2 = wfn_c2;
    r->v = t;
    r->prev = lastWF;
    lastWF = r;
    #if RT_VERIFY
      r->r1 = r1Objs[v(t)->flags-1];
    #elif RT_PERF
      r->c1t = 0; r->c1a = 0;
      r->c2t = 0; r->c2a = 0;
    #endif
    return tag(r,FUN_TAG);
  }
  #if RT_PERF || WRAP_NNBI
    if (isMd1(t)) {
      WMd1* r = mm_alloc(sizeof(WMd1), t_md1Wrap);
      r->extra = v(t)->extra;
      r->flags = v(t)->flags;
      r->c1 = wm1_c1;
      r->c2 = wm1_c2;
      r->v = t;
      r->prev = lastWM1;
      lastWM1 = r;
      r->c1a = 0; r->c2a = 0;
      r->c1t = 0; r->c2t = 0;
      return tag(r,MD1_TAG);
    }
    if (isMd2(t)) {
      Md2* fc = c(Md2,t);
      WMd2* r = mm_alloc(sizeof(WMd2), t_md2Wrap);
      r->c1 = wm2_c1;
      r->c2 = wm2_c2;
      r->extra = fc->extra;
      r->flags = fc->flags;
      r->v = t;
      r->prev = lastWM2;
      lastWM2 = r;
      r->c1a = 0; r->c2a = 0;
      r->c1t = 0; r->c2t = 0;
      return tag(r,MD2_TAG);
    }
  #endif
  return t;
}
B rtWrap_unwrap(B x) { // consumes x
  if (!isVal(x)) return x;
  if (TY(x)==t_funWrap) { B r = c(WFun,x)->v; decG(x); return inc(r); }
  if (TY(x)==t_md1Wrap) { B r = c(WMd1,x)->v; decG(x); return inc(r); }
  if (TY(x)==t_md2Wrap) { B r = c(WMd2,x)->v; decG(x); return inc(r); }
  return x;
}


B wfn_uc1(B t,    B o,                B x) { B t2 =  c(WFun,t)->v; return TI(t2,fn_uc1)(      t2,  o,          x); }
B wfn_ucw(B t,    B o,           B w, B x) { B t2 =  c(WFun,t)->v; return TI(t2,fn_ucw)(      t2,  o,       w, x); }
B wm1_uc1(Md1* t, B o, B f,           B x) { B t2 = ((WMd1*)t)->v; return TI(t2,m1_uc1)(c(Md1,t2), o, f,       x); }
B wm1_ucw(Md1* t, B o, B f,      B w, B x) { B t2 = ((WMd1*)t)->v; return TI(t2,m1_ucw)(c(Md1,t2), o, f,    w, x); }
B wm2_uc1(Md2* t, B o, B f, B g,      B x) { B t2 = ((WMd2*)t)->v; return TI(t2,m2_uc1)(c(Md2,t2), o, f, g,    x); }
B wm2_ucw(Md2* t, B o, B f, B g, B w, B x) { B t2 = ((WMd2*)t)->v; return TI(t2,m2_ucw)(c(Md2,t2), o, f, g, w, x); }

static B m1BI_d(B t, B f     ) { return m_md1D(c(Md1,t), f   ); }
static B m2BI_d(B t, B f, B g) { return m_md2D(c(Md2,t), f, g); }
void rtWrap_init(void) {
  TIi(t_funWrap,visit) = wfn_visit; TIi(t_funWrap,identity) = wfn_identity;
  TIi(t_md1Wrap,visit) = wm1_visit; TIi(t_md1Wrap,m1_d) = m1BI_d;
  TIi(t_md2Wrap,visit) = wm2_visit; TIi(t_md2Wrap,m2_d) = m2BI_d;
  TIi(t_funWrap,fn_uc1) = wfn_uc1;
  TIi(t_funWrap,fn_ucw) = wfn_ucw;
  TIi(t_md1Wrap,m1_uc1) = wm1_uc1;
  TIi(t_md1Wrap,m1_ucw) = wm1_ucw;
  TIi(t_md2Wrap,m2_uc1) = wm2_uc1;
  TIi(t_md2Wrap,m2_ucw) = wm2_ucw;
}
#else
void rtWrap_init(void) { }
#endif
void rtWrap_print(void) {
  #if RT_PERF
    WFun* cf = lastWF;
    while (cf) {
      printI(c1(bi_glyph, tag(cf,FUN_TAG)));
      printf(": m=%d %.3fms | d=%d %.3fms\n", cf->c1a, cf->c1t/1e6, cf->c2a, cf->c2t/1e6);
      cf = cf->prev;
    }
    WMd1* cm1 = lastWM1;
    while (cm1) {
      printI(c1(bi_glyph, tag(cm1,MD1_TAG)));
      printf(": m=%d %.3fms | d=%d %.3fms\n", cm1->c1a, cm1->c1t/1e6, cm1->c2a, cm1->c2t/1e6);
      cm1 = cm1->prev;
    }
    WMd2* cm2 = lastWM2;
    while (cm2) {
      printI(c1(bi_glyph, tag(cm2,MD2_TAG)));
      printf(": m=%d %.3fms | d=%d %.3fms\n", cm2->c1a, cm2->c1t/1e6, cm2->c2a, cm2->c2t/1e6);
      cm2 = cm2->prev;
    }
  #endif
}
