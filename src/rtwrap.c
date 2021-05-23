#include "core.h"
#include "vm.h"


#ifdef RT_WRAP
typedef struct WFun WFun;
struct WFun {
  struct Fun;
  B v;
  WFun* prev;
  #ifdef RT_PERF
    u64 c1t, c2t;
    u32 c1a, c2a;
  #else
    B r1;
  #endif
};
WFun* lastWF;
void wf_visit(Value* x) { mm_visit(((WFun*)x)->v); }
B wf_identity(B x) {
  B f = c(WFun,x)->v;
  return inc(TI(f).identity(f));
}
#define CHK(A,B) { if (!eequal(A,B)) { print(f); printf(": failed RT_VERIFY\n"); vm_pstLive(); exit(1); } }
u64 fwTotal;
B wf_c1(B t, B x) {
  WFun* c = c(WFun,t);
  B f = c->v;
  BB2B fi = c(Fun,f)->c1;
  #ifdef RT_PERF
    u64 s = nsTime();
    B r = fi(f, x);
    u64 e = nsTime();
    c->c1a++;
    c->c1t+= e-s;
    fwTotal+= e-s+20;
  #else
    B exp = c1(c->r1, inc(x));
    B r = fi(f, x);
    CHK(exp, r);
    dec(exp);
  #endif
  return r;
}
B wf_c2(B t, B w, B x) {
  WFun* c = c(WFun,t);
  B f = c->v;
  BBB2B fi = c(Fun,f)->c2;
  #ifdef RT_PERF
    u64 s = nsTime();
    B r = fi(f, w, x);
    u64 e = nsTime();
    c->c2a++;
    c->c2t+= e-s;
    fwTotal+= e-s+20;
  #else
    B exp = c2(c->r1, inc(w), inc(x));
    B r = fi(f, w, x);
    CHK(exp, r);
    dec(exp);
  #endif
  return r;
}
#undef CHK

typedef struct WMd1 WMd1;
struct WMd1 {
  struct Md1;
  u64 c1t, c2t;
  u32 c1a, c2a;
  B v;
  WMd1* prev;
};
WMd1* lastWM1;
void wm1_visit(Value* x) { mm_visit(((WMd1*)x)->v); }
typedef struct WMd2 WMd2;
struct WMd2 {
  struct Md2;
  u64 c1t, c2t;
  u32 c1a, c2a;
  B v;
  WMd2* prev;
};
WMd2* lastWM2;
void wm2_visit(Value* x) { mm_visit(((WMd2*)x)->v); }

B wm1_c1(B d, B x) { B f = c(Md1D,d)->f; B t = c(Md1D,d)->m1;
  u64 pfwt=fwTotal; fwTotal = 0;
  WMd1* c = c(WMd1,t);
  B om = c->v;
  u64 s = nsTime();
  B fn = m1_d(inc(om), inc(f));
  B r = c1(fn, x);
  u64 e = nsTime();
  dec(fn);
  c->c1a++;
  c->c1t+= e-s - fwTotal;
  fwTotal = pfwt + e-s + 30;
  return r;
}
B wm1_c2(B d, B w, B x) { B f = c(Md1D,d)->f; B t = c(Md1D,d)->m1;
  u64 pfwt=fwTotal; fwTotal = 0;
  WMd1* c = c(WMd1,t);
  B om = c->v;
  u64 s = nsTime();
  B fn = m1_d(inc(om), inc(f));
  B r = c2(fn, w, x);
  u64 e = nsTime();
  dec(fn);
  c->c2a++;
  c->c2t+= e-s - fwTotal;
  fwTotal = pfwt + e-s + 30;
  return r;
}

B wm2_c1(B d, B x) { B f = c(Md2D,d)->f; B g = c(Md2D,d)->g; B t = c(Md2D,d)->m2;
  u64 pfwt=fwTotal; fwTotal = 0;
  WMd1* c = c(WMd1,t);
  B om = c->v;
  u64 s = nsTime();
  B fn = m2_d(inc(om), inc(f), inc(g));
  B r = c1(fn, x);
  u64 e = nsTime();
  dec(fn);
  c->c1a++;
  c->c1t+= e-s - fwTotal;
  fwTotal = pfwt + e-s + 30;
  return r;
}
B wm2_c2(B d, B w, B x) { B f = c(Md2D,d)->f; B g = c(Md2D,d)->g; B t = c(Md2D,d)->m2;
  u64 pfwt=fwTotal; fwTotal = 0;
  WMd1* c = c(WMd1,t);
  B om = c->v;
  u64 s = nsTime();
  B fn = m2_d(inc(om), inc(f), inc(g));
  B r = c2(fn, w, x);
  u64 e = nsTime();
  dec(fn);
  c->c2a++;
  c->c2t+= e-s - fwTotal;
  fwTotal = pfwt + e-s + 30;
  return r;
}





