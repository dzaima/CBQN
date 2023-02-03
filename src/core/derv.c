#include "../core.h"
#include "../nfns.h"
#include "../builtins.h"

DEF_FREE(md1D) { ptr_dec(((Md1D*)x)->m1); dec(((Md1D*)x)->f);                     }
DEF_FREE(md2D) { ptr_dec(((Md2D*)x)->m2); dec(((Md2D*)x)->f); dec(((Md2D*)x)->g); }
DEF_FREE(md2H) { ptr_dec(((Md2H*)x)->m2); dec(((Md2H*)x)->g);                     }
DEF_FREE(fork) {     dec(((Fork*)x)->f ); dec(((Fork*)x)->g); dec(((Fork*)x)->h); }
DEF_FREE(atop) {                          dec(((Atop*)x)->g); dec(((Atop*)x)->h); }

static void md1D_visit(Value* x) { mm_visitP(((Md1D*)x)->m1); mm_visit(((Md1D*)x)->f);                          }
static void md2D_visit(Value* x) { mm_visitP(((Md2D*)x)->m2); mm_visit(((Md2D*)x)->f); mm_visit(((Md2D*)x)->g); }
static void md2H_visit(Value* x) { mm_visitP(((Md2H*)x)->m2); mm_visit(((Md2H*)x)->g);                          }
static void fork_visit(Value* x) { mm_visit (((Fork*)x)->f ); mm_visit(((Fork*)x)->g); mm_visit(((Fork*)x)->h); }
static void atop_visit(Value* x) {                            mm_visit(((Atop*)x)->g); mm_visit(((Atop*)x)->h); }

static void md1D_print(FILE* f, B x) { fprintf(f,"(md1D ");fprintI(f,c(Md1D,x)->f);fprintf(f," ");fprintI(f,tag(c(Md1D,x)->m1,MD1_TAG));                                       fprintf(f,")"); }
static void md2D_print(FILE* f, B x) { fprintf(f,"(md2D ");fprintI(f,c(Md2D,x)->f);fprintf(f," ");fprintI(f,tag(c(Md2D,x)->m2,MD2_TAG));fprintf(f," ");fprintI(f,c(Md2D,x)->g);fprintf(f,")"); }
static void md2H_print(FILE* f, B x) { fprintf(f,"(md2H ");                                       fprintI(f,tag(c(Md2H,x)->m2,MD2_TAG));fprintf(f," ");fprintI(f,c(Md2H,x)->g);fprintf(f,")"); }
static void fork_print(FILE* f, B x) { fprintf(f,"(fork ");fprintI(f,c(Fork,x)->f);fprintf(f," ");fprintI(f,    c(Fork,x)->g          );fprintf(f," ");fprintI(f,c(Fork,x)->h);fprintf(f,")"); }
static void atop_print(FILE* f, B x) { fprintf(f,"(atop ");                                       fprintI(f,    c(Atop,x)->g          );fprintf(f," ");fprintI(f,c(Atop,x)->h);fprintf(f,")"); }

B md1D_c1(B t,      B x) { Md1D* tc = c(Md1D, t); return ((Md1*)tc->m1)->c1(tc,    x); }
B md1D_c2(B t, B w, B x) { Md1D* tc = c(Md1D, t); return ((Md1*)tc->m1)->c2(tc, w, x); }
B md2D_c1(B t,      B x) { Md2D* tc = c(Md2D, t); return ((Md2*)tc->m2)->c1(tc,    x); }
B md2D_c2(B t, B w, B x) { Md2D* tc = c(Md2D, t); return ((Md2*)tc->m2)->c2(tc, w, x); }
B tr2D_c1(B t,      B x) { return c1(c(Atop,t)->g, c1(c(Atop,t)->h,    x)); }
B tr2D_c2(B t, B w, B x) { return c1(c(Atop,t)->g, c2(c(Atop,t)->h, w, x)); }
B fork_c1(B t, B x) {
  B f = c(Fork,t)->f; errMd(f);
  B h = c(Fork,t)->h;
  if (isFun(f)) {
    B hr = c1(h, inc(x));
    return c2(c(Fork,t)->g, c(Fun,f)->c1(f,x), hr);
  } else {
    return c2(c(Fork,t)->g, inc(f), c1(h,x));
  }
}
B fork_c2(B t, B w, B x) {
  B f = c(Fork,t)->f; errMd(f);
  B h = c(Fork,t)->h;
  if (isFun(f)) {
    B hr = c2(h, inc(w), inc(x));
    return c2(c(Fork,t)->g, c(Fun,f)->c2(f,w,x), hr);
  } else {
    return c2(c(Fork,t)->g, inc(f), c2(h,w,x));
  }
}
B md2H_c1(Md1D* m,      B x) { Md2H* t=(Md2H*) m->m1; return md2D_c1(m_md2D(t->m2, m->f, t->g),    x); }
B md2H_c2(Md1D* m, B w, B x) { Md2H* t=(Md2H*) m->m1; return md2D_c2(m_md2D(t->m2, m->f, t->g), w, x); }

