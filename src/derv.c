#include "h.h"

typedef struct Md1D { // F _md
  struct Fun;
  B m; // known to be Md1 at creation; kept as type B as refc-- might want to know tagged ptr info
  B f;
} Md1D;
typedef struct Md2D { // F _md_ G
  struct Fun;
  B m;
  B f, g;
} Md2D;
typedef struct Md2H { // _md_ G
  struct Md1;
  B m;
  B g;
} Md2H;
typedef struct Fork {
  struct Fun;
  B f, g, h;
} Fork;
typedef struct Atop {
  struct Fun;
  B g, h;
} Atop;

void md1D_free(B x) { dec(c(Md1D,x)->m); dec(c(Md1D,x)->f);                    }
void md2D_free(B x) { dec(c(Md2D,x)->m); dec(c(Md2D,x)->f); dec(c(Md2D,x)->g); }
void md2H_free(B x) { dec(c(Md2H,x)->m); dec(c(Md2H,x)->g);                    }
void fork_free(B x) { dec(c(Fork,x)->f); dec(c(Fork,x)->g); dec(c(Fork,x)->h); }
void atop_free(B x) {                    dec(c(Atop,x)->g); dec(c(Atop,x)->h); }

void md1D_print(B x) { printf("(md1D ");print(c(Md1D,x)->f);printf(" ");print(c(Md1D,x)->m);                                printf(")"); }
void md2D_print(B x) { printf("(md2D ");print(c(Md2D,x)->f);printf(" ");print(c(Md2D,x)->m);printf(" ");print(c(Md2D,x)->g);printf(")"); }
void md2H_print(B x) { printf("(md2H ");                                print(c(Md2H,x)->m);printf(" ");print(c(Md2H,x)->g);printf(")"); }
void fork_print(B x) { printf("(fork ");print(c(Fork,x)->f);printf(" ");print(c(Fork,x)->g);printf(" ");print(c(Fork,x)->h);printf(")"); }
void atop_print(B x) { printf("(atop ");                                print(c(Atop,x)->g);printf(" ");print(c(Atop,x)->h);printf(")"); }

B md1D_c1(B t     , B x) { return c(Md1,c(Md1D, t)->m)->c1(c(Md1D, t)->m, c(Md1D, t)->f,                   x); }
B md1D_c2(B t, B w, B x) { return c(Md1,c(Md1D, t)->m)->c2(c(Md1D, t)->m, c(Md1D, t)->f,                w, x); }
B md2D_c1(B t     , B x) { return c(Md2,c(Md2D, t)->m)->c1(c(Md2D, t)->m, c(Md2D, t)->f, c(Md2D, t)->g,    x); }
B md2D_c2(B t, B w, B x) { return c(Md2,c(Md2D, t)->m)->c2(c(Md2D, t)->m, c(Md2D, t)->f, c(Md2D, t)->g, w, x); }
B atop_c1(B t     , B x) { return c1(c(Atop,t)->g, c1(c(Atop,t)->h,    x)); }
B atop_c2(B t, B w, B x) { return c1(c(Atop,t)->g, c2(c(Atop,t)->h, w, x)); }
B fork_c1(B t     , B x) { B g=c1(c(Fork,t)->h,          inci(x)); return c2(c(Fork,t)->g, c1(c(Fork,t)->f,    x), g); }
B fork_c2(B t, B w, B x) { B g=c2(c(Fork,t)->h, inci(w), inci(x)); return c2(c(Fork,t)->g, c2(c(Fork,t)->f, w, x), g); }
B md2H_c1(B t, B f     , B x) { return c(Md2,c(Md2H, t)->m)->c1(c(Md2H,t)->m, f, c(Md2H,t)->g,    x); }
B md2H_c2(B t, B f, B w, B x) { return c(Md2,c(Md2H, t)->m)->c2(c(Md2H,t)->m, f, c(Md2H,t)->g, w, x); }

