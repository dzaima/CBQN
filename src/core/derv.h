#pragma once

typedef struct Md1D { // F _md
  struct Fun;
  B m1;
  B f;
} Md1D;
typedef struct Md2D { // F _md_ G
  struct Fun;
  B m2;
  B f, g;
} Md2D;
typedef struct Md2H { // _md_ G
  struct Md1;
  B m2;
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

B md1D_c1(B t,      B x);
B md1D_c2(B t, B w, B x);
B md2D_c1(B t,      B x);
B md2D_c2(B t, B w, B x);
B tr2D_c1(B t,      B x);
B tr2D_c2(B t, B w, B x);
B fork_c1(B t,      B x);
B fork_c2(B t, B w, B x);
B md2H_c1(B d,      B x);
B md2H_c2(B d, B w, B x);
// consume all args
static B m_md1D(B m, B f     ) { Md1D* r = mm_alloc(sizeof(Md1D), t_md1D); r->f = f; r->m1 = m;           r->c1=md1D_c1; r->c2=md1D_c2; return tag(r,FUN_TAG); }
static B m_md2D(B m, B f, B g) { Md2D* r = mm_alloc(sizeof(Md2D), t_md2D); r->f = f; r->m2 = m; r->g = g; r->c1=md2D_c1; r->c2=md2D_c2; return tag(r,FUN_TAG); }
static B m_md2H(B m,      B g) { Md2H* r = mm_alloc(sizeof(Md2H), t_md2H);           r->m2 = m; r->g = g; r->c1=md2H_c1; r->c2=md2H_c2; return tag(r,MD1_TAG); }
static B m_fork(B f, B g, B h) { Fork* r = mm_alloc(sizeof(Fork), t_fork); r->f = f; r->g  = g; r->h = h; r->c1=fork_c1; r->c2=fork_c2; return tag(r,FUN_TAG); }
static B m_atop(     B g, B h) { Atop* r = mm_alloc(sizeof(Atop), t_atop);           r->g  = g; r->h = h; r->c1=tr2D_c1; r->c2=tr2D_c2; return tag(r,FUN_TAG); }

// consume all args
static B m1_d(B m, B f     ) { if(isMd1(m)) return TI(m,m1_d)(m, f   ); thrM("Interpreting non-1-modifier as 1-modifier"); }
static B m2_d(B m, B f, B g) { if(isMd2(m)) return TI(m,m2_d)(m, f, g); thrM("Interpreting non-2-modifier as 2-modifier"); }
static B m2_h(B m,      B g) {              return     m_md2H(m,    g); }