static B md1D_decompose(B x) { B r=m_hVec3(m_i32(4),inc(c(Md1D,x)->f),tag(ptr_inc(c(Md1D,x)->m1),MD1_TAG)                   ); decR(x); return r; }
static B md2D_decompose(B x) { B r=m_hVec4(m_i32(5),inc(c(Md2D,x)->f),tag(ptr_inc(c(Md2D,x)->m2),MD2_TAG), inc(c(Md2D,x)->g)); decR(x); return r; }
static B md2H_decompose(B x) { B r=m_hVec3(m_i32(6),                  tag(ptr_inc(c(Md2H,x)->m2),MD2_TAG), inc(c(Md2H,x)->g)); decR(x); return r; }
static B fork_decompose(B x) { B r=m_hVec4(m_i32(3),inc(c(Fork,x)->f),        inc(c(Fork,x)->g ),          inc(c(Fork,x)->h)); decR(x); return r; }
static B atop_decompose(B x) { B r=m_hVec3(m_i32(2),                          inc(c(Atop,x)->g ),          inc(c(Atop,x)->h)); decR(x); return r; }

static B md2D_uc1(B t, B o, B x) {
  Md2* m = c(Md2D, t)->m2;
  B f = c(Md2D, t)->f;
  B g = c(Md2D, t)->g;
  return TIv(m,m2_uc1)(m, o, f, g, x);
}

static B toConstant(B x) { // doesn't consume x
  if (!isCallable(x)) return inc(x);
  if (TY(x) == t_md1D) {
    Md1D* d = c(Md1D,x);
    Md1* m1 = d->m1;
    if (PTY(m1)==t_md1BI && m1->flags==n_const) return inc(d->f);
  }
  return bi_N;
}
static NFnDesc* ucwWrapDesc;
static NFnDesc* uc1WrapDesc;

static B fork_uc1(B t, B o, B x) {
  B f = toConstant(c(Fork, t)->f);
  B g = c(Fork, t)->g;
  B h = c(Fork, t)->h;
  if (RARE(q_N(f) | !isFun(g) | !isFun(h))) { dec(f); return def_fn_uc1(t, o, x); }
  B args[] = {g, o, f};
  B tmp = m_nfn(ucwWrapDesc, tag(args, RAW_TAG));
  B r = TI(h,fn_uc1)(h,tmp,x);
  // f is consumed by the eventual ucwWrap call. this hopes that everything is nice and calls o only once, and within the under call, so any under interface must make sure they can't
  decG(tmp);
  return r;
}
static B ucwWrap_c1(B t, B x) {
  B* args = c(B, nfn_objU(t));
  B g = args[0];
  return TI(g,fn_ucw)(g, args[1], args[2], x);
}

NOINLINE B atop_uc1_impl(B x, B o, B g, B h) {
  B args[] = {g, o};
  B tmp = m_nfn(uc1WrapDesc, tag(args, RAW_TAG));
  B r = TI(h,fn_uc1)(h,tmp,x);
  decG(tmp);
  return r;
}
static B atopM2_uc1(Md2* t, B o, B g, B h, B x) {
  if (RARE(!isFun(g) | !isFun(h))) return def_m2_uc1(t, g, h, o, x);
  return atop_uc1_impl(x, o, g, h);
}
static B atop_uc1(B t, B o, B x) {
  B g = c(Atop, t)->g;
  B h = c(Atop, t)->h;
  if (RARE(!isFun(g) | !isFun(h))) return def_fn_uc1(t, o, x);
  return atop_uc1_impl(x, o, g, h);
}
static B uc1Wrap_c1(B t, B x) {
  B* args = c(B, nfn_objU(t));
  B g = args[0];
  return TI(g,fn_uc1)(g, args[1], x);
}

