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
static B m_md1D(B m, B f     ) { B r = mm_alloc(sizeof(Md1D), t_md1D, ftag(FUN_TAG)); c(Md1D,r)->f = f; c(Md1D,r)->m1 = m;                   c(Md1D,r)->c1=md1D_c1; c(Md1D,r)->c2=md1D_c2; return r; }
static B m_md2D(B m, B f, B g) { B r = mm_alloc(sizeof(Md2D), t_md2D, ftag(FUN_TAG)); c(Md2D,r)->f = f; c(Md2D,r)->m2 = m; c(Md2D,r)->g = g; c(Md2D,r)->c1=md2D_c1; c(Md2D,r)->c2=md2D_c2; return r; }
static B m_md2H(B m,      B g) { B r = mm_alloc(sizeof(Md2H), t_md2H, ftag(MD1_TAG));                   c(Md2H,r)->m2 = m; c(Md2H,r)->g = g; c(Md2H,r)->c1=md2H_c1; c(Md2H,r)->c2=md2H_c2; return r; }
static B m_fork(B f, B g, B h) { B r = mm_alloc(sizeof(Fork), t_fork, ftag(FUN_TAG)); c(Fork,r)->f = f; c(Fork,r)->g  = g; c(Fork,r)->h = h; c(Fork,r)->c1=fork_c1; c(Fork,r)->c2=fork_c2; return r; }
static B m_atop(     B g, B h) { B r = mm_alloc(sizeof(Atop), t_atop, ftag(FUN_TAG));                   c(Atop,r)->g  = g; c(Atop,r)->h = h; c(Atop,r)->c1=tr2D_c1; c(Atop,r)->c2=tr2D_c2; return r; }

// consume all args
static B m1_d(B m, B f     ) { if(isMd1(m)) return TI(m).m1_d(m, f   ); thrM("Interpreting non-1-modifier as 1-modifier"); }
static B m2_d(B m, B f, B g) { if(isMd2(m)) return TI(m).m2_d(m, f, g); thrM("Interpreting non-2-modifier as 2-modifier"); }
static B m2_h(B m,      B g) {              return     m_md2H(m,    g); }