B md1D_decompose(B x) { B r=m_v3(m_i32(4),inci(c(Md1D,x)->f),inci(c(Md1D,x)->m)                    ); dec(x); return r; }
B md2D_decompose(B x) { B r=m_v4(m_i32(5),inci(c(Md2D,x)->f),inci(c(Md2D,x)->m), inci(c(Md2D,x)->g)); dec(x); return r; }
B md2H_decompose(B x) { B r=m_v3(m_i32(6),                   inci(c(Md2H,x)->m), inci(c(Md2H,x)->g)); dec(x); return r; }
B fork_decompose(B x) { B r=m_v4(m_i32(3),inci(c(Fork,x)->f),inci(c(Fork,x)->g), inci(c(Fork,x)->h)); dec(x); return r; }
B atop_decompose(B x) { B r=m_v3(m_i32(2),                   inci(c(Atop,x)->g), inci(c(Atop,x)->h)); dec(x); return r; }

// consume all args
B m_md1D(B m, B f     ) { B r = mm_alloc(sizeof(Md1D), t_md1D, ftag(FUN_TAG)); c(Md1D,r)->f = f; c(Md1D,r)->m = m;                   c(Md1D,r)->c1=md1D_c1; c(Md1D,r)->c2=md1D_c2; c(Md1D,r)->extra=pf_md1d; return r; }
B m_md2D(B m, B f, B g) { B r = mm_alloc(sizeof(Md2D), t_md2D, ftag(FUN_TAG)); c(Md2D,r)->f = f; c(Md2D,r)->m = m; c(Md2D,r)->g = g; c(Md2D,r)->c1=md2D_c1; c(Md2D,r)->c2=md2D_c2; c(Md2D,r)->extra=pf_md2d; return r; }
B m_md2H(B m,      B g) { B r = mm_alloc(sizeof(Md2H), t_md2H, ftag(MD1_TAG));                   c(Md2H,r)->m = m; c(Md2H,r)->g = g; c(Md2H,r)->c1=md2H_c1; c(Md2H,r)->c2=md2H_c2;                           return r; }
B m_fork(B f, B g, B h) { B r = mm_alloc(sizeof(Fork), t_fork, ftag(FUN_TAG)); c(Fork,r)->f = f; c(Fork,r)->g = g; c(Fork,r)->h = h; c(Fork,r)->c1=fork_c1; c(Fork,r)->c2=fork_c2; c(Fork,r)->extra=pf_fork; return r; }
B m_atop(     B g, B h) { B r = mm_alloc(sizeof(Atop), t_atop, ftag(FUN_TAG));                   c(Atop,r)->g = g; c(Atop,r)->h = h; c(Atop,r)->c1=atop_c1; c(Atop,r)->c2=atop_c2; c(Atop,r)->extra=pf_atop; return r; }

// consume all args
B m1_d(B m, B f     ) { if(isMd1(m)) return TI(m).m1_d(m, f   ); return err("Interpreting non-1-modifier as 1-modifier"); }
B m2_d(B m, B f, B g) { if(isMd2(m)) return TI(m).m2_d(m, f, g); return err("Interpreting non-2-modifier as 2-modifier"); }
B m2_h(B m,      B g) {              return     m_md2H(m,    g); }



void derv_init() {
  ti[t_md1D].free = md1D_free; ti[t_md1D].print = md1D_print; ti[t_md1D].decompose = md1D_decompose;
  ti[t_md2D].free = md2D_free; ti[t_md2D].print = md2D_print; ti[t_md2D].decompose = md2D_decompose;
  ti[t_md2H].free = md2H_free; ti[t_md2H].print = md2H_print; ti[t_md2H].decompose = md2H_decompose;
  ti[t_fork].free = fork_free; ti[t_fork].print = fork_print; ti[t_fork].decompose = fork_decompose;
  ti[t_atop].free = atop_free; ti[t_atop].print = atop_print; ti[t_atop].decompose = atop_decompose;
  ti[t_md1_def].m1_d = m_md1D;
  ti[t_md2_def].m2_d = m_md2D;
}