static B md1D_im(B t,      B x) { Md1D* d = c(Md1D,t); return TIv(d->m1,m1_im)(d,    x); }
static B md1D_iw(B t, B w, B x) { Md1D* d = c(Md1D,t); return TIv(d->m1,m1_iw)(d, w, x); }
static B md1D_ix(B t, B w, B x) { Md1D* d = c(Md1D,t); return TIv(d->m1,m1_ix)(d, w, x); }
static B md2D_im(B t,      B x) { Md2D* d = c(Md2D,t); return TIv(d->m2,m2_im)(d,    x); }
static B md2D_iw(B t, B w, B x) { Md2D* d = c(Md2D,t); return TIv(d->m2,m2_iw)(d, w, x); }
static B md2D_ix(B t, B w, B x) { Md2D* d = c(Md2D,t); return TIv(d->m2,m2_ix)(d, w, x); }
static B m1BI_d(B t, B f     ) { return m_md1D(c(Md1,t), f   ); }
static B m2BI_d(B t, B f, B g) { return m_md2D(c(Md2,t), f, g); }

static B md1D_identity(B t) {
  Md1D* d = c(Md1D, t);
  if (d->m1==c(Md1,bi_tbl)) {
    B i = TI(d->f,identity)(d->f);
    return q_N(i)? i : m_atomUnit(i);
  }
  return bi_N;
}

void derv_init(void) {
  TIi(t_md1D,freeO) = md1D_freeO; TIi(t_md1D,freeF) = md1D_freeF; TIi(t_md1D,visit) = md1D_visit; TIi(t_md1D,print) = md1D_print; TIi(t_md1D,decompose) = md1D_decompose;
  TIi(t_md2D,freeO) = md2D_freeO; TIi(t_md2D,freeF) = md2D_freeF; TIi(t_md2D,visit) = md2D_visit; TIi(t_md2D,print) = md2D_print; TIi(t_md2D,decompose) = md2D_decompose;
  TIi(t_md2H,freeO) = md2H_freeO; TIi(t_md2H,freeF) = md2H_freeF; TIi(t_md2H,visit) = md2H_visit; TIi(t_md2H,print) = md2H_print; TIi(t_md2H,decompose) = md2H_decompose;
  TIi(t_fork,freeO) = fork_freeO; TIi(t_fork,freeF) = fork_freeF; TIi(t_fork,visit) = fork_visit; TIi(t_fork,print) = fork_print; TIi(t_fork,decompose) = fork_decompose;
  TIi(t_atop,freeO) = atop_freeO; TIi(t_atop,freeF) = atop_freeF; TIi(t_atop,visit) = atop_visit; TIi(t_atop,print) = atop_print; TIi(t_atop,decompose) = atop_decompose;
  TIi(t_md1BI,m1_d) = m1BI_d;
  TIi(t_md2BI,m2_d) = m2BI_d;
  TIi(t_md2D,fn_uc1) = md2D_uc1; // not in post so later init can utilize it
  TIi(t_md1D,fn_im) = md1D_im; TIi(t_md2D,fn_im) = md2D_im;
  TIi(t_md1D,fn_iw) = md1D_iw; TIi(t_md2D,fn_iw) = md2D_iw;
  TIi(t_md1D,fn_ix) = md1D_ix; TIi(t_md2D,fn_ix) = md2D_ix;
  TIi(t_md1D,identity) = md1D_identity;
}
void dervPost_init(void) {
  ucwWrapDesc = registerNFn(m_c8vec_0("(temporary function for ⌾)"), ucwWrap_c1, c2_bad);
  uc1WrapDesc = registerNFn(m_c8vec_0("(temporary function for ⌾)"), uc1Wrap_c1, c2_bad);
  TIi(t_fork,fn_uc1) = fork_uc1; // in post probably to make sure it's not used while not fully initialized or something? idk
  TIi(t_atop,fn_uc1) = atop_uc1;
  c(BMd2,bi_atop)->uc1 = atopM2_uc1;
}