B rtWrap_wrap(B t) {
  if (isFun(t)) {
    #ifdef RT_VERIFY
      if(v(t)->flags==0) return t;
    #endif
    B r = mm_alloc(sizeof(WFun), t_funWrap, ftag(FUN_TAG));
    c(Value,r)->extra = v(t)->extra;
    c(Value,r)->flags = v(t)->flags;
    c(Fun,r)->c1 = wf_c1;
    c(Fun,r)->c2 = wf_c2;
    c(WFun,r)->v = t;
    c(WFun,r)->prev = lastWF;
    lastWF = c(WFun,r);
    #ifdef RT_VERIFY
      c(WFun,r)->r1 = r1Objs[v(t)->flags-1];
    #else
      c(WFun,r)->c1t = 0; c(WFun,r)->c1a = 0;
      c(WFun,r)->c2t = 0; c(WFun,r)->c2a = 0;
    #endif
    return r;
  }
  #ifdef RT_PERF
  if (isMd1(t)) {
    B r = mm_alloc(sizeof(WMd1), t_md1Wrap, ftag(MD1_TAG));
    c(Value,r)->extra = v(t)->extra;
    c(Value,r)->flags = v(t)->flags;
    c(Md1,r)->c1 = wm1_c1;
    c(Md1,r)->c2 = wm1_c2;
    c(WMd1,r)->v = t;
    c(WMd1,r)->prev = lastWM1;
    c(WMd1,r)->c1t = 0; c(WMd1,r)->c1a = 0;
    c(WMd1,r)->c2t = 0; c(WMd1,r)->c2a = 0;
    lastWM1 = c(WMd1,r);
    return r;
  }
  if (isMd2(t)) {
    Md2* fc = c(Md2,t);
    B r = mm_alloc(sizeof(WMd2), t_md2Wrap, ftag(MD2_TAG));
    c(Md2,r)->c1 = wm2_c1;
    c(Md2,r)->c2 = wm2_c2;
    c(Md2,r)->extra = fc->extra;
    c(Md2,r)->flags = fc->flags;
    c(WMd2,r)->v = t;
    c(WMd2,r)->prev = lastWM2;
    c(WMd2,r)->c1t = 0; c(WMd2,r)->c1a = 0;
    c(WMd2,r)->c2t = 0; c(WMd2,r)->c2a = 0;
    lastWM2 = c(WMd2,r);
    return r;
  }
  #endif
  return t;
}



B wfn_uc1(B t, B o,                B x) { B t2 = c(WFun,t)->v; return TI(t2).fn_uc1(t2, o,          x); }
B wfn_ucw(B t, B o,           B w, B x) { B t2 = c(WFun,t)->v; return TI(t2).fn_ucw(t2, o,       w, x); }
B wm1_uc1(B t, B o, B f,           B x) { B t2 = c(WMd1,t)->v; return TI(t2).m1_uc1(t2, o, f,       x); }
B wm1_ucw(B t, B o, B f,      B w, B x) { B t2 = c(WMd1,t)->v; return TI(t2).m1_ucw(t2, o, f,    w, x); }
B wm2_uc1(B t, B o, B f, B g,      B x) { B t2 = c(WMd2,t)->v; return TI(t2).m2_uc1(t2, o, f, g,    x); }
B wm2_ucw(B t, B o, B f, B g, B w, B x) { B t2 = c(WMd2,t)->v; return TI(t2).m2_ucw(t2, o, f, g, w, x); }

void rtWrap_init() {
  ti[t_funWrap].visit =  wf_visit; ti[t_funWrap].identity = wf_identity;
  ti[t_md1Wrap].visit = wm1_visit; ti[t_md1Wrap].m1_d = m_md1D;
  ti[t_md2Wrap].visit = wm2_visit; ti[t_md2Wrap].m2_d = m_md2D;
  ti[t_funWrap].fn_uc1 = wfn_uc1;
  ti[t_funWrap].fn_ucw = wfn_ucw;
  ti[t_md1Wrap].m1_uc1 = wm1_uc1;
  ti[t_md1Wrap].m1_ucw = wm1_ucw;
  ti[t_md2Wrap].m2_uc1 = wm2_uc1;
  ti[t_md2Wrap].m2_ucw = wm2_ucw;
}
#else
void rtWrap_init() { }
#endif
void rtWrap_print() {
  #ifdef RT_PERF
    WFun* cf = lastWF;
    while (cf) {
      printRaw(c1(bi_fmtF, tag(cf,FUN_TAG)));
      printf(": m=%d %.3fms | d=%d %.3fms\n", cf->c1a, cf->c1t/1e6, cf->c2a, cf->c2t/1e6);
      cf = cf->prev;
    }
    WMd1* cm1 = lastWM1;
    while (cm1) {
      printRaw(c1(bi_fmtF, tag(cm1,MD1_TAG)));
      printf(": m=%d %.3fms | d=%d %.3fms\n", cm1->c1a, cm1->c1t/1e6, cm1->c2a, cm1->c2t/1e6);
      cm1 = cm1->prev;
    }
    WMd2* cm2 = lastWM2;
    while (cm2) {
      printRaw(c1(bi_fmtF, tag(cm2,MD2_TAG)));
      printf(": m=%d %.3fms | d=%d %.3fms\n", cm2->c1a, cm2->c1t/1e6, cm2->c2a, cm2->c2t/1e6);
      cm2 = cm2->prev;
    }
  #endif
}
